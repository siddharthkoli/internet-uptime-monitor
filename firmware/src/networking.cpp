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

// Configure an existing HTTPClient instance to avoid copying/moving issues
void createHTTPClient(HTTPClient &http, const String &url, const String &jwt, String contentType) {
  http.begin(url);
  http.addHeader(HEADER_CONTENT_TYPE, contentType);
  http.addHeader(HEADER_AUTHORIZATION, jwt.c_str());
}

bool httpPost(const String &jsonStr, const String url, const String jwt) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  createHTTPClient(http, url, jwt);
  Serial.println(url);
  Serial.println(jwt);
  // logf("Payload: %s\n", jsonStr.c_str());
  Serial.printf("Payload: %s\n", jsonStr.c_str());
  int code = http.POST(jsonStr);
  bool success = (code > 0 && code < 300);
  http.end();
  return success;
}

bool retryWithBackoff(const std::function<bool()> &operation, int maxRetries, unsigned long retryDelayMs) {
  int attempt = 0;
  while (attempt <= maxRetries) {
    if (operation()) return true;
    if (attempt == maxRetries) {
      Serial.printf("Operation failed after all %d attempts. Aborting.\n", maxRetries);
      break;
    }
    Serial.printf("Operation failed, retrying in %lu ms (attempt %d/%d)\n", retryDelayMs, attempt + 1, maxRetries);
    delay(retryDelayMs);
    attempt++;
  }
  return false;
}
