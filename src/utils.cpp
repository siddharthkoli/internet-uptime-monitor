#include <Arduino.h>

void blinkLED(int delay) {
    digitalWrite(LED_BUILTIN, HIGH);
    ::delay(delay);
    digitalWrite(LED_BUILTIN, LOW);
    ::delay(delay);
}

void blinkLED(int times, int delay) {
    for (int i = 0; i < times; i++) {
        blinkLED(delay);
    }
}