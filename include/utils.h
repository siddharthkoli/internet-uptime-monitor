#ifndef UTILS_H
#define UTILS_H

// Blink defaults
static const int DEFAULT_BLINK_DELAY_MS = 250;

// Blink helpers
void blinkLED(int delayMs);
void blinkLED(int times, int delayMs = DEFAULT_BLINK_DELAY_MS);

#endif
