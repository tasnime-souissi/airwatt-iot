# Jumeau numérique Wokwi

Simulation du nœud capteur sur [Wokwi](https://wokwi.com/projects/481848859547936769). Elle
reprend la logique du firmware réel (lecture du PMS5003, indice de qualité de l'air, régulation
du ventilateur, calcul de l'énergie) et publie directement en Wi-Fi et MQTT vers ThingsBoard,
Wokwi ne simulant pas le BLE.

| Élément | Composant Wokwi | Broche ESP32 | Prototype réel |
| --- | --- | --- | --- |
| Particules PM2.5, PM10 | Puce personnalisée `pms5003` | GPIO 16 (RX2), 17 (TX2) | PMS5003 |
| Température, humidité | DHT22 | GPIO 15 | BME280 (I2C, GPIO 21, 22) |
| Ventilateur | LED bleue (luminosité = vitesse) | GPIO 25 | Ventilateur 12 V via MOSFET |
| Indicateur de qualité de l'air | LED RGB | GPIO 26, 27, 32 | LED RGB |
| Alarme | Buzzer | GPIO 33 | Buzzer |

La puce `pms5003` émet chaque seconde une véritable trame PMS5003 de 32 octets en UART ; le
programme la lit avec le même code que le nœud réel ([`pms5003.h`](pms5003.h)).

## Réglages de la puce PMS5003

Cliquer sur la puce pendant la simulation :

| Curseur | Valeur | Effet |
| --- | --- | --- |
| PM2.5 / PM10 | 0 à 200 / 0 à 300 | Concentrations envoyées lorsque le scénario est à 0 |
| Scénario auto | 1 | Cycle de 2 min rejoué en boucle : air propre, montée, pic à 65 µg/m³, retour au calme |
| Panne | 1 | Capteur muet : plus aucune trame |
| Panne | 2 | Trames corrompues (somme de contrôle fausse), rejetées par le firmware |

## Comportements à observer

| Scénario | Réglage | Résultat attendu |
| --- | --- | --- |
| Pic de pollution | Scénario auto = 1, ou PM2.5 à 60 | LED orange puis rouge, ventilateur à 100 %, buzzer, alarme critique |
| Retour au calme | PM2.5 à 10 | Alarmes effacées, ventilateur au ralenti |
| Capteur en panne | Panne = 1 | Après 5 s : LED bleue, `pms_ok` faux, ventilateur à 100 %, alarme de panne |
| Trames corrompues | Panne = 2 | Trames rejetées, même mise en sécurité |
| Nœud hors ligne | Simulation arrêtée 60 s | Alarme « Nœud hors ligne » |

## Fichiers

| Fichier | Rôle |
| --- | --- |
| `sketch.ino` | Programme de l'ESP32 |
| `diagram.json` | Câblage de la simulation |
| `libraries.txt` | Bibliothèques Arduino |
| `pms5003.chip.json`, `pms5003.chip.c` | Capteur PMS5003 simulé |
| `airwatt_protocol.h`, `pms5003.h` | Copies de [`common/AirWattProtocol/`](../common/AirWattProtocol) |
| `secrets.example.h` | Modèle du fichier privé `secrets.h` (serveur et jeton ThingsBoard) |

## Créer le projet sur wokwi.com

1. Créer un projet **ESP32** (Arduino) sur [wokwi.com](https://wokwi.com).
2. Remplacer `sketch.ino` et `diagram.json` par ceux de ce dossier.
3. Ajouter les fichiers `libraries.txt`, `airwatt_protocol.h`, `pms5003.h`, `pms5003.chip.json`
   et `pms5003.chip.c`.
4. Créer `secrets.h` à partir de `secrets.example.h`, avec le jeton de l'appareil ThingsBoard.
5. Lancer la simulation et agir sur les curseurs de la puce PMS5003.

La connexion réseau est non bloquante : en cas de coupure, la ventilation et les alertes locales
continuent et la connexion est retentée automatiquement.
