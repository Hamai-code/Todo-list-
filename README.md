# Projet 4 - Gestionnaire de taches collaboratif (C + HTML/CSS/JS)

Projet realise sans Python/PHP.

## Technologies
- Backend/API + serveur web: C (`server_c.c`)
- Logique de persistance taches: C (`taches.c`)
- Frontend: `index.html`, `style.css`, `script.js`

## Fonctionnalites obligatoires (C)
Structure `Tache`:
- `id` (auto-incremente)
- `description`
- `dateLimite` (JJ/MM/AAAA)
- `etat` (0 ou 1)

Operations:
- `ajouterTache`
- `supprimerTache`
- `marquerFaite`
- `listerTaches`
- `modifierTache` (extension interface)

Persistance:
- Fichier texte `taches.txt`
- Format: `id|description|dateLimite|etat`

## API exposee par le serveur C
- `GET /taches`
- `POST /ajout`
- `POST /faite/<id>`
- `DELETE /supprimer/<id>`
- `PUT /modifier/<id>`

## Lancement
1. Compiler le gestionnaire de taches:
```bash
gcc taches.c -o taches.exe
```

2. Compiler le serveur C (Windows):
```bash
gcc server_c.c -o server_c.exe -lws2_32
```

3. Lancer le serveur:
```bash
./server_c.exe
```

4. Ouvrir dans le navigateur:
- `http://127.0.0.1:5000`

## Organisation
- Etudiant 1: `taches.c` (manipulation + persistance)
- Etudiant 2: `server_c.c` + frontend (`index.html`, `style.css`, `script.js`)
