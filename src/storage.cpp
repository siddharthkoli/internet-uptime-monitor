#include <FS.h>
#include <SPIFFS.h>
#include "storage.h"

void storageInit() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS init failed!");
  }
}

void saveFailedLog(String jsonStr) {
  File f = SPIFFS.open("/failed_logs.txt", FILE_APPEND);
  if (!f) return;
  f.println(jsonStr);
  f.close();
}

String readFailedLogs() {
  File f = SPIFFS.open("/failed_logs.txt", FILE_READ);
  if (!f) return "[]";
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
  SPIFFS.remove("/failed_logs.txt");
}
