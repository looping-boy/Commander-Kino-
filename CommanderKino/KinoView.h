#ifndef KINO_VIEW_H
#define KINO_VIEW_H

// Écran KINO : la roulette de la super contrainte.
// Une touche n'importe où sous les onglets lance la roue. Elle tourne,
// ralentit, tombe sur rouge, jaune ou bleu, et la veste fait exactement la
// même chose au même moment.

#include <math.h>
#include "Config.h"
#include "RouletteLogic.h"
#include "Wheel.h"
#include "EspNowLink.h"

#define WHEEL_CX 88
#define WHEEL_CY 106
#define WHEEL_R 58
#define WHEEL_HUB_R 14
#define PANEL_X 168
#define PANEL_Y 50
#define PANEL_W 146
#define PANEL_H 112

Roulette roulette = {};
int wheelBase = 0;                // décalage de la roue au début du tirage (cases)
float wheelRest = 0.5f;           // position de repos, en cases absolues
RoulettePhase lastDrawnPhase = ROULETTE_IDLE;

static inline void polar(float deg, float radius, int &x, int &y) {
  float rad = deg * (float)M_PI / 180.0f;
  x = WHEEL_CX + (int)lroundf(radius * sinf(rad));
  y = WHEEL_CY - (int)lroundf(radius * cosf(rad));
}

void drawWheel(uint32_t now) {
  float advance = wheelAdvance(roulette, now);
  float rotation = roulette.phase == ROULETTE_IDLE ? wheelRest * WHEEL_SECTOR_DEG
                                                   : wheelRotationDeg(wheelBase, advance);

  sprite.fillRect(0, TAB_BAR_HEIGHT, PANEL_X - 2, SCREEN_HEIGHT - TAB_BAR_HEIGHT, TFT_BLACK);

  // Cases : éventails de triangles tous les 5 degrés.
  for (int s = 0; s < WHEEL_SECTORS; s++) {
    float a0 = s * WHEEL_SECTOR_DEG - rotation;
    uint16_t color = ROULETTE_SCREEN_COLORS[wheelSectorColor(s)];
    const int slices = 6;
    for (int k = 0; k < slices; k++) {
      int x1, y1, x2, y2;
      polar(a0 + k * WHEEL_SECTOR_DEG / slices, WHEEL_R, x1, y1);
      polar(a0 + (k + 1) * WHEEL_SECTOR_DEG / slices, WHEEL_R, x2, y2);
      sprite.fillTriangle(WHEEL_CX, WHEEL_CY, x1, y1, x2, y2, color);
    }
  }
  // Séparations entre les cases
  for (int s = 0; s < WHEEL_SECTORS; s++) {
    int x, y;
    polar(s * WHEEL_SECTOR_DEG - rotation, WHEEL_R, x, y);
    sprite.drawLine(WHEEL_CX, WHEEL_CY, x, y, TFT_BLACK);
  }
  sprite.drawCircle(WHEEL_CX, WHEEL_CY, WHEEL_R, TFT_WHITE);
  sprite.drawCircle(WHEEL_CX, WHEEL_CY, WHEEL_R + 1, TFT_WHITE);
  sprite.fillCircle(WHEEL_CX, WHEEL_CY, WHEEL_HUB_R, TFT_BLACK);
  sprite.drawCircle(WHEEL_CX, WHEEL_CY, WHEEL_HUB_R, TFT_WHITE);

  // Pointeur à 3 heures, qui pique dans la roue
  int tipX = WHEEL_CX + WHEEL_R - 8;
  sprite.fillTriangle(tipX, WHEEL_CY, tipX + 20, WHEEL_CY - 10, tipX + 20, WHEEL_CY + 10, TFT_WHITE);
  sprite.drawTriangle(tipX, WHEEL_CY, tipX + 20, WHEEL_CY - 10, tipX + 20, WHEEL_CY + 10, TFT_BLACK);
}

void drawKinoPanel() {
  const int x = PANEL_X, y = PANEL_Y, w = PANEL_W, h = PANEL_H;
  const int cx = x + w / 2, cy = y + h / 2;
  sprite.fillRect(x - 1, TAB_BAR_HEIGHT, SCREEN_WIDTH - x + 1, SCREEN_HEIGHT - TAB_BAR_HEIGHT, TFT_BLACK);
  sprite.setTextDatum(MC_DATUM);

  if (roulette.phase == ROULETTE_IDLE) {
    sprite.fillRoundRect(x, y, w, h, 14, 0x2104);
    sprite.drawRoundRect(x, y, w, h, 14, TFT_WHITE);
    sprite.setTextColor(TFT_WHITE);
    sprite.drawString("LANCER", cx, cy - 16, 4);
    sprite.drawString("touche l'ecran", cx, cy + 14, 2);
    for (int c = 0; c < 3; c++) sprite.fillCircle(cx - 24 + c * 24, cy + 38, 6, ROULETTE_SCREEN_COLORS[c]);
  } else {
    uint8_t c = roulette.color;
    bool lit = roulette.lit;
    bool finished = roulette.phase != ROULETTE_SPINNING;
    if (lit) {
      sprite.fillRoundRect(x, y, w, h, 14, ROULETTE_SCREEN_COLORS[c]);
      sprite.setTextColor(ROULETTE_TEXT_COLORS[c]);
    } else {
      sprite.fillRoundRect(x, y, w, h, 14, TFT_BLACK);
      sprite.drawRoundRect(x, y, w, h, 14, ROULETTE_SCREEN_COLORS[c]);
      sprite.setTextColor(ROULETTE_SCREEN_COLORS[c]);
    }
    char label[12];
    snprintf(label, sizeof(label), finished ? "%s !" : "%s", ROULETTE_NAMES[c]);
    sprite.drawString(label, cx, roulette.phase == ROULETTE_DONE ? cy - 10 : cy, 4);
    if (roulette.phase == ROULETTE_DONE) {
      sprite.drawString("touche = relancer", cx, cy + 26, 2);
    }
  }
  sprite.setTextDatum(TL_DATUM);
}

void drawKinoView(uint32_t now) {
  drawWheel(now);
  drawKinoPanel();
  lastDrawnPhase = roulette.phase;
}

void launchRoulette(uint32_t now) {
  uint8_t start = esp_random() % ROULETTE_COLOR_COUNT;
  wheelBase = wheelBaseFor(wheelRest, start);
  rouletteStart(roulette, now, start);
  startRouletteSends(now, start);
  // Position de repos à la fin de ce tirage, pour enchaîner le suivant sans saut.
  wheelRest = wheelBase + ROULETTE_STEPS + 0.5f;
  while (wheelRest >= WHEEL_SECTORS) wheelRest -= WHEEL_SECTORS;
}

// Retourne true si l'écran doit être redessiné.
bool handleKinoTouch(int x, int y, uint32_t now) {
  (void)x;
  (void)y;
  if (rouletteIsBusy(roulette)) return false;  // pas de relance pendant que ça tourne
  launchRoulette(now);
  return true;
}

// L'écran KINO doit-il être redessiné à ce tour de boucle ?
bool kinoNeedsRedraw() {
  return rouletteIsBusy(roulette) || roulette.phase != lastDrawnPhase;
}

#endif  // KINO_VIEW_H
