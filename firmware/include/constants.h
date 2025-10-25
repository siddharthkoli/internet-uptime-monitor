#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "secrets.h"

// WIFI Credentials
static const char* WIFI_SSID = ENV_WIFI_SSID;
static const char* WIFI_PASS = ENV_WIFI_PASS;

// Edge function that stores uptime data
static const char* EDGE_FUNCTION_URL = ENV_EDGE_FUNCTION_URL;
static const char* EDGE_FUNCTION_JWT = ENV_EDGE_FUNCTION_JWT;

// Log Ingestion Endpoint
static const char* LOG_INGESTION_URL = ENV_LOG_INGESTION_URL;
static const char* LOG_INGESTION_JWT = ENV_LOG_INGESTION_JWT;

// Device ID
static const char* DEVICE_ID = "ESP32-01";

// LED Blink patterns
static const int LED_BLINK_SUCCESS = 1;
static const int LED_BLINK_FAILURE = 2;
static const int DEFAULT_BLINK_DELAY_MS = 250;

// Device storage
static const int STORAGE_CLEAR_BEFORE_BEGIN = false;

// Network
static const long SEND_INTERVAL = 60000;  // 1 min

#endif