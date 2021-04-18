#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>

void DrawComet(CRGB* g_LEDs, int numLEDs, int deltaTime, uint8_t speed, int cometSize) {
    const byte fadeAmt = 128;
    const int deltaHue = 4;

    static byte hue = HUE_RED;
    static int iDirection = 1; // Current direction (+/- 1)
    static int iPos = 0; // Current comet position on strips

    static int cometDeltaTime = 0;
    const int speeds[10] = {54, 48, 42, 36, 30, 24, 18, 12, 6, 0};

    cometDeltaTime += deltaTime;

    if (cometDeltaTime >= speeds[speed - 1]) {
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

        cometDeltaTime = 0;
    }
}