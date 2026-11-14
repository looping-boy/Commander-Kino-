// Tests sur ordinateur de la roulette de l'écran KINO.
//   c++ -std=c++17 -O2 -I../CommanderKino wheel_test.cpp -o wheel_test && ./wheel_test
//
// Vérifie :
//  1. la case sous le pointeur a toujours la couleur de la roulette (donc de la veste) ;
//  2. la roue ne recule jamais pendant un tirage et ne saute presque pas au lancement ;
//  3. la veste, qu'elle reçoive le 1er, 2e, 3e ou 4e message, affiche la même
//     couleur que l'écran au même moment ;
//  4. chaque couleur sort une fois sur trois.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include "RouletteLogic.h"
#include "Wheel.h"

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { if (fails < 20) { printf("FAIL: " __VA_ARGS__); printf("\n"); } fails++; } } while (0)

int main() {
  std::mt19937 rng(std::random_device{}());

  // 1 et 2 : couleur sous le pointeur, avance monotone, petit saut au lancement
  float rest = 0.5f;
  long samples = 0;
  for (int spin = 0; spin < 2000; spin++) {
    uint8_t start = rng() % 3;
    int base = wheelBaseFor(rest, start);
    float jump = fmodf(base - rest + 2.0f * WHEEL_SECTORS, (float)WHEEL_SECTORS);
    if (jump > WHEEL_SECTORS / 2.0f) jump -= WHEEL_SECTORS;
    CHECK(jump >= -0.5f && jump <= 1.5f, "launch jump %.2f sectors", jump);
    CHECK(base % 3 == start, "base %d not aligned with start %u", base, start);

    Roulette r = {};
    uint32_t t0 = 1000 + rng() % 100000;
    rouletteStart(r, t0, start);
    float prev = -1;
    for (uint32_t t = t0; r.phase != ROULETTE_DONE || t < t0 + 9000; t += 1 + rng() % 3) {
      rouletteUpdate(r, t);
      float adv = wheelAdvance(r, t);
      CHECK(adv + 1e-4f >= prev, "wheel went back: %.4f -> %.4f (spin %d, t+%u)", prev, adv, spin, t - t0);
      prev = adv;
      int sector = wheelSectorUnderPointer(base, adv);
      CHECK(wheelSectorColor(sector) == r.color, "pointer color %u != roulette color %u (spin %d, t+%u, adv %.3f)",
            wheelSectorColor(sector), r.color, spin, t - t0, adv);
      samples++;
      if (t - t0 > 20000) break;
    }
    CHECK(r.phase == ROULETTE_DONE, "spin %d never finished", spin);
    rest = base + ROULETTE_STEPS + 0.5f;
    while (rest >= WHEEL_SECTORS) rest -= WHEEL_SECTORS;
  }
  printf("1-2. %ld moments checked over 2000 spins: pointer color always matches, wheel never goes back\n", samples);

  // 3 : la veste reçoit la répétition n (envoyée à t0 + n x 60 ms) et rattrape son retard
  for (int spin = 0; spin < 500; spin++) {
    uint8_t start = rng() % 3;
    uint8_t received = rng() % ROULETTE_RESENDS;  // première répétition reçue
    uint32_t radioDelay = rng() % 4;              // quelques ms de transmission
    uint32_t t0 = 5000;
    Roulette screen = {}, vest = {};
    rouletteStart(screen, t0, start);
    uint32_t arrival = t0 + received * ROULETTE_RESEND_GAP_MS + radioDelay;
    uint8_t value = rouletteEncodeStart(start, received);
    bool vestStarted = false;
    int run = 0, longestRun = 0;
    for (uint32_t t = t0; t < t0 + 9000; t++) {
      if (!vestStarted && t >= arrival) {
        rouletteStart(vest, t - rouletteDecodeLateness(value), rouletteDecodeStart(value));
        vestStarted = true;
      }
      rouletteUpdate(screen, t);
      if (vestStarted) {
        rouletteUpdate(vest, t);
        if (vest.color != screen.color || vest.lit != screen.lit) {
          run++;
          if (run > longestRun) longestRun = run;
        } else {
          run = 0;
        }
      }
    }
    CHECK(vest.phase == ROULETTE_DONE && vest.color == screen.color, "vest ended on %u, screen on %u", vest.color, screen.color);
    // La veste n'a que le délai radio de retard à chaque changement, jamais plus.
    CHECK(longestRun <= (int)radioDelay, "vest behind by %d ms (radio %u ms, received resend %u)", longestRun, radioDelay, received);
  }
  printf("3. 500 spins with the vest catching the 1st to 4th message: same colors as the screen, never more than the radio delay behind\n");

  // 4 : équité
  const int N = 300000;
  long count[3] = {0, 0, 0};
  for (int i = 0; i < N; i++) {
    Roulette r = {};
    rouletteStart(r, 0, rng() % 3);
    for (uint32_t t = 0; rouletteIsBusy(r); t += 20) rouletteUpdate(r, t);
    count[r.color]++;
  }
  double e = N / 3.0, chi = 0;
  for (long c : count) chi += (c - e) * (c - e) / e;
  printf("4. %d spins: rouge %.2f%%, jaune %.2f%%, bleu %.2f%% (chi2 %.2f)\n", N, 100.0 * count[0] / N,
         100.0 * count[1] / N, 100.0 * count[2] / N, chi);
  CHECK(chi < 13.8, "chi2 %.2f", chi);

  printf(fails ? "\n%d CHECK(S) FAILED\n" : "\nALL CHECKS PASSED\n", fails);
  return fails ? 1 : 0;
}
