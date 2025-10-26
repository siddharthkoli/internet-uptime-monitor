#include <WiFi.h>
#include <HTTPClient.h>
#include <functional>

#include "networking.h"
#include "logger.h"

const String SERVICE  = "networking";

void networkingInit() {
  logln("\nConfiguring network settings", LOG_LEVEL_INFO, SERVICE);
  #if defined(WIFI_AP_STA) || defined(WIFI_STA)
  WiFi.setAutoReconnect(true);
  #endif
}

// Configure an existing HTTPClient instance to avoid copying/moving issues
void createHTTPClient(HTTPClient &http, const String &url, const String &jwt, String contentType) {
  http.begin(url);
  http.addHeader(HEADER_CONTENT_TYPE, contentType);
  http.addHeader(HEADER_AUTHORIZATION, jwt);
}

bool httpPost(const String &jsonStr, const String url, const String jwt) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  createHTTPClient(http, url, jwt);
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
      logf("Operation failed after all %d attempts. Aborting.\n", LOG_LEVEL_ERROR, SERVICE, maxRetries);
      break;
    }
    logf("Operation failed, retrying in %lu ms (attempt %d/%d)\n", LOG_LEVEL_WARNING, SERVICE, retryDelayMs, attempt + 1, maxRetries);
    delay(retryDelayMs);
    attempt++;
  }
  return false;
}
