# Contrat de données BLE et MQTT

Règle : toute modification de ce fichier est annoncée et validée par les membres 2 (nœud)
et 3 (passerelle) **avant** d'être codée.

## Service GATT « AirWatt » (le nœud capteur est le serveur BLE)

Nom BLE annoncé par le nœud : `AirWatt-Node`

| Élément | UUID | Propriétés | Contenu |
| --- | --- | --- | --- |
| Service | `6e7a0001-3c1b-4a5e-9d2f-a1b2c3d4e5f6` | — | Service AirWatt |
| Mesures | `6e7a0002-3c1b-4a5e-9d2f-a1b2c3d4e5f6` | READ, NOTIFY (toutes les 2 s) | Paquet de 12 octets |
| Commande | `6e7a0003-3c1b-4a5e-9d2f-a1b2c3d4e5f6` | WRITE | 3 octets : code + valeur |

Pourquoi un paquet binaire : sans négociation de MTU, une notification BLE ne transporte que
20 octets utiles. Une trame texte comme `PM25:22;PM10:40;TEMP:28.5;HUM:55` (32 caractères)
serait coupée.

## Paquet « Mesures » (12 octets, petit-boutiste)

| Octets | Champ | Type | Unité et codage |
| --- | --- | --- | --- |
| 0–1 | pm25 | uint16 | µg/m³, moyenne sur 5 lectures |
| 2–3 | pm10 | uint16 | µg/m³, moyenne sur 5 lectures |
| 4–5 | temp | int16 | 0,1 °C (285 = 28,5 °C) |
| 6 | hum | uint8 | % d'humidité relative |
| 7 | fan | uint8 | vitesse du ventilateur en % |
| 8 | aqi | uint8 | 0 = vert, 1 = orange, 2 = rouge |
| 9 | flags | uint8 | bit 0 = mode auto, bit 1 = erreur PMS5003, bit 2 = erreur BME280 |
| 10–11 | seq | uint16 | compteur de paquets (détecte les pertes BLE) |

Structure C commune aux deux firmwares :

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
} AirWattPacket;   // sizeof == 12
```

## Commandes (caractéristique WRITE, 3 octets)

| Octet 0 (code) | Commande | Octets 1–2 (uint16, petit-boutiste) |
| --- | --- | --- |
| 0x01 | setMode | 0 = manuel, 1 = auto |
| 0x02 | setFan | 0 à 100 (%), pris en compte en mode manuel |
| 0x03 | setThreshold | seuil rouge PM2.5 en µg/m³ |

## Seuils de l'indice de qualité de l'air (valeurs de départ)

| État | PM2.5 (µg/m³) | PM10 (µg/m³) |
| --- | --- | --- |
| Vert (0) | < 15 | < 45 |
| Orange (1) | 15 à 35 | 45 à 100 |
| Rouge (2) | > 35 | > 100 |

15 et 45 µg/m³ : repères 24 h de l'OMS (lignes directrices 2021). 35 et 100 : repères
pédagogiques du prototype.

## Message MQTT publié par la passerelle vers ThingsBoard

Sujet : `v1/devices/me/telemetry` (jeton de l'appareil comme nom d'utilisateur MQTT).

```json
{"pm25":22,"pm10":40,"temp":28.5,"hum":55,"fan":40,"aqi":0,"mode":"auto","energy_wh":1.8,"saved_wh":6.4,"saved_pct":78,"rssi":-67,"seq":1532}
```

Commandes reçues sur `v1/devices/me/rpc/request/+` : `setMode`, `setFan`, `setThreshold`,
traduites par la passerelle en écriture BLE sur la caractéristique Commande.
