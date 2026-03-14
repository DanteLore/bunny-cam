# bunny-cam — Project Plan

An ESP32-CAM based IP webcam for monitoring a rabbit hutch, built in C using ESP-IDF.

---

## Hardware

- **Board:** AI Thinker ESP32-CAM
- **Camera:** OV2640 (onboard)
- **Programmer:** ESP32-CAM-MB (micro-USB add-on board, CH340C USB-to-serial chip)
- **Flash:** 4MB (partition table configured for 2MB — needs fixing)
- **Chip revision:** v3.1, dual-core, 160MHz

### Key hardware notes

- The MB programmer board handles flashing automatically — no button juggling needed
- COM port: COM3 (Windows Device Manager)
- Flash LED is on GPIO 4 — useful for illuminating the hutch at night
- Ribbon cable between OV2640 and ESP32-CAM board is fragile — ensure it is fully seated

---

## Development Environment

### Toolchain

| Tool | Version / Location |
|------|--------------------|
| OS | Windows 11 |
| ESP-IDF | v5.5.3 at `C:\esp\v5.5.3\esp-idf` |
| ESP-IDF tools | `C:\Espressif\tools` |
| Compiler | xtensa-esp32-elf-gcc 14.2.0 |
| Python (venv) | 3.13.12 at `C:\Espressif\tools\python\v5.5.3\venv` |
| VS Code extension | ESP-IDF v2.0.2 |
| Git | 2.52.0 |

### VS Code workflow

All build, flash, and monitor actions run via the ESP-IDF extension command palette (`Ctrl+Shift+P`):

- `ESP-IDF: Build your project`
- `ESP-IDF: Flash your device` (select UART when prompted)
- `ESP-IDF: Monitor your device` (115200 baud)

The blue status bar at the bottom of VS Code shows the current target (esp32) and port (COM3).

### Important gotchas

- Close Arduino IDE before flashing from VS Code — it holds COM3 open
- Do not use DevContainers — native Windows install is already working
- The `sdkconfig` is generated on first build — missing file errors before that are normal
- Flash size warning at boot ("detected 4096k, using 2048k") — partition table needs updating for camera use

---

## Project structure

Standard ESP-IDF layout:

```
bunny-cam/
  main/
    main.c          -- application entry point
    CMakeLists.txt
  CMakeLists.txt
  sdkconfig         -- generated; do not commit
  build/            -- generated; do not commit
```

WiFi credentials and camera settings should be exposed via `Kconfig` / `menuconfig` as `CONFIG_` values, not hardcoded.

---

## Dependencies

- `esp32-camera` component — OV2640 camera driver
- `esp_http_server` — built into ESP-IDF, used for the HTTP API
- `esp_wifi` — built into ESP-IDF
- `nvs_flash` — non-volatile storage for credentials and settings

The `esp32-camera` component must be added to the project. Add it via `idf_component_manager` or by cloning into a `components/` directory:

```
https://github.com/espressif/esp32-camera
```

---

## Phase 1 — Single image endpoint (start here)

**Goal:** Hit a URL in a browser and see a live photo from the hutch.

- Connect to WiFi using credentials from `Kconfig`
- Initialise OV2640 with correct AI Thinker pin assignments
- Capture a single JPEG frame on demand
- Serve it over HTTP at `GET /image`
- Log the device IP to serial on boot

**Acceptance test:** Open `http://[device-ip]/image` in a browser and see a photo.

### AI Thinker camera pin config (for reference)

```c
.pin_pwdn    = 32,
.pin_reset   = -1,
.pin_xclk    = 0,
.pin_sccb_sda = 26,
.pin_sccb_scl = 27,
.pin_d7      = 35,
.pin_d6      = 34,
.pin_d5      = 39,
.pin_d4      = 36,
.pin_d3      = 21,
.pin_d2      = 19,
.pin_d1      = 18,
.pin_d0      = 5,
.pin_vsync   = 25,
.pin_href    = 23,
.pin_pclk    = 22,
.xclk_freq_hz = 20000000,
.ledc_timer   = LEDC_TIMER_0,
.ledc_channel = LEDC_CHANNEL_0,
.pixel_format = PIXFORMAT_JPEG,
.frame_size   = FRAMESIZE_VGA,
.jpeg_quality = 12,
.fb_count     = 1,
```

---

## Phase 2 — Still image improvements

- `GET /status` — JSON response with uptime, free heap, WiFi RSSI
- Tunable resolution and quality via `menuconfig`
- `POST /settings` — adjust brightness, contrast, resolution at runtime
- Store WiFi credentials in NVS (non-volatile storage) rather than compiled in
- Fix partition table to use full 4MB flash

---

## Phase 3 — MJPEG stream

- `GET /stream` — Motion JPEG stream, browser-native
- Tune frame rate vs JPEG quality tradeoff
- Consider PSRAM usage for frame buffering (ESP32-CAM has 4MB PSRAM)

---

## Phase 4 — Reliability and polish

- Hardware watchdog — auto-reboot if camera or WiFi hangs
- WiFi reconnection logic on drop
- Flash LED control via `GET /led?state=on|off` — useful in a dark hutch
- Deep sleep between captures if power saving is needed

---

## Suggested first Claude Code prompt

Paste this at the start of a session:

```
I have an ESP32-CAM (AI Thinker board) project using ESP-IDF v5.5.3.
The project is called bunny-cam and lives in a standard ESP-IDF directory structure.

Create main.c that:
- Connects to WiFi using CONFIG_WIFI_SSID and CONFIG_WIFI_PASSWORD from Kconfig
- Initialises the OV2640 camera using the AI Thinker pin assignments
- Starts an HTTP server
- Serves a single JPEG capture at GET /image

Use esp32-camera for camera access and esp_http_server for the web server.
Keep the code short and readable. Fail fast on errors — no defensive guard code.
```

---

## Coding preferences (Dan)

Working style is XP-style pair programming — objective, brief, and efficient. Flag issues proactively rather than silently working around them.

### C style for this project

- Short files, short functions
- Self-documenting names; comments only where logic is not obvious
- No premature abstractions — start simple, add structure as the project grows
- Fail fast: do not add null checks or defensive guards unless there is a specific reason; let errors surface naturally
- All `#include` directives at the top of the file
- Plain ASCII in comments — no em dashes, no special characters

### General preferences (from dantelore.com/AI.md)

- Readability over performance unless told otherwise
- No over-engineered structure or premature abstractions
- Wrap external APIs in a dedicated module, separate from business logic
- Remove all warnings; deal with deprecations rather than suppressing them
- British English in comments and docs; US English in code identifiers (e.g. `color` not `colour`)
- Based in the UK — metric and SI units

### Git

Dan handles git himself. Do not run `git add`, `git commit`, or `git push` unless explicitly asked. Read-only git commands (log, diff, status) are fine.

### When stuck

Stop, explain the problem clearly, and suggest alternatives. Do not silently iterate.