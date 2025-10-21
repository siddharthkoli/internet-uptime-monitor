#ifndef STORAGE_H
#define STORAGE_H

// Storage helpers (SPIFFS)
void storageInit(bool clearBeforeBegin = false);
void saveFailedLog(String jsonStr);
String readFailedLogs();
void clearFailedLogs();

#endif
