// AirWatt : lecture du capteur de particules PMS5003 (UART 9600 bauds) et
// moyenne glissante. Utilisé par le nœud réel et par le jumeau Wokwi.
//
// Trame PMS5003 (32 octets) : 0x42 0x4D, longueur (28), 13 mots de données
// big-endian, somme de contrôle (somme des octets 0 à 29).
//   octets 12-13 : PM2.5 « atmosphérique », octets 14-15 : PM10 « atmosphérique ».
//
// La resynchronisation se fait octet par octet : une trame corrompue ne fait
// perdre que quelques octets, et non les 32 suivants.
#pragma once
#include <Arduino.h>

namespace airwatt {

class Pms5003 {
 public:
  explicit Pms5003(Stream& stream) : stream_(stream) {}

  // Lit tous les octets disponibles. Retourne true dès qu'une trame valide est
  // décodée ; pm25 et pm10 reçoivent alors les valeurs en µg/m³.
  bool poll(uint16_t& pm25, uint16_t& pm10) {
    while (stream_.available()) {
      uint8_t b = stream_.read();
      if (idx_ == 0) {
        if (b != 0x42) continue;
      } else if (idx_ == 1) {
        if (b != 0x4D) {
          idx_ = (b == 0x42) ? 1 : 0;  // 0x42 0x42 0x4D : repartir du second 0x42
          continue;
        }
      }
      buf_[idx_++] = b;
      if (idx_ < FRAME_LEN) continue;

      idx_ = 0;
      uint16_t len = word16(2);
      uint16_t sum = 0;
      for (uint8_t i = 0; i < FRAME_LEN - 2; i++) sum += buf_[i];
      if (len != FRAME_LEN - 4 || sum != word16(FRAME_LEN - 2)) {
        errors_++;
        continue;
      }
      pm25 = word16(12);
      pm10 = word16(14);
      lastFrameMs_ = millis();
      return true;
    }
    return false;
  }

  // Vrai si une trame valide a été reçue récemment (le PMS5003 en envoie environ une par seconde).
  bool alive(uint32_t timeoutMs = 5000) const {
    return lastFrameMs_ != 0 && millis() - lastFrameMs_ < timeoutMs;
  }

  uint32_t errors() const { return errors_; }

 private:
  static constexpr uint8_t FRAME_LEN = 32;
  uint16_t word16(uint8_t i) const { return (uint16_t)(buf_[i] << 8) | buf_[i + 1]; }

  Stream& stream_;
  uint8_t buf_[FRAME_LEN];
  uint8_t idx_ = 0;
  uint32_t errors_ = 0;
  uint32_t lastFrameMs_ = 0;
};

// Moyenne glissante sur les N dernières valeurs.
template <uint8_t N>
class MovingAverage {
 public:
  void add(uint16_t v) {
    sum_ -= values_[pos_];
    values_[pos_] = v;
    sum_ += v;
    pos_ = (pos_ + 1) % N;
    if (count_ < N) count_++;
  }
  uint16_t value() const { return count_ ? (uint16_t)(sum_ / count_) : 0; }
  bool empty() const { return count_ == 0; }

 private:
  uint16_t values_[N] = {};
  uint32_t sum_ = 0;
  uint8_t pos_ = 0;
  uint8_t count_ = 0;
};

}  // namespace airwatt
