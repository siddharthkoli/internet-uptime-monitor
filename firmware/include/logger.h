#ifndef LOGGER_H
#define LOGGER_H

void loggerInit();
void log(const String message);
void logf(const char* format, ...);
void logln(const String message);
void uploadLog(void* parameter);

#endif