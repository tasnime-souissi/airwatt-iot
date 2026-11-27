// AirWatt — passerelle (ESP32 n°2)
//
// Rôle : trouver le nœud AirWatt en BLE, s'abonner à ses mesures, les décoder,
// calculer l'énergie économisée et relayer les commandes vers le nœud.
//
// Étape 2 : les mesures sont affichées en JSON sur le moniteur série.
// L'étape 5 ajoutera l'envoi MQTT vers ThingsBoard (même JSON).
//
// Commandes du moniteur série (115200 bauds), envoyées au nœud en BLE :
//   mode auto | mode manuel | fan 60 | seuil 40

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "airwatt_protocol.h"

using namespace airwatt;

// Puissance nominale du ventilateur (W) : 12 V x 0,2 A pour un ventilateur
// 80 mm courant. À remplacer par la valeur inscrite sur l'étiquette.
constexpr float FAN_RATED_W = 2.4f;

// ---- État BLE ----------------------------------------------------------------
const NimBLEAdvertisedDevice* target = nullptr;
volatile bool doConnect = false;
volatile bool connected = false;
NimBLEClient* client = nullptr;
NimBLERemoteCharacteristic* cmdChar = nullptr;

// Dernier paquet reçu (écrit par la tâche BLE, lu par loop)
portMUX_TYPE packetMux = portMUX_INITIALIZER_UNLOCKED;
Packet lastPacket;
volatile bool packetReady = false;

// ---- Statistiques et énergie -------------------------------------------------
uint32_t received = 0;
uint32_t lost = 0;
int32_t  lastSeq = -1;
uint32_t lastPacketAt = 0;
double   energyWh = 0;    // énergie consommée par le ventilateur piloté
double   baselineWh = 0;  // énergie d'un ventilateur toujours à 100 %

// ---- Callbacks BLE -----------------------------------------------------------
class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* dev) override {
    if (dev->isAdvertisingService(NimBLEUUID(SERVICE_UUID))) {
      Serial.printf("[BLE] Nœud AirWatt trouvé : %s (RSSI %d dBm)\n",
                    dev->getAddress().toString().c_str(), dev->getRSSI());
      NimBLEDevice::getScan()->stop();
      target = dev;
      doConnect = true;
    }
  }
} scanCallbacks;

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* c) override {
    Serial.println("[BLE] Connecté au nœud");
  }
  void onDisconnect(NimBLEClient* c, int reason) override {
    connected = false;
    cmdChar = nullptr;
    Serial.printf("[BLE] Nœud perdu (raison %d), nouvelle recherche\n", reason);
  }
} clientCallbacks;

void onMeasure(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
  if (len != sizeof(Packet)) return;
  portENTER_CRITICAL(&packetMux);
  memcpy(&lastPacket, data, sizeof(Packet));
  packetReady = true;
  portEXIT_CRITICAL(&packetMux);
}

// ---- Connexion ---------------------------------------------------------------
void lancerScan() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->clearResults();
  scan->start(0, false, true);  // 0 = sans fin, jusqu'à trouver le nœud
  Serial.println("[BLE] Recherche du nœud AirWatt...");
}

bool connecter() {
  client = NimBLEDevice::getDisconnectedClient();
  if (!client) {
    client = NimBLEDevice::createClient();
    client->setClientCallbacks(&clientCallbacks, false);
    client->setConnectTimeout(5000);
  }
  if (!client->connect(target)) {
    Serial.println("[BLE] Échec de connexion");
    return false;
  }
  NimBLERemoteService* service = client->getService(SERVICE_UUID);
  if (!service) {
    Serial.println("[BLE] Service AirWatt absent");
    client->disconnect();
    return false;
  }
  NimBLERemoteCharacteristic* meas = service->getCharacteristic(MEAS_UUID);
  cmdChar = service->getCharacteristic(CMD_UUID);
  if (!meas || !meas->canNotify() || !meas->subscribe(true, onMeasure)) {
    Serial.println("[BLE] Abonnement aux mesures impossible");
    client->disconnect();
    return false;
  }
  lastSeq = -1;
  lastPacketAt = 0;
  Serial.println("[BLE] Abonné aux mesures, réception toutes les 2 s");
  return true;
}

// ---- Commandes vers le nœud --------------------------------------------------
bool envoyerCommande(uint8_t code, uint16_t value) {
  if (!connected || !cmdChar) {
    Serial.println("[CMD] Nœud non connecté");
    return false;
  }
  Command cmd{code, value};
  bool ok = cmdChar->writeValue(reinterpret_cast<uint8_t*>(&cmd), sizeof(cmd), true);
  Serial.printf("[CMD] code 0x%02X valeur %u : %s\n", code, value, ok ? "envoyée" : "échec");
  return ok;
}

void lireCommandeSerie() {
  if (!Serial.available()) return;
  String ligne = Serial.readStringUntil('\n');
  ligne.trim();
  if (ligne == "mode auto") envoyerCommande(CMD_SET_MODE, 1);
  else if (ligne == "mode manuel") envoyerCommande(CMD_SET_MODE, 0);
  else if (ligne.startsWith("fan ")) envoyerCommande(CMD_SET_FAN, ligne.substring(4).toInt());
  else if (ligne.startsWith("seuil ")) envoyerCommande(CMD_SET_THRESHOLD, ligne.substring(6).toInt());
  else if (ligne.length()) Serial.println("[CMD] Inconnue. Essayer : mode auto | mode manuel | fan 60 | seuil 40");
}

// ---- Traitement d'un paquet --------------------------------------------------
void traiterPaquet(const Packet& p) {
  received++;
  if (lastSeq >= 0) {
    uint16_t expected = (uint16_t)(lastSeq + 1);
    if (p.seq != expected) lost += (uint16_t)(p.seq - expected);
  }
  lastSeq = p.seq;

  // Énergie : la puissance d'un ventilateur varie comme le cube de sa vitesse.
  uint32_t now = millis();
  if (lastPacketAt) {
    double hours = min<uint32_t>(now - lastPacketAt, 10000) / 3600000.0;
    double ratio = p.fan / 100.0;
    energyWh   += FAN_RATED_W * ratio * ratio * ratio * hours;
    baselineWh += FAN_RATED_W * hours;
  }
  lastPacketAt = now;
  double savedWh  = baselineWh - energyWh;
  double savedPct = baselineWh > 0 ? 100.0 * savedWh / baselineWh : 0;

  int rssi = client ? client->getRssi() : 0;

  // Même format que le message MQTT de l'étape 5 (docs/contrat-donnees-ble.md)
  Serial.printf("{\"pm25\":%u,\"pm10\":%u,\"temp\":%.1f,\"hum\":%u,\"fan\":%u,\"aqi\":%u,"
                "\"mode\":\"%s\",\"energy_wh\":%.4f,\"saved_wh\":%.4f,\"saved_pct\":%.0f,"
                "\"rssi\":%d,\"seq\":%u,\"lost\":%lu}\n",
                p.pm25, p.pm10, p.temp_x10 / 10.0f, p.hum, p.fan, p.aqi,
                (p.flags & FLAG_AUTO_MODE) ? "auto" : "manuel",
                energyWh, savedWh, savedPct, rssi, p.seq, (unsigned long)lost);
}

// ---- Programme principal -----------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== AirWatt — passerelle ===");

  NimBLEDevice::init("AirWatt-Gateway");
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(&scanCallbacks, false);
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(100);
  lancerScan();
}

void loop() {
  lireCommandeSerie();

  if (doConnect) {
    doConnect = false;
    connected = connecter();
    if (!connected) {
      delay(1000);
      lancerScan();
    }
  }

  if (!connected && !doConnect && !NimBLEDevice::getScan()->isScanning()) {
    lancerScan();
  }

  if (packetReady) {
    Packet p;
    portENTER_CRITICAL(&packetMux);
    p = lastPacket;
    packetReady = false;
    portEXIT_CRITICAL(&packetMux);
    traiterPaquet(p);
  }

  delay(10);
}
