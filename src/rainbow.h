#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>

enum Direction {
    CLOCKWISE, COUNTERCLOCKWISE
};

void DrawRainbow(CRGB* g_LEDs, int numLEDs, int deltaTime, uint8_t speed, Direction direction) {
    static uint8_t initialHue = 0; // it's static so that we don't lose its value between iterations
    const uint8_t deltaHue = 16;

    const uint8_t speeds[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
 
    if (direction == CLOCKWISE) {
        fill_rainbow(g_LEDs, numLEDs, initialHue += (speeds[speed - 1]), deltaHue);
    } else if (direction == COUNTERCLOCKWISE) {
        fill_rainbow(g_LEDs, numLEDs, initialHue -= (speeds[speed - 1]), deltaHue);
    }
}