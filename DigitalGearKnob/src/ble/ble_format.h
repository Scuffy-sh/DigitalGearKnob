#pragma once

#include <cstdio>
#include <cstddef>
#include <cstdint>

// ------------------------------------------------------------------
// ble_format.h — pure formatting seams for the v2 BLE protocol.
//
// Header-only and free of any BLE/Arduino dependency so the exact
// v2 wire strings (design "Protocolo BLE v2" table) are provable in
// the native test env (test_decoder/test_ble_format.cpp).
// ------------------------------------------------------------------

// Formats the v2 debug notification line:
//   debug:<GEAR>,<RPM>,<SPEED>,<CANSTAT>
// GEAR is R/1..5/N ("??" when out of range), RPM/SPEED are rounded to
// whole numbers, CANSTAT is "ok" when the bus is online and "offline"
// otherwise.
//
// Writes to out (NUL-terminated). Returns true when the full line fit
// into cap bytes; false on null output or a too-small buffer.
inline bool ble_format_debug(char* out, size_t cap, int8_t gear,
                             float rpm, float speed, bool can_online)
{
    if (out == nullptr || cap == 0)
    {
        return false;
    }

    static const char* gearNames[] = {"R", "1", "2", "3", "4", "5", "N"};
    const char* gearName =
        (gear >= 0 && gear <= 6) ? gearNames[gear] : "??";

    int written = snprintf(out, cap, "debug:%s,%.0f,%.0f,%s",
                           gearName, rpm, speed,
                           can_online ? "ok" : "offline");
    return written >= 0 && static_cast<size_t>(written) < cap;
}
