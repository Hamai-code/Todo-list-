#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int getNextId() {
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

int ajouterTache(char description[], char date[]) {
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

void listerTaches() {
    FILE *f = fopen(FILE_NAME, "r");
    Tache t;
    int first = 1;

    if (f == NULL) {
        printf("[]");
        return;
    }

    printf("[");
    while (readTask(f, &t)) {
        if (!first) {
            printf(",");
        }
        first = 0;
        printf("{\"id\":%d,\"description\":\"%s\",\"dateLimite\":\"%s\",\"etat\":%d}", t.id, t.description, t.dateLimite, t.etat);
    }
    printf("]");
    fclose(f);
}

int marquerFaite(int id) {
    return rewriteFileForOperation(id, "", "", 1, 1);
}

int supprimerTache(int id) {
    return rewriteFileForOperation(id, "", "", 0, 3);
}

int modifierTache(int id, char description[], char date[]) {
    sanitize(description);
    sanitize(date);
    return rewriteFileForOperation(id, description, date, 0, 2);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: taches [list|add|done|delete|update]\n");
        return 1;
    }

    if (strcmp(argv[1], "list") == 0) {
        listerTaches();
        return 0;
    }

    if (strcmp(argv[1], "add") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: taches add \"description\" \"JJ/MM/AAAA\"\n");
            return 1;
        }
        return ajouterTache(argv[2], argv[3]) ? 0 : 1;
    }

    if (strcmp(argv[1], "done") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: taches done id\n");
            return 1;
        }
        return marquerFaite(atoi(argv[2])) ? 0 : 1;
    }

    if (strcmp(argv[1], "delete") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: taches delete id\n");
            return 1;
        }
        return supprimerTache(atoi(argv[2])) ? 0 : 1;
    }

    if (strcmp(argv[1], "update") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Usage: taches update id \"description\" \"JJ/MM/AAAA\"\n");
            return 1;
        }
        return modifierTache(atoi(argv[2]), argv[3], argv[4]) ? 0 : 1;
    }

    fprintf(stderr, "Commande inconnue: %s\n", argv[1]);
    return 1;
}
