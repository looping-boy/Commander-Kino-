// Télécommande LilyGo T-Display S3 de la veste LED, version Kino.
//
// Deux écrans :
//   ANIMS : les animations habituelles de la veste, la couleur et la luminosité.
//   KINO  : la roulette de la super contrainte (rouge, jaune ou bleu).
//
// La veste doit tourner avec le firmware LedVestProto2 de la branche
// feat/roulette-kino (ESP-NOW + message de roulette 11).
//
// Boutons physiques : luminosité de l'écran de la télécommande (comme avant).

#define TOUCH_MODULES_CST_SELF
#include <Arduino.h>
#include <Wire.h>
#include "TFT_eSPI.h"  // configuré pour la T-Display S3 (Setup206_LilyGo_T_Display_S3)
#include "TouchLib.h"
#include "pin_config.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS820_SLAVE_ADDRESS, PIN_TOUCH_RES);

#include "Config.h"
#include "EspNowLink.h"
#include "AnimsView.h"
#include "KinoView.h"

uint8_t activeTab = TAB_ANIMS;
uint8_t backLight = 120;
bool touchHeld = false;
uint32_t lastBacklightCheck = 0;

#define TAB_W 100
#define TAB_H 32
#define TAB_Y 4
#define TAB_ANIMS_X 4
#define TAB_KINO_X 110

void drawTab(int x, const char *label, bool active) {
  sprite.fillRoundRect(x, TAB_Y, TAB_W, TAB_H, 10, active ? TFT_WHITE : TFT_BLACK);
  sprite.drawRoundRect(x, TAB_Y, TAB_W, TAB_H, 10, TFT_WHITE);
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(active ? TFT_BLACK : TFT_WHITE);
  sprite.drawString(label, x + TAB_W / 2, TAB_Y + TAB_H / 2 + 1, 2);
  sprite.setTextDatum(TL_DATUM);
}

void drawTabBar() {
  sprite.fillRect(0, 0, SCREEN_WIDTH, TAB_BAR_HEIGHT, TFT_BLACK);
  drawTab(TAB_ANIMS_X, "ANIMS", activeTab == TAB_ANIMS);
  drawTab(TAB_KINO_X, "KINO", activeTab == TAB_KINO);
  sprite.setTextDatum(MR_DATUM);
  sprite.setTextColor(espNowReady ? 0x8410 : TFT_RED);
  sprite.drawString(espNowReady ? "LED VEST" : "RADIO KO", SCREEN_WIDTH - 6, TAB_Y + TAB_H / 2 + 1, 2);
  sprite.setTextDatum(TL_DATUM);
}

void drawScreen(uint32_t now) {
  sprite.fillSprite(TFT_BLACK);
  drawTabBar();
  if (activeTab == TAB_ANIMS) {
    drawAnimsView();
  } else {
    drawKinoView(now);
  }
  sprite.pushSprite(0, 0);
}

void setBacklight(uint8_t level) {
  backLight = level;
  ledcWrite(0, backLight);
}

void setup() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_TOUCH_RES, OUTPUT);
  digitalWrite(PIN_TOUCH_RES, LOW);
  delay(500);
  digitalWrite(PIN_TOUCH_RES, HIGH);

  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);

  ledcSetup(0, 10000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  setBacklight(backLight);

  tft.begin();
  tft.setRotation(1);
  sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);

  setupEspNow();
  drawScreen(millis());
}

void handleTouch(int x, int y, uint32_t now) {
  bool redraw = false;
  if (y < TAB_BAR_HEIGHT) {
    uint8_t tab = activeTab;
    if (x < TAB_KINO_X - 3) tab = TAB_ANIMS;
    else if (x < TAB_KINO_X + TAB_W + 3) tab = TAB_KINO;
    redraw = tab != activeTab;
    activeTab = tab;
  } else if (activeTab == TAB_ANIMS) {
    redraw = handleAnimsTouch(x, y);
  } else {
    redraw = handleKinoTouch(x, y, now);
  }
  if (redraw) drawScreen(millis());
}

void loop() {
  uint32_t now = millis();

  updateRouletteSends(now);
  rouletteUpdate(roulette, now);

  // Luminosité de l'écran avec les deux boutons physiques
  if (now - lastBacklightCheck >= 200) {
    lastBacklightCheck = now;
    if (digitalRead(PIN_BUTTON_2) == LOW && backLight <= 245) setBacklight(backLight + 10);
    if (digitalRead(PIN_BUTTON_1) == LOW && backLight >= 20) setBacklight(backLight - 10);
  }

  // Toucher : une action par appui (au moment où le doigt se pose)
  if (touch.read()) {
    if (!touchHeld) {
      touchHeld = true;
      TP_Point t = touch.getPoint(0);
      if (t.x < SCREEN_HEIGHT) {
        handleTouch(t.y, SCREEN_HEIGHT - t.x, now);  // repère écran : x vers la droite, y vers le bas
      }
    }
  } else {
    touchHeld = false;
  }

  if (activeTab == TAB_KINO && kinoNeedsRedraw()) {
    drawScreen(now);
  }

  delay(rouletteIsBusy(roulette) ? 1 : 10);
}
