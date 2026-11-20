// The SSD1677, the reTerminal Sticky V1.0's controller (glossy glass): full, partial and four-grey refreshes with the
// controller's own waveforms, in Seeed's command sequences (STICKY_HARDWARE_SPECS §4.5).

#pragma once

#include <memory>

#include "epaper.h"

namespace ssd1677 {

// Takes the bus for its lifetime. The controller is reset on the first refresh.
std::unique_ptr<epaper::Panel> Create(epaper::Bus &bus);

}  // namespace ssd1677
