#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>

enum Direction {
    CLOCKWISE, COUNTERCLOCKWISE
};

void DrawRainbow(CRGB* g_LEDs, int numLEDs, int deltaTime, Direction direction) {
    static uint8_t initialHue = 0; // it's static so that we don't lose its value between iterations
    const uint8_t deltaHue = 16;
    const uint8_t hueDensity = 4;
    if (direction == CLOCKWISE) {
        fill_rainbow(g_LEDs, numLEDs, initialHue += hueDensity, deltaHue);
    } else if (direction == COUNTERCLOCKWISE) {
        fill_rainbow(g_LEDs, numLEDs, initialHue -= hueDensity, deltaHue);
    }
}