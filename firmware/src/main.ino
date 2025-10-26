#include <WiFi.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "time.h"

#include "utils.h"
#include "time_utils.h"
#include "storage.h"
#include "networking.h"
#include "logger.h"
#include "constants.h"


// ==== Globals ====
Preferences prefs;

const String SERVICE = "main";

// ==== SETUP ====
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  loggerInit();
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  logln("Connected to Wifi!", LOG_LEVEL_INFO, SERVICE);
  
  networkingInit();
  storageInit(STORAGE_CLEAR_BEFORE_BEGIN);

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

  logf("Minute %d -> sending log...\n", LOG_LEVEL_INFO, SERVICE, minuteCounter);

  // Try sending to Supabase with retries
  auto sendOp = [&]() {
    return httpPost(jsonStr, EDGE_FUNCTION_URL, EDGE_FUNCTION_JWT);
  };
  if (retryWithBackoff(sendOp)) {
    logln(" Sent successfully.", LOG_LEVEL_INFO, SERVICE);
    blinkLED(LED_BLINK_SUCCESS, DEFAULT_BLINK_DELAY_MS);
    // Now retry old logs if any
    String oldLogs = readFailedLogs();
    if (oldLogs != "[]" && httpPost(oldLogs, EDGE_FUNCTION_URL, EDGE_FUNCTION_JWT)) {
        logln("Sent previous failed logs.", LOG_LEVEL_INFO, SERVICE);
        clearFailedLogs();
    }
  } else {
    blinkLED(LED_BLINK_FAILURE, DEFAULT_BLINK_DELAY_MS);
    logln("Send failed — saving locally.", LOG_LEVEL_ERROR, SERVICE);
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
