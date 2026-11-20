#pragma once

#include <cstdint>
#include <ctime>

#include "driver/i2c_master.h"

bool pcf8563_probe(i2c_master_dev_handle_t i2c_dev);
bool pcf8563_disable_clkout(i2c_master_dev_handle_t i2c_dev);
// False on bus error, VL, invalid BCD or invalid date; output is unchanged.
bool pcf8563_get_time(i2c_master_dev_handle_t i2c_dev, std::tm &timeinfo);
bool pcf8563_set_time(i2c_master_dev_handle_t i2c_dev, const std::tm &timeinfo);
