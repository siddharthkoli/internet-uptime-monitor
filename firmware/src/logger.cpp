#include <Arduino.h>
#include <ArduinoJson.h>
#include <queue>

#include "logger.h"
#include "networking.h"
#include "constants.h"

std::queue<String> buffer;

TaskHandle_t consumerHandle = NULL;
SemaphoreHandle_t mtx = NULL;

const int UPLOAD_TASK_STACK_SIZE = 10'000;

void loggerInit() {
    if (mtx == NULL) {
        mtx = xSemaphoreCreateMutex();
    }

    xTaskCreatePinnedToCore(
        uploadLog,              // Task function
        "uploadLog",            // Task name
        UPLOAD_TASK_STACK_SIZE, // Stack size (bytes)
        NULL,                   // Parameters
        0,                      // Priority
        &consumerHandle,        // Task handle
        1                       // Core 1
    );
}

void log(const String message) {
    Serial.print(message);

    if (mtx == NULL) {
        mtx = xSemaphoreCreateMutex();
    }

    xSemaphoreTake(mtx, portMAX_DELAY);
    buffer.push(message);
    xSemaphoreGive(mtx);
}

void logf(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    log(String(buf));
}

void logln(const String message) {
    log(message + "\n");
}

String constructLogIngestionPayload(const String &logMessage) {
    JsonDocument doc;
    JsonArray requests = doc.createNestedArray("requests");
    JsonObject executeReq = requests.createNestedObject();
    executeReq["type"] = "execute";
    JsonObject stmt = executeReq.createNestedObject("stmt");
    stmt["sql"] = "insert into t_serial_logs (message) values(?)";
    JsonArray args = stmt.createNestedArray("args");
    JsonObject arg0 = args.createNestedObject();
    arg0["type"] = "text";
    arg0["value"] = logMessage;
    JsonObject closeReq = requests.createNestedObject();
    closeReq["type"] = "close";
    String jsonStr;
    serializeJson(doc, jsonStr);
    return jsonStr;
}

void uploadLog(void* parameter) {
    while (true) {
        if (mtx == NULL) {
            mtx = xSemaphoreCreateMutex();
        }

        xSemaphoreTake(mtx, portMAX_DELAY);
        if (!buffer.empty()) {
            String logEntry = buffer.front();
            buffer.pop();
            xSemaphoreGive(mtx);

            // Attempt to send the log entry with retries
            bool success = retryWithBackoff([&]() {
                return sendSingleLog(logEntry, LOG_INGESTION_URL, LOG_INGESTION_JWT);
            });
        } else {
            xSemaphoreGive(mtx);
            // No logs to process, yield to other tasks
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}