#include <WiFi.h>
#include <HTTPClient.h>
#include <functional>

#include "networking.h"
#include "logger.h"

void networkingInit() {
  logln("\nConfiguring network settings");
  #if defined(WIFI_AP_STA) || defined(WIFI_STA)
  WiFi.setAutoReconnect(true);
  #endif
}

HTTPClient createHTTPClient(const char* url, const char* jwt, const char* contentType = CONTENT_TYPE_JSON) {
  HTTPClient http;
  http.begin(url);
  http.addHeader(HEADER_CONTENT_TYPE, contentType);
  http.addHeader(HEADER_AUTHORIZATION, jwt);
  return http;
}

bool sendBatchLogs(const String &batchJson, const char* url, const char* jwt) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http = createHTTPClient(url, jwt);
  logf("Batch Payload: %s\n", batchJson.c_str());
  int code = http.POST(batchJson);
  bool success = (code > 0 && code < 300);
  http.end();
  return success;
}

bool sendSingleLog(const String &jsonStr, const char* url, const char* jwt) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http = createHTTPClient(url, jwt);
  logf("Payload: %s\n", jsonStr.c_str());
  int code = http.POST(jsonStr);
  bool success = (code > 0 && code < 300);
  http.end();
  return success;
}

bool retryWithBackoff(const std::function<bool()> &operation, int maxRetries, unsigned long retryDelayMs) {
  int attempt = 0;
  while (attempt <= maxRetries) {
    if (operation()) return true;
    if (attempt == maxRetries) break;
    logf("Operation failed, retrying in %lu ms (attempt %d/%d)\n", retryDelayMs, attempt + 1, maxRetries);
    delay(retryDelayMs);
    attempt++;
  }
  return false;
}
