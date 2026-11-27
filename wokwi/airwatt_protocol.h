// AirWatt : protocole BLE commun au nœud capteur et à la passerelle.
// Spécification : docs/protocole-ble.md.
#pragma once
#include <stdint.h>

namespace airwatt {

// ---- Identifiants BLE --------------------------------------------------------
constexpr const char* DEVICE_NAME  = "AirWatt-Node";
constexpr const char* SERVICE_UUID = "6e7a0001-3c1b-4a5e-9d2f-a1b2c3d4e5f6";
constexpr const char* MEAS_UUID    = "6e7a0002-3c1b-4a5e-9d2f-a1b2c3d4e5f6";  // READ + NOTIFY
constexpr const char* CMD_UUID     = "6e7a0003-3c1b-4a5e-9d2f-a1b2c3d4e5f6";  // WRITE

constexpr uint32_t NOTIFY_PERIOD_MS = 2000;

// ---- Paquet « Mesures » (12 octets, petit-boutiste comme l'ESP32) -----------
typedef struct __attribute__((packed)) {
  uint16_t pm25;      // µg/m³, moyenne sur 5 lectures
  uint16_t pm10;      // µg/m³, moyenne sur 5 lectures
  int16_t  temp_x10;  // 0,1 °C (285 = 28,5 °C)
  uint8_t  hum;       // % d'humidité relative
  uint8_t  fan;       // vitesse du ventilateur, 0 à 100 %
  uint8_t  aqi;       // voir Aqi
  uint8_t  flags;     // voir FLAG_*
  uint16_t seq;       // compteur de paquets
} Packet;

static_assert(sizeof(Packet) == 12, "Le paquet BLE doit faire 12 octets");

enum Aqi : uint8_t { AQI_GREEN = 0, AQI_ORANGE = 1, AQI_RED = 2 };

constexpr uint8_t FLAG_AUTO_MODE = 1 << 0;
constexpr uint8_t FLAG_PMS_ERROR = 1 << 1;
constexpr uint8_t FLAG_BME_ERROR = 1 << 2;

// ---- Commandes (caractéristique WRITE, 3 octets : code + uint16) ------------
enum Cmd : uint8_t { CMD_SET_MODE = 0x01, CMD_SET_FAN = 0x02, CMD_SET_THRESHOLD = 0x03 };

typedef struct __attribute__((packed)) {
  uint8_t  code;
  uint16_t value;
} Command;

static_assert(sizeof(Command) == 3, "La commande BLE doit faire 3 octets");

// ---- Seuils de l'indice de qualité de l'air ---------------------------------
// 15 et 45 µg/m³ : repères 24 h de l'OMS (2021). 35 et 100 : repères du prototype.
constexpr uint16_t PM25_ORANGE = 15;
constexpr uint16_t PM25_RED_DEFAULT = 35;
constexpr uint16_t PM10_ORANGE = 45;
constexpr uint16_t PM10_RED = 100;

inline uint8_t computeAqi(uint16_t pm25, uint16_t pm10, uint16_t pm25Red = PM25_RED_DEFAULT) {
  if (pm25 > pm25Red || pm10 > PM10_RED) return AQI_RED;
  if (pm25 >= PM25_ORANGE || pm10 >= PM10_ORANGE) return AQI_ORANGE;
  return AQI_GREEN;
}

inline const char* aqiName(uint8_t aqi) {
  switch (aqi) {
    case AQI_GREEN:  return "vert";
    case AQI_ORANGE: return "orange";
    default:         return "rouge";
  }
}

}  // namespace airwatt
