// A Wi-Fi network row: a network's name over its facts, and its signal at the right end. The connected network and the one
// being joined are the chosen inversion; a press on a row is the request to join it (DESIGN → Components → The kit).

#pragma once

#include <cstdint>

#include "lvgl.h"

namespace widgets {

// A network in range is at rest; the one in use is connected (a check before "Connected"); the one being joined shows
// "Connecting…" and no check. A failed join is no row state: the row goes back to rest and a dialog says why.
enum class NetworkState : uint8_t { kRest, kConnected, kConnecting };

// A row `spacing.list-row-height` high, full width, `rounded.lg`, in the focus group with its ring outside it: the name in
// `caption`, up to 28 characters and then three full stops; under it the facts line in `caption` — an open network says
// "Open", a secured one carries the lock before "Locked", the connected one a check before "Connected" and then its lock
// or "Open", the joining one "Connecting…" — and, at the right end, the signal: its glyph alone, centred on the row's height, with no
// word. `signal` is 0 to 3 bars lit; the unlit bars stay in the light dither (the dark dither on an inverted row),
// the lit ones black (white on an inverted row). The caller owns what a click does (LV_EVENT_CLICKED), and puts the row in a
// list view's root when there are more than a page of them.
lv_obj_t *MakeNetworkRow(lv_obj_t *parent, const char *name, bool locked, int signal, NetworkState state);
// A network's facts changed: its lock, its signal or its state. The row keeps its place and its focus.
void SetNetworkRow(lv_obj_t *row, bool locked, int signal, NetworkState state);

}  // namespace widgets
