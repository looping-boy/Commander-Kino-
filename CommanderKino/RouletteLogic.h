#ifndef ROULETTE_LOGIC_H
#define ROULETTE_LOGIC_H

// Logique de la roulette : COPIE EXACTE de la partie logique de
// LedVestProto2/LedCoatAnim/Roulette.h (entre « #include <stdint.h> » et
// « #ifdef ARDUINO »). La télécommande et la veste déroulent la même séquence
// à partir de la même couleur de départ : si tu changes un réglage ici,
// change-le aussi dans la veste. test/check_sync.sh vérifie que les deux
// copies sont identiques.

#include <stdint.h>

#define ROULETTE_COLOR_COUNT 3
#define ROULETTE_FIRST_INTERVAL_MS 50.0f  // premier pas : 50 ms par couleur
#define ROULETTE_SLOWDOWN 1.1f            // chaque pas dure 10 % de plus que le précédent
#define ROULETTE_STEPS 26                 // 26 pas : 50 ms -> 541 ms, environ 5,5 s en tout
#define ROULETTE_STOP_HOLD_MS 650         // la couleur gagnante reste allumée avant de clignoter
#define ROULETTE_BLINKS 4                 // nombre de clignotements
#define ROULETTE_BLINK_MS 230             // durée d'une demi-période de clignotement

enum RoulettePhase : uint8_t {
  ROULETTE_IDLE,
  ROULETTE_SPINNING,
  ROULETTE_BLINKING,
  ROULETTE_DONE
};

struct Roulette {
  RoulettePhase phase;
  uint8_t color;        // 0 rouge, 1 jaune, 2 bleu
  uint8_t stepsDone;
  uint8_t togglesLeft;
  bool lit;             // false pendant les phases éteintes du clignotement
  float interval;       // durée du pas en cours, en ms
  uint32_t nextAt;      // date du prochain changement, en ms (millis)
};

inline bool rouletteIsBusy(const Roulette &r) {
  return r.phase == ROULETTE_SPINNING || r.phase == ROULETTE_BLINKING;
}

inline void rouletteStart(Roulette &r, uint32_t now, uint32_t randomValue) {
  r.phase = ROULETTE_SPINNING;
  r.color = randomValue % ROULETTE_COLOR_COUNT;
  r.stepsDone = 0;
  r.togglesLeft = 0;
  r.lit = true;
  r.interval = ROULETTE_FIRST_INTERVAL_MS;
  r.nextAt = now + (uint32_t)(r.interval + 0.5f);
}

// À appeler à chaque tour de boucle. Les dates sont calculées à partir de la
// date prévue, pas de l'heure d'appel : un tour de boucle en retard ne change ni
// la durée totale ni le nombre de pas, donc ni le résultat.
inline void rouletteUpdate(Roulette &r, uint32_t now) {
  while (rouletteIsBusy(r) && (int32_t)(now - r.nextAt) >= 0) {
    if (r.phase == ROULETTE_SPINNING) {
      r.color = (r.color + 1) % ROULETTE_COLOR_COUNT;
      r.stepsDone++;
      if (r.stepsDone >= ROULETTE_STEPS) {
        r.phase = ROULETTE_BLINKING;
        r.togglesLeft = ROULETTE_BLINKS * 2;
        r.nextAt += ROULETTE_STOP_HOLD_MS;
      } else {
        r.interval *= ROULETTE_SLOWDOWN;
        r.nextAt += (uint32_t)(r.interval + 0.5f);
      }
    } else {  // ROULETTE_BLINKING
      r.lit = !r.lit;
      r.togglesLeft--;
      if (r.togglesLeft == 0) {
        r.phase = ROULETTE_DONE;
        r.lit = true;
      } else {
        r.nextAt += ROULETTE_BLINK_MS;
      }
    }
  }
}

// Lancement par la télécommande Kino : c'est elle qui tire la couleur de
// départ, l'affiche sur sa roue et l'envoie à la veste (ESP-NOW, type 11). Le
// message part ROULETTE_RESENDS fois, à ROULETTE_RESEND_GAP_MS d'intervalle.
// Sa valeur vaut départ + 3 x numéro de répétition : si la veste rate les
// premiers, elle sait de combien elle est en retard et se recale sur l'écran.
#define ROULETTE_RESENDS 4
#define ROULETTE_RESEND_GAP_MS 60

inline uint8_t rouletteEncodeStart(uint8_t start, uint8_t resend) {
  return start % ROULETTE_COLOR_COUNT + ROULETTE_COLOR_COUNT * resend;
}

inline uint8_t rouletteDecodeStart(uint8_t value) {
  return value % ROULETTE_COLOR_COUNT;
}

inline uint32_t rouletteDecodeLateness(uint8_t value) {
  return (uint32_t)(value / ROULETTE_COLOR_COUNT) * ROULETTE_RESEND_GAP_MS;
}

#endif  // ROULETTE_LOGIC_H
