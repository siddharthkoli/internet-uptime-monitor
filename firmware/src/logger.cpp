#include <Arduino.h>
#include <queue>

#include "logger.h"
#include "networking.h"

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
                return sendSingleLog(logEntry, EDGE_FUNCTION_URL, JWT);
            });

            if (!success) {
                // If sending failed after retries, re-enqueue the log entry
                xSemaphoreTake(mtx, portMAX_DELAY);
                buffer.push(logEntry);
                xSemaphoreGive(mtx);
            }
        } else {
            xSemaphoreGive(mtx);
            // No logs to process, yield to other tasks
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}