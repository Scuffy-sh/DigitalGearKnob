#include "gearsource/gearsource.h"

#include <Arduino.h>

//=========================================================================
// GEAR SOURCE — Arduino-side glue (design gear-source glue, D7/D8/D9)
//=========================================================================
// Firmware-only: this file is NOT in the native build_src_filter (it uses
// millis() and the CAN HAL). The native-testable decision logic lives in
// the inline gear_source_decide() in gearsource.h.

namespace
{
VehicleCanConfig g_vehicle_config; // Golf 6 default (design D1, D9)
RatioEstimator *g_estimator = nullptr;
} // namespace

void gear_source_init(void)
{
    // D9: the default layout ships with signals_configured = false, so the
    // snapshot stays invalid until the sniff stage confirms IDs/offsets and
    // calls can_set_vehicle_config() with the real layout. Until then the
    // source honestly reports offline ("--" on the knob).
    g_vehicle_config = make_golf6_default_config();
    can_set_vehicle_config(g_vehicle_config);

    // Function-local static: constructed on first call, after the config
    // above is ready. Static lifetime — the source lives for the whole run.
    static RatioEstimator estimator(g_vehicle_config.ratios);
    g_estimator = &estimator;
}

int8_t gear_source_poll(void)
{
    if (g_estimator == nullptr)
    {
        return -1;
    }

    CanSnapshot snap;
    can_get_snapshot(snap);

    return gear_source_decide(can_is_online(), snap, *g_estimator, millis());
}
