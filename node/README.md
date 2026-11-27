# Nœud capteur (ESP32 n°1)

Rôle : mesurer l'air, régler le ventilateur selon la pollution, exposer le service GATT
AirWatt en BLE. Responsable : Membre 2. Format des données : `../docs/contrat-donnees-ble.md`.

## Deux configurations (`platformio.ini`)

| Configuration | Matériel | Mesures | Interaction |
| --- | --- | --- | --- |
| `carte_seule` (par défaut) | ESP32 seule | Simulées dans la carte | Bouton **BOOT** = pic de pollution ; LED intégrée (GPIO 2) : luminosité = vitesse du ventilateur, clignote si l'air est mauvais, clignote vite si capteur en défaut |
| `montage_complet` | ESP32 + PMS5003 + BME280 + ventilateur, LED RGB, buzzer | Réelles (moyenne sur 5 trames PMS5003, vérification de la puce BME280) | Actionneurs réels |

Dans les deux cas, le **BLE est réel** : même service, mêmes paquets de 12 octets.

## Téléverser et tester

```
pio run -t upload                        (carte seule)
pio run -e montage_complet -t upload     (montage complet)
pio device monitor
```

Exemple de ligne du moniteur série (toutes les 2 s) :

```
[#12] PM2.5=9 PM10=14 T=28.4°C H=55% ventilateur=20% air=vert capteur=ok mode=auto passerelle=non
```

Commandes du moniteur : `pic` (pic de pollution), `auto` (pic automatique toutes les 3 min),
`etat`. Procédure complète de test : `../docs/checklist-carte.md`.

| Broche | Rôle |
| --- | --- |
| GPIO 0 | Bouton BOOT (pic de pollution en mode carte seule) |
| GPIO 2 | LED intégrée |
| GPIO 25 | PWM du ventilateur (grille du MOSFET) |
| GPIO 26, 27, 32 | LED RGB (rouge, vert, bleu) |
| GPIO 33 | Buzzer |
| GPIO 16, 17 | PMS5003 (UART2) |
| GPIO 21, 22 | BME280 (I2C) |
