#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include "time.h"

#include "time_utils.h"
#include "logger.h"

const String SERVICE = "time_utils";

extern Preferences prefs;

bool timeSynced = false;

// ==== NTP Config ====
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;   // for IST (UTC+5:30)
const int daylightOffset_sec = 0;

void syncTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("Time synced");
    timeSynced = true;
  } else {
    Serial.println("Failed to sync time");
    timeSynced = false;
  }
}

int getMinuteOfDay() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    return timeinfo.tm_hour * 60 + timeinfo.tm_min;
  } else {
    return prefs.getInt("minute", 0);  // fallback
  }
}

void saveMinute(int minute) {
  prefs.putInt("minute", minute);
}

unsigned long long getCurrentTimestampNano() {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  unsigned long long timestamp_ns = (unsigned long long)tv.tv_sec * 1000000000ULL + (unsigned long long)tv.tv_usec * 1000ULL;
  return timestamp_ns;
}

void waitUntilNextFullMinute() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  int seconds = timeinfo.tm_sec;
  int delayMs = (60 - seconds) * 1000;
  logf("Waiting %d seconds to align with next full minute...\n", LOG_LEVEL_INFO, SERVICE, 60 - seconds);
  delay(delayMs);
}
