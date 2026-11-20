// The board as firmware sees it: every pin and size the code uses, from the hardware spec (STICKY_HARDWARE_SPECS §3).
// A value changes in the spec first, then here — never only here. Code takes a pin from this file, never as a number;
// sticky_check.py pins proves the two agree (the sticky-hardware skill).

#pragma once

#include <cstdint>

#include "driver/gpio.h"

namespace board_config {

// --- Pins, one per signal in STICKY_HARDWARE_SPECS §3, named after it -------------------------------------------

// The e-paper panel (STICKY_HARDWARE_SPECS §3.1). SCK and SDI are the shared SPI bus's.
inline constexpr gpio_num_t kEpSck = GPIO_NUM_13;
inline constexpr gpio_num_t kEpSdi = GPIO_NUM_14;
inline constexpr gpio_num_t kEpCs = GPIO_NUM_15;
inline constexpr gpio_num_t kEpDc = GPIO_NUM_16;
inline constexpr gpio_num_t kEpRst = GPIO_NUM_17;
inline constexpr gpio_num_t kEpBusy = GPIO_NUM_18;
inline constexpr gpio_num_t kEpPwrEn = GPIO_NUM_47;

// The touch panel (STICKY_HARDWARE_SPECS §3.2).
inline constexpr gpio_num_t kTpI2cScl = GPIO_NUM_2;
inline constexpr gpio_num_t kTpI2cSda = GPIO_NUM_3;
inline constexpr gpio_num_t kTpInt = GPIO_NUM_21;
inline constexpr gpio_num_t kTpRst = GPIO_NUM_41;
inline constexpr gpio_num_t kTpPwrEn = GPIO_NUM_42;

// MicroSD (STICKY_HARDWARE_SPECS §3.3). CLK and CMD share the panel's SCK and SDI.
inline constexpr gpio_num_t kSdClk = GPIO_NUM_13;
inline constexpr gpio_num_t kSdCmd = GPIO_NUM_14;
inline constexpr gpio_num_t kSdD0 = GPIO_NUM_12;
inline constexpr gpio_num_t kSdD3 = GPIO_NUM_8;
inline constexpr gpio_num_t kSdPwrEn = GPIO_NUM_10;
inline constexpr gpio_num_t kSdDetect = GPIO_NUM_11;

// Sensors, buttons, audio (STICKY_HARDWARE_SPECS §3.4). BFG_INT and IMU_INT are one pin.
inline constexpr gpio_num_t kMiscI2cScl = GPIO_NUM_0;
inline constexpr gpio_num_t kMiscI2cSda = GPIO_NUM_1;
inline constexpr gpio_num_t kBfgInt = GPIO_NUM_7;
inline constexpr gpio_num_t kImuInt = GPIO_NUM_7;
inline constexpr gpio_num_t kButtonUp = GPIO_NUM_5;
inline constexpr gpio_num_t kButtonDown = GPIO_NUM_6;
inline constexpr gpio_num_t kBuzzerPwm = GPIO_NUM_48;
inline constexpr gpio_num_t kPdmClk = GPIO_NUM_19;
inline constexpr gpio_num_t kPdmData = GPIO_NUM_20;
inline constexpr gpio_num_t kPdmEn = GPIO_NUM_38;

// Power and console (STICKY_HARDWARE_SPECS §3.5).
inline constexpr gpio_num_t kPwrButton = GPIO_NUM_4;
inline constexpr gpio_num_t kPwrHold = GPIO_NUM_45;
inline constexpr gpio_num_t kPwrLock = GPIO_NUM_46;
inline constexpr gpio_num_t kEnBatChg = GPIO_NUM_39;
inline constexpr gpio_num_t kChargeState = GPIO_NUM_40;
inline constexpr gpio_num_t kPwrInVolt = GPIO_NUM_9;
inline constexpr gpio_num_t kU0txd = GPIO_NUM_43;
inline constexpr gpio_num_t kU0rxd = GPIO_NUM_44;

// --- Buses and addresses (STICKY_HARDWARE_SPECS §4.1, §4.2) ---------------------------------------------------

inline constexpr int kSharedSpiHost = 1;  // SPI2_HOST — the panel and the card; initialised once, never freed
inline constexpr int kSharedSpiMaxTransfer = 48000;  // Seeed's largest transfer, in bytes
// The panel's SPI clock, by direction (STICKY_HARDWARE_SPECS §4.5): the SSD2677 reads at 2.5 MHz at most and writes at
// 20 MHz; the SSD1677 writes at 20 MHz. The ID probe reads; every refresh after it only writes.
inline constexpr int kPanelReadHz = 2 * 1000 * 1000;
inline constexpr int kPanelWriteHz = 20 * 1000 * 1000;
inline constexpr int kSensorI2cPort = 1;  // I2C_NUM_1
inline constexpr int kTouchI2cPort = 0;  // I2C_NUM_0
inline constexpr uint32_t kI2cHz = 400000;  // Seeed runs both buses at 400 kHz

inline constexpr uint8_t kAddrBq27220 = 0x55;
inline constexpr uint8_t kAddrPcf8563 = 0x51;
inline constexpr uint8_t kAddrLsm6ds3 = 0x6A;
inline constexpr uint8_t kAddrSht40 = 0x44;

// --- The glass (STICKY_HARDWARE_SPECS §2; DESIGN → Layout → The one glass) --------------------------------------

inline constexpr int kPanelWidth = 800;   // native, landscape, as the controller stores it
inline constexpr int kPanelHeight = 480;
inline constexpr int kScreenWidth = 480;  // logical portrait, as FolloUp draws it
inline constexpr int kScreenHeight = 800;

// --- Timings (STICKY_HARDWARE_SPECS §7) -------------------------------------------------------------------------

inline constexpr int kPanelPowerSettleMs = 100;
inline constexpr int kTouchPowerSettleMs = 250;
inline constexpr int kMicPowerSettleMs = 20;  // Seeed's

// The buzzer (STICKY_HARDWARE_SPECS §4.2): LEDC, 10-bit, 4 kHz, half duty for the loudest tone.
inline constexpr uint32_t kBuzzerHz = 4000;
inline constexpr uint32_t kBuzzerDuty = 512;

}  // namespace board_config
