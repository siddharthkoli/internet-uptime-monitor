#include <WiFi.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "time.h"

#include "utils.h"
#include "time_utils.h"
#include "storage.h"
#include "networking.h"
#include "secrets.h"

// ==== CONFIG ====
const char* WIFI_SSID = ENV_WIFI_SSID;
const char* WIFI_PASS = ENV_WIFI_PASS;
const char* EDGE_FUNCTION_URL = ENV_EDGE_FUNCTION_URL;
const char* DEVICE_ID = "ESP32-01";
const char* JWT = ENV_JWT;
const long SEND_INTERVAL = 60000;  // 1 min
const int LED_BLINK_SUCCESS = 1;
const int LED_BLINK_FAILURE = 2;

// ==== Globals ====
Preferences prefs;

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

  storageInit();
  networkingInit();

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

  // Try sending to Supabase with retries
  auto sendOp = [&]() {
    return sendSingleLog(jsonStr, EDGE_FUNCTION_URL, JWT);
  };
  if (retryWithBackoff(sendOp)) {
    Serial.println(" Sent successfully.");
    blinkLED(LED_BLINK_SUCCESS);
    // Now retry old logs if any
    String oldLogs = readFailedLogs();
    if (oldLogs != "[]" && sendBatchLogs(oldLogs, EDGE_FUNCTION_URL, JWT)) {
        Serial.println("Sent previous failed logs.");
        clearFailedLogs();
      }
  } else {
    blinkLED(LED_BLINK_FAILURE);
    Serial.println("Send failed — saving locally.");
    doc["status"] = "DOWN";
    String failJson;
    serializeJson(doc, failJson);
    saveFailedLog(failJson);
  }

  // Wait until the next real minute tick
  waitUntilNextFullMinute();
}
