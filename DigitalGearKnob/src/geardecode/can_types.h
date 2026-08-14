#pragma once

#include <cmath>
#include <cstdint>

#include "geardecode/gear_types.h"

//=====================================================
// CAN FRAME + SIGNAL TYPES (pure — zero Arduino/HAL)
//=====================================================
// Stage-1 building blocks of the v2 CAN gear chain (design D1):
// pure types that compile unchanged in the native test env.

// A raw CAN frame as delivered by the TWAI RX task (spec: frame delivery).
struct CanFrame
{
    uint32_t id;     // full identifier (standard 11-bit or extended 29-bit)
    uint8_t data[8]; // payload; bytes 0..dlc-1 are valid
    uint8_t dlc;     // data length code, 0..8
    bool ext;        // true = extended (29-bit) frame
    uint64_t ts;     // host timestamp, ms
};

// Description of one signal inside a CAN frame.
// Config-driven (design D9): the extractor never hard-codes frame layouts.
struct SignalDesc
{
    uint32_t frame_id; // frame that carries this signal
    uint8_t start_bit; // little-endian start bit (0 = LSB of byte 0)
    uint8_t length;    // signal width in bits, 1..32
    float scale;       // physical = raw * scale + offset
    float offset;
    bool is_signed;    // true = sign-extend the raw value

    bool is_valid() const;
};

// Per-gear ratio table for the ratio estimator.
struct GearRatioConfig
{
    // ratio_rpm_per_kmh[g] = engine RPM / wheel speed (km/h) in gear g.
    // Indexed by GearPosition: GEAR_R..GEAR_5 are used; GEAR_N has no ratio.
    float ratio_rpm_per_kmh[GEAR_COUNT];
    float band_half_pct; // band half-width as % of the center ratio (8.0 = +/-8%)
    uint32_t stale_ms;   // input timeout before the estimator reports neutral
    float min_speed_kmh; // below this wheel speed the ratio is meaningless (standstill/clutch)

    bool is_valid() const;
};

// Vehicle-specific CAN decoding configuration.
// The single vehicle-specific data point (design D1).
struct VehicleCanConfig
{
    uint32_t bitrate = 500000;
    SignalDesc rpm_sig;
    SignalDesc speed_sig;
    GearRatioConfig ratios;
    bool signals_configured; // D9: false until sniff confirms the frame layout

    bool is_valid() const;
};

//=====================================================
// Validation
//=====================================================

inline bool SignalDesc::is_valid() const
{
    const uint32_t last_bit = static_cast<uint32_t>(start_bit) + length;
    return length >= 1 && length <= 32 &&
           last_bit <= 64 && // must fit inside the 8-byte payload
           std::isfinite(scale) && std::isfinite(offset);
}

inline bool GearRatioConfig::is_valid() const
{
    // A band must be positive and sane; 50% half-width already makes
    // adjacent bands indistinguishable.
    if (!(band_half_pct > 0.0f && band_half_pct <= 50.0f))
        return false;
    if (stale_ms == 0)
        return false;
    if (min_speed_kmh < 0.0f)
        return false;
    for (int g = GEAR_R; g < GEAR_N; ++g)
    {
        const float r = ratio_rpm_per_kmh[g];
        // !(r > 0) also rejects NaN.
        if (!(r > 0.0f) || !std::isfinite(r))
            return false;
    }
    return true;
}

inline bool VehicleCanConfig::is_valid() const
{
    if (bitrate == 0)
        return false;
    if (!ratios.is_valid())
        return false;
    if (signals_configured)
        return rpm_sig.is_valid() && speed_sig.is_valid();
    return true; // unconfigured signals are ignored: source stays offline (D9)
}

//=====================================================
// Default Golf 6 (2010, PQ35) starting values
//=====================================================

// Ratios: MQ250-5F published ratios x final drive x wheel revs/km / 60
// (205/55 R16, ~503.7 wheel revs/km).
// STARTING VALUES ONLY — verify in-car before relying on them (design
// open question). R and 1st are close on this gearbox: ratio-only reverse
// detection is ambiguous (design risk R5); Stage 2 direct decode supersedes.
inline GearRatioConfig make_golf6_ratio_config()
{
    GearRatioConfig cfg{};
    cfg.ratio_rpm_per_kmh[GEAR_R] = 102.4f;
    cfg.ratio_rpm_per_kmh[GEAR_1] = 107.5f;
    cfg.ratio_rpm_per_kmh[GEAR_2] = 58.7f;
    cfg.ratio_rpm_per_kmh[GEAR_3] = 38.3f;
    cfg.ratio_rpm_per_kmh[GEAR_4] = 27.5f;
    cfg.ratio_rpm_per_kmh[GEAR_5] = 22.0f;
    cfg.ratio_rpm_per_kmh[GEAR_N] = 0.0f; // unused
    cfg.band_half_pct = 8.0f;
    cfg.stale_ms = 500;
    cfg.min_speed_kmh = 5.0f;
    return cfg;
}

// 500 kbps drivetrain bus + candidate RPM/speed frames from Stage-1 sniff
// planning (0x540 RPM, 0x48A speed — UNVERIFIED layout).
// signals_configured=false keeps the gear source offline until the sniff
// stage confirms IDs/offsets (design D9 / risk R7).
inline VehicleCanConfig make_golf6_default_config()
{
    VehicleCanConfig cfg{};
    cfg.bitrate = 500000;
    cfg.ratios = make_golf6_ratio_config();
    cfg.rpm_sig = SignalDesc{0x540, 16, 16, 0.25f, 0.0f, false};
    cfg.speed_sig = SignalDesc{0x48A, 0, 16, 0.01f, 0.0f, false};
    cfg.signals_configured = false;
    return cfg;
}
