#ifndef WHEEL_H
#define WHEEL_H

// Position de la roue de l'écran KINO. Calcul pur, sans écran, testé sur
// ordinateur (test/wheel_test.cpp).
//
// La roue a WHEEL_SECTORS cases : rouge, jaune, bleu, rouge, jaune, bleu...
// Le pointeur est à 3 heures. La roue avance d'une case par pas de la
// roulette. La case sous le pointeur a donc toujours la couleur que la veste
// affiche au même moment (roulette.color).

#include <math.h>
#include "RouletteLogic.h"

#define WHEEL_SECTORS 12                       // multiple de 3
#define WHEEL_SECTOR_DEG (360.0f / WHEEL_SECTORS)
#define WHEEL_POINTER_DEG 90.0f                // pointeur à 3 heures : 3 cases, multiple de 3

// Avancée de la roue depuis le début du tirage, en cases.
// - pendant le défilement : nombre de pas faits + fraction du pas en cours ;
// - pendant la pause qui suit l'arrêt : la roue glisse d'une demi-case en
//   ralentissant, pour finir pile au milieu de la case gagnante ;
// - ensuite : ROULETTE_STEPS + 0,5.
// Au repos avant le premier tirage : 0,5 (milieu d'une case).
inline float wheelAdvance(const Roulette &r, uint32_t now) {
  if (r.phase == ROULETTE_SPINNING) {
    uint32_t stepMs = (uint32_t)(r.interval + 0.5f);
    int32_t left = (int32_t)(r.nextAt - now);
    if (left < 0) left = 0;
    if ((uint32_t)left > stepMs) left = stepMs;
    float frac = 1.0f - (float)left / (float)stepMs;
    if (frac >= 1.0f) frac = 0.999f;  // la case change au pas suivant, pas avant
    return r.stepsDone + frac;
  }
  if (r.phase == ROULETTE_BLINKING && r.togglesLeft == ROULETTE_BLINKS * 2) {
    int32_t left = (int32_t)(r.nextAt - now);
    if (left < 0) left = 0;
    if (left > ROULETTE_STOP_HOLD_MS) left = ROULETTE_STOP_HOLD_MS;
    float u = 1.0f - (float)left / ROULETTE_STOP_HOLD_MS;
    return ROULETTE_STEPS + 0.5f * (1.0f - (1.0f - u) * (1.0f - u));
  }
  if (r.phase == ROULETTE_IDLE) return 0.5f;
  return ROULETTE_STEPS + 0.5f;
}

// Décalage de la roue (en cases) au lancement d'un tirage : on part de la
// position de repos et on choisit la case de départ de la bonne couleur la
// plus proche, pour que la roue ne saute presque pas.
inline int wheelBaseFor(float restPosition, uint8_t startColor) {
  int restIndex = (int)floorf(restPosition);
  int shift = ((int)startColor - restIndex) % 3;
  if (shift < 0) shift += 3;
  int base = (restIndex + shift) % WHEEL_SECTORS;
  if (base < 0) base += WHEEL_SECTORS;
  return base;
}

// Angle de rotation de la roue, en degrés.
inline float wheelRotationDeg(int base, float advance) {
  return (base + advance) * WHEEL_SECTOR_DEG;
}

// Couleur de la case n (fixe sur la roue).
inline uint8_t wheelSectorColor(int sector) {
  return ((sector % 3) + 3) % 3;
}

// Case sous le pointeur, pour vérifier le dessin.
inline int wheelSectorUnderPointer(int base, float advance) {
  float a = wheelRotationDeg(base, advance) + WHEEL_POINTER_DEG;
  int s = (int)floorf(a / WHEEL_SECTOR_DEG + 1e-4f);
  return ((s % WHEEL_SECTORS) + WHEEL_SECTORS) % WHEEL_SECTORS;
}

#endif  // WHEEL_H
