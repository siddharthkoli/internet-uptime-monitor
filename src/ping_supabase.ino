#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <FS.h>
#include <SPIFFS.h>
#include "time.h"

#include "utils.h"
#include "secrets.h"

// ==== CONFIG ====
const char* WIFI_SSID = ENV_WIFI_SSID;
const char* WIFI_PASS = ENV_WIFI_PASS;
const char* EDGE_FUNCTION_URL = ENV_EDGE_FUNCTION_URL;
const char* DEVICE_ID = "ESP32-01";
const char* JWT = ENV_JWT;
const long SEND_INTERVAL = 60000;  // 1 min

// ==== Globals ====
Preferences prefs;
bool timeSynced = false;

// ==== NTP Config ====
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;   // for IST (UTC+5:30)
const int daylightOffset_sec = 0;

// ==== Utility ====

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

void waitUntilNextFullMinute() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  int seconds = timeinfo.tm_sec;
  int delayMs = (60 - seconds) * 1000;
  Serial.printf("Waiting %d seconds to align with next full minute...\n", 60 - seconds);
  delay(delayMs);
}

// Append JSON line to local file
void saveFailedLog(String jsonStr) {
  File f = SPIFFS.open("/failed_logs.txt", FILE_APPEND);
  if (!f) return;
  f.println(jsonStr);
  f.close();
}

// Read all logs
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

// Clear logs after success
void clearFailedLogs() {
  SPIFFS.remove("/failed_logs.txt");
}

// Send a batch of logs
bool sendBatchLogs(String batchJson) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  http.begin(EDGE_FUNCTION_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", JWT);
  Serial.printf("Batch Payload: %s\n", batchJson.c_str());
  int code = http.POST(batchJson);
  bool success = (code > 0 && code < 300);
  http.end();
  return success;
}

// Send single log
bool sendSingleLog(String jsonStr) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  http.begin(EDGE_FUNCTION_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", JWT);
  Serial.printf("Payload: %s\n", jsonStr.c_str());
  int code = http.POST(jsonStr);
  bool success = (code > 0 && code < 300);
  http.end();
  return success;
}

// ==== SETUP ====
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("\nConnecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS init failed!");
  }

  prefs.begin("netmon", false);
  syncTime();

  // Align to the next real minute before starting loop
  waitUntilNextFullMinute();
}

void loop() {
  int minuteCounter = getMinuteOfDay();
  saveMinute(minuteCounter);

  // Prepare current log
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["minute"] = minuteCounter;
  doc["status"] = "UP";
  String jsonStr;
  serializeJson(doc, jsonStr);

  Serial.printf("Minute %d -> sending log...\n", minuteCounter);

  // Try sending to Supabase
  if (sendSingleLog(jsonStr)) {
    Serial.println(" Sent successfully.");
    blinkLED(250);
    // Now retry old logs if any
    String oldLogs = readFailedLogs();
    if (oldLogs != "[]" && sendBatchLogs(oldLogs)) {
      Serial.println("Sent previous failed logs.");
      clearFailedLogs();
    }
  } else {
    blinkLED(2, 250);
    Serial.println("Send failed — saving locally.");
    doc["status"] = "DOWN";
    String failJson;
    serializeJson(doc, failJson);
    saveFailedLog(failJson);
  }

  // Wait until the next real minute tick
  waitUntilNextFullMinute();

  // String oldLogs = readFailedLogs();
  // Serial.printf("Old Logs: %s\n", oldLogs.c_str());
  // clearFailedLogs();
  // oldLogs = readFailedLogs();
  // Serial.printf("Old Logs after clear: %s\n", oldLogs.c_str());
  // delay(5000);
}
