# DigitalGearKnob — ESP32 Firmware

Firmware for a digital gear indicator knob built with an ESP32 (LilyGo T-Display S3), a BNO085 IMU sensor, and a CO5300 round display. Communicates with the **ScuffyApp** Flutter companion app via BLE.

## Hardware

| Component | Details |
|-----------|---------|
| Board | LilyGo T-Display S3 (ESP32-S3) |
| Display | CO5300 round 466×466 (QSPI) |
| IMU | BNO085 (I2C — SDA: GPIO7, SCL: GPIO6) |
| BLE | NimBLE-Arduino |

## Features

- **Gear detection** via 2D roll/pitch zones relative to an auto-recalibrated neutral reference
- **Gear calibration** stored in NVS (survives reboots)
- **Theme color** customizable from the companion app
- **BLE protocol** for bidirectional communication with ScuffyApp
- **BNO085 recovery** with I2C bus recovery and automatic reset on sensor lock-up

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
pio run
```

## BLE Protocol

| Command | Direction | Description |
|---------|-----------|-------------|
| `set_color:#RRGGBB` | App → ESP32 | Set theme color |
| `gear:X` | App → ESP32 | Manual gear selection |
| `calibrate:X` | App → ESP32 | Calibrate gear position |
| `calibration_ok` | ESP32 → App | Calibration saved |
| `stream:on/off` | Bidirectional | Enable/disable quaternion streaming |
| `get_state` | App → ESP32 | Request current state |
| `theme:#RRGGBB` | ESP32 → App | Current theme color |
| `cal_status:RRRRRRR` | ESP32 → App | Calibration status per gear |
| `cal_data:X:w,x,y,z` | ESP32 → App | Calibration quaternion data |
| `quat:w,x,y,z` | ESP32 → App | Live quaternion stream |

## Project Structure

```
src/
├── bno/          # BNO085 IMU driver and I2C recovery
├── ble/          # BLE communication and command parsing
├── calibration/  # Gear calibration with NVS persistence
├── display/      # CO5300 display driver
├── gears/        # Gear detection algorithm
├── lvgl_port/    # LVGL integration
├── theme/        # Theme management with NVS persistence
├── boot/         # Boot animation
└── ui/           # SquareLine Studio generated UI
```
