# Protocole BLE et MQTT

Ce document définit les échanges entre le nœud capteur, la passerelle et ThingsBoard. Il est
implémenté une seule fois dans [`common/AirWattProtocol/airwatt_protocol.h`](../common/AirWattProtocol/airwatt_protocol.h),
partagé par tous les programmes du projet.

## Service GATT AirWatt

Le nœud capteur est le serveur BLE. Il s'annonce sous le nom `AirWatt-Node` : l'UUID du service
figure dans l'annonce et le nom dans la réponse au scan.

| Élément | UUID | Propriétés | Contenu |
| --- | --- | --- | --- |
| Service | `6e7a0001-3c1b-4a5e-9d2f-a1b2c3d4e5f6` | | Service AirWatt |
| Mesures | `6e7a0002-3c1b-4a5e-9d2f-a1b2c3d4e5f6` | READ, NOTIFY (toutes les 2 s) | Paquet de 12 octets |
| Commandes | `6e7a0003-3c1b-4a5e-9d2f-a1b2c3d4e5f6` | WRITE | 3 octets : code et valeur |

Le format binaire tient dans une seule notification : sans négociation du MTU, une notification
BLE transporte au plus 20 octets utiles.

## Paquet de mesures (12 octets, petit-boutiste)

| Octets | Champ | Type | Unité et codage |
| --- | --- | --- | --- |
| 0-1 | `pm25` | uint16 | µg/m³, moyenne sur 5 lectures |
| 2-3 | `pm10` | uint16 | µg/m³, moyenne sur 5 lectures |
| 4-5 | `temp_x10` | int16 | 0,1 °C (285 = 28,5 °C) |
| 6 | `hum` | uint8 | % d'humidité relative |
| 7 | `fan` | uint8 | vitesse du ventilateur, 0 à 100 % |
| 8 | `aqi` | uint8 | 0 = bon, 1 = moyen, 2 = mauvais |
| 9 | `flags` | uint8 | bit 0 = mode auto, bit 1 = panne PMS5003, bit 2 = panne BME280 |
| 10-11 | `seq` | uint16 | numéro de séquence, pour détecter les paquets perdus |

```c
typedef struct __attribute__((packed)) {
  uint16_t pm25;
  uint16_t pm10;
  int16_t  temp_x10;
  uint8_t  hum;
  uint8_t  fan;
  uint8_t  aqi;
  uint8_t  flags;
  uint16_t seq;
} Packet;   // sizeof == 12
```

## Commandes (3 octets)

| Code | Commande | Valeur (uint16, petit-boutiste) |
| --- | --- | --- |
| `0x01` | Mode | 0 = manuel, 1 = automatique |
| `0x02` | Vitesse du ventilateur | 0 à 100 %, appliquée en mode manuel |
| `0x03` | Seuil rouge PM2.5 | 16 à 500 µg/m³ |

Exemple : `02 47 00` règle le ventilateur à 71 % (0x0047).

## Indice de qualité de l'air

| Indice | PM2.5 (µg/m³) | PM10 (µg/m³) |
| --- | --- | --- |
| Bon (0) | < 15 | < 45 |
| Moyen (1) | 15 à 35 | 45 à 100 |
| Mauvais (2) | > 35 (seuil réglable) | > 100 |

Les valeurs de 15 et 45 µg/m³ sont les repères sur 24 heures des lignes directrices de l'OMS
(2021).

## Messages MQTT vers ThingsBoard

Télémétrie publiée sur `v1/devices/me/telemetry` (le jeton de l'appareil sert de nom
d'utilisateur MQTT) :

```json
{"pm25": 9, "pm10": 14, "aqi": 0, "temp": 28.4, "hum": 55, "fan": 20, "mode": "auto",
 "energy_wh": 0.0021, "saved_wh": 0.2381, "saved_pct": 99, "pms_ok": true,
 "seq": 812, "lost": 0, "link": "ble", "rssi": -58}
```

Les valeurs de particules sont omises lorsque le PMS5003 est en panne, et celles d'ambiance
lorsque le BME280 l'est.

Commandes reçues sur `v1/devices/me/rpc/request/+` : `setMode`, `setFan`, `setThreshold` et
leurs lectures `getMode`, `getFan`, `getThreshold`. La passerelle les traduit en écritures BLE et
ne confirme la commande que si l'écriture a réussi.
