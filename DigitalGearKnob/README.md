# DigitalGearKnob — ESP32 Firmware

Firmware for a digital gear indicator knob built with an ESP32 (LilyGo T-Display S3), a Waveshare SN65HVD230 CAN transceiver, and a CO5300 round display. Reads the engaged gear from the car's drivetrain CAN bus (listen-only) and communicates with the **ScuffyApp** Flutter companion app via BLE.

## Hardware

| Component | Details |
|-----------|---------|
| Board | LilyGo T-Display S3 (ESP32-S3) |
| Display | CO5300 round 466×466 (QSPI) |
| CAN | Waveshare SN65HVD230 transceiver · TWAI listen-only · 500 kbps · GPIO 3 (TX) / GPIO 5 (RX) |
| BLE | NimBLE-Arduino |

## Features

- **Gear detection** from the CAN bus — the engine RPM ÷ wheel-speed ratio is matched against a per-gear table (R, 1–5, N) with hysteresis, standstill/clutch rules, and a 500 ms stale timeout
- **CAN sniff mode** — rate-limited frame dump (20 Hz) over BLE and Serial for reverse-engineering frame layouts; `can:no_frames` signals a silent bus
- **Listen-only safety** — TWAI is hard-coded to listen-only mode with no transmit API; the knob can never write to the vehicle bus
- **Theme color** customizable from the companion app
- **BLE protocol v2** for bidirectional communication with ScuffyApp

## Wiring

| SN65HVD230 | ESP32-S3 | Function |
|-----------|----------|----------|
| **VCC** | **3.3V** | Power |
| **GND** | **GND** | Ground |
| **TXD** | **GPIO 3** | TWAI TX (never driven — listen-only) |
| **RXD** | **GPIO 5** | TWAI RX |
| **CANH** | Drivetrain CAN_H | Bus high |
| **CANL** | Drivetrain CAN_L | Bus low |

> **Termination jumper.** The onboard 120 Ω termination of the Waveshare board must be **disabled** when tapping the car's bus — a third 120 Ω in parallel drops the bus to ~40 Ω and risks car-wide communication faults. Bench tests on an isolated mini-bus may keep it. The SN65HVD230 is 3.3 V logic; GPIO 3 must not be pulled high at boot (bench checklist 5.3).

### CAN bus selection

A car can carry several independent CAN buses at different bitrates. On this VW generation:

| Bus | Bitrate | Carries | Use it? |
|-----|---------|---------|---------|
| **Drivetrain CAN** (Antriebs-CAN) | 500 kbps | Engine (RPM), ABS (wheel speed), gateway | ✅ — tap here |
| **Comfort CAN** (Komfort-CAN) | 100 kbps | Doors, central locking, windows | ❌ |
| **Infotainment CAN** | 100 kbps | Radio, navigation | ❌ |

Tap the drivetrain bus **directly** (e.g. engine ECU or ABS connectors). The OBD-II port (pins 6/14) may sit on a separate diagnostic bus behind the gateway where frames are filtered or re-mapped. Typical VW wire colours — verify with a multimeter: drivetrain CAN-H orange/black, convenience CAN-H orange/green, infotainment CAN-H orange/violet, CAN-L orange/brown (all buses). Both lines sit at ~2.5 V at rest; CAN-H rises to ~3.5 V and CAN-L drops to ~1.5 V while frames are active.

> **The firmware confirms the right pair.** It listens at a fixed 500 kbps, so tapping the wrong bus shows `can:no_frames` (or bus errors) on the sniff screen instead of frames.

What you can display per bus:

| Bus | Signals available for the display |
|-----|-----------------------------------|
| **Drivetrain CAN** (500 kbps) | Engine RPM · vehicle speed · coolant · fuel level · battery voltage · odometer · engine load · throttle · reverse light · gear on automatics (Stage 2) |
| **Comfort CAN** (100 kbps) | Doors · central locking · windows · lights · key state |
| **Infotainment CAN** (100 kbps) | Media metadata · volume · navigation |

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
pio run
```

Native host tests (no hardware required):

```bash
pio test -e native
```

## BLE Protocol (v2)

**App → ESP32 commands:**

| Command | Direction | Description |
|---------|-----------|-------------|
| `set_color:#RRGGBB` | App → ESP32 | Set theme color |
| `gear:X` | App → ESP32 | Manual gear override (R/1–5/N) |
| `sniff:on` / `sniff:off` | App → ESP32 | Enable/disable the CAN frame dump |
| `debug:on` / `debug:off` | App → ESP32 | Enable/disable debug notifications |
| `get_state` | App → ESP32 | Request current state |

**ESP32 → App notifications:**

| Notification | Description |
|--------------|-------------|
| `theme:#RRGGBB` | Current theme color |
| `can:ok` / `can:offline` | CAN bus state (in `get_state` and on change) |
| `proto:2` | BLE protocol version (in `get_state`) |
| `debug:<GEAR>,<RPM>,<SPEED>,<CANSTAT>` | Live debug values (gear R/1..5/N, RPM, km/h, ok/offline) |
| `sniff:<E\|S><8hexid>:<datahex>` | One rate-limited sniffed frame (E = 29-bit, S = 11-bit) |
| `can:no_frames` | No frames while sniffing (silent bus, rate-limited) |
| `error:removed:<cmd>` | Rejected v1 command (`calibrate:`, `stream:`, `quat:`, ...) |

Removed in v2: `calibrate:X`, `stream:on/off`, `quat:...`, `calibration_ok`, `cal_status:`, `cal_data:`, `bno:ok/error` — stale clients receive `error:removed:<cmd>`.

## Verification

The CAN path is verified manually — it needs real hardware, so it is never a CI gate. Two stages:

1. **Bench** — [CAN listen & sniff bench checklist](docs/bench-checklist.md): 24 checks on an isolated 500 kbps mini-bus (listen-only proof, frame flow and rate cap, overflow and recovery, safe-fail, boot strapping).
2. **In-car** — the checklist below, on the car's drivetrain bus.

### In-car checklist

Manual verification on the car's drivetrain bus (500 kbps, 29-bit IDs). The termination jumper MUST be disabled (R10). Engine running where noted.

| # | Check | Expected | Result |
|---|-------|----------|--------|
| 1 | Engine running, sniff on, car on the bus | 29-bit (extended) frames appear as `sniff:E<8hexid>:<datahex>` at ≤ 20 lines/s | [ ] |
| 2 | Sniff over BLE and Serial at the same time | The same frames on both channels | [ ] |
| 3 | Ground / common-mode with the engine running (R10) | Knob on its own supply: no resets, no lock-ups, stable frame flow | [ ] |
| 4 | Drive through the gears and compare with the shifter | Indicator matches R/1–5/N; N at standstill; no flicker at band edges | [ ] |

## Project Structure

```
src/
├── can/          # TWAI listen-only driver + sniff mode (RX task, status, counters)
├── geardecode/   # Pure CAN types, signal extractor, ratio estimator (native-tested)
├── gearsource/   # CAN snapshot → gear glue
├── gears/        # Gear display state + arc animation (gears.h API)
├── ble/          # BLE communication, command parsing, sniff drain, debug format
├── calibration/  # Gear enum + fromString (no NVS)
├── bno/          # v1 IMU driver — read-only reference, excluded from the build
├── display/      # CO5300 display driver
├── lvgl_port/    # LVGL integration
├── theme/        # Theme management
├── boot/         # Boot animation
└── ui/           # SquareLine Studio generated UI

test/
├── test_calibration/  # fromString + enum tests
└── test_decoder/      # estimator, signals, sniff, BLE format tests (native env)
```
