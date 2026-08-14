#pragma once

#include <cstddef>
#include <cstdint>

#include "geardecode/can_types.h"

//=========================================================================
// CAN TWAI LISTEN-ONLY HAL — interface + native-testable seams
//=========================================================================
// Safety contract (design D4 / spec cross-cutting): this module can ONLY
// listen. The TWAI mode is hard-coded to LISTEN_ONLY, the TX queue length
// is forced to 0, and NO transmit API exists anywhere in the wrapper.
//
// Only src/can/can.cpp includes driver/twai.h; this header stays HAL-free
// so the pure seams below (CanHwConfig validation, CanStatusTracker) compile
// and run in the native test env (design D1: HAL -> pure, never reverse).

// Hard-coded wiring (task 3.1 / design D4): Waveshare SN65HVD230 on
// GPIO 3 (TWAI TX / transceiver CTX) and GPIO 5 (TWAI RX).
#define CAN_TX_GPIO 3
#define CAN_RX_GPIO 5

// TWAI bitrates with timing macros available in the ESP32-S3 SDK
// (hal/twai_types.h). src/can/can.cpp keeps its bitrate -> timing switch
// in sync with this list.
static constexpr uint32_t kCanSupportedBitrates[] = {125000, 250000, 500000,
                                                     1000000};

inline bool can_bitrate_supported(uint32_t bitrate)
{
    for (uint32_t b : kCanSupportedBitrates)
    {
        if (b == bitrate) return true;
    }
    return false;
}

//-----------------------------------------------------------------
// Native-testable seam: HAL configuration (design D4, spec listen-only)
//-----------------------------------------------------------------
// Defaults are the task's hard-coded values; is_valid() enforces the safety
// invariants so no configuration that could open a transmit path ever
// reaches the TWAI driver.
struct CanHwConfig
{
    bool listen_only = true;    // safety: MUST stay true (hard-coded)
    uint8_t tx_gpio = CAN_TX_GPIO;
    uint8_t rx_gpio = CAN_RX_GPIO;
    uint32_t bitrate = 500000;  // Golf 6 drivetrain bus
    uint32_t tx_queue_len = 0;  // safety: MUST stay 0 (no TX queue)
    uint32_t rx_queue_len = 64; // design D5: absorb nominal bus load

    bool is_valid() const;
};

inline bool CanHwConfig::is_valid() const
{
    // The two safety invariants are non-negotiable (design D4):
    if (!listen_only) return false;     // a TX-capable mode is never acceptable
    if (tx_queue_len != 0) return false; // any TX queue opens a transmit path
    if (tx_gpio == rx_gpio) return false;
    if (tx_gpio > 48 || rx_gpio > 48) return false; // ESP32-S3 GPIO range
    if (rx_queue_len == 0) return false; // design D5 requires a real queue
    return can_bitrate_supported(bitrate);
}

//-----------------------------------------------------------------
// Native-testable seam: CAN status state machine (spec: alert reporting)
//-----------------------------------------------------------------
enum class CanBusState : uint8_t
{
    OFFLINE,   // not started, or init failed (spec: non-fatal)
    ONLINE,    // listening (RX task draining frames)
    BUS_OFF,   // controller left the bus (error condition)
    RECOVERING // bus recovery in progress (spec: recovery without TX)
};

class CanStatusTracker
{
public:
    void on_started() { state_ = CanBusState::ONLINE; }
    void on_start_failed() { state_ = CanBusState::OFFLINE; }
    void on_bus_off_alert() { state_ = CanBusState::BUS_OFF; }
    void on_recovery_started()
    {
        if (state_ == CanBusState::BUS_OFF) state_ = CanBusState::RECOVERING;
    }
    void on_bus_recovered_alert() { state_ = CanBusState::ONLINE; }
    void on_error_passive_alert() { error_passive_ = true; } // latched

    CanBusState state() const { return state_; }
    bool is_online() const { return state_ == CanBusState::ONLINE; }
    bool is_error_passive() const { return error_passive_; }

private:
    CanBusState state_ = CanBusState::OFFLINE;
    bool error_passive_ = false;
};

//-----------------------------------------------------------------
// Runtime snapshot (design data flow) + counters
//-----------------------------------------------------------------
struct CanSnapshot
{
    float rpm = 0.0f;
    float speed_kmh = 0.0f;
    uint32_t ts_ms = 0;
    bool valid = false; // false until configured signals decode (design D9)
};

struct CanCounters
{
    uint64_t rx_frames = 0;  // frames received by the RX task
    uint64_t rx_dropped = 0; // frames lost: RX queue full / FIFO overrun
    uint64_t bus_errors = 0; // bit/stuff/crc/form/ack errors on the bus
    uint64_t recoveries = 0; // bus-off recoveries initiated
};

//-----------------------------------------------------------------
// Listen-only API (NO TX API exists — design D4)
//-----------------------------------------------------------------
// Installs + starts TWAI in TWAI_MODE_LISTEN_ONLY (500 kbps, GPIO 3/5,
// tx_queue_len 0, rx_queue_len 64) and spawns the RX task. Returns true on
// success; failure is non-fatal — the caller reports "CAN offline"
// (spec: init failure is non-fatal).
bool can_init(void);

bool can_is_online(void);
bool can_is_error_passive(void);

void can_get_snapshot(CanSnapshot &out);
void can_get_counters(CanCounters &out);

// D9: install the sniff-confirmed frame layout before the source goes live.
void can_set_vehicle_config(const VehicleCanConfig &cfg);

// FreeRTOS task entry (prio 8, core 0, 4 KB stack — design D5).
void can_rx_task(void *arg);

// Sniff drain (design D6): consumes one formatted line from the internal
// queue; used by the loop's BLE/Serial path (Phase 5). Returns false when
// the queue is empty or the buffer is too small.
bool can_sniff_take(char *out, size_t cap);

// Sniff queue dimensions (design D6: 32 lines).
#define CAN_SNIFF_QUEUE_LEN 32
#define CAN_SNIFF_TEXT_MAX 64
