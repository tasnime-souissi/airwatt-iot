# Passerelle sur PC (BLE -> ThingsBoard)

Remplace l'ESP32 passerelle quand l'équipe n'a qu'une seule carte. Le Bluetooth du PC
(adaptateur MediaTek, compatible BLE) se connecte au nœud `AirWatt-Node`, décode les paquets
de 12 octets et publie sur ThingsBoard **le même JSON que le jumeau Wokwi** : le tableau de
bord fonctionne sans modification. Les commandes du tableau de bord sont relayées au nœud.

## Lancer

1. Le nœud ESP32 est allumé et programmé (`node/`, configuration `carte_seule`).
2. Le Bluetooth du PC est activé.
3. Double-cliquer sur **`lancer_passerelle.bat`**. La première fois, il installe les
   bibliothèques Python (`bleak`, `paho-mqtt`) dans `%LOCALAPPDATA%\airwatt\venv`.

Affichage attendu :

```
[MQTT] Connecté à thingsboard.cloud
[BLE] Nœud trouvé : 24:6F:28:xx:xx:xx (signal -55 dBm)
[BLE] Connecté et abonné aux mesures (une toutes les 2 s)
[BLE->MQTT] {"fan": 20, "mode": "auto", ..., "pm25": 9, "pm10": 14, "aqi": 0, "link": "ble"}
```

Ne pas faire tourner la simulation Wokwi en même temps : les deux enverraient leurs mesures au
même appareil ThingsBoard.

## Fichiers

| Fichier | Rôle |
| --- | --- |
| `gateway_pc.py` | La passerelle |
| `requirements.txt` | Bibliothèques Python |
| `secrets.json` | Serveur et jeton ThingsBoard (privé, ignoré par Git) |
| `secrets.example.json` | Modèle de `secrets.json` |
| `lancer_passerelle.bat` | Lancement en un double-clic |

## Données publiées en plus de Wokwi

| Clé | Signification |
| --- | --- |
| `link` | `ble` : la mesure est passée par le vrai lien BLE |
| `rssi` | Force du signal BLE à la détection (dBm) |
| `lost` | Paquets BLE perdus (trous dans le compteur `seq`) |

Testée hors ligne le 27/11/2026 (décodage, paquets perdus, panne capteur, commandes) ;
reste à tester avec la carte.
