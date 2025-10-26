#include <Arduino.h>
#include <ArduinoJson.h>
#include <queue>

#include "logger.h"
#include "networking.h"
#include "constants.h"

std::queue<LogEntry> buffer;

TaskHandle_t consumerHandle = NULL;
SemaphoreHandle_t mtx = NULL;

const int UPLOAD_TASK_STACK_SIZE = 10000;

void loggerInit() {
    if (mtx == NULL) {
        mtx = xSemaphoreCreateMutex();
    }

    xTaskCreatePinnedToCore(
        uploadLog,              // Task function
        "uploadLog",            // Task name
        UPLOAD_TASK_STACK_SIZE, // Stack size (bytes)
        NULL,                   // Parameters
        1,                      // Priority
        &consumerHandle,        // Task handle
        0                       // Core 0
    );
}

void log(const String message, LogLevel level, const String serviceName) {
    Serial.print(message);

    if (mtx == NULL) {
        mtx = xSemaphoreCreateMutex();
    }

    xSemaphoreTake(mtx, portMAX_DELAY);

    LogEntry logEntry;
    logEntry.message = message;
    logEntry.level = level;
    logEntry.timestamp = getCurrentTimestampNano();
    logEntry.serviceName = serviceName;
    logEntry.deviceId = DEVICE_ID;

    buffer.push(logEntry);

    xSemaphoreGive(mtx);
}

void logf(const char* format, LogLevel level, const String serviceName, ...) {
    char buf[256];
    va_list args;
    va_start(args, serviceName);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    log(String(buf), level, serviceName);
}

void logln(const String message, LogLevel level, const String serviceName) {
    log(message + "\n", level, serviceName);
}

unsigned long long getCurrentTimestampNano() {
    return (millis() * 1000000ULL);
}

String getLogLevelString(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_INFO:
            return "INFO";
        case LOG_LEVEL_WARNING:
            return "WARNING";
        case LOG_LEVEL_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

String constructLogIngestionPayload(LogEntry logEntry) {
    /*
    GRAFANA LOGGING FORMAT:
    {
        "streams": [
            {
                // metadata
                "stream": {
                    "device": DEVICE_ID,
                    "service": SERVICE_NAME,
                    "level": LogLevel,
                },
                // logging data
                "values": [
                    [
                        unix timestamp in nanoseconds as string,
                        log message
                    ]
                ]
            }
        ]
    }
    */
    JsonDocument doc;

    JsonObject streams_0 = doc["streams"].add<JsonObject>();

    JsonObject streams_0_stream = streams_0["stream"].to<JsonObject>();
    streams_0_stream["device"] = logEntry.deviceId;
    streams_0_stream["service"] = logEntry.serviceName;
    streams_0_stream["level"] = getLogLevelString(logEntry.level);

    JsonArray streams_0_values_0 = streams_0["values"].add<JsonArray>();
    streams_0_values_0.add(String(logEntry.timestamp));
    streams_0_values_0.add(logEntry.message);

    String output;

    doc.shrinkToFit();

    serializeJson(doc, output);

    return output;
}

// FreeRTOS task
void uploadLog(void* parameter) {
    while (true) {
        if (mtx == NULL) {
            mtx = xSemaphoreCreateMutex();
        }

        xSemaphoreTake(mtx, portMAX_DELAY);
        if (!buffer.empty()) {
            LogEntry logEntry = buffer.front();
            buffer.pop();
            xSemaphoreGive(mtx);

            // Attempt to send the log entry with retries
            bool success = retryWithBackoff([&]() {
                return httpPost(constructLogIngestionPayload(logEntry), LOG_INGESTION_URL, LOG_INGESTION_JWT);
            });
        } else {
            xSemaphoreGive(mtx);
            // No logs to process, yield to other tasks
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}