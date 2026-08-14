#pragma once

#include <stdint.h>

void ble_init();
void ble_update();

void ble_set_debug(bool enabled);
bool ble_is_debug();
void ble_send_debug(int8_t gear, float rpm, float speed, bool can_online);