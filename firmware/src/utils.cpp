#include <Arduino.h>
#include "utils.h"

void blinkLED(int delayMs) {
    digitalWrite(LED_BUILTIN, HIGH);
    ::delay(delayMs);
    digitalWrite(LED_BUILTIN, LOW);
    ::delay(delayMs);
}

void blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        blinkLED(delayMs);
    }
}