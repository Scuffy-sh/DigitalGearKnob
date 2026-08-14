# Digital Gear Knob

**Read this in Spanish → [README_ES.md](README_ES.md)**

I created this digital gear indicator from scratch — hardware, firmware, and app. An ESP32-S3 mounted on the gear stick reads the selected gear straight from the car's own CAN bus — the drivetrain network of the car — through a Waveshare SN65HVD230 transceiver, and shows it on an AMOLED display with an LVGL animated arc. The same device exposes a Bluetooth Low Energy service that a Flutter companion app ("scuffy") uses for live debugging, CAN sniffing, and theming.

## Key Features

- **7-gear detection** (R, 1–5, N) read directly from the car's drivetrain CAN bus — no sensor on the stick and no calibration.
- **AMOLED display with LVGL arc animation** — the selected gear is shown with a 2-second "loading" arc animation instead of a hard value jump.
- **BLE connectivity to a Flutter app** — the app shows the live CAN status and gear, changes the accent color, and toggles the sniff mode.
- **Live BLE debug mode** — sending `debug:on` makes the device notify `debug:<GEAR>,<RPM>,<SPEED>,<CANSTAT>` on every detection cycle.
- **CAN sniff mode for reverse-engineering** — `sniff:on` dumps rate-limited frames (ID + payload) over BLE and Serial, so unknown frame layouts can be confirmed before they are configured.
- **Listen-only by design** — the TWAI controller is hard-coded to listen-only mode with no transmit path; a listening node can never disturb the vehicle bus.

## Architecture Overview

```
┌────────────────────────────────────────────────────────────────┐
│                    ESP32-S3 · T-Display S3                     │
│                    C++ / Arduino / PlatformIO                  │
│                                                                │
│  ┌──────────────┐   TWAI · listen-only  ┌─────────────┐        │
│  │ SN65HVD230   │ ─────────────────────▶│  gears      │        │
│  │ CAN trans-   │   500 kbps · GPIO 3/5 │  (ratio     │        │
│  │ ceiver       │   RX only             │  estimator) │        │
│  └──────▲───────┘                       └──────┬──────┘        │
│         │                                      │ LVGL 8.3.11   │
│         │                                      ▼               │
│         │                               ┌──────────────┐       │
│         │                               │ 1.43" AMOLED │       │
│         │                               │ arc + gear   │       │
│         │                               └──────────────┘       │
│         │                                      │               │
│         │                                 NimBLE "SCUFFY"      │
└─────────┼───────────────────────────────────────┬──────────────┘
          │ CAN_H / CAN_L (drivetrain bus)        │ BLE
          ▼                                       ▼
┌───────────────────────────┐            ┌────────────────────┐
│ The car's drivetrain CAN  │            │  Flutter app        │
│ 500 kbps · 29-bit IDs     │            │  "scuffy"           │
│ tapped (listen-only)      │            │  flutter_blue_plus  │
└───────────────────────────┘            │  scan · connect ·   │
                                         │  sniff · debug ·    │
                                         │  theme              │
                                         └────────────────────┘
```

Two data paths meet on the board: frames from the vehicle's drivetrain CAN bus arrive through the SN65HVD230 transceiver, are turned into a gear by the ratio estimator, and are rendered on the AMOLED via LVGL; in parallel, the same board runs a NimBLE GATT server so the phone app can observe, sniff, and configure it.

## Components

### ESP32-S3 T-Display AMOLED

<img src="images/t-display-s3-amoled.jpg" width="400" alt="ESP32-S3 T-Display AMOLED">

**Technologies:** ESP32-S3 (dual-core, 240 MHz) · Wi-Fi/BLE · 1.43" AMOLED · QSPI · LVGL 8.3.11

The brain and the screen in one. This LilyGO T-Display S3 board runs the whole firmware — CAN reception and decoding, gear detection, LVGL rendering, and the BLE server — and its 1.43" AMOLED shows the current gear with the animated loading arc. The firmware also supports other AMOLED panels of the same board family; the active panel is selected with a define in `include/pin_config.h`.

### SN65HVD230 CAN transceiver

<img src="images/sn65hvd230-can.jpg" width="400" alt="Waveshare SN65HVD230 CAN transceiver">

**Technologies:** Waveshare SN65HVD230 breakout · CAN 2.0B transceiver · 3.3 V logic · onboard 120 Ω termination (jumper, disabled for the vehicle tap)

The bridge between the car and the knob. CAN is a differential two-wire bus (CAN_H / CAN_L); the SN65HVD230 converts it to the 3.3 V logic levels that the ESP32-S3's built-in TWAI controller can read. It connects to the car's drivetrain bus — the network that carries the RPM and wheel-speed signals the gear detection needs.

#### Wiring

The transceiver is wired directly to the ESP32-S3 pins:

| SN65HVD230 | ESP32-S3 | Function |
| ---------- | -------- | -------- |
| **VCC**    | **3.3V** | Power    |
| **GND**    | **GND**  | Ground   |
| **TXD**    | **GPIO 3** | TWAI TX (never driven — listen-only) |
| **RXD**    | **GPIO 5** | TWAI RX |
| **CANH**   | Drivetrain CAN_H | Bus high |
| **CANL**   | Drivetrain CAN_L | Bus low |

> **Safety: listen-only and termination.** The firmware starts TWAI in listen-only mode and exposes no transmit path — the knob can never write to the car's bus. The onboard 120 Ω termination jumper of the Waveshare board must be **disabled** for the in-car tap: the drivetrain bus is already terminated at both ends, and a third 120 Ω in parallel would drop the bus to ~40 Ω and risk communication faults across the car. (Bench tests on an isolated mini-bus may keep it.)

#### CAN bus selection

A car can carry several independent CAN buses running at different bitrates. On this VW generation the network splits into three:

| Bus | Bitrate | Carries | Use it? |
| --- | --- | --- | --- |
| **Drivetrain CAN** (Antriebs-CAN) | 500 kbps | Engine (RPM), ABS (wheel speed), gateway | ✅ — tap here |
| **Comfort CAN** (Komfort-CAN) | 100 kbps | Doors, central locking, windows | ❌ |
| **Infotainment CAN** | 100 kbps | Radio, navigation | ❌ |

The drivetrain bus is the one carrying the RPM and wheel-speed signals the ratio estimator needs. Tap it **directly** (e.g. at the engine ECU or ABS connectors). The OBD-II diagnostic port (pins 6/14) may sit on a separate diagnostic bus behind the gateway, where frames are filtered or re-mapped — prefer the physical drivetrain bus.

Typical wire colours on VW drivetrain pairs (verify with a multimeter):

| Signal | Typical colour |
| --- | --- |
| CAN-H (drivetrain) | orange/black |
| CAN-L (drivetrain) | orange/brown |
| CAN-H (comfort) | orange/violet |
| CAN-L (comfort) | orange/brown |

**Multimeter check:** both lines sit at ~2.5 V at rest; CAN-H rises to ~3.5 V and CAN-L drops to ~1.5 V while frames are active.

> **The firmware itself confirms the right pair.** It listens at a fixed 500 kbps, so tapping the wrong bus shows `can:no_frames` (or bus errors) on the sniff screen instead of frames — a wrong pair is obvious, not confusing.

#### What each bus can show

| Bus | Signals available for the display |
| --- | --- |
| **Drivetrain CAN** (500 kbps) | Engine RPM · vehicle speed · coolant temperature · fuel level · battery voltage · odometer/trip · engine load · throttle position · reverse light · engaged gear on automatic gearboxes (Stage 2, via the transmission ECU) |
| **Comfort CAN** (100 kbps) | Door open/closed · central locking · window positions · light status · ignition/key state |
| **Infotainment CAN** (100 kbps) | Media metadata (track/station) · volume · navigation data |

The drivetrain bus is the interesting one for a gear knob: besides the gear itself, it carries every engine and chassis signal that fits on the small AMOLED.

### Gear knob

<img src="images/gear-knob.avif" width="400" alt="Gear knob">

The physical knob that replaces the stock one. The electronics are embedded inside the knob — the ESP32-S3 board with the display and the small CAN transceiver that taps the car's drivetrain bus — which is the mechanical integration of the whole project into the car: power and electronics live inside the knob, and the display faces the driver.

#### Assembly

The open view shows the ESP32-S3 wired inside the knob before the electronics were glued in place (the photo predates the v2 CAN transceiver). The final product keeps the display facing the driver, showing the current gear when powered on.

<img src="images/knob-open.jpeg" width="280" alt="Knob open: ESP32-S3 wired inside (v1 build)"> <img src="images/knob-final-off.jpeg" width="280" alt="Final knob, display off"> <img src="images/knob-final-on.jpeg" width="280" alt="Final knob, display on">

### Flutter app (scuffy)

<img src="images/flutter-app.jpeg" width="260" alt="Flutter companion app">

**Technologies:** Flutter · Dart · flutter_blue_plus · permission_handler · shared_preferences

The BLE client companion app. It scans for the device (advertised as `SCUFFY`), connects to the command characteristic, and shows the live CAN status and the current gear, with the debug values (gear, RPM, speed, CAN status) in a panel when the DEBUG toggle is on. It also has a sniff view that dumps raw CAN frames for reverse-engineering, and lets the user change the display accent color. Per-gear calibration is gone in v2 — the gear now comes from the car itself.

## How It Works

### Detection algorithm

1. **Listen to the drivetrain bus.** The SN65HVD230 converts the differential CAN bus to 3.3 V logic; the ESP32-S3's built-in TWAI controller receives frames in listen-only mode at 500 kbps (GPIO 3 TX / GPIO 5 RX).
2. **Extract RPM and wheel speed.** The firmware decodes the two signals from their CAN frames using a configurable layout — the frame IDs and bit positions live in `VehicleCanConfig` and are confirmed with the sniff mode before going live.
3. **Compute the ratio.** Every detection cycle, the current ratio `RPM ÷ speed` is calculated.
4. **Match against the gearbox ratio table.** Each gear has a ratio band; the estimator matches the live ratio to the closest band with **hysteresis** at the edges, so the display does not flicker around a shift boundary.
5. **Apply the neutral, standstill, and clutch rules.** Below a minimum speed the result is N; if the ratio matches no band (neutral, clutch disengaged, wheel slip) the result is N; if frames stop arriving for 500 ms the result is N — a stale gear is never shown.
6. **Debounce and animate the arc.** A gear change is only accepted after **3 consecutive identical detections** (detection runs every 200 ms), which rejects transient readings while the stick is mid-shift. On acceptance, a 2-second ease-in-out LVGL arc animation plays toward the target value while the label shows the target gear the whole time — it never counts through intermediate gears on the way down (no N-5-4-3-2-1 flicker).

### CAN bus reading and sniff mode

Because the gear is estimated from the RPM ÷ speed ratio, the indicator works without any sensor on the stick — and without the v1 limitations: no calibration, no slope compensation, and no mounting-angle drift. The remaining edge cases (neutral, clutch, wheel slip) are handled by the N rules above, and reverse vs. 1st gear is close on this gearbox (see [Gear ratio table](#gear-ratio-table)).

The frame layout of the car's drivetrain bus is confirmed on the bench and in the car with the **sniff mode**: sending `sniff:on` over BLE (or typing it on Serial) makes the knob dump frames as `sniff:<E|S><8-hex-id>:<payload-hex>` on both channels, rate-limited to 20 lines per second; `sniff:off` stops the dump. If no frames arrive while sniffing for 2 seconds, the knob reports `can:no_frames` — a silent bus (wrong bitrate, swapped CANH/CANL, or a bad tap) is obvious instead of puzzling. There is a dedicated safety rule for the tap: the controller only listens and the termination jumper stays disabled (see the [wiring section](#sn65hvd230-can-transceiver)).

## Tech Stack

| Layer | Technology |
| --- | --- |
| Firmware | C++ · Arduino framework · PlatformIO |
| MCU | ESP32-S3 (LilyGO T-Display S3), dual-core 240 MHz |
| CAN bus | Waveshare SN65HVD230 transceiver · ESP32-S3 TWAI (listen-only) · 500 kbps |
| Display | 1.43" AMOLED · LVGL 8.3.11 · GFX Library for Arduino 1.4.9 |
| BLE (firmware) | NimBLE-Arduino ^1.4.1 |
| App | Flutter · Dart |
| BLE (app) | flutter_blue_plus ^1.35.5 |
| App support | permission_handler ^12.0.1 · flutter_colorpicker ^1.1.0 · shared_preferences ^2.2.0 |

## Project Structure

```
ChatGPT/
├── DigitalGearKnob/            # ESP32-S3 firmware (PlatformIO)
│   ├── platformio.ini          # envs: lilygo-t-display-s3, native (host tests)
│   ├── include/
│   │   ├── pin_config.h        # pin mapping + AMOLED panel selection
│   │   └── lgfx_user.hpp       # LGFX configuration
│   ├── src/
│   │   ├── main.cpp            # boot sequence + main loop
│   │   ├── display/            # QSPI AMOLED init (GFX library)
│   │   ├── lvgl_port/          # LVGL ↔ display integration
│   │   ├── ui/                 # SquareLine-generated LVGL screens (arc + gear label)
│   │   ├── boot/               # logo splash + fade into the gear screen
│   │   ├── theme/              # accent color / theming
│   │   ├── can/                # TWAI listen-only driver + sniff mode
│   │   ├── geardecode/         # pure CAN types, signal extractor, ratio estimator
│   │   ├── gearsource/         # CAN snapshot → gear glue
│   │   ├── gears/              # gear display state + arc animation
│   │   ├── ble/                # NimBLE GATT server + debug mode + sniff drain
│   │   ├── calibration/        # gear enum + fromString (no NVS)
│   │   └── bno/                # v1 IMU driver — kept as read-only reference, not built
│   └── test/                   # host-side Unity tests (native env)
│       ├── test_calibration/   # fromString + enum tests
│       └── test_decoder/       # estimator, signals, sniff, BLE format tests
├── scuffy/                     # Flutter companion app
│   └── lib/
│       ├── services/ble_service.dart   # BLE client singleton (scan, notify, commands)
│       ├── screens/                    # home, sniff, splash
│       ├── widgets/                    # gear card, theme card, status UI
│       ├── models/                     # gear, theme, BLE messages, sniff session
│       └── data/                       # theme presets
├── images/                         # product images used by this README
```

## Gear Ratio Table

The estimator matches the RPM ÷ wheel-speed ratio against a per-gear table in `VehicleCanConfig` — the single vehicle-specific data point. The defaults come from the published ratios of the car's gearbox, scaled by the final drive and the wheel revs/km for its tyres (~503.7 revs/km):

| Gear | Ratio (RPM per km/h) |
| --- | --- |
| R | 102.4 |
| 1 | 107.5 |
| 2 | 58.7 |
| 3 | 38.3 |
| 4 | 27.5 |
| 5 | 22.0 |

Each band is the center value ±8%, with a 500 ms stale timeout and a 5 km/h minimum speed below which the ratio is meaningless (N). These are **starting values to verify in-car** — R and 1st are close on this gearbox, so ratio-only reverse detection is ambiguous; the direct decode of the gearbox messages planned for a future stage removes that ambiguity entirely.

## Verification

The CAN path is verified manually — it needs real hardware, so it is never a CI gate:

- **Bench** — the [CAN listen & sniff bench checklist](DigitalGearKnob/docs/bench-checklist.md): 24 checks on an isolated 500 kbps mini-bus (listen-only proof, frame flow and rate cap, overflow and recovery, safe-fail on wrong bitrate or swapped wires, boot strapping).
- **In-car** — the in-car checklist in the [firmware README](DigitalGearKnob/README.md#verification): live 29-bit frames on the car's bus, sniff over BLE and Serial, ground/common-mode with the engine running, and gear accuracy while driving.

## Future Improvements

**Reading the gear directly from the VW gear messages (0x540 / 0x48A).** The current v2 implementation (Stage 1) ships the full CAN infrastructure — a listen-only tap that can never disturb the bus, with the termination jumper disabled — plus a ratio-based estimator that derives the gear from RPM and wheel speed. My next step is to decode the gearbox messages themselves, so the car reports the engaged gear directly instead of the knob estimating it from the ratio. That removes the ratio ambiguity between reverse and 1st gear and the neutral/clutch edge cases; the exact frame layout gets confirmed with the sniff mode first.

**And the display unlocks much more than the gear.** The AMOLED is already there — the knob is now connected to the CAN bus, which turns it into a real dashboard on the stick. The car broadcasts dozens of live signals, and the knob can render any of them on the 1.43" screen:

- **Speed and engine RPM** — real-time values on the stick, no need to look away from the road.
- **Engine coolant temperature** — an early warning for overheating.
- **Fuel level** — remaining range at a glance.
- **Battery voltage** — alternator health without extra sensors.
- **Odometer / trip data** — where the car has been and how far.

The gear becomes just one channel among many; the same knob, display, and BLE link already built for this project can present the whole vehicle. The image below shows the kind of data that is available over CAN:

<img src="images/can-data.jpeg" width="280" alt="Vehicle data available over the CAN bus">

Other planned improvements:
- **Dedicated `gear:X` BLE notification** so the phone app can display the current gear without debug mode or polling.
- **Accept-only frame filtering** — once the sniff stage confirms the relevant frame IDs, filter the RX path to just those frames to cut bus noise.
- **More dashboard signals on the AMOLED** — coolant temperature, fuel level, battery voltage, and odometer are already on the same bus.
- **More vehicles** — `VehicleCanConfig` is per-vehicle, so adding another car becomes a configuration task plus the sniff workflow, not a firmware rewrite.

## License

This project is licensed under the [MIT License](LICENSE).
