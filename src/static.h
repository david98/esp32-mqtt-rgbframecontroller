#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>

void DrawStatic(CRGB* g_LEDs, int numLEDs, int deltaTime, uint8_t speed, CRGB color) {
    FastLED.clear(false);
    fill_solid(g_LEDs, numLEDs, color);
}
