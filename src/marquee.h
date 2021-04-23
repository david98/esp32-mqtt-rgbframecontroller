#include <Arduino.h>
#include "helpers.h"
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

void DrawFluidMarquee(CRGB* g_LEDs, int numLEDs, int deltaTime, uint8_t speed, CRGB color)
{
    FastLED.clear(false);
    static float scroll = -5.0f;

    const float speeds[10] = { 0.01, 0.03, 0.05, 0.07, 0.09, 0.1, 0.12, 0.14, 0.16, 0.18 };

    scroll += speeds[speed - 1];
    if (scroll > 0.0)
        scroll -= 5.0;

    for (float i = scroll; i < numLEDs; i+= 5)
    {
        DrawPixels(i, 3, color);
    }
}