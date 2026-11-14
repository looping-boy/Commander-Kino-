#ifndef CONFIG_H
#define CONFIG_H

// Réglages communs : écran, couleurs, numéros d'animations de la veste.

#include <stdint.h>

// ----- Écran LilyGo T-Display S3 (paysage) -----
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 170
#define TAB_BAR_HEIGHT 40

// ----- Onglets -----
#define TAB_ANIMS 0
#define TAB_KINO 1

// ----- Animations de la veste (mêmes numéros que LedCoatAnim/Constant.h) -----
#define ANIM_STATIC_FULL 0
#define ANIM_ALERT_FULL 1
#define ANIM_VERTICAL_LINE 2
#define ANIM_HORIZONTAL_LINE 3
#define ANIM_DIAGONAL_LINE 4
#define ANIM_CIRCLE_LINE 5
#define ANIM_CROSS_VERTICAL_LINE 6
#define ANIM_CROSS_HORIZONTAL_LINE 7
#define ANIM_COLOR_FULL 8
#define ANIM_ALERT_COLOR_FULL 9
#define ANIM_VERTICAL_RAINBOW 10
#define ANIM_HORIZONTAL_RAINBOW 11
#define ANIM_DIAGONAL_RAINBOW 12
#define ANIM_CIRCLE_RAINBOW 13
#define ANIM_COLORFULL_RAIN 14
#define ANIM_COLORFULL_SPARKLE 15
#define NO_ANIM 255

// ----- Couleurs de la roulette, dans l'ordre de la veste : 0 rouge, 1 jaune, 2 bleu -----
static const char *const ROULETTE_NAMES[3] = {"ROUGE", "JAUNE", "BLEU"};

// RGB565 pour l'écran (le bleu est éclairci pour rester lisible sur l'écran).
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
static const uint16_t ROULETTE_SCREEN_COLORS[3] = {
  (uint16_t)((0xF8 << 8)),                         // rouge  255,0,0
  (uint16_t)((0xF8 << 8) | (0xDC << 3)),           // jaune  255,220,0
  (uint16_t)(((0x28 & 0xF8) << 8) | ((0x6E & 0xFC) << 3) | (0xFF >> 3))  // bleu 40,110,255
};
static const uint16_t ROULETTE_TEXT_COLORS[3] = {0xFFFF, 0x0000, 0xFFFF};  // blanc, noir, blanc

// Luminosités proposées sur l'écran ANIMS (la veste démarre à 100).
static const uint8_t BRIGHTNESS_LEVELS[] = {10, 25, 50, 75, 100, 150, 200, 255};
#define BRIGHTNESS_LEVEL_COUNT (sizeof(BRIGHTNESS_LEVELS) / sizeof(BRIGHTNESS_LEVELS[0]))
#define BRIGHTNESS_START_INDEX 4

// Teinte de départ de la veste (LedCoatAnim/Constant.h : hue = 200).
#define HUE_START 200

#endif  // CONFIG_H
