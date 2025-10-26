#include <FS.h>
#include <SPIFFS.h>

#include "storage.h"
#include "logger.h"

const String SERVICE = "storage";

void storageInit(bool clearBeforeBegin) {
  if (!SPIFFS.begin(true)) {
    logln("SPIFFS init failed!", LOG_LEVEL_ERROR, SERVICE);
    return;
  }
  if (clearBeforeBegin) {
    clearFailedLogs();
  }
}

void saveFailedLog(String jsonStr) {
  File f = SPIFFS.open("/failed_logs.txt", FILE_APPEND);
  if (!f) {
    logln("Failed to open failed_logs.txt for writing", LOG_LEVEL_ERROR, SERVICE);
    return;
  }
  f.println(jsonStr);
  f.close();
}

String readFailedLogs() {
  File f = SPIFFS.open("/failed_logs.txt", FILE_READ);
  if (!f) {
    logln("Failed to open failed_logs.txt for reading", LOG_LEVEL_ERROR, SERVICE);
    return "[]";
  }
  String all = "[";
  bool first = true;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (!first) all += ",";
    all += line;
    first = false;
  }
  all += "]";
  f.close();
  return all;
}

void clearFailedLogs() {
  logln("\nClearing previously failed logs", LOG_LEVEL_INFO, SERVICE);
  for (int i = 0; i < 3; i++) {
    bool success = SPIFFS.remove("/failed_logs.txt");
    if (success) {
      logln("Successfully cleared failed_logs.txt", LOG_LEVEL_INFO, SERVICE);
    } else {
      logln("Failed to clear failed_logs.txt. Possibly doesn't exist.", LOG_LEVEL_WARNING, SERVICE);
    }
    delay(2000);
  }
}
