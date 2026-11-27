# Passerelle ESP32

Firmware d'une seconde carte ESP32 jouant le rôle de client BLE : elle découvre le nœud
AirWatt, s'abonne à ses mesures, calcule l'énergie économisée et relaie les commandes vers le
nœud. Les mesures sont publiées en JSON sur le moniteur série, dans le format utilisé par
ThingsBoard ; l'envoi MQTT est la prochaine évolution de ce firmware. En attendant, la
[passerelle sur PC](../gateway_pc) assure le relais vers la plateforme.

## Compiler et téléverser

Le nœud doit être allumé à proximité.

```
pio run -t upload
pio device monitor
```

La passerelle affiche un message JSON toutes les 2 s :

```
{"pm25":9,"pm10":14,"temp":28.4,"hum":55,"fan":20,"aqi":0,"mode":"auto","energy_wh":0.0001,"saved_wh":0.0130,"saved_pct":99,"rssi":-58,"seq":12,"lost":0}
```

Commandes du moniteur, transmises au nœud en BLE : `mode auto`, `mode manuel`, `fan 60`,
`seuil 40`.

`rssi` indique la puissance du signal BLE reçu (environ -50 dBm à 1 m) et `lost` le nombre de
paquets perdus.
