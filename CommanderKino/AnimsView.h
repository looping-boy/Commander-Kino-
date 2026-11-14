#ifndef ANIMS_VIEW_H
#define ANIMS_VIEW_H

// Écran ANIMS : les animations habituelles de la veste.
//   Ligne 1 : les arcs-en-ciel et animations multicolores.
//   Ligne 2 : les animations d'une seule teinte (celle de la barre de couleur).
//   Ligne 3 : barre de couleur (teinte) et luminosité - / +.
// Les animations GIF (16 à 21) ne sont pas codées dans la veste : elles n'y sont pas.

#include "Config.h"
#include "EspNowLink.h"

#define TILE_SIZE 30
#define TILE_INNER 26
#define TILE_STEP 40
#define TILE_X0 4
#define ROW1_Y (TAB_BAR_HEIGHT + 8)
#define ROW2_Y (ROW1_Y + TILE_STEP)
#define ROW3_Y (ROW2_Y + TILE_STEP)

#define HUE_BAR_X 4
#define HUE_BAR_W 192
#define BRIGHT_MINUS_X 204
#define BRIGHT_PLUS_X 284
#define BRIGHT_BUTTON_W 32

enum AnimIcon : uint8_t {
  ICON_RAINBOW_V, ICON_RAINBOW_H, ICON_RAINBOW_D, ICON_RAINBOW_C,
  ICON_RAIN, ICON_SPARKLE, ICON_RAINBOW_FADE, ICON_RAINBOW_PULSE,
  ICON_SOLID, ICON_PULSE, ICON_LINE_V, ICON_LINE_H, ICON_LINE_D, ICON_CIRCLE, ICON_CROSS, ICON_X
};

struct AnimTile {
  uint8_t anim;
  AnimIcon icon;
};

const AnimTile ANIM_ROWS[2][8] = {
  {{ANIM_VERTICAL_RAINBOW, ICON_RAINBOW_V}, {ANIM_HORIZONTAL_RAINBOW, ICON_RAINBOW_H},
   {ANIM_DIAGONAL_RAINBOW, ICON_RAINBOW_D}, {ANIM_CIRCLE_RAINBOW, ICON_RAINBOW_C},
   {ANIM_COLORFULL_RAIN, ICON_RAIN}, {ANIM_COLORFULL_SPARKLE, ICON_SPARKLE},
   {ANIM_COLOR_FULL, ICON_RAINBOW_FADE}, {ANIM_ALERT_COLOR_FULL, ICON_RAINBOW_PULSE}},
  {{ANIM_STATIC_FULL, ICON_SOLID}, {ANIM_ALERT_FULL, ICON_PULSE},
   {ANIM_VERTICAL_LINE, ICON_LINE_V}, {ANIM_HORIZONTAL_LINE, ICON_LINE_H},
   {ANIM_DIAGONAL_LINE, ICON_LINE_D}, {ANIM_CIRCLE_LINE, ICON_CIRCLE},
   {ANIM_CROSS_VERTICAL_LINE, ICON_CROSS}, {ANIM_CROSS_HORIZONTAL_LINE, ICON_X}}
};

uint8_t selectedAnim = NO_ANIM;
uint8_t hueSelected = HUE_START;
uint8_t brightnessIndex = BRIGHTNESS_START_INDEX;

// Teinte FastLED (0 rouge, 64 jaune, 96 vert, 160 bleu...) vers RGB565, pour
// que l'écran montre à peu près la couleur de la veste.
uint16_t hueToColor(uint8_t hue, uint8_t value = 255) {
  static const uint8_t anchors[9][3] = {
    {255, 0, 0}, {255, 128, 0}, {255, 255, 0}, {0, 255, 0}, {0, 255, 255},
    {0, 0, 255}, {128, 0, 255}, {255, 0, 128}, {255, 0, 0}
  };
  uint8_t section = hue / 32;
  float t = (hue % 32) / 32.0f;
  uint8_t rgb[3];
  for (int c = 0; c < 3; c++) {
    float v = anchors[section][c] + t * (anchors[section + 1][c] - anchors[section][c]);
    rgb[c] = (uint8_t)(v * value / 255);
  }
  return rgb565(rgb[0], rgb[1], rgb[2]);
}

void drawAnimIcon(AnimIcon icon, int x, int y) {
  const int n = TILE_INNER;
  const uint16_t hueColor = hueToColor(hueSelected);
  sprite.fillRoundRect(x, y, n, n, 6, TFT_BLACK);
  switch (icon) {
    case ICON_RAINBOW_V:
      for (int i = 0; i < n; i++) sprite.drawFastVLine(x + i, y + 1, n - 2, hueToColor(i * 255 / n));
      break;
    case ICON_RAINBOW_H:
      for (int j = 0; j < n; j++) sprite.drawFastHLine(x + 1, y + j, n - 2, hueToColor(j * 255 / n));
      break;
    case ICON_RAINBOW_D:
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) sprite.drawPixel(x + i, y + j, hueToColor((i + j) * 255 / (2 * n)));
      break;
    case ICON_RAINBOW_C:
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
          float d = sqrtf((i - n / 2.0f) * (i - n / 2.0f) + (j - n / 2.0f) * (j - n / 2.0f));
          sprite.drawPixel(x + i, y + j, hueToColor((uint8_t)(d * 255 / (n * 0.72f))));
        }
      break;
    case ICON_RAIN:
      for (int k = 0; k < 7; k++) {
        int cx = x + 3 + k * 3 + (k % 2);
        int cy = y + 3 + (k * 7) % 14;
        sprite.drawFastVLine(cx, cy, 7, hueToColor(k * 36));
      }
      break;
    case ICON_SPARKLE:
      for (int k = 0; k < 12; k++) {
        int px = x + 3 + (k * 7) % 20;
        int py = y + 3 + (k * 11) % 20;
        sprite.fillRect(px, py, 2, 2, hueToColor(k * 21));
      }
      break;
    case ICON_RAINBOW_FADE:
      sprite.fillRoundRect(x, y, n / 2, n / 2, 3, hueToColor(0));
      sprite.fillRoundRect(x + n / 2, y, n / 2, n / 2, 3, hueToColor(64));
      sprite.fillRoundRect(x + n / 2, y + n / 2, n / 2, n / 2, 3, hueToColor(128));
      sprite.fillRoundRect(x, y + n / 2, n / 2, n / 2, 3, hueToColor(192));
      break;
    case ICON_RAINBOW_PULSE:
      for (int r = 12; r > 0; r -= 3) sprite.fillCircle(x + n / 2, y + n / 2, r, hueToColor(r * 18, 80 + r * 14));
      break;
    case ICON_SOLID:
      sprite.fillRoundRect(x, y, n, n, 6, hueColor);
      break;
    case ICON_PULSE:
      sprite.fillRoundRect(x, y, n, n, 6, hueToColor(hueSelected, 90));
      sprite.drawCircle(x + n / 2, y + n / 2, 8, hueColor);
      break;
    case ICON_LINE_V:
      sprite.fillRoundRect(x + n / 2 - 2, y + 2, 4, n - 4, 2, hueColor);
      break;
    case ICON_LINE_H:
      sprite.fillRoundRect(x + 2, y + n / 2 - 2, n - 4, 4, 2, hueColor);
      break;
    case ICON_LINE_D:
      for (int w = -1; w <= 1; w++) sprite.drawLine(x + 3 + w, y + n - 3, x + n - 3 + w, y + 3, hueColor);
      break;
    case ICON_CIRCLE:
      sprite.drawCircle(x + n / 2, y + n / 2, 9, hueColor);
      sprite.drawCircle(x + n / 2, y + n / 2, 8, hueColor);
      break;
    case ICON_CROSS:
      sprite.fillRect(x + n / 2 - 1, y + 3, 3, n - 6, hueColor);
      sprite.fillRect(x + 3, y + n / 2 - 1, n - 6, 3, hueColor);
      break;
    case ICON_X:
      for (int w = -1; w <= 1; w++) {
        sprite.drawLine(x + 4 + w, y + 4, x + n - 4 + w, y + n - 4, hueColor);
        sprite.drawLine(x + n - 4 + w, y + 4, x + 4 + w, y + n - 4, hueColor);
      }
      break;
  }
}

void drawAnimTiles() {
  for (int row = 0; row < 2; row++) {
    int y = row == 0 ? ROW1_Y : ROW2_Y;
    for (int i = 0; i < 8; i++) {
      int x = TILE_X0 + i * TILE_STEP;
      const AnimTile &tile = ANIM_ROWS[row][i];
      drawAnimIcon(tile.icon, x + 2, y + 2);
      bool selected = tile.anim == selectedAnim;
      sprite.drawRoundRect(x, y, TILE_SIZE, TILE_SIZE, 8, selected ? TFT_WHITE : 0x4208);
      if (selected) sprite.drawRoundRect(x - 1, y - 1, TILE_SIZE + 2, TILE_SIZE + 2, 9, TFT_WHITE);
    }
  }
}

void drawHueAndBrightness() {
  // Barre de couleur
  for (int i = 0; i < HUE_BAR_W; i++) {
    sprite.drawFastVLine(HUE_BAR_X + i, ROW3_Y + 2, TILE_SIZE - 4, hueToColor(i * 255 / (HUE_BAR_W - 1)));
  }
  sprite.drawRoundRect(HUE_BAR_X - 1, ROW3_Y + 1, HUE_BAR_W + 2, TILE_SIZE - 2, 4, 0x4208);
  int markerX = HUE_BAR_X + hueSelected * (HUE_BAR_W - 1) / 255;
  sprite.fillRect(markerX - 2, ROW3_Y - 1, 5, TILE_SIZE + 2, TFT_WHITE);
  sprite.fillRect(markerX - 1, ROW3_Y + 1, 3, TILE_SIZE - 2, hueToColor(hueSelected));

  // Luminosité
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.drawRoundRect(BRIGHT_MINUS_X, ROW3_Y, BRIGHT_BUTTON_W, TILE_SIZE, 8, TFT_WHITE);
  sprite.drawString("-", BRIGHT_MINUS_X + BRIGHT_BUTTON_W / 2, ROW3_Y + TILE_SIZE / 2, 4);
  sprite.drawRoundRect(BRIGHT_PLUS_X, ROW3_Y, BRIGHT_BUTTON_W, TILE_SIZE, 8, TFT_WHITE);
  sprite.drawString("+", BRIGHT_PLUS_X + BRIGHT_BUTTON_W / 2, ROW3_Y + TILE_SIZE / 2, 4);
  char level[8];
  snprintf(level, sizeof(level), "%d", BRIGHTNESS_LEVELS[brightnessIndex]);
  int midX = (BRIGHT_MINUS_X + BRIGHT_BUTTON_W + BRIGHT_PLUS_X) / 2;
  sprite.fillRect(BRIGHT_MINUS_X + BRIGHT_BUTTON_W + 1, ROW3_Y, BRIGHT_PLUS_X - BRIGHT_MINUS_X - BRIGHT_BUTTON_W - 2, TILE_SIZE, TFT_BLACK);
  sprite.drawString(level, midX, ROW3_Y + 9, 2);
  sprite.drawString("LUM", midX, ROW3_Y + 23, 1);
  sprite.setTextDatum(TL_DATUM);
}

void drawAnimsView() {
  drawAnimTiles();
  drawHueAndBrightness();
}

// x, y : coordonnées écran (x vers la droite, y vers le bas).
// Retourne true si l'écran doit être redessiné.
bool handleAnimsTouch(int x, int y) {
  // Lignes d'animations
  for (int row = 0; row < 2; row++) {
    int rowY = row == 0 ? ROW1_Y : ROW2_Y;
    if (y >= rowY - 4 && y < rowY + TILE_SIZE + 4) {
      int i = (x - TILE_X0 + 5) / TILE_STEP;
      if (i < 0 || i > 7) return false;
      selectedAnim = ANIM_ROWS[row][i].anim;
      sendCommand(MESSAGE_TYPE_PATTERN, selectedAnim);
      return true;
    }
  }
  // Barre de couleur et luminosité
  if (y >= ROW3_Y - 4 && y < ROW3_Y + TILE_SIZE + 6) {
    if (x >= HUE_BAR_X - 4 && x < HUE_BAR_X + HUE_BAR_W + 4) {
      int pos = constrain(x - HUE_BAR_X, 0, HUE_BAR_W - 1);
      hueSelected = pos * 255 / (HUE_BAR_W - 1);
      sendCommand(MESSAGE_TYPE_HUE, hueSelected);
      return true;
    }
    if (x >= BRIGHT_MINUS_X - 4 && x < BRIGHT_MINUS_X + BRIGHT_BUTTON_W + 4) {
      if (brightnessIndex > 0) brightnessIndex--;
      sendBrightness(BRIGHTNESS_LEVELS[brightnessIndex]);
      return true;
    }
    if (x >= BRIGHT_PLUS_X - 4 && x < BRIGHT_PLUS_X + BRIGHT_BUTTON_W + 4) {
      if (brightnessIndex < BRIGHTNESS_LEVEL_COUNT - 1) brightnessIndex++;
      sendBrightness(BRIGHTNESS_LEVELS[brightnessIndex]);
      return true;
    }
  }
  return false;
}

#endif  // ANIMS_VIEW_H
