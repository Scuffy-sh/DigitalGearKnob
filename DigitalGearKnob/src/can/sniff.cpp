#include "can/sniff.h"

#include <cstring>

namespace
{

std::atomic<bool> g_sniff_enabled{false};

char hex_digit(unsigned value)
{
    return value < 10 ? static_cast<char>('0' + value)
                      : static_cast<char>('A' + value - 10);
}

} // namespace

void sniff_set_enabled(bool enabled)
{
    g_sniff_enabled.store(enabled);
}

bool sniff_enabled()
{
    return g_sniff_enabled.load();
}

bool sniff_format_frame(const CanFrame &frame, char *out, size_t cap)
{
    if (out == nullptr || cap == 0)
    {
        return false;
    }

    // "sniff:" + type char + 8 id hex + ':' + dlc*2 payload hex + NUL
    const size_t payload_chars = static_cast<size_t>(frame.dlc) * 2;
    const size_t needed = 6 + 1 + 8 + 1 + payload_chars;
    if (cap < needed + 1)
    {
        return false;
    }

    char *p = out;
    std::memcpy(p, "sniff:", 6);
    p += 6;
    *p++ = frame.ext ? 'E' : 'S';
    for (int shift = 28; shift >= 0; shift -= 4)
    {
        *p++ = hex_digit((frame.id >> shift) & 0xF);
    }
    *p++ = ':';
    for (uint8_t i = 0; i < frame.dlc; ++i)
    {
        *p++ = hex_digit(frame.data[i] >> 4);
        *p++ = hex_digit(frame.data[i] & 0xF);
    }
    *p = '\0';
    return true;
}
