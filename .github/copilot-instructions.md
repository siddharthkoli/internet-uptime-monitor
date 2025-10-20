## Quick orientation for AI coding agents

This repository is a small PlatformIO/Arduino project for an ESP32-based "internet uptime" monitor that pings an edge function (Supabase) every minute and stores failed events locally in SPIFFS.

Be concise and make minimal, well-tested changes. The guidance below focuses on code patterns, integration points, and exact file locations so you can be productive immediately.

## Big picture
- Runtime: ESP32 (Arduino framework) using PlatformIO. See `platformio.ini` (env: `esp32doit-devkit-v1`).
- Main loop: `src/ping_supabase.ino` — constructs a small JSON per minute and either POSTs it or saves it locally.
- Networking: `src/networking.cpp` / `include/networking.h` — HTTP helpers: `sendSingleLog`, `sendBatchLogs`, and `retryWithBackoff` (uses `std::function<bool()>`).
- Storage: `src/storage.cpp` / `include/storage.h` — SPIFFS file `/failed_logs.txt` stores newline-delimited JSON (one JSON object per line). `readFailedLogs()` returns a JSON array assembled from lines.
- Time: `src/time_utils.cpp` / `include/time_utils.h` — NTP via `configTime()`. `getMinuteOfDay()` and `waitUntilNextFullMinute()` control the 1-minute cadence. Preferences (non-volatile) are used as a fallback.

## Key files to read/modify
- `src/ping_supabase.ino` — entry point, global `Preferences prefs`, device constants (DEVICE_ID, SEND_INTERVAL), and main control flow (align to minute, send, retry old logs).
- `src/networking.cpp` — HTTPClient usage; sets `Content-Type: application/json` and `Authorization` header (JWT). Important: functions check `WiFi.status() != WL_CONNECTED` early.
- `src/storage.cpp` — SPIFFS lifecycle, newline JSON write (`println`) and read (`readStringUntil('\n')`). Clearing removes `/failed_logs.txt`.
- `include/secrets.example.h` — environment macros `ENV_WIFI_SSID`, `ENV_WIFI_PASS`, `ENV_EDGE_FUNCTION_URL`, `ENV_JWT`. Copy to `include/secrets.h` (local, not committed) and populate.
- `platformio.ini` — build environment and `lib_deps` (ArduinoJson ^7.4.2).

## Data flow / invariants
- Each minute: create JSON { device_id, minute, status } and call `sendSingleLog(json, EDGE_FUNCTION_URL, JWT)`.
- On success: blink LED briefly and attempt to read `readFailedLogs()`; if not empty (`"[]"`) call `sendBatchLogs()` and `clearFailedLogs()` on success.
- On failure: change status to "DOWN" and `saveFailedLog()` which appends a line to `/failed_logs.txt`.
- `readFailedLogs()` returns a JSON array string (e.g. "[ {...}, {...} ]"). Do not change the newline-delimited on-disk format unless you update both writer and reader.

## Build, flash and monitor (developer workflows)
- Build: `platformio run` (or `pio run`).
- Upload: `platformio run -e esp32doit-devkit-v1 -t upload` (or `pio run -e esp32doit-devkit-v1 -t upload`).
- Serial monitor: `platformio device monitor -b 115200` (or `pio device monitor -b 115200`). The project sets `monitor_speed = 115200` in `platformio.ini`.
- Tests: PlatformIO unit tests would live in `test/` and can be run with `platformio test`. There are no unit tests currently; prefer small integration or unit tests when refactoring low-risk helpers (networking/storage/time_utils).

## Project-specific conventions & patterns
- Header location: `include/*.h` and implementation in `src/*.cpp`. Header guards are used consistently.
- Use of Arduino types: `String`, `File`, `Preferences`, `WiFi`, `HTTPClient`, `SPIFFS` — prefer to keep API usage consistent rather than switching to STL unless adding isolated helpers.
- SPIFFS storage format: newline-delimited JSON objects in `/failed_logs.txt`. `readFailedLogs()` expects that exact layout.
- Retry semantics: `retryWithBackoff(operation, maxRetries, retryDelayMs)` defaults to 1 retry and 1000ms delay. Callers rely on its return (bool) and side-effects.

## Editing cautions (do not break these)
- Do not commit real secrets. Use `include/secrets.h` locally (copy `secrets.example.h`) and add to .gitignore if not already ignored.
- Preserve `WiFi.status()` checks in networking functions — code assumes offline check before making HTTP calls.
- When modifying storage, keep both writer (`saveFailedLog`) and reader (`readFailedLogs`) in sync with any format change.
- `prefs` is a global `Preferences` instance initialized in `ping_supabase.ino` with namespace `"netmon"`; avoid creating conflicting preference namespaces without updating callers.

## Small examples (how to change safely)
- Change retry behavior: update call sites of `retryWithBackoff` (in `src/ping_supabase.ino`) to pass desired `maxRetries` and `retryDelayMs` rather than changing the default constant.
- Add a custom header: modify `src/networking.cpp` and add `http.addHeader("X-Device-ID", DEVICE_ID);` — keep `Content-Type` and `Authorization` headers.
- To add structured unit tests: put tests under `test/` and target small helpers (e.g., `retryWithBackoff` with a stubbed operation). Use PlatformIO's Unity test runner.

## PR checklist for behavioral changes
- Build succeeds with `platformio run`.
- No secrets added to the repo.
- If changing storage format, include a migration plan or update both reader and writer in same PR.
- If changing timing logic, validate serial logs using `platformio device monitor` for at least two minutes to ensure alignment to full minute ticks.

If anything in these notes is unclear or you want the agent to expand any section (examples, tests, or a migration helper for storage), say which part to iterate on.
