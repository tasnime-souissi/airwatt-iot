<p align="center">
  <img src="docs/logo/AirWatt-logo-clair.png" alt="AirWatt" width="460">
</p>

# AirWatt : ventilation intelligente pilotée par la qualité de l'air

Projet de semestre du module Internet des objets, classe 3 SETP, École Nationale des Sciences
et Technologies Avancées à Borj Cédria (ENSTAB), année universitaire 2026-2027.

AirWatt adapte la ventilation d'un atelier de caoutchouc à la pollution réelle. Un nœud ESP32
mesure les particules fines (PM2.5, PM10), la température et l'humidité, calcule un indice de
qualité de l'air et règle la vitesse du ventilateur de 20 à 100 %. Il transmet ses mesures en
**Bluetooth Low Energy (BLE)** à une passerelle, qui les publie sur la plateforme
**ThingsBoard** : tableau de bord, alarmes et commandes à distance.

La puissance d'un ventilateur variant comme le cube de sa vitesse, une ventilation à mi-vitesse
ne consomme que 12,5 % de la puissance nominale : AirWatt protège la santé des ouvriers tout en
réduisant la consommation d'énergie.

## Architecture

```
Nœud capteur ESP32  --BLE-->  Passerelle (PC ou ESP32)  --Wi-Fi / MQTT-->  ThingsBoard Cloud
        ^                                                                       |
        +--------------------- commandes (RPC -> écriture BLE) -----------------+
```

| Étage | Rôle | Technologies |
| --- | --- | --- |
| Nœud capteur | Mesurer, décider, agir ; publier les mesures | ESP32, PMS5003, BME280, serveur GATT BLE (NimBLE) |
| Passerelle | Relayer entre BLE et MQTT ; calculer l'énergie économisée | Python (bleak, paho-mqtt) ou ESP32 |
| Plateforme | Afficher, alerter, commander | ThingsBoard Cloud, MQTT, RPC |
| Jumeau numérique | Tester sans matériel, simuler les pannes | Wokwi, Wi-Fi et MQTT |

## Organisation du dépôt

| Dossier | Contenu |
| --- | --- |
| [`common/`](common/AirWattProtocol) | Protocole BLE partagé et lecture du capteur PMS5003 |
| [`node/`](node) | Firmware du nœud capteur (PlatformIO) |
| [`gateway_pc/`](gateway_pc) | Passerelle Python BLE vers ThingsBoard |
| [`gateway/`](gateway) | Passerelle ESP32, cliente BLE |
| [`wokwi/`](wokwi) | Jumeau numérique et capteur PMS5003 simulé |
| [`docs/`](docs) | Protocole, schéma de câblage, estimation de prix, plateforme, validation |

## Démarrage rapide

**Nœud capteur** (VS Code et PlatformIO, carte ESP32 reliée en USB) :

```
cd node
pio run -t upload
pio device monitor
```

**Passerelle sur PC** (Bluetooth activé) : créer `gateway_pc/secrets.json` à partir de
`secrets.example.json`, puis lancer `gateway_pc/lancer_passerelle.bat`.

**Jumeau numérique** : voir [`wokwi/README.md`](wokwi/README.md).

## Documentation

| Document | Contenu |
| --- | --- |
| [Protocole BLE et MQTT](docs/protocole-ble.md) | Service GATT, format des paquets, commandes, messages MQTT |
| [Schéma de câblage](docs/schema/schema-cablage.md) | Brochage, alimentation, choix des composants, bilan de consommation |
| [Estimation de prix](docs/prix/estimation-prix.md) | Coût du prototype et d'un déploiement en atelier |
| [Plateforme ThingsBoard](docs/plateforme-thingsboard.md) | Appareil, alarmes, tableau de bord, commandes |
| [Validation BLE](docs/validation-ble.md) | Essais sur carte réelle et chaîne complète |
| [Livrables](LIVRABLES.md) | Correspondance avec le cahier des charges |

## Résultats

- Communication BLE validée sur carte ESP32 réelle, depuis un PC et depuis un téléphone.
- Chaîne complète ESP32, BLE, passerelle, ThingsBoard : 212 messages transmis sans perte,
  commandes à distance appliquées par la carte.
- Pannes du capteur détectées et mises en sécurité (ventilateur à 100 %), validées sur le
  jumeau numérique.

## Équipe

Tasnime Souissi, Nour Elhouda Hafyen, Arij Zneti, Eya Ben Mohamed (3 SETP).
Encadrante : Dr. Ikbal Chaamkhi Msadaa.

Le jeton d'accès ThingsBoard n'est pas publié : les fichiers `secrets.*` sont exclus du dépôt
et les modèles `secrets.example.*` en donnent le format.
