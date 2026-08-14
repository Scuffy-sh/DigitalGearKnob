# ScuffyApp — Flutter Companion

Flutter mobile app for the **DigitalGearKnob** project. Connects to the ESP32 gear indicator via BLE to show the live CAN status and gear, sniff CAN frames for reverse-engineering, and configure the theme.

## Features

- **BLE connection** with automatic scanning for "SCUFFY" device
- **Live CAN status** — a status chip reflects `can:ok` / `can:offline` (and a grey disconnected state)
- **Sniff view** — toggles the firmware's CAN frame dump, browses rate-limited frames, and surfaces a silent bus (`can:no_frames`)
- **Gear display** — the seven gears (R, 1–5, N) as reference cards; the current value comes from the knob
- **Theme selection** with preset colors and custom color picker
- **Firmware version check** — an update prompt appears when the knob reports a different BLE protocol version
- **State synchronization** — the app loads the cached theme, then syncs fresh data from ESP32 on connect
- **Splash screen** with animated logo

## Requirements

- Flutter 3.x
- Android SDK (Developer Mode enabled on Windows for symlinks)
- Physical Android or iOS device (BLE doesn't work on emulators)

## Building

```bash
flutter pub get
flutter run
```

## BLE Protocol

The app communicates with the ESP32 using a custom text-based BLE protocol over a single service/characteristic pair.

**App → ESP32 commands:**

| Command | Direction | Description |
|---------|-----------|-------------|
| `set_color:#RRGGBB` | App → ESP32 | Set theme color |
| `debug:on` / `debug:off` | App → ESP32 | Enable/disable debug notifications |
| `sniff:on` / `sniff:off` | App → ESP32 | Enable/disable the CAN frame dump |
| `get_state` | App → ESP32 | Request theme, CAN status, and protocol version |

**ESP32 → App responses:**

| Response | Description |
|----------|-------------|
| `theme:#RRGGBB` | Current theme color |
| `can:ok` / `can:offline` | CAN bus state |
| `proto:2` | BLE protocol version |
| `debug:<GEAR>,<RPM>,<SPEED>,<CANSTAT>` | Live debug values (gear R/1..5/N, RPM, km/h, ok/offline) |
| `sniff:<E\|S><8hexid>:<datahex>` | One rate-limited sniffed frame (E = 29-bit, S = 11-bit) |
| `can:no_frames` | No frames while sniffing (silent bus, rate-limited) |
| `error:removed:<cmd>` | Rejected v1 command (`calibrate:`, `stream:`, `quat:`, ...) |

The v1 calibration/streaming flows (`calibrate:X`, `stream:on/off`, `quat:...`, `cal_status:`, `cal_data:`, `calibration_ok`) were removed in protocol v2 — the gear now comes from the CAN bus, not from per-gear quaternion calibration.
