// AirWatt : jumeau numérique Wokwi
//
// Même logique que le nœud capteur (lecture PMS5003, indice de qualité de
// l'air, régulation du ventilateur, calcul d'énergie), avec :
//   - un PMS5003 simulé (puce pms5003.chip) qui envoie de vraies trames UART,
//     réglables avec ses curseurs PM2.5 et PM10 ;
//   - un DHT22 à la place du BME280 (absent de Wokwi) ;
//   - un envoi direct en Wi-Fi/MQTT vers ThingsBoard, car Wokwi ne simule pas
//     le BLE. Sur le vrai prototype, ce saut passe par le BLE et la passerelle.
//
// Avant de lancer : créer secrets.h (modèle : secrets.example.h) avec le jeton
// de l'appareil ThingsBoard.

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHTesp.h>
#include "airwatt_protocol.h"
#include "pms5003.h"

using namespace airwatt;

// ---- ThingsBoard -------------------------------------------------------------
// TB_HOST et TB_TOKEN sont dans secrets.h (privé, voir secrets.example.h).
#include "secrets.h"
const int TB_PORT = 1883;

// ---- Brochage (identique au nœud réel, sauf le DHT22) ------------------------
const int PIN_PMS_RX  = 16;  // <- TX du PMS5003
const int PIN_PMS_TX  = 17;  // -> RX du PMS5003
const int PIN_DHT     = 15;
const int PIN_FAN_PWM = 25;  // LED bleue = vitesse du ventilateur
const int PIN_LED_R   = 26;
const int PIN_LED_G   = 27;
const int PIN_LED_B   = 32;
const int PIN_BUZZER  = 33;

const float   FAN_RATED_W = 2.4f;  // puissance nominale du ventilateur (W)
const uint8_t FAN_MIN_PCT = 20;

WiFiClient   wifi;
PubSubClient mqtt(wifi);
DHTesp       dht;
Pms5003      pms(Serial2);
MovingAverage<5> avgPm25, avgPm10;

bool     autoMode     = true;
uint8_t  manualFanPct = 50;
uint16_t pm25RedLimit = PM25_RED_DEFAULT;
uint8_t  currentFan   = FAN_MIN_PCT;
uint8_t  lastAqi      = AQI_GREEN;
uint16_t seq          = 0;
double   energyWh = 0, baselineWh = 0;

// ---- Logique (identique au nœud) ---------------------------------------------
uint8_t calculerVentilateur(uint16_t pm25) {
  if (!autoMode) return manualFanPct;
  if (pm25 <= 10) return FAN_MIN_PCT;
  if (pm25 >= pm25RedLimit) return 100;
  return FAN_MIN_PCT + (uint32_t)(pm25 - 10) * (100 - FAN_MIN_PCT) / (pm25RedLimit - 10);
}

void appliquerVentilateur(uint8_t pct) {
  uint32_t duty = (uint32_t)pct * 255 / 100;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_FAN_PWM, duty);
#else
  ledcWrite(0, duty);
#endif
}

void appliquerAlerte(uint8_t aqi, bool pmsOk) {
  if (!pmsOk) {  // capteur en défaut : LED bleue, mesures de particules inconnues
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, HIGH);
    return;
  }
  digitalWrite(PIN_LED_R, aqi != AQI_GREEN);  // orange = rouge + vert
  digitalWrite(PIN_LED_G, aqi != AQI_RED);
  digitalWrite(PIN_LED_B, LOW);
  if (aqi == AQI_RED && lastAqi != AQI_RED) tone(PIN_BUZZER, 2000, 300);
  lastAqi = aqi;
}

// ---- Commandes RPC venant du tableau de bord ---------------------------------
// value = réponse déjà au format JSON : "true", "false" ou un nombre
void repondre(const String& requestId, const String& value) {
  String topic = "v1/devices/me/rpc/response/" + requestId;
  mqtt.publish(topic.c_str(), value.c_str());
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String t(topic);
  String requestId = t.substring(t.lastIndexOf('/') + 1);
  JsonDocument doc;
  if (deserializeJson(doc, payload, length)) return;
  String method = doc["method"] | "";
  JsonVariant params = doc["params"];

  if (method == "setMode") {
    // Interrupteur : true = auto ; accepte aussi "auto" / "manuel"
    autoMode = params.is<bool>() ? params.as<bool>() : (params.as<String>() == "auto");
    repondre(requestId, autoMode ? "true" : "false");
  } else if (method == "getMode") {
    repondre(requestId, autoMode ? "true" : "false");
  } else if (method == "setFan") {
    manualFanPct = constrain(params.as<int>(), 0, 100);
    repondre(requestId, String(manualFanPct));
  } else if (method == "getFan") {
    repondre(requestId, String(autoMode ? currentFan : manualFanPct));
  } else if (method == "setThreshold") {
    pm25RedLimit = constrain(params.as<int>(), PM25_ORANGE + 1, 500);
    repondre(requestId, String(pm25RedLimit));
  } else if (method == "getThreshold") {
    repondre(requestId, String(pm25RedLimit));
  } else {
    return;
  }
  Serial.printf("[RPC] %s -> mode=%s fan=%u seuil=%u\n", method.c_str(),
                autoMode ? "auto" : "manuel", manualFanPct, pm25RedLimit);
}

// ---- Réseau (non bloquant) ---------------------------------------------------
// Les mesures, le ventilateur et les alertes locales continuent de fonctionner
// sans réseau ; la connexion Wi-Fi puis MQTT est retentée en arrière-plan.
void gererReseau() {
  static bool     wifiAnnonce = false;
  static uint32_t dernierEssaiMqtt = 0, dernierMessageWifi = 0;

  if (WiFi.status() != WL_CONNECTED) {
    wifiAnnonce = false;
    if (millis() - dernierMessageWifi > 15000) {
      dernierMessageWifi = millis();
      Serial.println("[WIFI] Pas encore connecté, nouvel essai (mesures locales actives)");
      WiFi.disconnect();
      WiFi.begin("Wokwi-GUEST", "", 6);
    }
    return;
  }
  if (!wifiAnnonce) {
    wifiAnnonce = true;
    Serial.println("[WIFI] Connecté à Wokwi-GUEST");
  }
  if (mqtt.connected()) {
    mqtt.loop();
    return;
  }
  if (millis() - dernierEssaiMqtt < 5000) return;
  dernierEssaiMqtt = millis();
  Serial.printf("[MQTT] Connexion à %s...\n", TB_HOST);
  if (mqtt.connect("airwatt-wokwi", TB_TOKEN, nullptr)) {
    mqtt.subscribe("v1/devices/me/rpc/request/+");
    Serial.println("[MQTT] Connecté à ThingsBoard");
  } else {
    // État 5 = jeton refusé : vérifier TB_TOKEN et TB_HOST dans secrets.h
    Serial.printf("[MQTT] Échec (état %d), nouvel essai dans 5 s\n", mqtt.state());
  }
}

// ---- Programme principal -----------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("=== AirWatt — jumeau Wokwi ===");
  Serial2.begin(9600, SERIAL_8N1, PIN_PMS_RX, PIN_PMS_TX);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_FAN_PWM, 5000, 8);
#else
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PIN_FAN_PWM, 0);
#endif
  dht.setup(PIN_DHT, DHTesp::DHT22);

  Serial.println("[WIFI] Connexion à Wokwi-GUEST...");
  WiFi.begin("Wokwi-GUEST", "", 6);
  mqtt.setServer(TB_HOST, TB_PORT);
  mqtt.setBufferSize(512);
  mqtt.setCallback(onMqttMessage);
}

void loop() {
  static uint32_t lastSend = 0;
  gererReseau();

  // Le PMS5003 envoie environ une trame par seconde : on les lit toutes.
  uint16_t rawPm25, rawPm10;
  if (pms.poll(rawPm25, rawPm10)) {
    avgPm25.add(rawPm25);
    avgPm10.add(rawPm10);
  }

  if (millis() - lastSend < NOTIFY_PERIOD_MS) return;

  // Démarrage : attendre la première trame du PMS5003 (10 s au plus) pour ne pas
  // signaler à tort un capteur en panne ni mettre le ventilateur à fond.
  if (avgPm25.empty() && millis() < 10000) {
    Serial.println("[PMS5003] En attente de la première mesure...");
    lastSend = millis();
    return;
  }

  uint32_t dt = lastSend ? millis() - lastSend : 0;
  lastSend = millis();

  bool pmsOk = pms.alive();
  uint16_t pm25 = avgPm25.value();
  uint16_t pm10 = avgPm10.value();
  TempAndHumidity th = dht.getTempAndHumidity();
  bool dhtOk = !isnan(th.temperature) && !isnan(th.humidity);

  uint8_t aqi = computeAqi(pm25, pm10, pm25RedLimit);
  currentFan = pmsOk ? calculerVentilateur(pm25) : 100;  // capteur muet : sécurité, ventilation maximale
  appliquerVentilateur(currentFan);
  appliquerAlerte(aqi, pmsOk);

  double hours = dt / 3600000.0;
  double ratio = currentFan / 100.0;
  energyWh   += FAN_RATED_W * ratio * ratio * ratio * hours;
  baselineWh += FAN_RATED_W * hours;
  double savedWh = baselineWh - energyWh;

  JsonDocument doc;
  if (pmsOk) {  // en panne, on n'envoie pas de vieilles valeurs trompeuses
    doc["pm25"] = pm25;
    doc["pm10"] = pm10;
    doc["aqi"]  = aqi;
  }
  if (dhtOk) {
    doc["temp"] = round(th.temperature * 10) / 10.0;
    doc["hum"]  = (int)round(th.humidity);
  }
  doc["fan"]  = currentFan;
  doc["mode"] = autoMode ? "auto" : "manuel";
  doc["energy_wh"] = energyWh;
  doc["saved_wh"]  = savedWh;
  doc["saved_pct"] = baselineWh > 0 ? round(100.0 * savedWh / baselineWh) : 0;
  doc["pms_ok"] = pmsOk;
  doc["seq"] = seq++;

  String payload;
  serializeJson(doc, payload);
  bool envoye = mqtt.connected() && mqtt.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println(payload + (envoye ? "" : "  (hors ligne, non envoyé)"));
}
