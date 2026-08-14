#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "geardecode/can_types.h"

//=========================================================================
// CAN SNIFF — rate-limited frame dump + silent-bus signal (design D6)
//=========================================================================
// Pure + atomic only: this header and sniff.cpp compile in the native test
// env (no HAL includes — see the native build_src_filter). The CAN task
// (can.cpp) drives the limiter and watchdog; the loop's BLE/Serial path
// drains the formatted lines (Phase 5).
//
// Line formats (BLE protocol v2 table / design data flow):
//   sniff:<E|S><8hexid>:<payloadhex>   — one rate-limited frame
//   can:no_frames                      — silent bus while sniffing (2 s)

// Runtime toggle — atomic so the CAN task and the loop agree (design D6).
void sniff_set_enabled(bool enabled);
bool sniff_enabled();

// 20 Hz output cap for sniff frames (spec: rate limiting; design: 20 Hz).
// Pure: `allow(now_ms)` admits at most one frame per period_ms.
class SniffLimiter
{
public:
    explicit SniffLimiter(uint32_t period_ms) : period_ms_(period_ms) {}

    // True when a frame stamped `now_ms` may be published. The first call
    // always passes; afterwards at most one frame per period passes.
    // Unsigned subtraction keeps the comparison correct across wraparound.
    bool allow(uint32_t now_ms)
    {
        if (!has_allowed_ || (now_ms - last_allowed_ms_) >= period_ms_)
        {
            last_allowed_ms_ = now_ms;
            has_allowed_ = true;
            return true;
        }
        return false;
    }

    void reset() { has_allowed_ = false; }

private:
    uint32_t period_ms_;
    uint32_t last_allowed_ms_ = 0;
    bool has_allowed_ = false;
};

// Silent-bus signal (spec: silent bus scenario): while sniffing is enabled
// and no frame arrives within silence_ms, poll() reports at most once per
// report_period_ms so the operator knows the tap sees no traffic. The
// silence clock starts when observation begins (first poll with sniffing
// enabled, or the last received frame), so turning sniffing off and on
// restarts the 2 s window.
class SniffWatchdog
{
public:
    SniffWatchdog(uint32_t silence_ms, uint32_t report_period_ms)
        : silence_ms_(silence_ms), report_period_ms_(report_period_ms)
    {
    }

    // Call on every received frame (also when sniffing is off: the arrival
    // is a fact; the enable gate lives in poll()).
    void note_frame(uint32_t now_ms)
    {
        last_activity_ms_ = now_ms;
        has_frame_ = true;
    }

    // True at most once per report_period_ms while sniffing is enabled and
    // no frame arrived within silence_ms.
    bool poll(bool sniff_enabled, uint32_t now_ms)
    {
        if (!sniff_enabled)
        {
            observing_ = false; // discard the clock when sniffing turns off
            return false;
        }
        if (!observing_)
        {
            observing_ = true;
            if (!has_frame_)
            {
                last_activity_ms_ = now_ms; // clock starts at observation start
            }
        }
        const bool silent = (now_ms - last_activity_ms_) >= silence_ms_;
        if (!silent)
        {
            return false;
        }
        if (!has_reported_ || (now_ms - last_report_ms_) >= report_period_ms_)
        {
            last_report_ms_ = now_ms;
            has_reported_ = true;
            return true;
        }
        return false;
    }

    void reset()
    {
        has_frame_ = false;
        has_reported_ = false;
        observing_ = false;
    }

private:
    uint32_t silence_ms_;
    uint32_t report_period_ms_;
    uint32_t last_activity_ms_ = 0;
    bool has_frame_ = false;
    uint32_t last_report_ms_ = 0;
    bool has_reported_ = false;
    bool observing_ = false;
};

// Formats a frame as "sniff:<E|S><8hexid>:<payloadhex>" (design D6 / BLE
// protocol v2 table): E for extended (29-bit) frames, S for standard
// (11-bit), the full ID zero-padded to 8 hex digits, then the payload hex.
// Returns true on success; false when the buffer cannot hold the whole
// line including the terminating NUL.
bool sniff_format_frame(const CanFrame &frame, char *out, size_t cap);
