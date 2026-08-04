# ScuffyApp — Flutter Companion

Flutter mobile app for the **DigitalGearKnob** project. Connects to the ESP32 gear indicator via BLE to configure themes, calibrate gear positions, and view state.

## Features

- **BLE connection** with automatic scanning for "SCUFFY" device
- **Theme selection** with preset colors and custom color picker
- **Gear calibration** for R, 1, 2, 3, 4, 5, N positions
- **State synchronization** — app loads cached theme and calibrations, then syncs fresh data from ESP32 on connect
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

| Command | Direction | Description |
|---------|-----------|-------------|
| `set_color:#RRGGBB` | App → ESP32 | Set theme color |
| `get_state` | App → ESP32 | Request theme, calibration status, and data |
| `calibrate:X` | App → ESP32 | Calibrate gear X (R,1,2,3,4,5,N) |
| `stream:on/off` | App → ESP32 | Enable/disable quaternion streaming |

**ESP32 → App responses:**

| Response | Description |
|----------|-------------|
| `theme:#RRGGBB` | Current theme color |
| `cal_status:RRRRRRR` | 7-char string, '1' = calibrated, '0' = not |
| `cal_data:X:w,x,y,z` | Quaternion data for gear X |
| `calibration_ok` | Calibration saved successfully |
