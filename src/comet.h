#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>

void DrawComet(CRGB* g_LEDs, int numLEDs, int deltaTime, int cometSize) {
    const byte fadeAmt = 128;
    const int deltaHue = 4;

    static byte hue = HUE_RED;
    static int iDirection = 1; // Current direction (+/- 1)
    static int iPos = 0; // Current comet position on strips

    hue += deltaHue;
    iPos += iDirection;
    
    if (iPos == (numLEDs - cometSize) || iPos == 0) {
        iDirection *= -1;
    }

    for (int i = 0; i < cometSize; i++) {
        g_LEDs[iPos + i].setHue(hue);
    }

    for (int j = 0; j < numLEDs; j++) {
        if (random(2) == 1) {
            g_LEDs[j].fadeToBlackBy(fadeAmt);
        }
    }
}