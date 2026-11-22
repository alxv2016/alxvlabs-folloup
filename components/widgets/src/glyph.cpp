#include "widgets/glyph.h"

#include "internal/marks.h"

namespace widgets::detail {

const lv_image_dsc_t *GlyphImage(Glyph glyph)
{
    switch (glyph) {
    case Glyph::kBack:     return &widgets_mark_glyph_arrow_back;
    case Glyph::kClose:    return &widgets_mark_glyph_close;
    case Glyph::kPlay:     return &widgets_mark_glyph_play;
    case Glyph::kPause:    return &widgets_mark_glyph_pause;
    case Glyph::kStop:     return &widgets_mark_glyph_stop;
    case Glyph::kSoundOn:  return &widgets_mark_glyph_speaker_on;
    case Glyph::kSoundOff: return &widgets_mark_glyph_speaker_off;
    case Glyph::kCheck:    return &widgets_mark_glyph_check;
    case Glyph::kMic:      return &widgets_mark_glyph_mic;
    case Glyph::kRefresh:  return &widgets_mark_glyph_refresh;
    case Glyph::kEyeOpen:     return &widgets_mark_glyph_eye_on;
    case Glyph::kEyeSlashed:  return &widgets_mark_glyph_eye_off;
    case Glyph::kNone:     break;
    }
    return nullptr;
}

}  // namespace widgets::detail
