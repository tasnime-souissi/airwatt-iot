# Jumeau numérique Wokwi

Même logique que le nœud capteur (lecture PMS5003, indice de qualité de l'air, régulation
du ventilateur, calcul d'énergie), avec un envoi direct en Wi-Fi/MQTT vers ThingsBoard.
Wokwi ne simule pas le BLE : sur le vrai prototype, ce saut passe par le BLE et la passerelle.
Responsable : Membre 4.

| Élément | Composant Wokwi | Broche ESP32 | Sur le vrai prototype |
| --- | --- | --- | --- |
| Particules PM2.5, PM10 | Puce personnalisée `pms5003` (curseurs) | GPIO 16 (RX2), 17 (TX2) | PMS5003 |
| Température, humidité | DHT22 | GPIO 15 | BME280 (I2C, GPIO 21, 22) |
| Ventilateur | LED bleue (luminosité = vitesse) | GPIO 25 | Ventilateur 12 V via MOSFET |
| Indicateur vert / orange / rouge | LED RGB | GPIO 26, 27, 32 | LED RGB |
| Alarme | Buzzer | GPIO 33 | Buzzer |

La puce `pms5003` envoie chaque seconde une **vraie trame PMS5003** de 32 octets en UART.
Le programme la lit avec le même code que le nœud réel (`pms5003.h`) : la lecture du
capteur est donc déjà testée en simulation.

## Curseurs de la puce PMS5003 (cliquer sur la puce pendant la simulation)

| Curseur | Valeur | Effet |
| --- | --- | --- |
| PM2.5 / PM10 manuel | 0 à 200 / 0 à 300 | Valeurs envoyées quand le scénario est à 0 |
| Scénario auto | 1 | Cycle de 2 min rejoué en boucle : air propre, montée, pic à 65 µg/m³, retour au calme |
| Panne | 1 | Capteur muet : plus aucune trame |
| Panne | 2 | Trames abîmées (somme de contrôle fausse), rejetées par le firmware |

## Scénarios de démonstration

| Scénario | Réglage | Résultat attendu |
| --- | --- | --- |
| Pic de pollution | Scénario auto = 1 (ou PM2.5 manuel à 60) | LED orange puis rouge, ventilateur à 100 %, buzzer, alarme Critique |
| Retour au calme | Laisser tourner le scénario, ou PM2.5 manuel à 10 | Alarmes effacées, ventilateur au ralenti |
| Capteur en panne | Panne = 1 | Après 5 s : LED bleue, `pms_ok` = faux, plus de valeurs de particules envoyées, ventilateur à 100 % par sécurité, alarme « Capteur PMS5003 en défaut » |
| Liaison série perturbée | Panne = 2 | Même résultat : le firmware rejette les trames dont la somme de contrôle est fausse |
| Fin de panne | Panne = 0 | Mesures à nouveau valides, alarme capteur effacée |
| Nœud hors ligne | Arrêter la simulation 60 s | Alarme « Noeud hors ligne » |

## Fichiers du projet Wokwi

| Fichier | Rôle |
| --- | --- |
| `sketch.ino` | Programme de l'ESP32 |
| `diagram.json` | Schéma de câblage de la simulation |
| `libraries.txt` | Bibliothèques Arduino à charger |
| `pms5003.chip.json`, `pms5003.chip.c` | Capteur PMS5003 simulé |
| `airwatt_protocol.h`, `pms5003.h` | Copies de `../common/AirWattProtocol/` |
| `secrets.h` | Serveur et jeton ThingsBoard (privé, non publié) |
| `secrets.example.h` | Modèle de `secrets.h` sans le jeton |

## Lancer la simulation sur wokwi.com

1. Suivre `../docs/guide-thingsboard.md` (sections 1 et 2) pour obtenir le jeton de l'appareil.
2. Sur [wokwi.com](https://wokwi.com), créer un nouveau projet **ESP32** (Arduino).
3. Remplacer le contenu de `sketch.ino` et `diagram.json` par ceux de ce dossier.
4. Avec la flèche à côté des onglets > **New file**, créer et remplir avec ce dossier :
   `libraries.txt`, `airwatt_protocol.h`, `pms5003.h`, `pms5003.chip.json`, `pms5003.chip.c`,
   `secrets.h`.
5. `secrets.h` contient le serveur et le jeton ThingsBoard. Il est ignoré par Git ; sur un
   autre PC, le recréer à partir de `secrets.example.h`.
6. Lancer la simulation, cliquer sur la puce PMS5003 et bouger le curseur PM2.5 :
   la LED RGB passe du vert à l'orange puis au rouge, la LED bleue (ventilateur) s'intensifie,
   le buzzer sonne au passage au rouge.
7. Enregistrer le projet et garder son lien (rapport, pitch). Projet **non public** tant
   qu'il contient le vrai jeton.

Après chaque modification de `../common/AirWattProtocol/`, recopier les deux en-têtes ici.

## Si le moniteur affiche « (hors ligne, non envoyé) »

Le Wi-Fi simulé de Wokwi passe par le serveur `netwi.wokwi.com`, qui a été instable le
27/11/2026 (il ne répondait plus par moments). Le programme continue de fonctionner en local
et réessaie la connexion toutes les 15 s : il suffit de laisser tourner la simulation.

Pour le Demo Day : enregistrer à l'avance une **vidéo** de la simulation connectée et du
tableau de bord ThingsBoard, à montrer si le Wi-Fi de Wokwi tombe pendant la démo.
