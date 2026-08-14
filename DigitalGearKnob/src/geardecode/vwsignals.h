#pragma once

#include <cstdint>

#include "geardecode/can_types.h"

//=====================================================
// VW SIGNAL EXTRACTOR (pure — design D9)
//=====================================================
// Little-endian (Intel) bit extractor for VW drivetrain frames.
// Config-driven: the frame layout comes from SignalDesc, never from
// hard-coded offsets.

// Unpacks the raw bits of `desc` from `frame` using LE bit numbering
// (start_bit 0 = LSB of byte 0; bits continue into the next byte).
// Returns false when the frame ID does not match or the DLC does not
// cover the whole signal.
bool vw_extract_raw(
    const CanFrame &frame,
    const SignalDesc &desc,
    uint32_t &raw);

// Unpacks and converts to the physical value:
//   (raw, sign-extended when desc.is_signed) * scale + offset.
// Returns false on ID mismatch or short DLC.
bool vw_decode_signal(
    const CanFrame &frame,
    const SignalDesc &desc,
    float &value);

// Decodes the configured RPM channel from `frame`.
// Returns false when signals are unconfigured or the frame is not the
// RPM frame (D9: gear source stays offline until configured).
bool vw_decode_rpm(
    const CanFrame &frame,
    const VehicleCanConfig &cfg,
    float &rpm);

// Decodes the configured wheel-speed channel from `frame`.
// Returns false when signals are unconfigured or the frame is not the
// speed frame.
bool vw_decode_speed(
    const CanFrame &frame,
    const VehicleCanConfig &cfg,
    float &speed);
