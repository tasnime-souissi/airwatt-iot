# Nœud capteur

Firmware de l'ESP32 qui mesure l'air, règle le ventilateur selon la pollution et expose le
service GATT AirWatt en BLE (voir le [protocole BLE](../docs/protocole-ble.md)).

## Configurations

| Configuration | Matériel | Mesures | Signalisation |
| --- | --- | --- | --- |
| `carte_seule` (par défaut) | ESP32 seule | Simulées dans la carte ; bouton BOOT = pic de pollution | LED intégrée : luminosité = vitesse du ventilateur ; clignotement = air mauvais ou capteur en défaut |
| `montage_complet` | ESP32, PMS5003, BME280, ventilateur, LED RGB, buzzer | Réelles (moyenne sur 5 trames PMS5003, puce BME280 vérifiée) | LED RGB et buzzer |

Dans les deux configurations, la communication BLE est réelle : même service, mêmes paquets.

## Compiler et téléverser

```
pio run -t upload                        # configuration carte_seule
pio run -e montage_complet -t upload     # configuration montage_complet
pio device monitor                       # moniteur série, 115200 bauds
```

Le moniteur affiche une ligne toutes les 2 s :

```
[#12] PM2.5=9 PM10=14 T=28.4°C H=55% ventilateur=20% air=vert capteur=ok mode=auto passerelle=non
```

Commandes du moniteur (configuration `carte_seule`) : `pic` déclenche un pic de pollution,
`auto` active un pic automatique toutes les 3 min, `etat` affiche l'état courant.

## Brochage

| Broche | Rôle |
| --- | --- |
| GPIO 0 | Bouton BOOT (pic de pollution, configuration `carte_seule`) |
| GPIO 2 | LED intégrée |
| GPIO 16, 17 | PMS5003 (UART2) |
| GPIO 21, 22 | BME280 (I2C) |
| GPIO 25 | PWM du ventilateur (grille du MOSFET) |
| GPIO 26, 27, 32 | LED RGB (rouge, vert, bleu) |
| GPIO 33 | Buzzer |

Le câblage complet est décrit dans le [schéma de câblage](../docs/schema/schema-cablage.md).
