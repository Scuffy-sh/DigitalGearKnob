#pragma once

// Native (host) shim for the ESP32 <Preferences.h> NVS header.
// Routes to the no-op mock classes in arduino_mock.h so the calibration
// module compiles and runs in the native test env without an ESP32 core.

#include "arduino_mock.h"
