#ifndef NETWORKING_H
#define NETWORKING_H

#include <functional>
#include <HTTPClient.h>

// Retry defaults (can be overridden by callers)
static const int DEFAULT_RETRY_COUNT = 1;
static const unsigned long DEFAULT_RETRY_DELAY_MS = 1000;

static const char HEADER_CONTENT_TYPE[] = "Content-Type";
static const char HEADER_AUTHORIZATION[] = "Authorization";
static const char CONTENT_TYPE_JSON[] = "application/json";

// Networking helpers (HTTP)
void networkingInit();
void createHTTPClient(HTTPClient &http, const String &url, const String &jwt, String contentType = CONTENT_TYPE_JSON);
bool httpPost(const String &jsonStr, const String url, const String jwt);

bool retryWithBackoff(const std::function<bool()> &operation, int maxRetries = DEFAULT_RETRY_COUNT, unsigned long retryDelayMs = DEFAULT_RETRY_DELAY_MS);

#endif
