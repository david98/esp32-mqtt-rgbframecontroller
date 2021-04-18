#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>

#define NUM_COLORS 5
static const CRGB TwinkleColors[NUM_COLORS] = {
    CRGB::Red,
    CRGB::Blue,
    CRGB::Purple,
    CRGB::Green,
    CRGB::Orange
};

void DrawTwinkle(CRGB* g_LEDs, int numLEDs, int deltaTime, uint8_t speed) {
    static int passCount = 0;
    static int twinkleDeltaTime = 0;

    const int speeds[10] = {450, 400, 350, 300, 250, 200, 150, 100, 50, 0};

    twinkleDeltaTime += deltaTime;

    if (twinkleDeltaTime > speeds[speed - 1]) {
        passCount++;
    
        if (passCount >= numLEDs / 4) {
            passCount = 0;
            FastLED.clear(false);
        }

        g_LEDs[random(numLEDs)] = TwinkleColors[random(NUM_COLORS)];
        twinkleDeltaTime = 0;
    }
}