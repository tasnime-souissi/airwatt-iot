# AirWatt — Ventilation intelligente pilotée par la qualité de l'air

Projet IoT de semestre, classe SETP (thème : énergie et durabilité).
Technologie imposée : **BLE** (Bluetooth Low Energy).

Un nœud capteur ESP32 mesure les particules (PM2.5, PM10), la température et l'humidité,
puis règle la vitesse d'un ventilateur selon la pollution. Il envoie ses mesures en BLE à une
passerelle ESP32, qui les publie en MQTT sur ThingsBoard (tableau de bord, alarmes, commandes).

```
Prototype réel :
Nœud capteur (ESP32) --BLE--> Passerelle (ESP32) --Wi-Fi/MQTT--> ThingsBoard
       ^                                                             |
       +----------------- commandes (RPC -> écriture BLE) -----------+

Simulation (Wokwi ne simule pas le BLE) :
Jumeau Wokwi (ESP32 + PMS5003 simulé) --Wi-Fi/MQTT--> ThingsBoard
```

## Structure du dossier

| Dossier | Contenu | Responsable |
| --- | --- | --- |
| `node/` | Firmware du nœud capteur (capteurs, ventilateur, serveur BLE) | Membre 2 |
| `gateway/` | Firmware de la passerelle ESP32 (client BLE ; MQTT à ajouter) | Membre 3 |
| `gateway_pc/` | Passerelle sur PC en Python (BLE -> ThingsBoard), si une seule ESP32 | Membre 3 |
| `wokwi/` | Jumeau numérique Wokwi, avec un PMS5003 simulé | Membre 4 |
| `common/` | Code partagé : protocole BLE, lecture du PMS5003 | Membres 2 et 3 |
| `docs/` | Contrat de données, guide ThingsBoard, schéma, prix, rapport, pitch | Tous |

Outils : VS Code + extension PlatformIO. `node/` et `gateway/` sont deux projets PlatformIO
indépendants (compiler : `pio run`, téléverser : `pio run -t upload`).

## Documents de référence

- `IoT_Semester_Project.pdf` : cahier des charges de l'enseignante.
- `20261121_134220.docx` : proposition initiale d'Arij.
- `docs/contrat-donnees-ble.md` : format des paquets BLE et MQTT (à respecter par tous).
- `docs/guide-thingsboard.md` : compte, appareil, alarmes, tableau de bord.

## Avancement

### Phase A — projet complet en simulation

- [x] A1. Structure du projet, contrat de données
- [x] A2. Jumeau Wokwi : PMS5003 simulé, DHT22, ventilateur, LED, buzzer, MQTT, commandes RPC
      — fonctionne sur wokwi.com ; réseau non bloquant (alertes locales même sans Wi-Fi)
- [x] A3. ThingsBoard : compte, appareil, alarmes, tableau de bord (`docs/guide-thingsboard.md`)
      — vérifié avec des mesures de test envoyées à la main
- [x] A4. Essai de bout en bout : Wokwi → ThingsBoard → commandes
      — validé le 27/11/2026 : mesures en direct, commandes mode et vitesse, pic de pollution
      avec alarme Critique
- [x] A5a. Schéma de câblage du montage complet (`docs/schema/`)
- [x] A5b. Estimation de prix sourcée (`docs/prix/estimation-prix.xlsx`)
- [ ] A5c. Rapport et pitch du Demo Day

### Phase B — prototype réel

Matériel disponible : **une ESP32 seule** (pas de capteurs ni d'actionneurs). Solution hybride :
BLE réel sur la carte, capteurs simulés dans la carte, bouton BOOT = pic de pollution, LED
intégrée = vitesse du ventilateur et alerte, PC = passerelle BLE -> ThingsBoard.

- [x] B1. Nœud « carte seule » (BLE, bouton BOOT, LED intégrée) et « montage complet »
      (PMS5003 + BME280) — compilés, **à tester sur la carte**
- [x] B2. Passerelle PC en Python — testée hors ligne, **à tester avec la carte**
- [ ] B3. Jalon noté : BLE vérifié avec nRF Connect (`docs/checklist-carte.md`, étape 2)
- [ ] B4. Chaîne réelle nœud -> BLE -> PC -> ThingsBoard (`docs/checklist-carte.md`, étape 3)
- [ ] B5. (Si capteurs disponibles) montage complet, configuration `montage_complet`
