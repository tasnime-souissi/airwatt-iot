# AirWatt — Correspondance avec le cahier des charges

Projet IoT de semestre, classe SETP · technologie imposée : **BLE**.
Code et documentation technique : ce dépôt GitHub. Documents à lire et vidéos : dossier
Google Drive de l'équipe (lien ci-dessous).

## Livrables (section 4 du sujet)

| Livrable demandé | Où le trouver | État |
| --- | --- | --- |
| Fiche sujet | Drive : `01 — Fiche sujet` | Rendue |
| Prototype fonctionnel (mesure et actionnement via BLE) | `node/` (nœud ESP32, serveur GATT BLE), `gateway_pc/` et `gateway/` (passerelles), `wokwi/` (jumeau numérique) | Simulation validée ; BLE sur carte : voir jalon ci-dessous |
| Schéma de câblage | `docs/schema/schema-cablage.md` (+ PNG et SVG) | Terminé |
| Plateforme IoT et tableau de bord | ThingsBoard Cloud : `docs/guide-thingsboard.md`, sauvegarde `docs/thingsboard/tableau-de-bord-airwatt.json` | Terminé, validé en direct |
| Estimation de prix | `docs/prix/estimation-prix.md` et `docs/prix/estimation-prix.xlsx` | Terminé (prix relevés le 27/11/2026) |
| Pitch et démo | Drive : `05 — Démo` (vidéo), `06 — Pitch` (diapositives) | Vidéo prête ; diapositives en cours |

## Jalons (section 5)

| Jalon | Preuve | État |
| --- | --- | --- |
| Technologie configurée et communicante (chapitre 3) | Procédure `docs/checklist-carte.md` ; captures et vidéo nRF Connect sur Drive `05 — Démo` | À réaliser à la réception de la carte |
| Demo Day | Démo sur ESP32 (BLE réel) + tableau de bord ThingsBoard ; Wokwi et vidéo en secours | En préparation |

## Architecture

```
Nœud ESP32 --BLE (imposé)--> Passerelle (PC ou ESP32) --Wi-Fi / MQTT--> ThingsBoard
```

| Dossier | Contenu |
| --- | --- |
| `common/` | Protocole BLE partagé (paquet de 12 octets) et lecture du PMS5003 |
| `node/` | Firmware du nœud : configurations `carte_seule` et `montage_complet` |
| `gateway_pc/` | Passerelle Python BLE → ThingsBoard (équipe avec une seule ESP32) |
| `gateway/` | Passerelle ESP32 (client BLE) |
| `wokwi/` | Jumeau numérique : PMS5003 simulé, scénario automatique, pannes simulées |
| `docs/` | Contrat de données, schéma, prix, guide ThingsBoard, checklist, état de conformité |

## Liens

| Ressource | Lien |
| --- | --- |
| Dossier Google Drive | *(à compléter)* |
| Projet Wokwi | https://wokwi.com/projects/481848859547936769 |
| Tableau de bord ThingsBoard (lecture seule) | *(à compléter)* |

Les fichiers `secrets.h` et `secrets.json` (jeton ThingsBoard) ne sont pas publiés ; les
modèles `secrets.example.*` indiquent leur format.
