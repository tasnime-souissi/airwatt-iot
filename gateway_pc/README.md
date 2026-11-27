# Passerelle sur PC

Programme Python qui relie le nœud capteur à ThingsBoard : le Bluetooth du PC se connecte au
nœud `AirWatt-Node`, décode les paquets de 12 octets, calcule l'énergie économisée et publie
les mesures en MQTT. Les commandes du tableau de bord sont retransmises au nœud en BLE.

## Lancer

1. Programmer le nœud ESP32 (dossier [`node/`](../node)) et le mettre sous tension.
2. Activer le Bluetooth du PC.
3. Créer `secrets.json` à partir de `secrets.example.json` (serveur et jeton ThingsBoard).
4. Lancer `lancer_passerelle.bat`. À la première exécution, il installe les bibliothèques
   Python (`bleak`, `paho-mqtt`) dans un environnement dédié.

Affichage attendu :

```
[MQTT] Connecté à thingsboard.cloud
[BLE] Nœud trouvé : 08:A6:F7:22:72:86 (signal -34 dBm)
[BLE] Connecté et abonné aux mesures (une toutes les 2 s)
[BLE->MQTT] {"fan": 20, "mode": "auto", ..., "pm25": 9, "pm10": 14, "aqi": 0, "link": "ble"}
```

Le jumeau Wokwi et la passerelle publient sur le même appareil ThingsBoard : ne pas les faire
fonctionner en même temps.

## Fichiers

| Fichier | Rôle |
| --- | --- |
| `gateway_pc.py` | Passerelle BLE vers MQTT |
| `requirements.txt` | Bibliothèques Python |
| `secrets.example.json` | Modèle du fichier de configuration privé `secrets.json` |
| `lancer_passerelle.bat` | Lancement sous Windows |

## Données ajoutées par la passerelle

| Clé | Signification |
| --- | --- |
| `link` | `ble` : la mesure a transité par le lien BLE |
| `rssi` | Puissance du signal BLE à la détection (dBm) |
| `lost` | Paquets BLE perdus, déduits du numéro de séquence |
| `energy_wh`, `saved_wh`, `saved_pct` | Énergie consommée et économisée par rapport à un ventilateur toujours à 100 % |
