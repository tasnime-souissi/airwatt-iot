// AirWatt : nœud capteur
//
// Rôle : mesurer l'air, régler le ventilateur selon la pollution, et exposer
// le service GATT AirWatt en BLE (voir docs/protocole-ble.md).
//
// Deux modes (réglage SIMULATED_SENSORS dans platformio.ini) :
//   1 = « carte seule » : mesures simulées dans la carte. Le bouton BOOT
//       déclenche un pic de pollution ; la LED intégrée montre la vitesse du
//       ventilateur (luminosité) et clignote en alerte. Le BLE est réel.
//   0 = montage complet : PMS5003 sur UART2 (GPIO 16/17), BME280 en I2C
//       (GPIO 21/22).
//
// Commandes du moniteur série (115200 bauds) :
//   pic   -> pic de pollution simulé (mode 1)
//   auto  -> active/désactive un pic automatique toutes les 3 min (mode 1)
//   etat  -> affiche l'état courant

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "airwatt_protocol.h"
#include "pms5003.h"
#if !SIMULATED_SENSORS
#include <Wire.h>
#include <Adafruit_BME280.h>
#endif

using namespace airwatt;

// ---- Brochage (voir docs/schema/schema-cablage.md) ---------------------------
constexpr int PIN_FAN_PWM = 25;  // grille du MOSFET IRL540N
constexpr int PIN_LED_R   = 26;
constexpr int PIN_LED_G   = 27;
constexpr int PIN_LED_B   = 32;
constexpr int PIN_BUZZER  = 33;
constexpr int PIN_PMS_RX  = 16;  // <- TX du PMS5003
constexpr int PIN_PMS_TX  = 17;  // -> RX du PMS5003
constexpr int PIN_BOOT    = 0;   // bouton BOOT de la carte (actif à l'état bas)
constexpr int PIN_LED_INT = 2;   // LED intégrée de l'ESP32 DevKit

constexpr uint32_t PWM_FREQ   = 25000;  // 25 kHz : inaudible pour le ventilateur
constexpr uint8_t  PWM_BITS   = 8;      // rapport cyclique de 0 à 255
constexpr uint8_t  FAN_MIN_PCT = 20;    // ventilation minimale, air propre

// ---- État partagé (modifié aussi par les callbacks BLE) ----------------------
volatile bool     autoMode     = true;
volatile uint8_t  manualFanPct = 50;
volatile uint16_t pm25RedLimit = PM25_RED_DEFAULT;
volatile bool     clientConnected = false;

struct Mesures {
  uint16_t pm25;
  uint16_t pm10;
  float    tempC;
  float    humPct;
  bool     pmsOk;
  bool     bmeOk;
};

NimBLECharacteristic* measChar = nullptr;
uint16_t seq = 0;
uint8_t  lastAqi = AQI_GREEN;
uint8_t  currentFan = FAN_MIN_PCT;
uint32_t buzzerOffAt = 0;

// ---- Callbacks BLE -----------------------------------------------------------
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& info) override {
    clientConnected = true;
    Serial.printf("[BLE] Passerelle connectée : %s\n", info.getAddress().toString().c_str());
  }
  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& info, int reason) override {
    clientConnected = false;
    Serial.printf("[BLE] Passerelle déconnectée (raison %d), nouvelle annonce\n", reason);
    NimBLEDevice::startAdvertising();
  }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo& info) override {
    NimBLEAttValue v = chr->getValue();
    if (v.length() != sizeof(Command)) {
      Serial.printf("[CMD] Taille invalide : %u octets\n", (unsigned)v.length());
      return;
    }
    Command cmd;
    memcpy(&cmd, v.data(), sizeof(cmd));
    switch (cmd.code) {
      case CMD_SET_MODE:
        autoMode = cmd.value != 0;
        Serial.printf("[CMD] Mode %s\n", autoMode ? "auto" : "manuel");
        break;
      case CMD_SET_FAN:
        manualFanPct = cmd.value > 100 ? 100 : cmd.value;
        Serial.printf("[CMD] Ventilateur manuel à %u %%\n", manualFanPct);
        break;
      case CMD_SET_THRESHOLD:
        if (cmd.value > PM25_ORANGE && cmd.value <= 500) {
          pm25RedLimit = cmd.value;
          Serial.printf("[CMD] Seuil rouge PM2.5 = %u µg/m³\n", pm25RedLimit);
        }
        break;
      default:
        Serial.printf("[CMD] Code inconnu 0x%02X\n", cmd.code);
    }
  }
};

// ---- Capteurs ----------------------------------------------------------------
#if SIMULATED_SENSORS
uint32_t spikeStart = 0;   // début du pic simulé (0 = aucun)
bool     spikeAuto = false;

// Air propre, et pic de 75 s déclenché par le bouton BOOT, la commande « pic »,
// ou automatiquement toutes les 3 min si « auto » est activé.
Mesures lireCapteurs() {
  uint32_t now = millis();
  if (spikeAuto && spikeStart == 0 && (now / 1000) % 180 == 60) spikeStart = now;

  float pm25 = 8.0f + random(0, 40) / 10.0f;  // fond propre : 8 à 12
  if (spikeStart != 0) {
    float t = (now - spikeStart) / 1000.0f;
    if (t < 15)      pm25 += 55.0f * (t / 15.0f);             // montée
    else if (t < 30) pm25 += 55.0f;                           // plateau
    else if (t < 75) pm25 += 55.0f * (1.0f - (t - 30) / 45);  // retour au calme
    else             spikeStart = 0;
  }
  Mesures m;
  m.pm25   = (uint16_t)pm25;
  m.pm10   = (uint16_t)(pm25 * 1.6f);
  m.tempC  = 28.5f + random(-5, 6) / 10.0f;
  m.humPct = 55.0f + random(-20, 21) / 10.0f;
  m.pmsOk  = true;
  m.bmeOk  = true;
  return m;
}

void initCapteurs() {
  Serial.println("[CAPTEURS] Mode carte seule : mesures simulées, bouton BOOT = pic");
}

void lireBouton() {
  static bool previous = HIGH;
  bool now = digitalRead(PIN_BOOT);
  if (previous == HIGH && now == LOW && spikeStart == 0) {
    spikeStart = millis();
    Serial.println("[SIM] Bouton BOOT : pic de pollution déclenché");
  }
  previous = now;
}
#else
Pms5003 pms(Serial2);
MovingAverage<5> avgPm25, avgPm10;
Adafruit_BME280 bme;
bool bmePresent = false;

void initCapteurs() {
  Serial2.begin(9600, SERIAL_8N1, PIN_PMS_RX, PIN_PMS_TX);
  Wire.begin(21, 22);
  bmePresent = bme.begin(0x76) || bme.begin(0x77);
  if (!bmePresent) {
    Serial.println("[CAPTEURS] BME280 introuvable (vérifier le câblage ; un BMP280 n'a pas d'humidité)");
  } else if (bme.sensorID() != 0x60) {
    Serial.printf("[CAPTEURS] Puce 0x%02X : ce n'est pas un BME280 (0x58 = BMP280, sans humidité)\n",
                  (unsigned)bme.sensorID());
  }
  Serial.println("[CAPTEURS] Mode réel : PMS5003 sur UART2, BME280 en I2C");
}

// Le PMS5003 envoie environ une trame par seconde : à appeler souvent.
void pollPms() {
  uint16_t pm25, pm10;
  if (pms.poll(pm25, pm10)) {
    avgPm25.add(pm25);
    avgPm10.add(pm10);
  }
}

Mesures lireCapteurs() {
  Mesures m;
  m.pmsOk  = pms.alive();
  m.pm25   = avgPm25.value();
  m.pm10   = avgPm10.value();
  m.tempC  = bmePresent ? bme.readTemperature() : NAN;
  m.humPct = bmePresent ? bme.readHumidity() : NAN;
  m.bmeOk  = bmePresent && !isnan(m.tempC) && !isnan(m.humPct);
  return m;
}

void lireBouton() {}
#endif

// ---- Actionneurs -------------------------------------------------------------
void pwmAttach(int pin) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, PWM_FREQ, PWM_BITS);
#else
  static uint8_t channel = 0;
  ledcSetup(channel, PWM_FREQ, PWM_BITS);
  ledcAttachPin(pin, channel++);
#endif
}

void pwmWrite(int pin, uint8_t pct) {
  uint32_t duty = (uint32_t)pct * ((1 << PWM_BITS) - 1) / 100;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  ledcWrite(pin == PIN_FAN_PWM ? 0 : 1, duty);
#endif
}

uint8_t calculerVentilateur(const Mesures& m) {
  if (!autoMode) return manualFanPct;
  if (!m.pmsOk) return 100;  // capteur muet : ventilation maximale par sécurité
  uint16_t red = pm25RedLimit;
  if (m.pm25 <= 10) return FAN_MIN_PCT;
  if (m.pm25 >= red) return 100;
  return FAN_MIN_PCT + (uint32_t)(m.pm25 - 10) * (100 - FAN_MIN_PCT) / (red - 10);
}

void appliquerAlerte(uint8_t aqi, bool pmsOk) {
  if (!pmsOk) {  // capteur en défaut : LED bleue
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, HIGH);
    return;
  }
  digitalWrite(PIN_LED_R, aqi != AQI_GREEN);  // orange = rouge + vert
  digitalWrite(PIN_LED_G, aqi != AQI_RED);
  digitalWrite(PIN_LED_B, LOW);
  if (aqi == AQI_RED && lastAqi != AQI_RED) {  // bip au passage au rouge
    digitalWrite(PIN_BUZZER, HIGH);
    buzzerOffAt = millis() + 300;
  }
  lastAqi = aqi;
}

// LED intégrée : luminosité = vitesse du ventilateur ; clignote si l'air est
// mauvais (2 Hz) ou si le capteur est en défaut (rapide).
void majLedIntegree(bool pmsOk) {
  uint32_t now = millis();
  if (!pmsOk)                  pwmWrite(PIN_LED_INT, (now / 100) % 2 ? 100 : 0);
  else if (lastAqi == AQI_RED) pwmWrite(PIN_LED_INT, (now / 250) % 2 ? 100 : 0);
  else                         pwmWrite(PIN_LED_INT, currentFan);
}

// ---- Initialisation ----------------------------------------------------------
void initBle() {
  NimBLEDevice::init(DEVICE_NAME);
  NimBLEDevice::setPower(9);  // dBm, portée maximale pour l'atelier

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* service = server->createService(SERVICE_UUID);
  measChar = service->createCharacteristic(MEAS_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  NimBLECharacteristic* cmdChar = service->createCharacteristic(CMD_UUID, NIMBLE_PROPERTY::WRITE);
  cmdChar->setCallbacks(new CommandCallbacks());
  // NimBLE 2.x démarre le service avec le serveur, au lancement de l'annonce.

  // Annonce : l'UUID 128 bits dans l'annonce, le nom dans la réponse au scan
  // (les deux ensemble dépasseraient les 31 octets d'une annonce).
  NimBLEAdvertisementData advData;
  advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  advData.addServiceUUID(NimBLEUUID(SERVICE_UUID));
  NimBLEAdvertisementData scanData;
  scanData.setName(DEVICE_NAME);

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setAdvertisementData(advData);
  adv->setScanResponseData(scanData);
  adv->start();
  Serial.printf("[BLE] Annonce lancée sous le nom %s\n", DEVICE_NAME);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== AirWatt — nœud capteur ===");

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  pwmAttach(PIN_FAN_PWM);
  pwmAttach(PIN_LED_INT);
  pwmWrite(PIN_FAN_PWM, FAN_MIN_PCT);
  randomSeed(esp_random());

  initCapteurs();
  initBle();
}

// ---- Boucle principale -------------------------------------------------------
void lireCommandeSerie() {
  if (!Serial.available()) return;
  String ligne = Serial.readStringUntil('\n');
  ligne.trim();
#if SIMULATED_SENSORS
  if (ligne == "pic") {
    spikeStart = millis();
    Serial.println("[SIM] Pic de pollution déclenché");
    return;
  }
  if (ligne == "auto") {
    spikeAuto = !spikeAuto;
    Serial.printf("[SIM] Pic automatique toutes les 3 min : %s\n", spikeAuto ? "activé" : "désactivé");
    return;
  }
#endif
  if (ligne == "etat") {
    Serial.printf("[ETAT] mode=%s ventilateur=%u%% seuil=%u passerelle=%s\n", autoMode ? "auto" : "manuel",
                  currentFan, pm25RedLimit, clientConnected ? "connectée" : "absente");
  }
}

void loop() {
  static uint32_t lastSend = 0;
  static bool lastPmsOk = true;
  lireCommandeSerie();
  lireBouton();
#if !SIMULATED_SENSORS
  pollPms();
#endif
  majLedIntegree(lastPmsOk);

  if (buzzerOffAt && millis() > buzzerOffAt) {
    digitalWrite(PIN_BUZZER, LOW);
    buzzerOffAt = 0;
  }

  if (millis() - lastSend < NOTIFY_PERIOD_MS) return;
  lastSend = millis();

  Mesures m = lireCapteurs();
  lastPmsOk = m.pmsOk;
  uint8_t aqi = computeAqi(m.pm25, m.pm10, pm25RedLimit);
  currentFan = calculerVentilateur(m);
  pwmWrite(PIN_FAN_PWM, currentFan);
  appliquerAlerte(aqi, m.pmsOk);

  Packet p;
  p.pm25     = m.pm25;
  p.pm10     = m.pm10;
  p.temp_x10 = m.bmeOk ? (int16_t)lroundf(m.tempC * 10.0f) : 0;
  p.hum      = m.bmeOk ? (uint8_t)lroundf(m.humPct) : 0;
  p.fan      = currentFan;
  p.aqi      = aqi;
  p.flags    = (autoMode ? FLAG_AUTO_MODE : 0) | (m.pmsOk ? 0 : FLAG_PMS_ERROR) | (m.bmeOk ? 0 : FLAG_BME_ERROR);
  p.seq      = seq++;

  measChar->setValue(reinterpret_cast<uint8_t*>(&p), sizeof(p));
  if (clientConnected) measChar->notify();

  Serial.printf("[#%u] PM2.5=%u PM10=%u T=%.1f°C H=%u%% ventilateur=%u%% air=%s capteur=%s mode=%s passerelle=%s\n",
                p.seq, p.pm25, p.pm10, p.temp_x10 / 10.0f, p.hum, p.fan, aqiName(aqi), m.pmsOk ? "ok" : "EN DÉFAUT",
                autoMode ? "auto" : "manuel", clientConnected ? "oui" : "non");
}
