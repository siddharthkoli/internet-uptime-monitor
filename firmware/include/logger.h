#ifndef LOGGER_H
#define LOGGER_H

enum LogLevel {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
};

struct LogEntry {
    String message;
    LogLevel level;
    unsigned long long timestamp;
    String serviceName;
    String deviceId;
};

void loggerInit();
void log(const String message, LogLevel level, const String serviceName);
void logf(const char* format, LogLevel level, const String serviceName, ...);
void logln(const String message, LogLevel level, const String serviceName);
String getLogLevelString(LogLevel level);
String constructLogIngestionPayload(const String &logMessage, const String &serviceName, LogLevel level);
void uploadLog(void* parameter);

#endif