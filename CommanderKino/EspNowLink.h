#ifndef ESP_NOW_LINK_H
#define ESP_NOW_LINK_H

// Envoi des commandes à la veste en ESP-NOW (diffusion sur le canal 3), avec
// les mêmes messages de 2 octets que l'ancienne télécommande et que
// LedCoatAnim/ESPReceiver.h.

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "RouletteLogic.h"

#define WIFI_CHANNEL 3

const uint8_t MESSAGE_TYPE_BRIGHTNESS = 0;
const uint8_t MESSAGE_TYPE_HUE = 1;
const uint8_t MESSAGE_TYPE_PATTERN = 3;
const uint8_t MESSAGE_TYPE_FORCE_BRIGHTNESS = 5;  // 1 = la télécommande prend la main sur la molette du manteau V1
const uint8_t MESSAGE_TYPE_ROULETTE_FROM = 11;  // valeur = rouletteEncodeStart(départ, répétition)

typedef struct Message {
  uint8_t message_type;
  uint8_t value;
} Message;

uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool espNowReady = false;

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    return;
  }
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  espNowReady = esp_now_add_peer(&peerInfo) == ESP_OK;
}

void sendMessage(uint8_t type, uint8_t value) {
  if (!espNowReady) return;
  Message message = {type, value};
  esp_now_send(broadcastAddress, (uint8_t *)&message, sizeof(message));
}

// Animation, couleur, luminosité : envoyé 2 fois. La veste partage sa radio
// avec le Bluetooth et peut rater un message ; le recevoir deux fois ne change rien.
void sendCommand(uint8_t type, uint8_t value) {
  sendMessage(type, value);
  delay(25);
  sendMessage(type, value);
}

// Luminosité : le manteau V1 lit sa molette à chaque tour et écraserait la
// valeur reçue. On lui dit d'abord d'ignorer la molette (jusqu'à son
// redémarrage). La veste proto 2 n'a pas de molette et ignore ce message.
void sendBrightness(uint8_t level) {
  sendCommand(MESSAGE_TYPE_FORCE_BRIGHTNESS, 1);
  sendCommand(MESSAGE_TYPE_BRIGHTNESS, level);
}

// ----- Lancement de la roulette, sans bloquer l'animation de l'écran -----
uint8_t rouletteSendStart = 0;
uint8_t rouletteSendsDone = ROULETTE_RESENDS;  // rien à envoyer au démarrage
uint32_t rouletteSendT0 = 0;

void startRouletteSends(uint32_t now, uint8_t startColor) {
  rouletteSendStart = startColor;
  rouletteSendT0 = now;
  rouletteSendsDone = 0;
}

// À appeler à chaque tour de boucle : envoie les répétitions à l'heure prévue.
void updateRouletteSends(uint32_t now) {
  while (rouletteSendsDone < ROULETTE_RESENDS
         && (int32_t)(now - (rouletteSendT0 + rouletteSendsDone * ROULETTE_RESEND_GAP_MS)) >= 0) {
    sendMessage(MESSAGE_TYPE_ROULETTE_FROM, rouletteEncodeStart(rouletteSendStart, rouletteSendsDone));
    rouletteSendsDone++;
  }
}

#endif  // ESP_NOW_LINK_H
