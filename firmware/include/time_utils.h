#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Preferences.h>

// Time / NTP utilities
void syncTime();
int getMinuteOfDay();
void saveMinute(int minute);
void waitUntilNextFullMinute();
unsigned long long getCurrentTimestampNano();

// Exposed flag indicating whether NTP/time has been synced
extern bool timeSynced;

#endif