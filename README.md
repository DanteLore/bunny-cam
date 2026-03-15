# bunny-cam

An ESP32-CAM based IP webcam for monitoring a rabbit hutch, built in C using ESP-IDF.

---

## Hardware

- **Board:** AI Thinker ESP32-CAM
- **Camera:** OV2640 (onboard)
- **Programmer:** ESP32-CAM-MB (micro-USB add-on board, CH340C USB-to-serial chip)
- **Flash:** 4MB (custom partition table, 3MB app partition)
- **PSRAM:** 8MB physical; 4MB mapped (ESP32 hardware limit)
- **Chip revision:** v3.1, dual-core, 160MHz

### Key hardware notes

- The MB programmer board handles flashing automatically -- no button juggling needed
- COM port: COM3 (Windows Device Manager)
- Flash LED is on GPIO 4 -- not used; would disturb the rabbits
- Ribbon cable between OV2640 and ESP32-CAM board is fragile -- ensure it is fully seated
- IR illuminator is a future option for night vision (rabbits cannot see above ~700nm)

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

### Build and flash

Use `flash.ps1` from PowerShell (preferred) or `flash.bat` from cmd:

```powershell
.\flash.ps1 build                        # build only
.\flash.ps1 -p COM3 build flash monitor  # build, flash, and monitor
.\flash.ps1 -p COM3 monitor              # monitor only
```

`flash.ps1` reads `secrets.env` and injects WiFi credentials and upload secret into
`sdkconfig` automatically before building. Copy `secrets.example.env` to `secrets.env`
and fill in your values before first build.

### Important gotchas

- Close Arduino IDE before flashing -- it holds COM3 open
- Do not use DevContainers -- native Windows install is working
- `sdkconfig` is generated on first build and re-injected with secrets on every build
- `sdkconfig` contains WiFi credentials -- never commit it (already gitignored)
- `sdkconfig.defaults` captures all non-secret baseline config (PSRAM, flash size, watchdog)

---

## Project structure

```
bunny-cam/
  main/
    main.c               -- app entry point, deep sleep loop
    wifi.c / wifi.h      -- WiFi init, connect, disconnect
    camera.c / camera.h  -- OV2640 init, auto-adjustment, warmup
    http.c / http.h      -- HTTP server, GET /image, GET /status
    upload.c / upload.h  -- two-step S3 upload (presigned URL)
    Kconfig.projbuild    -- menuconfig entries (WiFi SSID/password, upload secret)
    idf_component.yml    -- component manager dependencies
    CMakeLists.txt
  CMakeLists.txt
  partitions.csv         -- custom 4MB partition table (3MB app partition)
  sdkconfig.defaults     -- baseline SDK config (PSRAM, flash size, stack, watchdog)
  sdkconfig              -- generated; do not commit (contains secrets)
  build/                 -- generated; do not commit
  secrets.env            -- local secrets; do not commit (gitignored)
  secrets.example.env    -- template for secrets.env
  flash.ps1              -- build/flash script for PowerShell
  flash.bat              -- build/flash script for cmd
  dependencies.lock      -- component manager lockfile; commit this
```

---

## Dependencies

Managed via IDF Component Manager (`main/idf_component.yml`):

- `espressif/esp32-camera` >= 2.0.0 -- OV2640 camera driver
- `espressif/mdns` >= 1.0.0 -- mDNS hostname (`bunny-cam.local`)

Built into ESP-IDF (no manifest entry needed):

- `esp_http_server` -- HTTP API
- `esp_wifi` -- WiFi stack
- `nvs_flash` -- non-volatile storage
- `esp_http_client` -- outbound HTTPS for upload
- `cJSON` -- JSON for status endpoint

---

## Secrets

Copy `secrets.example.env` to `secrets.env` and fill in:

```
WIFI_SSID=your_ssid
WIFI_PASSWORD=your_password
UPLOAD_SECRET=your_upload_secret
```

`secrets.env` is gitignored. `flash.ps1` injects these into `sdkconfig` before every build.

---

## HTTP API

The device is reachable at `http://bunny-cam.local` or by IP (logged to serial on boot).

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/image` | GET | Capture and return a JPEG frame |
| `/status` | GET | JSON status: uptime, heap, RSSI, channel, build timestamp |

The HTTP server is only active during the awake window (see Power management below).

---

## Image upload

Images are uploaded automatically to `cam.dantelore.com` via a two-step process:

1. `GET https://cam.dantelore.com/upload?secret=...&camera_id=bunnycam` -- returns a presigned S3 URL
2. `PUT <presigned_url>` with the JPEG body

See `../webcam/README.md` for the full API specification.

---

## Power management

The device uses deep sleep between uploads to save power.

```
wake -> camera init (300ms warmup) -> WiFi connect -> capture -> upload
     -> HTTP server starts (brief awake window)
     -> if no API hit within 10s: deep sleep for 20s
     -> if API hit: stay awake until 120s of inactivity, then sleep
```

Total cycle time is approximately 30s. Reset code `rst:0x5` in boot log is normal
(deep sleep wake); it is not a crash.

Timing constants in `main.c`:

| Constant | Value | Purpose |
|----------|-------|---------|
| `SLEEP_DURATION_US` | 20s | Deep sleep duration |
| `BRIEF_AWAKE_WINDOW_US` | 10s | Minimum awake time after upload |
| `API_ACTIVE_WINDOW_US` | 120s | Stay-awake window after last API hit |
| `UPLOAD_INTERVAL_US` | 30s | Upload interval while awake |

---

## Camera settings

OV2640 is configured with full automatic adjustment on boot (`camera.c`):

- AEC2 auto exposure
- AGC with 8x gain ceiling (helps in low light)
- Auto white balance
- Black/white pixel correction, gamma correction, lens correction
- 3 warmup frames discarded on boot to let AE settle

---

## Coding preferences (Dan)

Working style is XP-style pair programming -- objective, brief, and efficient.
Flag issues proactively rather than silently working around them.

### C style for this project

- Short files, short functions
- Self-documenting names; comments only where logic is not obvious
- No premature abstractions -- start simple, add structure as the project grows
- Fail fast: do not add null checks or defensive guards unless there is a specific reason
- All `#include` directives at the top of the file
- Plain ASCII in comments -- no em dashes, no special characters

### General preferences

- Readability over performance unless told otherwise
- No over-engineered structure or premature abstractions
- Wrap external APIs in a dedicated module, separate from business logic
- Remove all warnings; deal with deprecations rather than suppressing them
- British English in comments and docs; US English in code identifiers (e.g. `color` not `colour`)
- Based in the UK -- metric and SI units

### Git

Dan handles git himself. Do not run `git add`, `git commit`, or `git push` unless
explicitly asked. Read-only git commands (log, diff, status) are fine.

### When stuck

Stop, explain the problem clearly, and suggest alternatives. Do not silently iterate.
