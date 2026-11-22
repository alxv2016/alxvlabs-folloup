// The 1-bit marks make_marks.py compiles from the agent deck's and BYTE-90's art (STICKY_DESIGN_TOKENS §9; DESIGN → Components →
// The kit, Icons). Private to widgets.

#pragma once

#include "lvgl.h"
#include "widgets/glyph.h"

extern "C" {
extern const lv_image_dsc_t widgets_mark_check;            // a checkbox card's check, 26 x 18
extern const lv_image_dsc_t widgets_mark_status_check;     // the status block's marks, 57 x 41, 73 x 73, 73 x 68
extern const lv_image_dsc_t widgets_mark_status_clock;
extern const lv_image_dsc_t widgets_mark_status_link_off;
// The glyphs, at the art box `spacing.icon` (40 px); the ink size is each one's own.
extern const lv_image_dsc_t widgets_mark_glyph_arrow_back;  // 27 x 26
extern const lv_image_dsc_t widgets_mark_glyph_close;       // 22 x 22
extern const lv_image_dsc_t widgets_mark_glyph_play;        // 18 x 22
extern const lv_image_dsc_t widgets_mark_glyph_pause;       // 24 x 26
extern const lv_image_dsc_t widgets_mark_glyph_stop;        // 22 x 22
extern const lv_image_dsc_t widgets_mark_glyph_speaker_on;  // 32 x 28
extern const lv_image_dsc_t widgets_mark_glyph_speaker_off; // 33 x 34
extern const lv_image_dsc_t widgets_mark_glyph_check;       // 26 x 20: the checkbox's check, and Save's
extern const lv_image_dsc_t widgets_mark_glyph_mic;         // 24 x 33
extern const lv_image_dsc_t widgets_mark_glyph_refresh;     // 28 x 28
// The Wi-Fi network row's and the dropdown's glyphs, each cut to a box of its own, not trimmed.
extern const lv_image_dsc_t widgets_mark_glyph_signal_weak;    // 40 x 30, the three signal glyphs share one canvas
extern const lv_image_dsc_t widgets_mark_glyph_signal_fair;    // 40 x 30
extern const lv_image_dsc_t widgets_mark_glyph_signal_strong;  // 40 x 30, also the unlit bars under the lit ones
extern const lv_image_dsc_t widgets_mark_glyph_lock;           // 19 x 24
extern const lv_image_dsc_t widgets_mark_glyph_fact_check;     // 28 x 20
extern const lv_image_dsc_t widgets_mark_glyph_chevron;        // 14 x 28
// A password field's eyes, one shared crop so the pair registers: open (show) and slashed (hide).
extern const lv_image_dsc_t widgets_mark_glyph_eye_on;         // 30 x 26
extern const lv_image_dsc_t widgets_mark_glyph_eye_off;        // 30 x 26
}

namespace widgets::detail {
// The image a glyph is, or nullptr for Glyph::kNone.
const lv_image_dsc_t *GlyphImage(Glyph glyph);
}  // namespace widgets::detail
