// The SSD2677, the reTerminal Sticky V2.0's controller (matte glass): full, partial and four-grey refreshes with the
// waveforms in its OTP, chosen by Seeed's temperature slots (STICKY_HARDWARE_SPECS §4.5).

#pragma once

#include <cstdint>
#include <memory>

#include "epaper.h"

namespace ssd2677 {

inline constexpr uint8_t kChipId = 0x07;  // what command 0x70 returns

// Takes the bus for its lifetime. The controller is reset and initialised on the first refresh.
std::unique_ptr<epaper::Panel> Create(epaper::Bus &bus);

}  // namespace ssd2677
