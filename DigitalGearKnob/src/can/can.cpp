#include "can/can.h"

#include <Arduino.h>

#include <cstring>
#include <mutex>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/twai.h"

#include "can/sniff.h"
#include "geardecode/vwsignals.h"

//=========================================================================
// TWAI LISTEN-ONLY HAL — ESP32-S3 (design D4/D5, spec can-transceiver)
//=========================================================================
// Safety: this is the ONLY file that includes driver/twai.h. The controller
// is started in TWAI_MODE_LISTEN_ONLY with tx_queue_len = 0 and this module
// exposes no transmit API — no code path can put a message on the bus
// (spec cross-cutting: no transmission, ever).
//
// The native test env compiles can.h (pure seams) only; this file is
// firmware-only and NOT in the native build_src_filter. Its compile gate is
// the Phase 4 firmware build (`pio run -e lilygo-t-display-s3`, task 4.4);
// the twai.h API used here was verified against the esp32s3 SDK headers at
// apply time (slice policy forbids building the firmware in Phase 3).

namespace
{

constexpr uint32_t kRxTimeoutMs = 20; // twai_receive / alert poll (design D5)
constexpr uint32_t kAlertsEnabled =
    TWAI_ALERT_RX_DATA | TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_BUS_ERROR |
    TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED;
constexpr uint32_t kSniffPeriodMs = 1000 / 20; // 20 Hz rate cap (design D6)
constexpr uint32_t kNoFramesSilenceMs = 2000;  // 2 s silent bus (design data flow)
constexpr uint32_t kNoFramesReportMs = 2000;   // can:no_frames rate limit
constexpr uint32_t kTaskStackWords = 1024;     // 4 KB (design D5)
constexpr UBaseType_t kTaskPriority = 8;       // design D5
constexpr BaseType_t kTaskCore = 0;            // design D5

std::mutex g_snapshot_mutex; // guards g_snapshot + g_config + g_counters
CanSnapshot g_snapshot;
VehicleCanConfig g_config = make_golf6_default_config();
CanCounters g_counters;
CanStatusTracker g_status;
QueueHandle_t g_sniff_queue = nullptr;
SniffLimiter g_sniff_limiter(kSniffPeriodMs);
SniffWatchdog g_sniff_watchdog(kNoFramesSilenceMs, kNoFramesReportMs);

// Maps a validated bitrate to the TWAI timing config. Keep the case list in
// sync with kCanSupportedBitrates in can.h.
bool can_timing_for_bitrate(uint32_t bitrate, twai_timing_config_t &out)
{
    switch (bitrate)
    {
        case 125000: out = TWAI_TIMING_CONFIG_125KBITS(); return true;
        case 250000: out = TWAI_TIMING_CONFIG_250KBITS(); return true;
        case 500000: out = TWAI_TIMING_CONFIG_500KBITS(); return true;
        case 1000000: out = TWAI_TIMING_CONFIG_1MBITS(); return true;
        default: return false;
    }
}

} // namespace

//-----------------------------------------------------------------
// can_init — install + start listen-only TWAI (spec: boot to listening)
//-----------------------------------------------------------------
bool can_init(void)
{
    // Safety invariant (design D4): the hard-coded listen-only config must
    // pass validation or the module refuses to start. The TWAI mode below is
    // a constant — never read from runtime input.
    CanHwConfig cfg; // LISTEN_ONLY, GPIO 3/5, 500k, tx_queue 0, rx_queue 64
    if (!cfg.is_valid())
    {
        g_status.on_start_failed();
        return false;
    }

    // Re-init safe: tear down any previous instance first.
    twai_stop();
    twai_driver_uninstall();

    twai_general_config_t g = {};
    g.mode = TWAI_MODE_LISTEN_ONLY; // no transmissions, no ACK (spec safety)
    g.tx_io = static_cast<gpio_num_t>(cfg.tx_gpio);
    g.rx_io = static_cast<gpio_num_t>(cfg.rx_gpio);
    g.clkout_io = TWAI_IO_UNUSED;
    g.bus_off_io = TWAI_IO_UNUSED;
    g.tx_queue_len = cfg.tx_queue_len; // 0 — no TX queue (design D4)
    g.rx_queue_len = cfg.rx_queue_len; // 64 — absorb nominal load (design D5)
    g.alerts_enabled = kAlertsEnabled;
    g.clkout_divider = 0;
    g.intr_flags = 0;

    twai_timing_config_t t = {};
    if (!can_timing_for_bitrate(cfg.bitrate, t))
    {
        g_status.on_start_failed();
        return false;
    }
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL(); // Stage-1 accept-all

    if (twai_driver_install(&g, &t, &f) != ESP_OK)
    {
        g_status.on_start_failed(); // spec: init failure is non-fatal
        return false;
    }
    if (twai_start() != ESP_OK)
    {
        g_status.on_start_failed();
        twai_driver_uninstall();
        return false;
    }

    // Sniff queue (design D6): 32 lines of up to CAN_SNIFF_TEXT_MAX chars.
    if (g_sniff_queue == nullptr)
    {
        g_sniff_queue = xQueueCreate(CAN_SNIFF_QUEUE_LEN, CAN_SNIFF_TEXT_MAX);
    }

    // RX task (design D5): dedicated drainer so the boot splash never
    // starves the driver RX queue. If it cannot be created, treat the
    // module as offline — nothing else drains the queue.
    if (xTaskCreatePinnedToCore(can_rx_task, "can_rx", kTaskStackWords, nullptr,
                                kTaskPriority, nullptr, kTaskCore) != pdPASS)
    {
        twai_stop();
        twai_driver_uninstall();
        g_status.on_start_failed();
        return false;
    }

    g_status.on_started();
    return true;
}

//-----------------------------------------------------------------
// can_rx_task — drains frames + alerts (design D5)
//-----------------------------------------------------------------
void can_rx_task(void * /*arg*/)
{
    for (;;)
    {
        // 1. Alerts: surface bus-state changes and drop counters. The poll
        //    also acts as the status heartbeat for the BUS_RECOVERED edge.
        uint32_t alerts = 0;
        if (twai_read_alerts(&alerts, pdMS_TO_TICKS(kRxTimeoutMs)) == ESP_OK &&
            alerts != 0)
        {
            if (alerts & TWAI_ALERT_RX_QUEUE_FULL) g_counters.rx_dropped++;
            if (alerts & TWAI_ALERT_BUS_ERROR) g_counters.bus_errors++;
            if (alerts & TWAI_ALERT_ERR_PASS) g_status.on_error_passive_alert();
            if (alerts & TWAI_ALERT_BUS_OFF)
            {
                g_status.on_bus_off_alert();
                // Listen-only recovery (spec: recovery without transmitting).
                if (twai_initiate_recovery() == ESP_OK)
                {
                    g_counters.recoveries++;
                    g_status.on_recovery_started();
                }
            }
            if (alerts & TWAI_ALERT_BUS_RECOVERED)
            {
                g_status.on_bus_recovered_alert();
            }
        }

        // 2. One frame per loop (design D5: receive with 20 ms timeout).
        twai_message_t msg;
        if (twai_receive(&msg, pdMS_TO_TICKS(kRxTimeoutMs)) != ESP_OK)
        {
            continue; // timeout — bus silent or wrong bitrate (safe fail)
        }

        CanFrame frame;
        frame.id = msg.identifier;
        frame.ext = msg.extd != 0;
        frame.dlc = msg.data_length_code <= 8 ? msg.data_length_code : 8;
        std::memcpy(frame.data, msg.data, frame.dlc);
        frame.ts = millis();

        {
            std::lock_guard<std::mutex> lock(g_snapshot_mutex);
            g_counters.rx_frames++;

            // Signal match into the snapshot (design data flow). D9: the
            // layout must be sniff-confirmed (signals_configured) first,
            // otherwise the gear source stays offline.
            if (g_config.signals_configured)
            {
                float value = 0.0f;
                if (frame.id == g_config.rpm_sig.frame_id &&
                    vw_decode_signal(frame, g_config.rpm_sig, value))
                {
                    g_snapshot.rpm = value;
                    g_snapshot.ts_ms = frame.ts;
                    g_snapshot.valid = true;
                }
                if (frame.id == g_config.speed_sig.frame_id &&
                    vw_decode_signal(frame, g_config.speed_sig, value))
                {
                    g_snapshot.speed_kmh = value;
                    g_snapshot.ts_ms = frame.ts;
                    g_snapshot.valid = true;
                }
            }
        }

        // 3. Sniff path (design D6): pure format -> 20 Hz limiter -> queue.
        const uint32_t now = millis();
        g_sniff_watchdog.note_frame(now);
        if (sniff_enabled() && g_sniff_limiter.allow(now) &&
            g_sniff_queue != nullptr)
        {
            char line[CAN_SNIFF_TEXT_MAX];
            if (sniff_format_frame(frame, line, sizeof(line)))
            {
                xQueueSend(g_sniff_queue, line, 0);
            }
        }

        // 4. Silent-bus signal (spec: silent bus scenario, design data flow).
        if (g_sniff_watchdog.poll(sniff_enabled(), now) &&
            g_sniff_queue != nullptr)
        {
            char no_frames[CAN_SNIFF_TEXT_MAX];
            std::snprintf(no_frames, sizeof(no_frames), "can:no_frames");
            xQueueSend(g_sniff_queue, no_frames, 0);
        }
    }
}

//-----------------------------------------------------------------
// Accessors
//-----------------------------------------------------------------

bool can_is_online(void)
{
    return g_status.is_online();
}

bool can_is_error_passive(void)
{
    return g_status.is_error_passive();
}

void can_get_snapshot(CanSnapshot &out)
{
    std::lock_guard<std::mutex> lock(g_snapshot_mutex);
    out = g_snapshot;
}

void can_get_counters(CanCounters &out)
{
    std::lock_guard<std::mutex> lock(g_snapshot_mutex);
    out = g_counters;
}

void can_set_vehicle_config(const VehicleCanConfig &cfg)
{
    std::lock_guard<std::mutex> lock(g_snapshot_mutex);
    g_config = cfg;
    g_snapshot = CanSnapshot{}; // new layout -> old values are stale
}

bool can_sniff_take(char *out, size_t cap)
{
    if (out == nullptr || cap < CAN_SNIFF_TEXT_MAX || g_sniff_queue == nullptr)
    {
        return false;
    }
    return xQueueReceive(g_sniff_queue, out, 0) == pdTRUE;
}
