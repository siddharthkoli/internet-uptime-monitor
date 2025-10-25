#ifndef NETWORKING_H
#define NETWORKING_H

#include <functional>
#include <HTTPClient.h>

// Retry defaults (can be overridden by callers)
static const int DEFAULT_RETRY_COUNT = 1;
static const unsigned long DEFAULT_RETRY_DELAY_MS = 1000;

const char* HEADER_CONTENT_TYPE = "Content-Type";
const char* HEADER_AUTHORIZATION = "Authorization";
const char* CONTENT_TYPE_JSON = "application/json";

// Networking helpers (HTTP)
void networkingInit();
HTTPClient createHTTPClient(const char* url, const char* jwt, const char* contentType = CONTENT_TYPE_JSON);
bool sendBatchLogs(const String &batchJson, const char* url, const char* jwt);
bool sendSingleLog(const String &jsonStr, const char* url, const char* jwt);

// Retry helper: takes an operation (returns bool for success), max retries, and delay between retries (ms)
bool retryWithBackoff(const std::function<bool()> &operation, int maxRetries = DEFAULT_RETRY_COUNT, unsigned long retryDelayMs = DEFAULT_RETRY_DELAY_MS);

#endif
