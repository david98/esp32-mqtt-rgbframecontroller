#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>

void DrawMarquee(CRGB* g_LEDs, int numLEDs, int deltaTime, uint8_t speed) {
    static byte j = HUE_BLUE;

    static int marqueeDeltaTime = 0;
    const int speeds[10] = {100, 90, 80, 70, 60, 50, 40, 30, 20, 10};    
    marqueeDeltaTime += deltaTime;

    if (marqueeDeltaTime >= speeds[speed - 1]) {
        j += 4;
        byte k = j;

        CRGB c;
        for (int i = 0; i < numLEDs; i++) {
            g_LEDs[i] = c.setHue(k += 8);
        }

        static int scroll = 0;
        scroll++;

        for (int i = scroll % 5; i < numLEDs; i += 5) {
            g_LEDs[i] = CRGB::Black;
        }
        marqueeDeltaTime = 0;
    }
}