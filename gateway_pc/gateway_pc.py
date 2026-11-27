"""AirWatt : passerelle sur PC, BLE (nœud ESP32) vers MQTT (ThingsBoard).

Le Bluetooth du PC se connecte au nœud « AirWatt-Node », décode les paquets de
12 octets (docs/protocole-ble.md), calcule l'énergie économisée et publie les
mesures sur ThingsBoard, dans le même format que le jumeau Wokwi. Les commandes du tableau de bord
(RPC) sont relayées vers le nœud en BLE.

Lancement : lancer_passerelle.bat (ou python gateway_pc.py)
Configuration : secrets.json (modèle : secrets.example.json)
"""

import asyncio
import json
import struct
import sys
import time
from pathlib import Path

import paho.mqtt.client as mqtt
from bleak import BleakClient, BleakScanner

# ---- Protocole (identique à common/AirWattProtocol/airwatt_protocol.h) --------
SERVICE_UUID = "6e7a0001-3c1b-4a5e-9d2f-a1b2c3d4e5f6"
MEAS_UUID = "6e7a0002-3c1b-4a5e-9d2f-a1b2c3d4e5f6"
CMD_UUID = "6e7a0003-3c1b-4a5e-9d2f-a1b2c3d4e5f6"

PACKET = struct.Struct("<HHhBBBBH")  # pm25, pm10, temp_x10, hum, fan, aqi, flags, seq = 12 octets
COMMAND = struct.Struct("<BH")       # code, valeur = 3 octets
CMD_SET_MODE, CMD_SET_FAN, CMD_SET_THRESHOLD = 0x01, 0x02, 0x03
FLAG_AUTO_MODE, FLAG_PMS_ERROR, FLAG_BME_ERROR = 0x01, 0x02, 0x04

FAN_RATED_W = 2.4  # puissance nominale du ventilateur du prototype (W)

# La console Windows (cp1252) ne sait pas afficher tous les caractères : forcer l'UTF-8.
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")


def log(msg: str) -> None:
    print(time.strftime("%H:%M:%S"), msg, flush=True)


def decoder_paquet(data: bytes) -> dict:
    """Décode un paquet « Mesures » de 12 octets."""
    pm25, pm10, temp_x10, hum, fan, aqi, flags, seq = PACKET.unpack(data)
    return {"pm25": pm25, "pm10": pm10, "temp": temp_x10 / 10.0, "hum": hum, "fan": fan,
            "aqi": aqi, "flags": flags, "seq": seq}


class Passerelle:
    def __init__(self, host: str, token: str):
        self.loop: asyncio.AbstractEventLoop | None = None
        self.client: BleakClient | None = None
        self.rssi = None
        self.dernier: dict | None = None
        self.seuil = 35
        self.vitesse_manuelle = 50
        self.derniere_seq = None
        self.perdus = 0
        self.dernier_paquet_t = None
        self.energie_wh = 0.0
        self.reference_wh = 0.0

        self.mqtt = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="airwatt-passerelle-pc")
        self.mqtt.username_pw_set(token)
        self.mqtt.on_connect = self._mqtt_connecte
        self.mqtt.on_message = self._mqtt_message
        self.mqtt.reconnect_delay_set(1, 30)
        self.host = host

    # ---- MQTT ----------------------------------------------------------------
    def demarrer_mqtt(self) -> None:
        self.mqtt.connect_async(self.host, 1883, keepalive=30)
        self.mqtt.loop_start()

    def _mqtt_connecte(self, client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            log(f"[MQTT] Connecté à {self.host}")
            client.subscribe("v1/devices/me/rpc/request/+")
        else:
            log(f"[MQTT] Connexion refusée ({reason_code}) : vérifier le jeton dans secrets.json")

    def _repondre(self, request_id: str, valeur) -> None:
        self.mqtt.publish(f"v1/devices/me/rpc/response/{request_id}", json.dumps(valeur))

    def _mqtt_message(self, client, userdata, msg):
        """Commande RPC venant du tableau de bord (thread MQTT)."""
        request_id = msg.topic.rsplit("/", 1)[-1]
        try:
            req = json.loads(msg.payload)
        except ValueError:
            return
        methode, params = req.get("method", ""), req.get("params")
        auto = bool(self.dernier and self.dernier["flags"] & FLAG_AUTO_MODE)

        if methode == "getMode":
            self._repondre(request_id, auto)
        elif methode == "getFan":
            self._repondre(request_id, self.dernier["fan"] if (self.dernier and auto) else self.vitesse_manuelle)
        elif methode == "getThreshold":
            self._repondre(request_id, self.seuil)
        elif methode in ("setMode", "setFan", "setThreshold"):
            if methode == "setMode":
                valeur = 1 if (params is True or params == "auto" or params == "true") else 0
                code = CMD_SET_MODE
            elif methode == "setFan":
                valeur = max(0, min(100, int(params)))
                self.vitesse_manuelle = valeur
                code = CMD_SET_FAN
            else:
                valeur = max(16, min(500, int(params)))
                self.seuil = valeur
                code = CMD_SET_THRESHOLD
            ok = self._envoyer_commande_depuis_thread(code, valeur)
            log(f"[RPC] {methode}({params}) -> {'transmis au nœud' if ok else 'ÉCHEC (nœud non connecté)'}")
            # Sans nœud, pas de réponse : le tableau de bord affiche l'échec au lieu
            # de croire la commande appliquée.
            if ok:
                self._repondre(request_id, bool(valeur) if methode == "setMode" else valeur)

    # ---- BLE -----------------------------------------------------------------
    def _envoyer_commande_depuis_thread(self, code: int, valeur: int) -> bool:
        if not (self.loop and self.client and self.client.is_connected):
            return False
        futur = asyncio.run_coroutine_threadsafe(
            self.client.write_gatt_char(CMD_UUID, COMMAND.pack(code, valeur), response=True), self.loop)
        try:
            futur.result(timeout=5)
            return True
        except Exception as exc:  # noqa: BLE001 - on journalise toute erreur BLE
            log(f"[BLE] Écriture de la commande impossible : {exc}")
            return False

    def _notification(self, _caracteristique, data: bytearray) -> None:
        if len(data) != PACKET.size:
            log(f"[BLE] Paquet de taille inattendue : {len(data)} octets")
            return
        p = decoder_paquet(bytes(data))

        if self.derniere_seq is not None:
            attendu = (self.derniere_seq + 1) & 0xFFFF
            self.perdus += (p["seq"] - attendu) & 0xFFFF
        self.derniere_seq = p["seq"]

        # Énergie : la puissance d'un ventilateur varie comme le cube de sa vitesse.
        maintenant = time.monotonic()
        if self.dernier_paquet_t is not None:
            heures = min(maintenant - self.dernier_paquet_t, 10) / 3600
            self.energie_wh += FAN_RATED_W * (p["fan"] / 100) ** 3 * heures
            self.reference_wh += FAN_RATED_W * heures
        self.dernier_paquet_t = maintenant
        self.dernier = p

        economie_wh = self.reference_wh - self.energie_wh
        pms_ok = not p["flags"] & FLAG_PMS_ERROR
        bme_ok = not p["flags"] & FLAG_BME_ERROR
        telemetrie = {"fan": p["fan"], "mode": "auto" if p["flags"] & FLAG_AUTO_MODE else "manuel",
                      "energy_wh": round(self.energie_wh, 5), "saved_wh": round(economie_wh, 5),
                      "saved_pct": round(100 * economie_wh / self.reference_wh) if self.reference_wh else 0,
                      "pms_ok": pms_ok, "seq": p["seq"], "lost": self.perdus, "link": "ble"}
        if pms_ok:  # en panne, on n'envoie pas de valeurs trompeuses
            telemetrie.update(pm25=p["pm25"], pm10=p["pm10"], aqi=p["aqi"])
        if bme_ok:
            telemetrie.update(temp=p["temp"], hum=p["hum"])
        if self.rssi is not None:
            telemetrie["rssi"] = self.rssi

        envoye = self.mqtt.is_connected()
        if envoye:
            self.mqtt.publish("v1/devices/me/telemetry", json.dumps(telemetrie))
        log(f"[BLE->MQTT] {json.dumps(telemetrie)}{'' if envoye else '  (MQTT hors ligne)'}")

    async def executer(self) -> None:
        self.loop = asyncio.get_running_loop()
        self.demarrer_mqtt()
        while True:
            log("[BLE] Recherche du nœud AirWatt-Node...")

            def est_airwatt(_appareil, annonce) -> bool:
                if SERVICE_UUID in [u.lower() for u in annonce.service_uuids]:
                    self.rssi = annonce.rssi  # force du signal à la détection (dBm)
                    return True
                return False

            appareil = await BleakScanner.find_device_by_filter(est_airwatt, timeout=15)
            if appareil is None:
                log("[BLE] Nœud introuvable, nouvel essai (la carte est-elle allumée et proche ?)")
                continue
            log(f"[BLE] Nœud trouvé : {appareil.address} (signal {self.rssi} dBm)")
            deconnecte = asyncio.Event()
            try:
                async with BleakClient(appareil, disconnected_callback=lambda _c: deconnecte.set()) as client:
                    self.client = client
                    await client.start_notify(MEAS_UUID, self._notification)
                    log("[BLE] Connecté et abonné aux mesures (une toutes les 2 s)")
                    self.derniere_seq, self.dernier_paquet_t = None, None
                    await deconnecte.wait()
                    log("[BLE] Nœud perdu, nouvelle recherche")
            except Exception as exc:  # noqa: BLE001
                log(f"[BLE] Erreur de connexion : {exc}")
            finally:
                self.client = None
            await asyncio.sleep(2)


def charger_config() -> tuple[str, str]:
    chemin = Path(__file__).with_name("secrets.json")
    if not chemin.exists():
        sys.exit("secrets.json manquant : copier secrets.example.json en secrets.json et y mettre le jeton.")
    cfg = json.loads(chemin.read_text(encoding="utf-8"))
    return cfg.get("tb_host", "thingsboard.cloud"), cfg["tb_token"]


if __name__ == "__main__":
    hote, jeton = charger_config()
    print("=== AirWatt — passerelle sur PC (BLE -> ThingsBoard) ===  Ctrl+C pour arrêter")
    try:
        asyncio.run(Passerelle(hote, jeton).executer())
    except KeyboardInterrupt:
        print("Arrêt de la passerelle.")
