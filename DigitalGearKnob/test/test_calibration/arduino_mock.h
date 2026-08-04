#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

class String
{
public:
    String() = default;
    String(const char *s) : _s(s ? s : "") {}
    String(const std::string &s) : _s(s) {}

    bool operator==(const char *rhs) const { return _s == rhs; }
    bool operator==(const String &rhs) const { return _s == rhs._s; }
    bool operator!=(const char *rhs) const { return _s != rhs; }

    const char *c_str() const { return _s.c_str(); }

private:
    std::string _s;
};

class Preferences
{
public:
    void begin(const char *, bool) {}
    size_t getBytes(const char *, void *, size_t) { return 0; }
    size_t putBytes(const char *, const void *, size_t) { return 0; }
};
