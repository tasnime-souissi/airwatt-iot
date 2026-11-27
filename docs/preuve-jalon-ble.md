# Preuve du jalon « Technologie configurée et communicante » — BLE

Jalon noté du cahier des charges (chapitre 3) : la technologie tirée au sort (**BLE**) est
configurée et communique avec le microcontrôleur. Validé le **27/11/2026** sur matériel réel.

| Élément | Valeur |
| --- | --- |
| Carte | ESP32 DevKit, puce **ESP32-D0WD-V3** (révision 3.1), Wi-Fi + Bluetooth |
| Firmware | `node/` (configuration `carte_seule`, NimBLE-Arduino 2.x) |
| Nom BLE annoncé | `AirWatt-Node` — adresse `08:A6:F7:22:72:86` |
| Type | **LE only** (Bluetooth Low Energy ; Bluetooth classique BR/EDR non supporté) |
| Service GATT | `6e7a0001-3c1b-4a5e-9d2f-a1b2c3d4e5f6` |
| Mesures | `6e7a0002-…` — READ, NOTIFY toutes les 2 s, paquet de 12 octets |
| Commandes | `6e7a0003-…` — WRITE, 3 octets |

## 1. Vérification avec un téléphone (nRF Connect)

Captures dans le dossier Drive `05 - Demo / Preuves jalon BLE` :

| Capture | Ce qu'elle montre |
| --- | --- |
| 1 — Scan | `AirWatt-Node`, « LE only », drapeaux « LE General Discoverable, BR/EDR Not Supported », UUID 128 bits du service AirWatt |
| 2 — Connexion | État « CONNECTED », service `6e7a0001-…` découvert |
| 3 — Caractéristiques | `6e7a0002` (NOTIFY, READ) et `6e7a0003` (WRITE) ; lecture d'un paquet de 12 octets |
| 4a à 4d — Notifications | « Notifications enabled » ; valeurs reçues en direct pendant un pic de pollution déclenché par le bouton BOOT |

Décodage des notifications reçues (format défini dans `docs/contrat-donnees-ble.md`,
petit-boutiste) :

| Paquet n° | Octets reçus | PM2.5 | PM10 | Temp. | Hum. | Ventilateur | Air |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 81 (lecture) | `0A-00-10-00-1E-01-36-14-00-01-51-00` | 10 | 16 | 28,6 °C | 54 % | 20 % | vert |
| 274 | `10-00-19-00-20-01-36-27-01-01-12-01` | 16 | 25 | 28,8 °C | 54 % | 39 % | orange |
| 278 | `2E-00-4A-00-1A-01-39-64-02-01-16-01` | 46 | 74 | 28,2 °C | 57 % | 100 % | rouge |
| 281 | `41-00-69-00-18-01-36-64-02-01-19-01` | 65 | 105 | 28,0 °C | 54 % | 100 % | rouge |
| 283 | `3F-00-65-00-22-01-37-64-02-01-1B-01` | 63 | 101 | 29,0 °C | 55 % | 100 % | rouge |

Exemple de lecture : `41-00` = 0x0041 = 65 µg/m³ ; `64` = 100 % ; `02` = rouge ;
`01` = mode automatique ; `19-01` = 0x0119 = paquet n° 281.

## 2. Vérification depuis le PC (client BLE Python, bibliothèque bleak)

```
Trouvé : AirWatt-Node | adresse 08:A6:F7:22:72:86 | signal -34 dBm
Service AirWatt : 6e7a0001-3c1b-4a5e-9d2f-a1b2c3d4e5f6
  caractéristique 6e7a0002-… ['read', 'notify']
  caractéristique 6e7a0003-… ['write']
  notification 12 octets : 0a 00 10 00 1c 01 38 14 00 01 3d 00 -> seq=61 PM2.5=10 PM10=16 T=28.4 H=56 ventilateur=20% aqi=0 auto=True
```

## 3. Chaîne complète : nœud → BLE → passerelle PC → ThingsBoard

Passerelle `gateway_pc/` (Bluetooth du PC) :

- plus de 200 mesures relayées en BLE vers ThingsBoard, **aucun paquet perdu** (compteur `seq`),
  signal BLE ≈ −33 dBm à proximité ;
- pic de pollution (bouton BOOT) visible sur le tableau de bord : PM2.5 jusqu'à 65 µg/m³,
  état « Mauvais », ventilateur à 100 %, alarme Critique ;
- commandes envoyées depuis le tableau de bord et reçues par la carte en BLE :

```
18:25:26 [RPC] setMode(False)  -> transmis au nœud   (carte : mode manuel)
18:26:04 [RPC] setFan(71.11)   -> transmis au nœud   (carte : ventilateur 71 %)
18:26:53 [RPC] setMode(True)   -> transmis au nœud   (carte : retour en automatique)
```

Le lien BLE est donc bidirectionnel : mesures du nœud vers la passerelle, commandes de la
passerelle vers le nœud.
