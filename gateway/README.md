# Passerelle (ESP32 n°2)

Rôle : trouver le nœud AirWatt en BLE, s'abonner à ses mesures, calculer l'énergie
économisée, relayer les commandes. Responsable : Membre 3.
Format des données : `../docs/contrat-donnees-ble.md`.

**État actuel (étape 2) :** les mesures sont affichées en JSON sur le moniteur série.
L'envoi MQTT vers ThingsBoard arrive à l'étape 5.

## Téléverser et tester

Le nœud doit être allumé à proximité. Dans un terminal ouvert dans ce dossier :

```
pio run -t upload
pio device monitor
```

La passerelle cherche le nœud, s'y connecte, puis affiche un JSON toutes les 2 s :

```
{"pm25":9,"pm10":14,"temp":28.4,"hum":55,"fan":20,"aqi":0,"mode":"auto","energy_wh":0.0001,"saved_wh":0.0130,"saved_pct":99,"rssi":-58,"seq":12,"lost":0}
```

Commandes à taper dans le moniteur (envoyées au nœud en BLE) :
`mode auto`, `mode manuel`, `fan 60`, `seuil 40`.

`rssi` = force du signal BLE (proche de -50 dBm à 1 m) ; `lost` = paquets perdus.
Avec une seule carte, téléverser d'abord le nœud et le tester avec nRF Connect.
