// AirWatt : capteur PMS5003 simulé pour Wokwi (puce personnalisée).
//
// Envoie chaque seconde une vraie trame PMS5003 de 32 octets sur sa broche TX
// (UART 9600 bauds), comme le capteur réel. Le firmware lit donc ces trames
// avec exactement le même code que sur le vrai capteur
// (common/AirWattProtocol/pms5003.h).
//
// Curseurs de la puce pendant la simulation :
//   - PM2.5 / PM10 manuel : valeurs envoyées quand le scénario est à 0 ;
//   - Scénario auto = 1   : cycle de 2 min rejoué en boucle (air propre, montée,
//                           pic, retour au calme), avec un léger bruit ;
//   - Panne = 1           : capteur muet, plus aucune trame envoyée ;
//   - Panne = 2           : trames abîmées (somme de contrôle fausse), que le
//                           firmware doit rejeter.

#include "wokwi-api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CYCLE_S 120  // durée du scénario automatique, en secondes

typedef struct {
  uart_dev_t uart;
  uint32_t pm25_attr;
  uint32_t pm10_attr;
  uint32_t scenario_attr;
  uint32_t panne_attr;
  uint8_t frame[32];
  bool busy;
  uint32_t tick;        // secondes écoulées depuis le démarrage
  uint32_t last_mode;   // pour annoncer les changements de mode
  uint32_t rng;         // générateur pseudo-aléatoire (bruit de mesure)
} chip_state_t;

static void put16(uint8_t *f, int offset, uint16_t value) {
  f[offset] = value >> 8;      // big-endian, comme le PMS5003
  f[offset + 1] = value & 0xFF;
}

// Bruit de -2 à +2 µg/m³ (générateur congruentiel simple)
static int bruit(chip_state_t *chip) {
  chip->rng = chip->rng * 1103515245u + 12345u;
  return (int)((chip->rng >> 16) % 5) - 2;
}

// Scénario automatique : PM2.5 selon la position dans le cycle de 2 minutes
static uint16_t pm25_scenario(chip_state_t *chip) {
  uint32_t t = chip->tick % CYCLE_S;
  int v;
  if (t < 30)       v = 9;                              // air propre
  else if (t < 45)  v = 9 + (int)(t - 30) * 56 / 15;    // montée (meulage)
  else if (t < 60)  v = 65;                             // pic
  else if (t < 100) v = 65 - (int)(t - 60) * 56 / 40;   // retour au calme
  else              v = 9;                              // air propre
  v += bruit(chip);
  return v < 0 ? 0 : (uint16_t)v;
}

static void on_write_done(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  chip->busy = false;
}

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  chip->tick++;

  uint32_t scenario = attr_read(chip->scenario_attr);
  uint32_t panne = attr_read(chip->panne_attr);
  uint32_t mode = panne * 10 + scenario;
  if (mode != chip->last_mode) {
    chip->last_mode = mode;
    if (panne == 1)      printf("PMS5003 simule : PANNE, capteur muet\n");
    else if (panne == 2) printf("PMS5003 simule : PANNE, trames abimees\n");
    else if (scenario)   printf("PMS5003 simule : scenario automatique\n");
    else                 printf("PMS5003 simule : valeurs manuelles\n");
  }

  if (panne == 1) return;   // capteur muet
  if (chip->busy) return;   // trame précédente encore en cours d'envoi

  uint16_t pm25, pm10;
  if (scenario) {
    pm25 = pm25_scenario(chip);
    pm10 = pm25 * 8 / 5 + 2;
  } else {
    pm25 = (uint16_t)attr_read(chip->pm25_attr);
    pm10 = (uint16_t)attr_read(chip->pm10_attr);
  }
  if (pm10 < pm25) pm10 = pm25;  // PM10 contient toujours les PM2.5
  uint16_t pm1 = pm25 * 2 / 3;

  uint8_t *f = chip->frame;
  f[0] = 0x42;
  f[1] = 0x4D;
  put16(f, 2, 28);                 // longueur : 13 mots + somme de contrôle
  put16(f, 4, pm1);                // PM1.0 (CF=1)
  put16(f, 6, pm25);               // PM2.5 (CF=1)
  put16(f, 8, pm10);               // PM10  (CF=1)
  put16(f, 10, pm1);               // PM1.0 atmosphérique
  put16(f, 12, pm25);              // PM2.5 atmosphérique
  put16(f, 14, pm10);              // PM10  atmosphérique
  for (int i = 16; i < 30; i += 2) put16(f, i, 0);  // comptages de particules, réservé

  uint16_t sum = 0;
  for (int i = 0; i < 30; i++) sum += f[i];
  if (panne == 2) sum ^= 0x5A5A;   // somme de contrôle volontairement fausse
  put16(f, 30, sum);

  chip->busy = uart_write(chip->uart, f, sizeof(chip->frame));
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->pm25_attr = attr_init("pm25", 10);
  chip->pm10_attr = attr_init("pm10", 18);
  chip->scenario_attr = attr_init("scenario", 0);
  chip->panne_attr = attr_init("panne", 0);
  chip->busy = false;
  chip->tick = 0;
  chip->last_mode = 0xFFFFFFFF;
  chip->rng = 12345;

  const uart_config_t uart_config = {
    .tx = pin_init("TX", INPUT_PULLUP),
    .rx = pin_init("RX", INPUT),
    .baud_rate = 9600,
    .rx_data = NULL,
    .write_done = on_write_done,
    .user_data = chip,
  };
  chip->uart = uart_init(&uart_config);

  const timer_config_t timer_config = {
    .callback = on_timer,
    .user_data = chip,
  };
  timer_t timer = timer_init(&timer_config);
  timer_start(timer, 1000000, true);  // une trame par seconde

  printf("PMS5003 simule : pret\n");
}
