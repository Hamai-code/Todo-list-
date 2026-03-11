#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 5000
#define BUFFER_SIZE 65536
#define FILE_NAME "taches.txt"
#define TMP_FILE_NAME "taches_tmp.txt"

typedef struct {
    int id;
    char description[200];
    char dateLimite[20];
    int etat;
} Tache;

static void sanitize(char *s) {
    size_t i;
    size_t len = strlen(s);
    for (i = 0; i < len; i++) {
        if (s[i] == '|' || s[i] == '\n' || s[i] == '\r') {
            s[i] = ' ';
        }
    }
}

static int readTask(FILE *f, Tache *t) {
    return fscanf(f, "%d|%199[^|]|%19[^|]|%d\n", &t->id, t->description, t->dateLimite, &t->etat) == 4;
}

static int getNextId(void) {
    FILE *f = fopen(FILE_NAME, "r");
    Tache t;
    int id = 1;

    if (f == NULL) {
        return 1;
    }

    while (readTask(f, &t)) {
        if (t.id >= id) {
            id = t.id + 1;
        }
    }

    fclose(f);
    return id;
}

static int rewriteFileForOperation(int targetId, const char *newDescription, const char *newDate, int newEtat, int mode) {
    FILE *f = fopen(FILE_NAME, "r");
    FILE *tmp = fopen(TMP_FILE_NAME, "w");
    Tache t;
    int changed = 0;

    if (tmp == NULL) {
        if (f != NULL) {
            fclose(f);
        }
        return 0;
    }

    if (f == NULL) {
        fclose(tmp);
        remove(TMP_FILE_NAME);
        return 0;
    }

    while (readTask(f, &t)) {
        if (t.id == targetId) {
            changed = 1;
            if (mode == 1) {
                t.etat = newEtat;
            } else if (mode == 2) {
                strncpy(t.description, newDescription, sizeof(t.description) - 1);
                t.description[sizeof(t.description) - 1] = '\0';
                strncpy(t.dateLimite, newDate, sizeof(t.dateLimite) - 1);
                t.dateLimite[sizeof(t.dateLimite) - 1] = '\0';
            } else if (mode == 3) {
                continue;
            }
        }

        fprintf(tmp, "%d|%s|%s|%d\n", t.id, t.description, t.dateLimite, t.etat);
    }

    fclose(f);
    fclose(tmp);
    remove(FILE_NAME);
    rename(TMP_FILE_NAME, FILE_NAME);
    return changed;
}

static int ajouterTache(char description[], char date[]) {
    FILE *f = fopen(FILE_NAME, "a");
    int id;

    if (f == NULL) {
        return 0;
    }

    sanitize(description);
    sanitize(date);
    id = getNextId();
    fprintf(f, "%d|%s|%s|0\n", id, description, date);
    fclose(f);
    return 1;
}

static int marquerFaite(int id) {
    return rewriteFileForOperation(id, "", "", 1, 1);
}

static int supprimerTache(int id) {
    return rewriteFileForOperation(id, "", "", 0, 3);
}

static int modifierTache(int id, char description[], char date[]) {
    sanitize(description);
    sanitize(date);
    return rewriteFileForOperation(id, description, date, 0, 2);
}

static void ddmmyyyy_to_iso(const char *in, char *out, size_t outSize) {
    int d, m, y;
    if (sscanf(in, "%d/%d/%d", &d, &m, &y) == 3) {
        snprintf(out, outSize, "%04d-%02d-%02d", y, m, d);
    } else {
        out[0] = '\0';
    }
}

static void iso_to_ddmmyyyy(const char *in, char *out, size_t outSize) {
    int y, m, d;
    if (sscanf(in, "%d-%d-%d", &y, &m, &d) == 3) {
        snprintf(out, outSize, "%02d/%02d/%04d", d, m, y);
    } else {
        out[0] = '\0';
    }
}

static void json_escape(const char *src, char *dst, size_t dstSize) {
    size_t i = 0;
    size_t j = 0;
    while (src[i] != '\0' && j + 2 < dstSize) {
        if (src[i] == '"' || src[i] == '\\') {
            dst[j++] = '\\';
            dst[j++] = src[i++];
        } else {
            dst[j++] = src[i++];
        }
    }
    dst[j] = '\0';
}

static int listerTachesJson(char *out, size_t outSize) {
    FILE *f = fopen(FILE_NAME, "r");
    Tache t;
    int first = 1;
    size_t used = 0;

    if (outSize < 3) {
        return 0;
    }

    out[0] = '[';
    out[1] = '\0';
    used = 1;

    if (f == NULL) {
        if (used + 2 > outSize) {
            return 0;
        }
        out[used++] = ']';
        out[used] = '\0';
        return 1;
    }

    while (readTask(f, &t)) {
        char descEsc[500];
        char dateIso[20];
        char item[900];
        int written;

        json_escape(t.description, descEsc, sizeof(descEsc));
        ddmmyyyy_to_iso(t.dateLimite, dateIso, sizeof(dateIso));

        written = snprintf(item, sizeof(item),
            "%s{\"id\":%d,\"description\":\"%s\",\"date\":\"%s\",\"date_iso\":\"%s\",\"etat\":%d}",
            first ? "" : ",", t.id, descEsc, t.dateLimite, dateIso, t.etat);
        if (written < 0 || used + (size_t)written + 2 > outSize) {
            fclose(f);
            return 0;
        }

        memcpy(out + used, item, (size_t)written);
        used += (size_t)written;
        out[used] = '\0';
        first = 0;
    }

    fclose(f);

    if (used + 2 > outSize) {
        return 0;
    }
    out[used++] = ']';
    out[used] = '\0';
    return 1;
}

static int get_content_length(const char *request) {
    const char *p = strstr(request, "Content-Length:");
    int len = 0;
    if (p == NULL) {
        return 0;
    }
    sscanf(p, "Content-Length: %d", &len);
    return len;
}

static int parse_json_value(const char *json, const char *key, char *out, size_t outSize) {
    char pattern[64];
    const char *start;
    const char *p;
    size_t j = 0;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    start = strstr(json, pattern);
    if (start == NULL) {
        return 0;
    }
    start = strchr(start, ':');
    if (start == NULL) {
        return 0;
    }
    start++;
    while (*start == ' ' || *start == '\t') {
        start++;
    }
    if (*start != '"') {
        return 0;
    }
    start++;

    p = start;
    while (*p != '\0' && *p != '"' && j + 1 < outSize) {
        if (*p == '\\' && *(p + 1) != '\0') {
            p++;
        }
        out[j++] = *p++;
    }
    out[j] = '\0';
    return 1;
}

static int read_file(const char *path, char *out, size_t outSize) {
    FILE *f = fopen(path, "rb");
    size_t n;
    if (f == NULL) {
        return 0;
    }
    n = fread(out, 1, outSize - 1, f);
    out[n] = '\0';
    fclose(f);
    return 1;
}

static void send_response(SOCKET client, int code, const char *status, const char *type, const char *body) {
    char header[512];
    int bodyLen = (int)strlen(body);
    int headerLen = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        code,
        status,
        type,
        bodyLen
    );

    send(client, header, headerLen, 0);
    send(client, body, bodyLen, 0);
}

static void handle_api(SOCKET client, const char *method, const char *path, const char *body) {
    char payload[131072];

    if (strcmp(method, "GET") == 0 && strcmp(path, "/taches") == 0) {
        if (!listerTachesJson(payload, sizeof(payload))) {
            send_response(client, 500, "Internal Server Error", "application/json", "{\"error\":\"Erreur lecture taches\"}");
            return;
        }
        send_response(client, 200, "OK", "application/json", payload);
        return;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/ajout") == 0) {
        char description[256];
        char dateIso[32];
        char dateFr[20];
        if (!parse_json_value(body, "description", description, sizeof(description)) ||
            !parse_json_value(body, "date", dateIso, sizeof(dateIso))) {
            send_response(client, 400, "Bad Request", "application/json", "{\"error\":\"Body invalide\"}");
            return;
        }
        iso_to_ddmmyyyy(dateIso, dateFr, sizeof(dateFr));
        if (dateFr[0] == '\0' || !ajouterTache(description, dateFr)) {
            send_response(client, 500, "Internal Server Error", "application/json", "{\"error\":\"Ajout impossible\"}");
            return;
        }
        send_response(client, 201, "Created", "application/json", "{\"message\":\"Tache ajoutee\"}");
        return;
    }

    if (strncmp(path, "/faite/", 7) == 0 && strcmp(method, "POST") == 0) {
        int id = atoi(path + 7);
        if (id <= 0 || !marquerFaite(id)) {
            send_response(client, 404, "Not Found", "application/json", "{\"error\":\"Tache introuvable\"}");
            return;
        }
        send_response(client, 200, "OK", "application/json", "{\"message\":\"Tache terminee\"}");
        return;
    }

    if (strncmp(path, "/supprimer/", 11) == 0 && strcmp(method, "DELETE") == 0) {
        int id = atoi(path + 11);
        if (id <= 0 || !supprimerTache(id)) {
            send_response(client, 404, "Not Found", "application/json", "{\"error\":\"Tache introuvable\"}");
            return;
        }
        send_response(client, 200, "OK", "application/json", "{\"message\":\"Tache supprimee\"}");
        return;
    }

    if (strncmp(path, "/modifier/", 10) == 0 && strcmp(method, "PUT") == 0) {
        int id = atoi(path + 10);
        char description[256];
        char dateIso[32];
        char dateFr[20];
        if (id <= 0 ||
            !parse_json_value(body, "description", description, sizeof(description)) ||
            !parse_json_value(body, "date", dateIso, sizeof(dateIso))) {
            send_response(client, 400, "Bad Request", "application/json", "{\"error\":\"Body invalide\"}");
            return;
        }
        iso_to_ddmmyyyy(dateIso, dateFr, sizeof(dateFr));
        if (dateFr[0] == '\0' || !modifierTache(id, description, dateFr)) {
            send_response(client, 404, "Not Found", "application/json", "{\"error\":\"Modification impossible\"}");
            return;
        }
        send_response(client, 200, "OK", "application/json", "{\"message\":\"Tache modifiee\"}");
        return;
    }

    send_response(client, 404, "Not Found", "application/json", "{\"error\":\"Route API inconnue\"}");
}

static void handle_static(SOCKET client, const char *path) {
    char content[131072];

    if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        if (!read_file("index.html", content, sizeof(content))) {
            send_response(client, 404, "Not Found", "text/plain", "index.html introuvable");
            return;
        }
        send_response(client, 200, "OK", "text/html; charset=utf-8", content);
        return;
    }

    if (strcmp(path, "/style.css") == 0) {
        if (!read_file("style.css", content, sizeof(content))) {
            send_response(client, 404, "Not Found", "text/plain", "style.css introuvable");
            return;
        }
        send_response(client, 200, "OK", "text/css; charset=utf-8", content);
        return;
    }

    if (strcmp(path, "/script.js") == 0) {
        if (!read_file("script.js", content, sizeof(content))) {
            send_response(client, 404, "Not Found", "text/plain", "script.js introuvable");
            return;
        }
        send_response(client, 200, "OK", "application/javascript; charset=utf-8", content);
        return;
    }

    send_response(client, 404, "Not Found", "text/plain", "Ressource introuvable");
}

int main(void) {
    WSADATA wsaData;
    SOCKET serverSock, clientSock;
    struct sockaddr_in serverAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "Erreur WSAStartup\n");
        return 1;
    }

    serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock == INVALID_SOCKET) {
        fprintf(stderr, "Erreur creation socket\n");
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        fprintf(stderr, "Erreur bind\n");
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "Erreur listen\n");
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    printf("Serveur C lance sur http://127.0.0.1:%d\n", PORT);

    while (1) {
        char request[BUFFER_SIZE];
        int received;
        char method[8];
        char path[256];
        char *headersEnd;
        char *body;
        int contentLength;
        int headerLen;
        int total;

        clientSock = accept(serverSock, NULL, NULL);
        if (clientSock == INVALID_SOCKET) {
            continue;
        }

        memset(request, 0, sizeof(request));
        received = recv(clientSock, request, sizeof(request) - 1, 0);
        if (received <= 0) {
            closesocket(clientSock);
            continue;
        }
        request[received] = '\0';

        headersEnd = strstr(request, "\r\n\r\n");
        if (headersEnd == NULL) {
            send_response(clientSock, 400, "Bad Request", "text/plain", "Requete invalide");
            closesocket(clientSock);
            continue;
        }

        headerLen = (int)(headersEnd - request) + 4;
        contentLength = get_content_length(request);
        total = received;

        while (total < headerLen + contentLength && total < (int)sizeof(request) - 1) {
            int more = recv(clientSock, request + total, sizeof(request) - 1 - total, 0);
            if (more <= 0) {
                break;
            }
            total += more;
            request[total] = '\0';
        }

        if (sscanf(request, "%7s %255s", method, path) != 2) {
            send_response(clientSock, 400, "Bad Request", "text/plain", "Ligne de requete invalide");
            closesocket(clientSock);
            continue;
        }

        body = request + headerLen;

        if (strcmp(path, "/taches") == 0 || strcmp(path, "/ajout") == 0 ||
            strncmp(path, "/faite/", 7) == 0 || strncmp(path, "/supprimer/", 11) == 0 ||
            strncmp(path, "/modifier/", 10) == 0) {
            handle_api(clientSock, method, path, body);
        } else {
            handle_static(clientSock, path);
        }

        closesocket(clientSock);
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}
