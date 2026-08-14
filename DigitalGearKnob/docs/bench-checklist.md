# Bench Checklist — CAN Listen & Sniff (HW-in-the-loop)

Manual verification for the v2 CAN features (`src/can/`). The native Unity
suites cover the pure logic (config validation, status tracker, sniff
limiter/formatter/watchdog); this checklist proves the ESP32-S3 TWAI
hardware path against a real CAN bus. It is NOT CI-runnable.

Run the checks in order. A failed check with a red box blocks the release
until fixed and re-verified.

## Setup

- DUT: LilyGO T-Display S3 with SN65HVD230 CAN transceiver (R9 strap
  LOW → 3.3 V logic), NO 120-ohm termination on the DUT side (only the
  bench bus has termination).
- Bus: 120-ohm terminated at both ends, at least 2 m, twisted pair.
- Peer: a known-good CAN generator (e.g. second controller or PC CAN
  adapter). For 500 kbps checks use a generator that reports its own TX
  error counters.
- Power: USB for the DUT; separate (isolated) supply for the generator
  if it drives the bus directly.
- Firmware: build with Phase 4/5 wiring (sniff output visible on Serial
  and/or BLE; CAN status in the status report).

## 1. Listen-only safety (prove no TX)

| # | Check | Expected | Result |
|---|-------|----------|--------|
| 1.1 | Peer sends frames at 500 kbps for 30 s while DUT runs | Peer error counter stays 0 (its frames are ACKed only by other bus nodes, and the DUT must NOT respond) | [ ] |
| 1.2 | DUT receives the full stream (see 2.x) | All frames visible; no TX errors reported in status | [ ] |

Why: `can.cpp` installs TWAI in `TWAI_MODE_LISTEN_ONLY` with
`tx_queue_len = 0` and exposes no transmit API. A listening node does not
ACK; if the DUT were transmitting, the peer would see errors on the wire.

## 2. Frame flow & rate cap (500 kbps)

| # | Check | Expected | Result |
|---|-------|----------|--------|
| 2.1 | Peer bursts 500 standard frames (ID 0x280, 8 bytes, as fast as possible) | Sniff lines appear at ≤ 20 lines/s (≈ 1 per 50 ms window) | [ ] |
| 2.2 | Count sniff lines for 10 s of max-rate traffic | ≈ 200 lines (20 Hz cap holds under burst) | [ ] |
| 2.3 | Line format matches `sniff:S00000280:<16 hex>` exactly | No extra chars, uppercase hex, 8-digit zero-padded ID | [ ] |
| 2.4 | Peer sends a 29-bit frame (e.g. ID 0x00000280) | Line starts `sniff:E00000280:` | [ ] |
| 2.5 | Peer sends a 29-bit frame with ID 0x1FFFFFFF | `sniff:E1FFFFFFF:` + payload | [ ] |
| 2.6 | Peer sends 0-length frames | `sniff:S<8 hex>: ` (empty payload) — no crash, no stall | [ ] |
| 2.7 | Stop the peer completely, leave DUT sniffing | `can:no_frames` appears within ≈ 2–3 s, then re-appears no faster than every 2 s | [ ] |
| 2.8 | Resume traffic | Sniff lines resume within one 50 ms window | [ ] |

## 3. Overflow & resilience

| # | Check | Expected | Result |
|---|-------|----------|--------|
| 3.1 | Peer floods faster than the loop drains for 60 s | Status shows RX queue-full/overflow counters incrementing but DUT never hangs or resets | [ ] |
| 3.2 | Unplug the CAN connector while DUT runs | No crash; status stays ONLINE (bus idle is normal); sniffing continues waiting | [ ] |
| 3.3 | Reconnect after 10 s | Frames resume without reboot | [ ] |

## 4. Safe-fail (misconfiguration)

| # | Check | Expected | Result |
|---|-------|----------|--------|
| 4.1 | Generator set to the WRONG bitrate (e.g. 125 kbps) | No frames decoded; `can:no_frames` may appear; no resets, no error spam on the UI; status may flag bus errors — still responsive | [ ] |
| 4.2 | CAN-H / CAN-L wires SWAPPED at the DUT connector | Same as 4.1: safe fail, DUT stays responsive, no TX on the wire | [ ] |
| 4.3 | Wire a short to ground on CAN-H for 5 s then remove | DUT recovers to ONLINE automatically (bus-off → recovery path) or reports BUS_OFF/RECOVERING in status without reboot | [ ] |
| 4.4 | Repeat 4.3 three times | Same behavior every time (repeatable recovery) | [ ] |

## 5. Boot & hardware strapping

| # | Check | Expected | Result |
|---|-------|----------|--------|
| 5.1 | Power cycle DUT with bus idle | Boots normally; sniffing starts; `can:no_frames` after 2 s | [ ] |
| 5.2 | Power cycle DUT with bus ACTIVE at 500 kbps | Boots normally; frames captured from the first window | [ ] |
| 5.3 | Confirm GPIO 3 is not pulled high at boot (R9 strap low) | SN65HVD230 is 3.3 V logic; CAN still decodes at 500 kbps (covers 5.1/5.2) | [ ] |

## 6. Status report integration (Phase 5 wiring)

| # | Check | Expected | Result |
|---|-------|----------|--------|
| 6.1 | Gears decoded from live traffic | Status shows the correct gear for known IDs (e.g. VW 0x280 ratio) once signals are configured | [ ] |
| 6.2 | Signals NOT configured | Status reports gear offline (snapshot gated by `signals_configured`) | [ ] |
| 6.3 | Sniff toggle off | Dump stops within one rate-limit window; `can:no_frames` stops too | [ ] |
| 6.4 | Sniff toggle on again | Dump resumes; 2 s silence window restarts cleanly | [ ] |

## Sign-off

| | |
|---|---|
| Bench operator | ____________ |
| Date | ____________ |
| Firmware commit/branch | ____________ |
| Checks passed | ___ / 24 |

Notes (deviations, observations):
