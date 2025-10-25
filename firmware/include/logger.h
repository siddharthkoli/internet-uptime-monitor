#ifndef LOGGER_H
#define LOGGER_H

void loggerInit();
void log(const String message);
void logf(const char* format, ...);
void logln(const String message);
String constructLogIngestionPayload(const String &logMessage);
void uploadLog(void* parameter);

#endif