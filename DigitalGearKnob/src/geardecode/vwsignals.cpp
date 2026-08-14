#include "geardecode/vwsignals.h"

bool vw_extract_raw(
    const CanFrame &frame,
    const SignalDesc &desc,
    uint32_t &raw)
{
    if (frame.id != desc.frame_id)
    {
        return false;
    }
    if (desc.length < 1 || desc.length > 32)
    {
        return false;
    }

    const uint32_t last_bit = static_cast<uint32_t>(desc.start_bit) + desc.length;
    if (last_bit > 64)
    {
        return false;
    }
    const uint8_t needed_bytes = static_cast<uint8_t>((last_bit + 7) / 8);
    if (frame.dlc < needed_bytes)
    {
        return false;
    }

    uint32_t value = 0;
    for (uint32_t i = 0; i < desc.length; ++i)
    {
        const uint32_t bit = static_cast<uint32_t>(desc.start_bit) + i;
        const uint8_t byte = frame.data[bit / 8];
        if (byte & (1u << (bit % 8)))
        {
            value |= (1u << i);
        }
    }

    raw = value;
    return true;
}

bool vw_decode_signal(
    const CanFrame &frame,
    const SignalDesc &desc,
    float &value)
{
    uint32_t raw = 0;
    if (!vw_extract_raw(frame, desc, raw))
    {
        return false;
    }

    int32_t signed_raw = static_cast<int32_t>(raw);
    if (desc.is_signed && desc.length < 32)
    {
        const uint32_t sign_bit = 1u << (desc.length - 1);
        if (raw & sign_bit)
        {
            signed_raw = static_cast<int32_t>(raw | (~0u << desc.length));
        }
    }

    value = static_cast<float>(signed_raw) * desc.scale + desc.offset;
    return true;
}

bool vw_decode_rpm(
    const CanFrame &frame,
    const VehicleCanConfig &cfg,
    float &rpm)
{
    if (!cfg.signals_configured)
    {
        return false;
    }
    return vw_decode_signal(frame, cfg.rpm_sig, rpm);
}

bool vw_decode_speed(
    const CanFrame &frame,
    const VehicleCanConfig &cfg,
    float &speed)
{
    if (!cfg.signals_configured)
    {
        return false;
    }
    return vw_decode_signal(frame, cfg.speed_sig, speed);
}
