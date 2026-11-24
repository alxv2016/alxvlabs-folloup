// The screen audit: every hardware-test screen, home, the status line, the UI kit page (a view per glass-full as it scrolls) and the kit's
// overlays, built by the firmware's own code (components/hw_test/hw_test.cpp, included below) on LVGL's real renderer with the
// real Bitcount fonts and widgets, 480 x 800, each pixel read as the glass reads it (`OneBitWhite`). It walks each view's object
// tree and measures what a reviewer otherwise judges by eye — the ui-designer judge's measured proxies (plan: a proxy outside
// its range caps its quality dimension):
//   hierarchy    distinct type sizes on a view (at most 3, TYPOGRAPHY §17.1); each text pair's size ratio (1.0–1.5: the
//                1.0–1.17 of TYPOGRAPHY §17.1 is out of reach on a pixel font, whose smallest step here is 30/20, TYPOGRAPHY
//                §17.3) and weight order (the secondary never heavier, and at most 400 lighter: body 400 under label 700), and
//                that the pair differs by size or weight at all (both TYPOGRAPHY §17.1)
//   spacing      a pair's gap between the two texts' own font boxes (the font's height, no leading) in the range its larger
//                text's size allows (LAYOUT §7.1); the pair's ink gap never more than the ink gap round its group (internal
//                ≤ external, LAYOUT §7.1), ink being the first and last inked rows of a text, or of the words a group holds
//                (of its drawn pixels when it holds none); a lone label between two blocks sits nearer one of them (the
//                nearer gap at most 0.8 of the other — 25 % apart, LAYOUT §7.3); padding, margins and gaps off DESIGN.md's
//                spacing tokens (one-offs, LAYOUT §7.5)
//   balance      distinct left edges of start-aligned words on a view (at most 3, a judgement: the margin, one inset, one nested);
//                words cut off by the box that holds them (none)
//   consistency  across views: a type size and weight that is no DESIGN.md role; the same pair of roles spaced the same on every
//                view, by font box (within spacing.xs); Back in one place on every screen
//   state        every focusable shows a ring when focused; no ring cut off — its outline box, focused and open for editing,
//                inside the glass and what each ancestor lets its children draw (its box, or with LV_OBJ_FLAG_OVERFLOW_VISIBLE
//                its box grown by its ext_draw_size), per side, in px; one ring drawn at once (BYTE-90 ui_probe's RingClip and
//                RingCount, STICKY_DESIGN_TOKENS §9); every disabled control says why in words — inside it, just under
//                it, or in the words it names as its reason (widgets::LinkDisabledReason) (DESIGN → Components)
//   medium       the share of the glass a ring move changes, at most kScanMaxChanged's 15 % (STICKY_DESIGN_TOKENS §5)
// It writes .cache/harness/proxies/ui-designer.json, a PNG of each view to build/host/audit/ and of each ringed stop to
// build/host/audit/rings/ (screen__stop_name.png, for cmp after a refactor). For the ux-designer judge it walks the routes
// the controls push from Home, and writes the screens opened and the presses to reach each (at most 3 deep) to
// .cache/harness/proxies/ux-designer.json. Every metric names the screens it measured ("screens"), so the wrapper,
// .agents/skills/ui-designer/scripts/screen_audit.py, keeps for a review only the screens the change touched; the program
// itself always measures the whole product ("scope": "product"). Before the screens it measures a fixture — two cards, a
// label over a caption at two known font-box gaps, a ring a box cuts 8 px at the bottom, and a disabled button whose
// reason stands 500 px above it, linked and not — and stops if a font box, an ink gap, a ring or a reason reads wrong.
// Otherwise it reports and never fails: it exits non-zero only when it cannot run or the fixture misreads. The wrapper
// builds it alone.
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp components/app_hooks/app_hooks.cpp components/app_shell/screen_home.cpp
// Build and run: bash tests/host/run.sh screen_audit

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <set>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

#include "board.h"
#include "esp_err.h"
#include "ui.h"
#include "src/core/lv_obj_draw_private.h"  // lv_obj_get_ext_draw_size: how far a container lets its children draw

// The board calls the hardware test makes, as the real board.h declares them; the stand-in board.h has only the display type.
namespace board {
esp_err_t SetChargingEnabled(bool enabled);
bool ChargingEnabled();
bool ChargeStatusLow();
int PowerInputMillivolts();
bool UsbPresent();
esp_err_t EnterDeepSleep(bool keep_panel_powered);
esp_err_t PowerOff();
}  // namespace board

// The firmware's hardware test, as it is: its screens, its routes and home's rows.
#include "../../components/hw_test/hw_test.cpp"
#include "../../components/app_shell/screen_home.h"

namespace dt = design_tokens;

// --- The board, its devices and lvgl_display, as a V2.0 on USB with every part answering ------------------------------------

namespace board {
namespace {
bool s_charging = true;
}
esp_err_t SetChargingEnabled(bool enabled) { s_charging = enabled; return ESP_OK; }
bool ChargingEnabled() { return s_charging; }
bool ChargeStatusLow() { return true; }
int PowerInputMillivolts() { return 2506; }
bool UsbPresent() { return true; }
esp_err_t EnterDeepSleep(bool) { return ESP_OK; }
esp_err_t PowerOff() { return ESP_OK; }
}  // namespace board

namespace devices {
Present Found() { Present p; p.sht40 = p.imu = p.gauge = p.rtc = p.buzzer = true; return p; }
esp_err_t ReadAmbient(Ambient *out) { out->temperature_c = 21.4f; out->humidity_pct = 46; return ESP_OK; }
esp_err_t ReadMotion(Motion *out)
{
    out->accel_g[0] = 0.01f; out->accel_g[1] = -0.02f; out->accel_g[2] = 1.00f;
    out->gyro_dps[0] = 0.4f; out->gyro_dps[1] = -0.3f; out->gyro_dps[2] = 0.1f;
    return ESP_OK;
}
esp_err_t ReadBattery(Battery *out)
{
    out->voltage_mv = 4012; out->current_ma = 182; out->percent = 78; out->health_pct = 97;
    out->remaining_mah = 1560; out->full_mah = 2000; out->temperature_c = 24.5f; out->cycles = 12;
    return ESP_OK;
}
esp_err_t ReadClock(std::tm *out)
{
    *out = std::tm{};
    out->tm_year = 126; out->tm_mon = 10; out->tm_mday = 23; out->tm_hour = 9; out->tm_min = 41;
    return ESP_OK;
}
bool RtcStartedBoard() { return false; }
esp_err_t ClearRtcWake() { return ESP_OK; }
esp_err_t SetClock(const std::tm &) { return ESP_OK; }
esp_err_t SetRtcWake(uint8_t) { return ESP_OK; }
void Play(sound::Word) {}
void Play(const sound::Sound &) {}
void Silence() {}
void Tick() {}
void Tone(uint32_t) {}
esp_err_t ListenLevel(uint32_t, float *rms, int *peak) { *rms = 0; *peak = 0; return ESP_OK; }
esp_err_t StartListening(uint32_t) { return ESP_OK; }
esp_err_t ReadLevel(uint32_t, float *rms, int *peak) { *rms = 0; *peak = 0; return ESP_OK; }
void StopListening() {}
esp_err_t ProbeCard(Card *out) { *out = Card{}; return ESP_OK; }
}  // namespace devices

namespace {
lv_group_t *s_group = nullptr;
lv_indev_t *s_keys = nullptr;
}  // namespace

namespace lvgl_display {
lv_group_t *Group() { return s_group; }
void ClearGroup()
{
    lv_group_set_editing(s_group, false);
    lv_group_remove_all_objs(s_group);
}
void SetGroup(lv_group_t *group)
{
    if (group == nullptr) {
        return;
    }
    s_group = group;
    lv_indev_set_group(s_keys, group);
    lv_group_set_default(group);
}
bool Lock(uint32_t) { return true; }
void Unlock() {}
void RequestFull() {}
void RequestGray() {}
void Redraw() {}
void SetPartialCounting(PartialCounting) {}
void NoteTyping() {}
void SetBackHandler(void (*)()) {}
void SetKeyObserver(void (*)(uint32_t)) {}
void SetKeyReadHook(void (*)(lv_indev_t *, lv_indev_data_t *)) {}
void SetTouchReadHook(void (*)(lv_indev_t *, lv_indev_data_t *)) {}
const char *const kTunings[] = {"Waveform rate", "75 Hz", "85 Hz"};
int TuningCount() { return 3; }
const char *TuningName(int preset) { return preset >= 0 && preset < 3 ? kTunings[preset] : ""; }
int Tuning() { return 2; }
int DefaultTuning() { return 2; }
esp_err_t SetTuning(int) { return ESP_OK; }
esp_err_t SleepPanel() { return ESP_OK; }
Stats GetStats()
{
    Stats s;
    s.ssd2677 = true;
    s.last = Refresh::kFull;
    s.last_ms = 1003; s.latch_ms = 41; s.stream_ms = 12; s.power_on_ms = 80; s.drive_ms = 870;
    s.temperature_c = 22;
    return s;
}
const uint8_t *GlassFrame() { return nullptr; }
void PressButton(Button, bool) {}
TouchPoint LastTouch() { return TouchPoint{}; }
bool TouchReady() { return true; }
bool Turned180() { return false; }
void PlayCue(Cue) {}
}  // namespace lvgl_display

namespace {

// --- The frame: LVGL draws into one L8 buffer the size of the glass, and each drawn area is read as the glass reads it ----------

constexpr int kW = 480, kH = 800;
uint8_t s_l8[kW * kH];
std::vector<uint8_t> s_glass(kW * kH, 1);  // 1 white, 0 black

void Flush(lv_display_t *display, const lv_area_t *a, uint8_t *)
{
    for (int y = std::max<int>(0, a->y1); y <= std::min<int>(kH - 1, a->y2); ++y) {
        for (int x = std::max<int>(0, a->x1); x <= std::min<int>(kW - 1, a->x2); ++x) {
            // The glass's own pixel for the dither: its column is 799 - y, its row x (display.cpp's ToNative).
            s_glass[y * kW + x] = lvgl_display::OneBitWhite(s_l8[y * kW + x], kH - 1 - y, x) ? 1 : 0;
        }
    }
    lv_display_flush_ready(display);
}

uint32_t s_ms = 0;
uint32_t Ticks() { return s_ms; }

void Settle()
{
    for (int i = 0; i < 6; ++i) {
        s_ms += 20;
        lv_timer_handler();
    }
    lv_obj_update_layout(lv_screen_active());
}

// Draws the active screen and the top layer in full, and returns the glass.
std::vector<uint8_t> Render()
{
    lv_obj_invalidate(lv_screen_active());
    lv_obj_invalidate(lv_layer_top());
    lv_refr_now(nullptr);
    return s_glass;
}

// Draws what changed since the last draw, and returns the glass.
std::vector<uint8_t> RenderChanged()
{
    lv_refr_now(nullptr);
    return s_glass;
}

// --- PNG: one bit a pixel, deflate's stored blocks — no library ----------------------------------------------------------

uint32_t Crc(const uint8_t *p, size_t n, uint32_t crc = 0xFFFFFFFFu)
{
    for (size_t i = 0; i < n; ++i) {
        crc ^= p[i];
        for (int b = 0; b < 8; ++b) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return crc;
}

void Be32(std::vector<uint8_t> &v, uint32_t x)
{
    v.push_back(static_cast<uint8_t>(x >> 24)); v.push_back(static_cast<uint8_t>(x >> 16));
    v.push_back(static_cast<uint8_t>(x >> 8)); v.push_back(static_cast<uint8_t>(x));
}

void Chunk(std::vector<uint8_t> &out, const char *type, const std::vector<uint8_t> &data)
{
    Be32(out, static_cast<uint32_t>(data.size()));
    std::vector<uint8_t> body(type, type + 4);
    body.insert(body.end(), data.begin(), data.end());
    out.insert(out.end(), body.begin(), body.end());
    Be32(out, ~Crc(body.data(), body.size()));
}

void SavePng(const std::string &path, const std::vector<uint8_t> &glass)
{
    std::vector<uint8_t> raw;
    for (int y = 0; y < kH; ++y) {
        raw.push_back(0);  // no filter
        for (int x = 0; x < kW; x += 8) {
            uint8_t byte = 0;
            for (int b = 0; b < 8; ++b) {
                byte = static_cast<uint8_t>(byte | (glass[y * kW + x + b] << (7 - b)));
            }
            raw.push_back(byte);
        }
    }
    std::vector<uint8_t> z = {0x78, 0x01};
    uint32_t a = 1, b = 0;
    for (size_t at = 0; at < raw.size(); at += 65535) {
        const size_t n = std::min<size_t>(65535, raw.size() - at);
        z.push_back(at + n == raw.size() ? 1 : 0);
        z.push_back(static_cast<uint8_t>(n)); z.push_back(static_cast<uint8_t>(n >> 8));
        z.push_back(static_cast<uint8_t>(~n)); z.push_back(static_cast<uint8_t>(~n >> 8));
        z.insert(z.end(), raw.begin() + static_cast<long>(at), raw.begin() + static_cast<long>(at + n));
    }
    for (uint8_t c : raw) {
        a = (a + c) % 65521;
        b = (b + a) % 65521;
    }
    Be32(z, (b << 16) | a);
    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::vector<uint8_t> ihdr;
    Be32(ihdr, kW); Be32(ihdr, kH);
    ihdr.insert(ihdr.end(), {1, 0, 0, 0, 0});  // one bit, greyscale
    Chunk(png, "IHDR", ihdr);
    Chunk(png, "IDAT", z);
    Chunk(png, "IEND", {});
    if (FILE *f = std::fopen(path.c_str(), "wb")) {
        std::fwrite(png.data(), 1, png.size(), f);
        std::fclose(f);
    }
}

// --- Boxes and what the glass shows of an object ------------------------------------------------------------------------

struct Box {
    int x1 = 0, y1 = 0, x2 = -1, y2 = -1;
    bool Empty() const { return x2 < x1 || y2 < y1; }
    int W() const { return x2 - x1 + 1; }
};

Box Coords(lv_obj_t *o)
{
    lv_area_t a;
    lv_obj_get_coords(o, &a);
    return {a.x1, a.y1, a.x2, a.y2};
}

Box Meet(Box a, Box b)
{
    return {std::max(a.x1, b.x1), std::max(a.y1, b.y1), std::min(a.x2, b.x2), std::min(a.y2, b.y2)};
}

Box Join(Box a, Box b)
{
    if (a.Empty()) return b;
    if (b.Empty()) return a;
    return {std::min(a.x1, b.x1), std::min(a.y1, b.y1), std::max(a.x2, b.x2), std::max(a.y2, b.y2)};
}

bool OverlapX(Box a, Box b) { return std::min(a.x2, b.x2) >= std::max(a.x1, b.x1); }

bool Hidden(lv_obj_t *o)
{
    for (; o != nullptr; o = lv_obj_get_parent(o)) {
        if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) {
            return true;
        }
    }
    return false;
}

// The part of an object on the glass: its box, cut by every ancestor that clips its children.
Box OnGlass(lv_obj_t *o)
{
    Box b = Meet(Coords(o), {0, 0, kW - 1, kH - 1});
    for (lv_obj_t *p = lv_obj_get_parent(o); p != nullptr && !b.Empty(); p = lv_obj_get_parent(p)) {
        if (!lv_obj_has_flag(p, LV_OBJ_FLAG_OVERFLOW_VISIBLE)) {
            b = Meet(b, Coords(p));
        }
    }
    return b;
}

// The box round the pixels inside `area` where `a` and `b` differ (b empty: round a's black pixels).
Box InkBox(const std::vector<uint8_t> &a, const std::vector<uint8_t> *b, Box area)
{
    Box k{9999, 9999, -1, -1};
    for (int y = area.y1; y <= area.y2; ++y) {
        for (int x = area.x1; x <= area.x2; ++x) {
            const int i = y * kW + x;
            if (b != nullptr ? a[i] != (*b)[i] : a[i] == 0) {
                k.x1 = std::min(k.x1, x); k.y1 = std::min(k.y1, y);
                k.x2 = std::max(k.x2, x); k.y2 = std::max(k.y2, y);
            }
        }
    }
    return k.x2 < 0 ? Box{} : k;
}

int Changed(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b)
{
    int n = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        n += a[i] != b[i];
    }
    return n;
}

// --- Type: which size and weight a label's font is, and its DESIGN.md role -----------------------------------------------

struct Face {
    int size = 0, weight = 0;
};
std::map<const lv_font_t *, Face> s_faces;

void MapFaces()
{
    for (int size : {20, 30, 40, 50, 60, 70, 80, 100, 120, 160}) {
        for (int weight : {400, 600, 700}) {
            if (const lv_font_t *f = ui::Bitcount(static_cast<int16_t>(size), static_cast<int16_t>(weight))) {
                s_faces[f] = {size, weight};
            }
        }
    }
}

std::string RoleOf(Face f)
{
    struct R { int size, weight; const char *name; };
    // The roles as the token header sets them, so a role moved to another face follows.
    static const R kRoles[] = {{dt::kTypographyHero.size_px, dt::kTypographyHero.weight, "hero"},
                               {dt::kTypographyDisplay.size_px, dt::kTypographyDisplay.weight, "display"},
                               {dt::kTypographyHeadline.size_px, dt::kTypographyHeadline.weight, "headline"},
                               {dt::kTypographyBody.size_px, dt::kTypographyBody.weight, "body"},
                               {dt::kTypographyLabel.size_px, dt::kTypographyLabel.weight, "label"},
                               {dt::kTypographyCaption.size_px, dt::kTypographyCaption.weight, "caption"}};
    for (const R &r : kRoles) {
        if (r.size == f.size && r.weight == f.weight) {
            return r.name;
        }
    }
    return "scale-" + std::to_string(f.size) + "-" + std::to_string(f.weight);
}

bool IsRole(Face f) { return RoleOf(f).rfind("scale-", 0) != 0; }

// --- A view's objects, as measured ------------------------------------------------------------------------------------------

struct Node {
    lv_obj_t *obj = nullptr;
    bool text = false;   // a label in a Bitcount face, with words
    Face face;
    std::string words;
    Box glass;           // on the glass
    Box ink;             // a text's ink; anything else, its black pixels
    Box font;            // a text's font box: its lines at the font's own height, no leading
    Box held;            // the ink of every text in it, itself included
};

struct Metric {
    std::string dimension, name;
    double value = 0, lo = 0, hi = 0;
    bool ok = true;
    std::string where;
    std::set<std::string> screens;  // the screens it measured: a review keeps only those its change touched
};

std::vector<Metric> s_metrics;
std::string s_screen;  // the screen being measured: a view's name without its page, " p2"

// A view's screen: "kit p3" is the kit's.
std::string ScreenOf(const std::string &view)
{
    const size_t at = view.rfind(" p");
    const bool page = at != std::string::npos && at + 2 < view.size() &&
                      std::all_of(view.begin() + static_cast<long>(at + 2), view.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
    return page ? view.substr(0, at) : view;
}

std::string Short(const std::string &s)
{
    std::string t = s.substr(0, s.find('\n'));
    return t.size() > 28 ? t.substr(0, 26) + "…" : t;
}

void Add(const std::string &dimension, const std::string &name, double value, double lo, double hi, const std::string &where,
         const std::set<std::string> &screens = {})
{
    Metric m;
    m.dimension = dimension; m.name = name; m.value = value; m.lo = lo; m.hi = hi;
    m.ok = value >= lo - 1e-9 && value <= hi + 1e-9;
    m.where = where;
    m.screens = screens.empty() && !s_screen.empty() ? std::set<std::string>{s_screen} : screens;
    s_metrics.push_back(m);
}

struct View {
    std::string name;
    bool specimen = false;  // the type scale: every size, by design (DESIGN → Exceptions)
    std::vector<uint8_t> frame, bare;  // the glass, and the glass with every text's ink taken away
    std::map<lv_obj_t *, Node> nodes;
    std::vector<lv_obj_t *> order;     // depth first
};

bool IsText(lv_obj_t *o, Face *face)
{
    if (!lv_obj_check_type(o, &lv_label_class)) {
        return false;
    }
    const auto it = s_faces.find(lv_obj_get_style_text_font(o, LV_PART_MAIN));
    const char *t = lv_label_get_text(o);
    if (it == s_faces.end() || t == nullptr) {
        return false;
    }
    bool words = false;
    for (const char *c = t; *c != '\0'; ++c) {
        words = words || (static_cast<unsigned char>(*c) > ' ');
    }
    *face = it->second;
    return words;
}

void Collect(lv_obj_t *o, std::vector<lv_obj_t *> &out)
{
    out.push_back(o);
    for (uint32_t i = 0; i < lv_obj_get_child_count(o); ++i) {
        Collect(lv_obj_get_child(o, static_cast<int32_t>(i)), out);
    }
}

std::vector<lv_obj_t *> AllObjects()
{
    std::vector<lv_obj_t *> all;
    Collect(lv_screen_active(), all);
    Collect(lv_layer_top(), all);
    return all;
}

// A label's font box: its content's top to the last line's foot, each line the font's own height — the label's padding (the
// leading ui::FitLineBox adds) left out.
Box FontBox(lv_obj_t *o)
{
    lv_area_t c;
    lv_obj_get_content_coords(o, &c);
    const int32_t h = lv_obj_get_self_height(o);  // a label's: its lines, at the font's height and its line space
    return {c.x1, c.y1, c.x2, std::min<int32_t>(c.y2, c.y1 + h - 1)};
}

// The glass, and the glass with every label's text drawn clear: their difference is each text's ink.
void DrawFrames(View &v)
{
    v.frame = Render();
    std::vector<std::pair<lv_obj_t *, lv_style_value_t>> kept;
    std::vector<lv_obj_t *> labels;
    for (lv_obj_t *o : AllObjects()) {
        if (lv_obj_check_type(o, &lv_label_class)) {
            lv_style_value_t value;
            if (lv_obj_get_local_style_prop(o, LV_STYLE_TEXT_OPA, &value, 0) == LV_STYLE_RES_FOUND) {
                kept.push_back({o, value});
            }
            labels.push_back(o);
            lv_obj_set_style_text_opa(o, LV_OPA_TRANSP, 0);
        }
    }
    v.bare = Render();
    for (lv_obj_t *o : labels) {
        lv_obj_remove_local_style_prop(o, LV_STYLE_TEXT_OPA, 0);
    }
    for (const auto &k : kept) {
        lv_obj_set_local_style_prop(k.first, LV_STYLE_TEXT_OPA, k.second, 0);
    }
    Render();
}

// An overlay — a floating child of the screen, as a dialog's catcher, or anything on the top layer — draws over the page:
// its opaque surfaces, with where each stands in the drawing order.
std::vector<std::pair<size_t, Box>> Covers(const std::vector<lv_obj_t *> &all)
{
    std::vector<std::pair<size_t, Box>> covers;
    for (size_t i = 0; i < all.size(); ++i) {
        bool overlay = false;
        for (lv_obj_t *p = all[i]; p != nullptr && !overlay; p = lv_obj_get_parent(p)) {
            lv_obj_t *up = lv_obj_get_parent(p);
            overlay = (up == lv_layer_top()) || (up == lv_screen_active() && lv_obj_has_flag(p, LV_OBJ_FLAG_FLOATING));
        }
        if (overlay && !Hidden(all[i]) && lv_obj_get_style_bg_opa(all[i], LV_PART_MAIN) >= LV_OPA_COVER) covers.push_back({i, OnGlass(all[i])});
    }
    return covers;
}

// Whether an overlay drawn after the object, and not its own ancestor, covers it: its ink there is the overlay's.
bool Covered(const std::vector<lv_obj_t *> &all, const std::vector<std::pair<size_t, Box>> &covers, size_t i, Box glass)
{
    for (const auto &[at, box] : covers) {
        bool under = at > i;
        for (lv_obj_t *c = all[at]; c != nullptr && under; c = lv_obj_get_parent(c)) under = c != all[i];
        if (under && !Meet(glass, box).Empty()) return true;
    }
    return false;
}

// One object as the glass shows it: its box, and a text's words, font box and ink, or anything else's black pixels.
Node MeasureNode(const View &v, lv_obj_t *o)
{
    Node n;
    n.obj = o;
    n.glass = OnGlass(o);
    n.text = IsText(o, &n.face);
    if (n.text) {
        n.words = lv_label_get_text(o);
        n.font = FontBox(o);
        n.ink = InkBox(v.frame, &v.bare, n.glass);
        n.text = !n.ink.Empty();  // drawn under something, or clear
    } else {
        n.ink = InkBox(v.frame, nullptr, n.glass);
    }
    return n;
}

void Capture(View &v)
{
    DrawFrames(v);
    v.order.clear();
    v.nodes.clear();
    const std::vector<lv_obj_t *> all = AllObjects();
    const std::vector<std::pair<size_t, Box>> covers = Covers(all);
    for (size_t i = 0; i < all.size(); ++i) {
        const Box glass = OnGlass(all[i]);
        if (Hidden(all[i]) || glass.Empty() || Covered(all, covers, i, glass)) continue;
        v.nodes[all[i]] = MeasureNode(v, all[i]);
        v.order.push_back(all[i]);
    }
    for (lv_obj_t *o : v.order) {  // each object holds the ink of every text in it
        const Node &n = v.nodes[o];
        if (!n.text) continue;
        for (lv_obj_t *p = o; p != nullptr; p = lv_obj_get_parent(p)) {
            const auto it = v.nodes.find(p);
            if (it != v.nodes.end()) it->second.held = Join(it->second.held, n.ink);
        }
    }
}

// What a group shows for spacing: the ink of the words it holds, or its drawn pixels when it holds none (LAYOUT §7.1).
Box GroupInk(const Node &n) { return n.held.Empty() ? n.ink : n.held; }

// A dialog's box: a surface that floats (ui::AddOffset at Elevation::kFloating). Its title is a modal's, which stands alone.
lv_event_cb_t s_offset_cb = nullptr;
void FindOffsetCb()
{
    lv_obj_t *probe = lv_obj_create(lv_screen_active());
    ui::AddOffset(probe, ui::Elevation::kFloating, ui::OffsetSide::kBottomRight);
    if (lv_obj_get_event_count(probe) > 0) s_offset_cb = lv_event_dsc_get_cb(lv_obj_get_event_dsc(probe, 0));
    lv_obj_delete(probe);
}

bool Floating(lv_obj_t *o)
{
    for (uint32_t i = 0; s_offset_cb != nullptr && i < lv_obj_get_event_count(o); ++i) {
        lv_event_dsc_t *dsc = lv_obj_get_event_dsc(o, i);
        const uintptr_t packed = reinterpret_cast<uintptr_t>(lv_event_dsc_get_user_data(dsc));
        if (lv_event_dsc_get_cb(dsc) == s_offset_cb && (packed & 0xFF) == static_cast<uint8_t>(ui::Elevation::kFloating)) return true;
    }
    return false;
}

// The pair gap a pair's larger text allows, between the two font boxes: about 0.3–0.5 em of the larger text, rounded to the
// spacing scale, never under 8 (LAYOUT §7.1); a modal's title over its message up to spacing.xl, as it stands alone. LAYOUT
// sets no numbers, so these are the approved pair tokens — pair-sm 8, pair-md 16, pair-lg 24, pair-xl 32 — on the scale's
// steps until the token header names them.
struct PairRange {
    int lo, hi;
    const char *tokens;
};

PairRange PairRangeFor(int larger, bool modal)
{
    PairRange r = larger <= 20   ? PairRange{dt::kSpacingSm, dt::kSpacingSm, "pair-sm"}
                  : larger <= 30 ? PairRange{dt::kSpacingSm, dt::kSpacingMd, "pair-sm to pair-md"}
                  : larger <= 40 ? PairRange{dt::kSpacingMd, dt::kSpacingMd, "pair-md"}
                                 : PairRange{dt::kSpacingLg, dt::kSpacingLg, "pair-lg"};
    if (modal && r.hi < dt::kSpacingXl) {
        r.hi = dt::kSpacingXl;
        r.tokens = r.lo == dt::kSpacingSm ? "pair-sm to pair-xl" : "pair-md to pair-xl";
    }
    return r;
}

const Node *Seen(const View &v, lv_obj_t *o)
{
    const auto it = v.nodes.find(o);
    return it == v.nodes.end() || it->second.ink.Empty() ? nullptr : &it->second;
}

// The children of `o` the glass shows, in order, laid out (no floating one).
std::vector<const Node *> Kids(const View &v, lv_obj_t *o)
{
    std::vector<const Node *> kids;
    for (uint32_t i = 0; i < lv_obj_get_child_count(o); ++i) {
        lv_obj_t *c = lv_obj_get_child(o, static_cast<int32_t>(i));
        if (lv_obj_has_flag(c, LV_OBJ_FLAG_FLOATING)) {
            continue;
        }
        if (const Node *n = Seen(v, c)) {
            kids.push_back(n);
        }
    }
    return kids;
}

// The first words in an object, to name it in a finding.
std::string Name(lv_obj_t *o)
{
    std::vector<lv_obj_t *> all;
    Collect(o, all);
    for (lv_obj_t *c : all) {
        Face f;
        if (IsText(c, &f)) {
            return "'" + Short(lv_label_get_text(c)) + "'";
        }
    }
    const Box b = Coords(o);
    return std::string(lv_obj_check_type(o, &lv_button_class) ? "a button" : "an object") + " at " + std::to_string(b.x1) + ", " +
           std::to_string(b.y1);
}

// A screen-level object — the screen, the top layer, or a column directly on either: what holds blocks, not a component.
bool PageLevel(lv_obj_t *o)
{
    lv_obj_t *p = lv_obj_get_parent(o);
    return p == nullptr || lv_obj_get_parent(p) == nullptr || lv_obj_has_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

int GapBetween(Box above, Box below) { return below.y1 - above.y2 - 1; }

std::string Join(const std::vector<std::string> &items, size_t most = 3)
{
    std::string s;
    for (size_t i = 0; i < items.size() && i < most; ++i) {
        s += (i ? "; " : "") + items[i];
    }
    if (items.size() > most) {
        s += "; +" + std::to_string(items.size() - most) + " more";
    }
    return s;
}

// --- The checks of one view ---------------------------------------------------------------------------------------------------

struct PairSeen {
    std::string roles;
    int gap;  // between the font boxes
};
std::vector<std::pair<std::string, PairSeen>> s_pairs_seen;  // view, pair — for the spread across views
std::map<std::string, std::string> s_off_role;                 // "size/weight in view" -> its screen

void CheckType(View &v)
{
    std::set<int> sizes;
    std::vector<std::string> named;
    for (lv_obj_t *o : v.order) {
        const Node &n = v.nodes[o];
        if (!n.text) continue;
        if (!IsRole(n.face) && !v.specimen) {
            s_off_role.emplace(RoleOf(n.face) + " '" + Short(n.words) + "' in " + v.name, ScreenOf(v.name));
        }
        if (sizes.insert(n.face.size).second) {
            named.push_back(std::to_string(n.face.size) + " px '" + Short(n.words) + "'");
        }
    }
    if (!v.specimen && !sizes.empty()) {
        Add("hierarchy", "type sizes on the view", static_cast<double>(sizes.size()), 1, 3, v.name + ": " + Join(named, 4));
    }
}

// A pair: two texts one over the other in one component (not the page's own column). Its gap is glued — set between the
// font boxes, in its larger text's range, and checked by ink; the ink gap round the component, or round the two texts where
// the component holds more, is spaced, and never less than the pair's (LAYOUT §7.1).
struct PairWorst {
    double ratio = 0, order = -1000, contrast = 1, glue = 0;
    std::string ratio_at, order_at, contrast_at, glue_at;
    bool glue_seen = false;
};
struct BoxWorst {
    int gap = 0, off = -1;
    PairRange range{};
    std::string at;
};
struct PairFindings {
    std::map<std::string, PairWorst> worst;    // by roles
    std::map<std::string, BoxWorst> box_worst;  // by roles, a modal's title apart
};

// The pair's type: its size ratio, the secondary never heavier, and a difference by size or weight at all.
void NoteType(PairWorst &w, const Node &a, const Node &b, const std::string &at)
{
    const double ratio = static_cast<double>(std::max(a.face.size, b.face.size)) / std::min(a.face.size, b.face.size);
    if (ratio > w.ratio) { w.ratio = ratio; w.ratio_at = at; }
    // The primary is the larger text, or the upper of two the same size: a key over its value, an eyebrow over its
    // items, gives way by size, and must not then stand heavier (TYPOGRAPHY §17.2).
    const Node &primary = a.face.size >= b.face.size ? a : b, &secondary = &primary == &a ? b : a;
    const double order = secondary.face.weight - primary.face.weight;
    if (order > w.order) { w.order = order; w.order_at = at; }
    const double contrast = (a.face.size != b.face.size || a.face.weight != b.face.weight) ? 1 : 0;
    if (contrast < w.contrast || w.contrast_at.empty()) { w.contrast = contrast; w.contrast_at = at; }
}

// The gap from a group's box to the blocks beside it in `around`, the nearer one; 1 << 20 when nothing is above or below.
// `beside` counts a block above or below that shares no column with the group.
int GapAround(const std::vector<const Node *> &around, size_t first, size_t last, Box group, bool beside = false)
{
    int gap = 1 << 20;
    if (first > 0) {
        const Box prev = GroupInk(*around[first - 1]);
        if (!prev.Empty() && prev.y2 < group.y1 && (beside || OverlapX(prev, group))) gap = std::min(gap, GapBetween(prev, group));
    }
    if (last + 1 < around.size()) {
        const Box next = GroupInk(*around[last + 1]);
        if (!next.Empty() && next.y1 > group.y2 && (beside || OverlapX(next, group))) gap = std::min(gap, GapBetween(group, next));
    }
    return gap;
}

// The ink gap round the pair's group: the component when it holds only the pair, lifted while its parent holds only it;
// else the two texts among the component's other children, or the component among its own neighbours.
int GroupGap(View &v, lv_obj_t *p, const std::vector<const Node *> &kids, size_t i)
{
    const Node &a = *kids[i], &b = *kids[i + 1];
    if (kids.size() == 2) {
        lv_obj_t *unit = p;
        while (!PageLevel(unit) && Kids(v, lv_obj_get_parent(unit)).size() == 1) unit = lv_obj_get_parent(unit);
        const std::vector<const Node *> around = Kids(v, lv_obj_get_parent(unit));
        size_t at = 0;
        for (size_t k = 0; k < around.size(); ++k) if (around[k]->obj == unit) at = k;
        return GapAround(around, at, at, GroupInk(v.nodes[unit]));
    }
    int gap = GapAround(kids, i, i + 1, Join(a.ink, b.ink));
    if (gap == (1 << 20) && !PageLevel(p)) {
        // The two texts share the component with something beside them: the component's own neighbours space it.
        const std::vector<const Node *> outer = Kids(v, lv_obj_get_parent(p));
        for (size_t k = 0; k < outer.size(); ++k) {
            if (outer[k]->obj == p) gap = std::min(gap, GapAround(outer, k, k, GroupInk(v.nodes[p]), true));
        }
    }
    return gap;
}

// One pair, `kids[i]` over `kids[i + 1]` in `p`: its type, its gap by font box against its range, its gap against its
// group's, and the gap kept for the spread across views.
void NotePair(View &v, lv_obj_t *p, const std::vector<const Node *> &kids, size_t i, PairFindings &found)
{
    const Node &a = *kids[i], &b = *kids[i + 1];
    const std::string roles = RoleOf(a.face) + "/" + RoleOf(b.face);
    const std::string at = "'" + Short(a.words) + "' over '" + Short(b.words) + "'";
    PairWorst &w = found.worst[roles];
    NoteType(w, a, b, at);
    // The pair's gap by font box against its larger text's range: the worst outside, else the first seen.
    const int pair_gap = GapBetween(a.ink, b.ink);
    const int box_gap = GapBetween(a.font, b.font);
    const bool modal = i == 0 && Floating(p);
    const PairRange range = PairRangeFor(std::max(a.face.size, b.face.size), modal);
    const int off = box_gap < range.lo ? range.lo - box_gap : box_gap > range.hi ? box_gap - range.hi : 0;
    BoxWorst &bw = found.box_worst[roles + (modal ? " in a modal" : "")];
    if (off > bw.off) {
        bw = {box_gap, off, range, at + ": " + std::to_string(box_gap) + " px by font box, " + std::to_string(pair_gap) + " px by ink"};
    }
    // A title over a wrapped text may take a step more than a label over its one line (LAYOUT §7.1): spread apart.
    const bool wrapped = b.font.y2 - b.font.y1 + 1 > lv_font_get_line_height(lv_obj_get_style_text_font(b.obj, LV_PART_MAIN));
    if (!v.specimen) s_pairs_seen.push_back({v.name, {roles + (wrapped ? " over wrapped lines" : ""), box_gap}});
    const int group_gap = GroupGap(v, p, kids, i);
    if (group_gap != (1 << 20) && group_gap > 0) {
        const double glue = static_cast<double>(pair_gap) / group_gap;
        if (!w.glue_seen || glue > w.glue) {
            w.glue = glue; w.glue_seen = true;
            w.glue_at = at + ": " + std::to_string(pair_gap) + " px inside, " + std::to_string(group_gap) + " px round, by ink";
        }
    }
}

void CheckPairs(View &v)
{
    PairFindings found;
    for (lv_obj_t *p : v.order) {
        if (PageLevel(p) || v.nodes[p].text) continue;
        const std::vector<const Node *> kids = Kids(v, p);
        for (size_t i = 0; i + 1 < kids.size(); ++i) {
            const Node &a = *kids[i], &b = *kids[i + 1];
            if (a.text && b.text && b.ink.y1 > a.ink.y2 && OverlapX(a.glass, b.glass)) NotePair(v, p, kids, i, found);
        }
    }
    if (v.specimen) return;
    for (const auto &[roles, w] : found.worst) {
        Add("hierarchy", "pair size ratio " + roles, w.ratio, 1.0, 1.5, v.name + ": " + w.ratio_at);
        Add("hierarchy", "pair weight order " + roles + " (secondary − primary)", w.order, -400, 0, v.name + ": " + w.order_at);
        Add("hierarchy", "pair differs by size or weight " + roles, w.contrast, 1, 1, v.name + ": " + w.contrast_at);
        if (w.glue_seen) Add("spacing", "pair gap / group gap by ink " + roles, w.glue, 0, 1, v.name + ": " + w.glue_at);
    }
    for (const auto &[key, bw] : found.box_worst) {
        Add("spacing", "pair gap by font box " + key + " (" + bw.range.tokens + ")", bw.gap, bw.range.lo, bw.range.hi,
            v.name + ": " + bw.at);
    }
}

// A label alone between two blocks in a column belongs to the nearer: the two gaps 25 % apart at least.
void CheckProximity(View &v)
{
    double worst = -1;
    std::vector<std::string> where;
    for (lv_obj_t *p : v.order) {
        if (v.nodes[p].text) continue;
        const lv_flex_flow_t flow = lv_obj_get_style_flex_flow(p, LV_PART_MAIN);
        if (lv_obj_get_style_layout(p, LV_PART_MAIN) != LV_LAYOUT_FLEX || (flow != LV_FLEX_FLOW_COLUMN && flow != LV_FLEX_FLOW_COLUMN_WRAP)) continue;
        const std::vector<const Node *> kids = Kids(v, p);
        for (size_t i = 1; i + 1 < kids.size(); ++i) {
            const Node &prev = *kids[i - 1], &l = *kids[i], &next = *kids[i + 1];
            if (!l.text || prev.text || next.text) continue;
            const int above = GapBetween(prev.ink, l.ink), below = GapBetween(l.ink, next.ink);
            if (above <= 0 || below <= 0) continue;
            const double r = static_cast<double>(std::min(above, below)) / std::max(above, below);
            if (r > 0.8) where.push_back("'" + Short(l.words) + "' " + std::to_string(above) + " px below a block, " + std::to_string(below) + " px above the next");
            worst = std::max(worst, r);
        }
    }
    if (worst >= 0) {
        Add("spacing", "lone label nearer one neighbour (nearer gap / farther)", worst, 0, 0.8,
            v.name + ": " + (where.empty() ? std::string("every lone label sits nearer one block") : Join(where)));
    }
}

// Padding, margins and gaps from DESIGN.md's spacing tokens only: the ladder, and the named component spacings.
bool OnScale(int px)
{
    static const std::set<int> kScale = {0, dt::kSpacingXs, dt::kSpacingSm, dt::kSpacingMd, dt::kSpacingLg, dt::kSpacingXl,
                                         dt::kSpacing2xl, dt::kSpacingEdge, dt::kSpacingFocusOffset, dt::kSpacingCardInset,
                                         dt::kSpacingKeyGap, dt::kSpacingStepGap, dt::kSpacingOverlayOffset};
    return px <= dt::kSpacingStrokeDefault || kScale.count(px) != 0;  // 3 px and under: a stroke's detail, not spacing
}

void CheckOneOffs(View &v)
{
    std::map<std::string, std::string> found;  // "28 px gap" -> where
    auto note = [&](int px, const std::string &what, lv_obj_t *o, int edge = 0) {
        if (OnScale(px) || (edge > 0 && OnScale(px + edge))) return;
        const std::string key = std::to_string(px) + " px " + what;
        if (!found.count(key)) found[key] = key + " at " + Name(o);
    };
    for (lv_obj_t *o : v.order) {
        const bool label = lv_obj_check_type(o, &lv_label_class);
        const int border = lv_obj_get_style_border_width(o, LV_PART_MAIN);
        const std::vector<const Node *> kids = Kids(v, o);
        const bool room = lv_obj_has_flag(o, LV_OBJ_FLAG_SCROLLABLE);
        const int pads[4] = {lv_obj_get_style_pad_top(o, LV_PART_MAIN), lv_obj_get_style_pad_bottom(o, LV_PART_MAIN),
                             lv_obj_get_style_pad_left(o, LV_PART_MAIN), lv_obj_get_style_pad_right(o, LV_PART_MAIN)};
        static const char *const kSides[4] = {"padding top", "padding bottom", "padding left", "padding right"};
        const lv_border_side_t sides = lv_obj_get_style_border_side(o, LV_PART_MAIN);
        static const lv_border_side_t kSideBits[4] = {LV_BORDER_SIDE_TOP, LV_BORDER_SIDE_BOTTOM, LV_BORDER_SIDE_LEFT, LV_BORDER_SIDE_RIGHT};
        for (int s = 0; s < 4; ++s) {
            if (label ? pads[s] > dt::kSpacingSm : (!kids.empty() && pads[s] > 0 && !(room && pads[s] > dt::kSpacingTarget))) {
                // A label's padding up to 8 is its line box's leading (ui::FitLineBox). A box's inset is on the scale as its
                // padding or as the padding with the stroke it sits inside (13 inside a 3 px edge is the 16 the eye sees).
                note(pads[s], kSides[s], o, (sides & kSideBits[s]) != 0 ? border : 0);
            }
        }
        const int margins[4] = {lv_obj_get_style_margin_top(o, LV_PART_MAIN), lv_obj_get_style_margin_bottom(o, LV_PART_MAIN),
                                lv_obj_get_style_margin_left(o, LV_PART_MAIN), lv_obj_get_style_margin_right(o, LV_PART_MAIN)};
        static const char *const kMargins[4] = {"margin top", "margin bottom", "margin left", "margin right"};
        for (int s = 0; s < 4; ++s) {
            if (margins[s] != 0) note(margins[s], kMargins[s], o);
        }
        if (lv_obj_get_style_layout(o, LV_PART_MAIN) != LV_LAYOUT_FLEX || kids.size() < 2) continue;
        const lv_flex_flow_t flow = lv_obj_get_style_flex_flow(o, LV_PART_MAIN);
        const bool column = flow == LV_FLEX_FLOW_COLUMN || flow == LV_FLEX_FLOW_COLUMN_WRAP;
        note(column ? lv_obj_get_style_pad_row(o, LV_PART_MAIN) : lv_obj_get_style_pad_column(o, LV_PART_MAIN),
             column ? "row gap" : "column gap", o);
        for (size_t i = 0; i + 1 < kids.size(); ++i) {
            const Box a = Coords(kids[i]->obj), b = Coords(kids[i + 1]->obj);
            const int gap = column ? b.y1 - a.y2 - 1 : b.x1 - a.x2 - 1;
            // Past a footer's height a gap is placement — a sheet set at the glass's foot — not spacing between blocks.
            if (gap > 0 && gap <= dt::kSpacingFooter && (column ? b.y1 > a.y2 : b.x1 > a.x2)) {
                note(gap, column ? "between blocks, above" : "between blocks, before", kids[i + 1]->obj);
            }
        }
    }
    std::vector<std::string> where;
    for (const auto &f : found) where.push_back(f.second);
    Add("spacing", "spacing values off the token scale", static_cast<double>(found.size()), 0, 0,
        v.name + ": " + (where.empty() ? std::string("none") : Join(where, 4)));
}

// Where start-aligned words begin: a label at its parent's content edge, not centred and not set to the right.
void CheckLeftEdges(View &v)
{
    if (v.specimen) return;
    std::vector<int> lefts;
    std::map<int, std::string> first;
    for (lv_obj_t *o : v.order) {
        const Node &n = v.nodes[o];
        if (!n.text) continue;
        const lv_text_align_t align = lv_obj_get_style_text_align(o, LV_PART_MAIN);
        if (align == LV_TEXT_ALIGN_CENTER || align == LV_TEXT_ALIGN_RIGHT) continue;
        lv_obj_t *p = lv_obj_get_parent(o);
        if (lv_obj_check_type(p, &lv_button_class) || lv_obj_get_style_width(p, LV_PART_MAIN) == LV_SIZE_CONTENT) {
            continue;  // a box that hugs its words — a compact button, a chip — gives them no edge to align
        }
        const int content_left = Coords(p).x1 + lv_obj_get_style_pad_left(p, LV_PART_MAIN) + lv_obj_get_style_border_width(p, LV_PART_MAIN);
        if (Coords(o).x1 - content_left > 3) continue;  // placed off the content edge: centred, or after a mark
        lefts.push_back(n.ink.x1);
        if (!first.count(n.ink.x1)) first[n.ink.x1] = "'" + Short(n.words) + "'";
    }
    if (lefts.empty()) return;
    std::sort(lefts.begin(), lefts.end());
    std::vector<std::string> edges;
    int last = -100;
    for (int x : lefts) {
        if (x - last > 2) edges.push_back(std::to_string(x) + " px " + first[x]);  // 2 px: a glyph's own bearing
        last = x;
    }
    Add("balance", "left edges of start-aligned words", static_cast<double>(edges.size()), 1, 3, v.name + ": " + Join(edges, 6));
}

// Words wider than the box that holds them are cut at its edge: a button's label past its sides. A page or a pager that
// scrolls them out of view is not a cut.
void CheckCut(View &v)
{
    std::vector<std::string> cut;
    for (lv_obj_t *o : v.order) {
        const Node &n = v.nodes[o];
        if (!n.text) continue;
        const Box whole = Coords(o);
        if (whole.x1 >= n.glass.x1 && whole.x2 <= n.glass.x2) continue;
        bool scrolled = false;
        for (lv_obj_t *p = lv_obj_get_parent(o); p != nullptr && !scrolled; p = lv_obj_get_parent(p)) {
            scrolled = lv_obj_has_flag(p, LV_OBJ_FLAG_SCROLLABLE) && (lv_obj_get_scroll_left(p) > 0 || lv_obj_get_scroll_right(p) > 0);
        }
        if (whole.x1 < 0 || whole.x2 >= kW || scrolled) continue;
        cut.push_back("'" + Short(n.words) + "' " + std::to_string(whole.W()) + " px in " + std::to_string(n.glass.W()));
    }
    Add("balance", "words cut off by their box", static_cast<double>(cut.size()), 0, 0,
        v.name + ": " + (cut.empty() ? std::string("none") : Join(cut)));
}

// The words a control names as why it is disabled (widgets::LinkDisabledReason), when they stand on this view.
bool LinkedReasonShown(View &v, lv_obj_t *control)
{
    lv_obj_t *reason = widgets::DisabledReason(control);
    const auto it = reason != nullptr ? v.nodes.find(reason) : v.nodes.end();
    return it != v.nodes.end() && it->second.text && !it->second.ink.Empty();
}

// Every disabled control says why in words: a second text inside it, a text just under it, or the words it names as its
// reason wherever they stand on the view — the keyboard's count line, above its footer (DESIGN → Components).
void CheckDisabled(View &v)
{
    std::vector<std::string> silent;
    int count = 0;
    for (lv_obj_t *o : v.order) {
        if (!lv_obj_has_state(o, LV_STATE_DISABLED) || lv_obj_check_type(o, &lv_label_class)) continue;
        lv_obj_t *p = lv_obj_get_parent(o);
        if (p != nullptr && lv_obj_has_state(p, LV_STATE_DISABLED)) continue;  // the outermost disabled object speaks for it
        // The control's row speaks for it when the control is the row's one control: a checkbox row's box, a toggle row's
        // switch.
        lv_obj_t *host = o;
        if (p != nullptr && !PageLevel(p)) {
            std::vector<lv_obj_t *> in_row;
            Collect(p, in_row);
            int controls = 0;
            for (lv_obj_t *c : in_row) controls += lv_obj_has_flag(c, LV_OBJ_FLAG_CLICKABLE) && !lv_obj_check_type(c, &lv_label_class);
            if (controls <= 1 + lv_obj_has_flag(p, LV_OBJ_FLAG_CLICKABLE)) host = p;
        }
        const Box box = v.nodes.count(host) ? v.nodes[host].glass : v.nodes[o].glass;
        // Where the reason would stand is off this view (the page scrolls on): the next view sees it.
        lv_obj_t *page = host;
        while (page != nullptr && !(lv_obj_has_flag(page, LV_OBJ_FLAG_SCROLLABLE) && PageLevel(page))) page = lv_obj_get_parent(page);
        if (Coords(host).y2 != box.y2 || (page != nullptr && box.y2 + dt::kSpacingXl > OnGlass(page).y2)) continue;
        ++count;
        std::vector<lv_obj_t *> inside;
        Collect(host, inside);
        int texts = 0;
        for (lv_obj_t *c : inside) texts += v.nodes.count(c) && v.nodes[c].text;
        bool told = texts >= 2 || LinkedReasonShown(v, o) || (host != o && LinkedReasonShown(v, host));
        for (lv_obj_t *c : v.order) {
            const Node &n = v.nodes[c];
            if (told || !n.text || std::find(inside.begin(), inside.end(), c) != inside.end()) continue;
            told = n.ink.y1 > box.y2 - 4 && n.ink.y1 - box.y2 <= dt::kSpacingXl && OverlapX(n.glass, box);
        }
        if (!told) silent.push_back(Name(o));
    }
    if (count > 0) {
        Add("state", "disabled controls with no reason in words", static_cast<double>(silent.size()), 0, 0,
            v.name + ": " + (silent.empty() ? std::to_string(count) + " disabled, each with its reason" : Join(silent, 4)));
    }
}

std::vector<lv_obj_t *> Focusables()
{
    std::vector<lv_obj_t *> out;
    lv_group_t *g = lvgl_display::Group();
    if (g == nullptr) return out;
    for (uint32_t i = 0; i < lv_group_get_obj_count(g); ++i) {
        lv_obj_t *o = lv_group_get_obj_by_index(g, i);
        if (!Hidden(o) && !lv_obj_has_state(o, LV_STATE_DISABLED)) out.push_back(o);
    }
    return out;
}

void Ring(lv_obj_t *o, bool on)
{
    if (on) {
        lv_group_focus_obj(o);
        lv_obj_add_state(o, LV_STATE_FOCUS_KEY);
    } else {
        lv_obj_remove_state(o, LV_STATE_FOCUS_KEY);
    }
    lv_obj_update_layout(lv_screen_active());
}

// The share of the glass the first ring move on the view changes: DOWN from the first control in view to the next.
void CheckRingMove(View &v)
{
    std::vector<lv_obj_t *> in_view;
    for (lv_obj_t *o : Focusables()) {
        const Box whole = Coords(o), seen = OnGlass(o);
        if (v.nodes.count(o) && whole.y1 == seen.y1 && whole.y2 == seen.y2) in_view.push_back(o);  // wholly in view: no scroll
    }
    if (in_view.size() < 2) return;
    lv_obj_t *scroller = nullptr;
    for (lv_obj_t *p = lv_obj_get_parent(in_view[0]); p != nullptr && scroller == nullptr; p = lv_obj_get_parent(p)) {
        if (lv_obj_has_flag(p, LV_OBJ_FLAG_SCROLLABLE) && PageLevel(p)) scroller = p;
    }
    const int32_t y = scroller != nullptr ? lv_obj_get_scroll_y(scroller) : 0;
    Ring(in_view[0], true);
    const std::vector<uint8_t> before = RenderChanged();
    Ring(in_view[0], false);
    Ring(in_view[1], true);
    const std::vector<uint8_t> after = RenderChanged();
    Ring(in_view[1], false);
    lv_group_focus_obj(nullptr);
    if (scroller != nullptr) lv_obj_scroll_to_y(scroller, y, LV_ANIM_OFF);
    const double share = static_cast<double>(Changed(before, after)) / (kW * kH);
    Add("medium", "glass changed by a ring move (share)", share, 0, 0.15,
        v.name + ": " + Name(in_view[0]) + " to " + Name(in_view[1]) + ", " + std::to_string(Changed(before, after)) + " px");
}

// --- The ring as LVGL draws it: its outline, and what its ancestors let it draw in (BYTE-90's ui_probe RingClip, RingCount) ---

// Where an object lets its children draw: its box, or — overflow visible — its box grown by its own ext_draw_size, as LVGL's
// lv_obj_redraw clips them; an overflow-visible container with no ext_draw_size still cuts at its box (ui::LetRingOut sets
// both).
Box ChildClip(lv_obj_t *p)
{
    Box b = Coords(p);
    if (lv_obj_has_flag(p, LV_OBJ_FLAG_OVERFLOW_VISIBLE)) {
        const int ext = lv_obj_get_ext_draw_size(p);
        b = {b.x1 - ext, b.y1 - ext, b.x2 + ext, b.y2 + ext};
    }
    return b;
}

constexpr const char *kRingSides[4] = {"left", "top", "right", "bottom"};

// A ring's outer edge, LVGL's: the object's box grown by its outline's width and pad. The pixels of it each side loses to
// the glass or to an ancestor, and which one cuts it. Not measured: a sibling or an overlay drawn over the ring (the status
// bar or the footer over scrolled content, an overlay over a lower screen), and an ancestor's clip_corner rounding the cut.
struct RingCut {
    Box ring;
    int px[4] = {};       // left, top, right, bottom
    std::string by[4];
    int Worst(int *side) const
    {
        int s = 0;
        for (int k = 1; k < 4; ++k) if (px[k] > px[s]) s = k;
        *side = s;
        return px[s];
    }
};

RingCut CutOf(lv_obj_t *o, int width, int pad)
{
    RingCut c;
    const Box box = Coords(o);
    const int grow = width + pad;
    c.ring = {box.x1 - grow, box.y1 - grow, box.x2 + grow, box.y2 + grow};
    Box seen{0, 0, kW - 1, kH - 1};
    std::string by[4] = {"the glass", "the glass", "the glass", "the glass"};
    for (lv_obj_t *p = lv_obj_get_parent(o); p != nullptr; p = lv_obj_get_parent(p)) {
        const Box clip = ChildClip(p);
        if (clip.x1 > seen.x1) { seen.x1 = clip.x1; by[0] = Name(p); }
        if (clip.y1 > seen.y1) { seen.y1 = clip.y1; by[1] = Name(p); }
        if (clip.x2 < seen.x2) { seen.x2 = clip.x2; by[2] = Name(p); }
        if (clip.y2 < seen.y2) { seen.y2 = clip.y2; by[3] = Name(p); }
    }
    const int cut[4] = {seen.x1 - c.ring.x1, seen.y1 - c.ring.y1, c.ring.x2 - seen.x2, c.ring.y2 - seen.y2};
    for (int k = 0; k < 4; ++k) {
        c.px[k] = std::max(0, cut[k]);
        c.by[k] = c.px[k] > 0 ? by[k] : "";
    }
    return c;
}

// The outline an object draws in its current state, if any: width and pad.
bool Outline(lv_obj_t *o, int *width, int *pad)
{
    *width = lv_obj_get_style_outline_width(o, LV_PART_MAIN);
    *pad = lv_obj_get_style_outline_pad(o, LV_PART_MAIN);
    return *width > 0 && lv_obj_get_style_outline_opa(o, LV_PART_MAIN) > LV_OPA_MIN &&
           lv_obj_get_style_opa(o, LV_PART_MAIN) > LV_OPA_MIN;
}

// The ring a control draws open for editing, from the styles ui::RingEditing sets at FOCUS_KEY | EDITED, read without
// putting the control in that state (a roller opens on it).
bool EditingOutline(lv_obj_t *o, int *width, int *pad)
{
    constexpr lv_style_selector_t kEditing = LV_STATE_FOCUS_KEY | LV_STATE_EDITED;
    lv_style_value_t w, p;
    if (lv_obj_get_local_style_prop(o, LV_STYLE_OUTLINE_WIDTH, &w, kEditing) != LV_STYLE_RES_FOUND || w.num <= 0) return false;
    *width = w.num;
    *pad = lv_obj_get_local_style_prop(o, LV_STYLE_OUTLINE_PAD, &p, kEditing) == LV_STYLE_RES_FOUND ? p.num
                                                                                                    : lv_obj_get_style_outline_pad(o, LV_PART_MAIN);
    return true;
}

// The objects in a focused state whose outline is drawn on the glass now: shown, its ring's box in what its ancestors show.
std::vector<lv_obj_t *> RingsDrawn()
{
    std::vector<lv_obj_t *> out;
    for (lv_obj_t *o : AllObjects()) {
        int width = 0, pad = 0;
        if (!lv_obj_has_state(o, LV_STATE_FOCUSED) && !lv_obj_has_state(o, LV_STATE_FOCUS_KEY)) continue;
        if (Hidden(o) || !Outline(o, &width, &pad)) continue;
        const RingCut c = CutOf(o, width, pad);
        if (c.px[0] + c.px[2] < c.ring.W() && c.px[1] + c.px[3] < c.ring.y2 - c.ring.y1 + 1) out.push_back(o);
    }
    return out;
}

std::string FileName(const std::string &s)
{
    std::string f;
    for (char c : s) {
        const bool keep = std::isalnum(static_cast<unsigned char>(c));
        if (keep) f += c;
        else if (!f.empty() && f.back() != '_') f += '_';
    }
    while (!f.empty() && f.back() == '_') f.pop_back();
    return f;
}

// Every control in the screen's walk shows a ring on the glass when focused — the focus brings it into view first. Each
// ringed frame is kept in build/host/audit/rings/, by screen and stop, so a refactor can be proven pixel-identical (cmp);
// the ring cut off by a container or the glass, and the rings drawn at once, are measured on it.
void CheckRings(const std::string &screen, lv_obj_t *scroller)
{
    s_screen = ScreenOf(screen);
    std::vector<std::string> bare;
    std::vector<std::pair<int, std::string>> cuts;  // px, where
    std::vector<std::string> many;
    int count = 0, worst_rings = 1;
    const std::vector<lv_obj_t *> walk = Focusables();
    for (size_t k = 0; k < walk.size(); ++k) {
        lv_obj_t *o = walk[k];
        lv_group_focus_obj(o);
        lv_obj_update_layout(lv_screen_active());
        const Box seen = OnGlass(o);
        if (seen.Empty()) continue;
        ++count;
        lv_obj_remove_state(o, LV_STATE_FOCUS_KEY);  // focused, as a touch leaves it: no ring
        const std::vector<uint8_t> plain = RenderChanged();
        lv_obj_add_state(o, LV_STATE_FOCUS_KEY);
        const std::vector<uint8_t> ringed = RenderChanged();
        const std::string stop = "stop " + std::to_string(k + 1) + " " + Name(o);
        char index[8];
        std::snprintf(index, sizeof index, "%02zu", k + 1);
        SavePng("build/host/audit/rings/" + FileName(screen) + "__" + index + "_" + FileName(Name(o)) + ".png", ringed);
        int width = 0, pad = 0;
        if (Outline(o, &width, &pad)) {
            const RingCut c = CutOf(o, width, pad);
            for (int s = 0; s < 4; ++s) {
                if (c.px[s] > 0) cuts.push_back({c.px[s], stop + " " + kRingSides[s] + " " + std::to_string(c.px[s]) + " px (by " + c.by[s] + ")"});
            }
        }
        if (EditingOutline(o, &width, &pad)) {
            const RingCut c = CutOf(o, width, pad);
            for (int s = 0; s < 4; ++s) {
                if (c.px[s] > 0) cuts.push_back({c.px[s], stop + " editing " + kRingSides[s] + " " + std::to_string(c.px[s]) + " px (by " + c.by[s] + ")"});
            }
        }
        const std::vector<lv_obj_t *> drawn = RingsDrawn();
        if (drawn.size() != 1) {
            std::vector<std::string> names;
            for (lv_obj_t *d : drawn) names.push_back(Name(d));
            many.push_back(stop + ": " + std::to_string(drawn.size()) + " rings (" + Join(names, 4) + ")");
            if (std::abs(static_cast<int>(drawn.size()) - 1) > std::abs(worst_rings - 1)) worst_rings = static_cast<int>(drawn.size());
        }
        lv_obj_remove_state(o, LV_STATE_FOCUS_KEY);
        if (Changed(plain, ringed) < 4 * seen.W()) bare.push_back(Name(o));  // a ring is at least a 4 px stroke along the top
    }
    lv_group_focus_obj(nullptr);
    if (scroller != nullptr) lv_obj_scroll_to_y(scroller, 0, LV_ANIM_OFF);
    if (count > 0) {
        Add("state", "focusable controls with no visible ring", static_cast<double>(bare.size()), 0, 0,
            screen + ": " + (bare.empty() ? std::to_string(count) + " controls, each ringed" : Join(bare, 4)));
        std::stable_sort(cuts.begin(), cuts.end(), [](const auto &a, const auto &b) { return a.first > b.first; });
        std::vector<std::string> hits;
        for (const auto &c : cuts) hits.push_back(c.second);
        Add("state", "focus rings cut off (px)", cuts.empty() ? 0 : cuts.front().first, 0, 0,
            screen + ": " + (hits.empty() ? std::to_string(count) + " rings, each whole" : Join(hits, hits.size())));
        Add("state", "rings drawn at once", worst_rings, 1, 1,
            screen + ": " + (many.empty() ? "one ring at each of " + std::to_string(count) + " stops" : Join(many, many.size())));
    }
}

// --- Views ---------------------------------------------------------------------------------------------------------------------

std::vector<std::string> s_views;
std::map<std::string, Box> s_back;  // the footer's Back on each screen

void Measure(const std::string &name, bool specimen)
{
    s_screen = ScreenOf(name);
    View v;
    v.name = name;
    v.specimen = specimen;
    Capture(v);
    CheckType(v);
    CheckPairs(v);
    CheckProximity(v);
    CheckOneOffs(v);
    CheckLeftEdges(v);
    CheckCut(v);
    CheckDisabled(v);
    CheckRingMove(v);
    std::string file = name;
    for (char &c : file) if (!std::isalnum(static_cast<unsigned char>(c))) c = '_';
    SavePng("build/host/audit/" + file + ".png", v.frame);
    s_views.push_back(name);
    for (lv_obj_t *o : v.order) {
        const Node &n = v.nodes[o];
        if (n.text && n.words == "Back" && n.glass.y1 >= kH - dt::kSpacingFooter && lv_obj_check_type(lv_obj_get_parent(o), &lv_button_class)) {
            s_back[name.substr(0, name.find(" p"))] = Coords(lv_obj_get_parent(o));
        }
    }
}

// The view's page that scrolls by pixel, if it has one.
lv_obj_t *Scroller()
{
    std::vector<lv_obj_t *> all;
    Collect(lv_screen_active(), all);
    for (lv_obj_t *o : all) {
        if (lv_obj_get_parent(o) == lv_screen_active() && lv_obj_has_flag(o, LV_OBJ_FLAG_SCROLLABLE) &&
            lv_obj_get_scroll_bottom(o) > 0 && !Hidden(o)) {
            return o;  // the screen's own column; a paged list view inside it turns pages, never scrolls by pixel
        }
    }
    return nullptr;
}

// A screen: each glass-full of it, then its rings.
void Screen(const std::string &name, bool specimen = false)
{
    Settle();
    lv_obj_t *scroller = Scroller();
    if (scroller == nullptr) {
        Measure(name, specimen);
    } else {
        // A glass-full a view, each overlapping the last by the page's own padding; the ring move inside a view may scroll, so
        // each view sets its own place.
        const int step = Coords(scroller).y2 - Coords(scroller).y1 + 1 - 2 * dt::kSpacingMd;
        lv_obj_scroll_to_y(scroller, 0, LV_ANIM_OFF);
        Settle();
        const int room = lv_obj_get_scroll_bottom(scroller);
        const int pages = 1 + (room + step - 1) / step;
        for (int page = 0; page < pages; ++page) {
            lv_obj_scroll_to_y(scroller, std::min(page * step, room), LV_ANIM_OFF);
            Settle();
            Measure(name + " p" + std::to_string(page + 1), specimen);
        }
        lv_obj_scroll_to_y(scroller, 0, LV_ANIM_OFF);
        Settle();
    }
    CheckRings(name, scroller);
}

// Presses a control by its words, as OK on it would.
bool Press(const char *words)
{
    for (lv_obj_t *o : AllObjects()) {
        Face f;
        if (!IsText(o, &f) || std::strcmp(lv_label_get_text(o), words) != 0) continue;
        for (lv_obj_t *p = o; p != nullptr; p = lv_obj_get_parent(p)) {
            if (lv_obj_has_flag(p, LV_OBJ_FLAG_CLICKABLE)) {
                if (lv_obj_has_state(p, LV_STATE_DISABLED)) break;  // the kit's disabled Delete is not the dialog's
                lv_obj_scroll_to_view_recursive(p, LV_ANIM_OFF);
                lv_obj_send_event(p, LV_EVENT_CLICKED, nullptr);
                Settle();
                return true;
            }
        }
    }
    return false;
}

// --- Across views -------------------------------------------------------------------------------------------------------------

// The screens Back stands apart on: those away from the place most screens share, or every screen when no place leads.
std::set<std::string> BackOutliers(const std::map<std::string, std::set<std::string>> &at_place)
{
    size_t most = 0, leaders = 0;
    for (const auto &[place, screens] : at_place) most = std::max(most, screens.size());
    for (const auto &[place, screens] : at_place) leaders += screens.size() == most;
    std::set<std::string> out;
    for (const auto &[place, screens] : at_place) {
        if (leaders > 1 || screens.size() != most) out.insert(screens.begin(), screens.end());
    }
    return out;
}

// Across views, each metric names the screens it found: the views with an off-role face, the two views a pair's gap spreads
// between, the screens whose Back stands apart.
void CheckAcross()
{
    s_screen.clear();
    std::vector<std::string> faces;
    std::set<std::string> face_screens;
    for (const auto &[entry, screen] : s_off_role) {
        faces.push_back(entry);
        face_screens.insert(screen);
    }
    Add("consistency", "type faces that are no DESIGN.md role", static_cast<double>(s_off_role.size()), 0, 0,
        faces.empty() ? std::string("every text is a role: hero, display, headline, body, label, caption")
                      : Join(faces, 4), face_screens);
    std::map<std::string, std::pair<int, int>> spread;  // roles -> min, max pair gap
    std::map<std::string, std::pair<std::string, std::string>> at;
    for (const auto &[view, p] : s_pairs_seen) {
        auto it = spread.find(p.roles);
        if (it == spread.end()) {
            spread[p.roles] = {p.gap, p.gap};
            at[p.roles] = {view, view};
            continue;
        }
        if (p.gap < it->second.first) { it->second.first = p.gap; at[p.roles].first = view; }
        if (p.gap > it->second.second) { it->second.second = p.gap; at[p.roles].second = view; }
    }
    for (const auto &[roles, mm] : spread) {
        Add("consistency", "pair gap spread across views, by font box " + roles, mm.second - mm.first, 0, dt::kSpacingXs,
            std::to_string(mm.first) + " px in " + at[roles].first + ", " + std::to_string(mm.second) + " px in " + at[roles].second,
            {ScreenOf(at[roles].first), ScreenOf(at[roles].second)});
    }
    if (!s_back.empty()) {
        std::map<std::string, std::set<std::string>> at_place;  // "x, y" -> the screens Back stands there on
        std::string where;
        for (const auto &[screen, b] : s_back) {
            const std::string place = std::to_string(b.x1) + ", " + std::to_string(b.y1);
            if (at_place[place].empty()) where += (where.empty() ? "" : "; ") + screen + " at " + place;
            at_place[place].insert(screen);
        }
        Add("consistency", "places Back stands across screens", static_cast<double>(at_place.size()), 1, 1, where,
            BackOutliers(at_place));
    }
}

std::string Escape(const std::string &s)
{
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else if (static_cast<unsigned char>(c) < 0x20) out += ' ';
        else out += c;
    }
    return out;
}

void WriteJson(const char *path, const char *judge, const std::vector<Metric> &metrics)
{
    char at[32];
    const std::time_t now = std::time(nullptr);
    std::strftime(at, sizeof at, "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
    FILE *f = std::fopen(path, "w");
    if (f == nullptr) {
        std::printf("screen audit: cannot write %s\n", path);
        return;
    }
    std::fprintf(f, "{\"judge\": \"%s\", \"at\": \"%s\", \"scope\": \"product\", \"metrics\": [\n", judge, at);
    for (size_t i = 0; i < metrics.size(); ++i) {
        const Metric &m = metrics[i];
        std::string screens;
        for (const std::string &name : m.screens) screens += (screens.empty() ? "\"" : ", \"") + Escape(name) + "\"";
        std::fprintf(f, "  {\"dimension\": \"%s\", \"name\": \"%s\", \"value\": %.4g, \"range\": [%.4g, %.4g], \"ok\": %s, \"where\": \"%s\", \"screens\": [%s]}%s\n",
                     m.dimension.c_str(), Escape(m.name).c_str(), m.value, m.lo, m.hi, m.ok ? "true" : "false",
                     Escape(m.where).c_str(), screens.c_str(), i + 1 < metrics.size() ? "," : "");
    }
    std::fprintf(f, "]}\n");
    std::fclose(f);
}

// --- The screens, by route -------------------------------------------------------------------------------------------------

// Every hardware-test screen the audit opens, by its route, and the name its views and metrics carry — the names
// screen_audit.py maps to the sources that build each screen.
struct Route { const char *id, *name; bool specimen; };
const Route kScreens[] = {
    {"hw.display", "display", false},         {"hw.display.grays", "four greys", false},
    {"hw.display.type-scale", "type scale", true}, {"hw.display.kit", "kit", false},
    {"hw.benchmark", "benchmark", false},     {"hw.touch", "touch", false},
    {"hw.buttons", "buttons", false},         {"hw.sensors", "sensors", false},
    {"hw.clock", "clock", false},             {"hw.welcome", "welcome", false},
    {"hw.power", "power", false},             {"hw.storage", "storage", false},
    {"hw.microphone", "microphone", false},   {"hw.reminders", "reminders", false},
    {"hw.about", "about", false},
};

std::string NameOfRoute(const std::string &id)
{
    for (const Route &r : kScreens) {
        if (id == r.id) return r.name;
    }
    return id;  // a route the audit does not open: screen_audit.py counts it touched by any UI change
}

// --- The ux-designer's proxy: the presses from Home to each screen, through the routes the controls push ---------------------

// The route a control pushes when OK presses it: a home row's entry, or a button's route by name (hw_test's own callbacks).
const char *RouteOf(lv_obj_t *o)
{
    for (uint32_t i = 0; i < lv_obj_get_event_count(o); ++i) {
        lv_event_dsc_t *dsc = lv_obj_get_event_dsc(o, i);
        const lv_event_cb_t cb = lv_event_dsc_get_cb(dsc);
        if (cb == hw_test::OnEntry) return static_cast<const hw_test::Entry *>(lv_event_dsc_get_user_data(dsc))->route;
        if (cb == hw_test::OnPushRoute) return static_cast<const char *>(lv_event_dsc_get_user_data(dsc));
    }
    return nullptr;
}

struct Reach { std::vector<std::string> path; int presses; std::string trail; };

// The screens a walk passes: Home and each it opens on the way.
std::set<std::string> PathScreens(const Reach &r)
{
    std::set<std::string> out = {"home"};
    for (const std::string &id : r.path) out.insert(NameOfRoute(id));
    return out;
}

// The ux-designer's metrics: the screens opened to reach each, and the most presses to reach any.
void WriteFlow(const std::map<std::string, Reach> &seen)
{
    const size_t before = s_metrics.size();
    s_screen.clear();
    int most = 0;
    const Reach *farthest = nullptr;
    for (const auto &[route, r] : seen) {
        if (route.empty()) continue;
        Add("flow", "screens opened from Home to " + route, static_cast<double>(r.path.size()), 0, 3,
            r.trail + ": " + std::to_string(r.presses) + " presses", PathScreens(r));
        if (r.presses > most) { most = r.presses; farthest = &r; }
    }
    // [J] ten presses: ten partial refreshes, about ten seconds on the glass
    Add("flow", "most presses from Home to a screen", most, 0, 10,
        (farthest ? farthest->trail : std::string()) + ": " + std::to_string(most) + " presses",
        farthest ? PathScreens(*farthest) : std::set<std::string>{"home"});
    const std::vector<Metric> flow(s_metrics.begin() + static_cast<long>(before), s_metrics.end());
    s_metrics.resize(before);
    router::PopToRoot();
    Settle();
    WriteJson(".cache/harness/proxies/ux-designer.json", "ux-designer", flow);
    std::printf("route walk: %zu screens reached from Home, the farthest %d presses — .cache/harness/proxies/ux-designer.json\n",
                seen.size() - 1, most);
}

// From Home, breadth first: a fresh screen's first press only rings its first control (DESIGN → Focus and input), DOWN or UP
// walks the walk one control a press, OK opens. Depth is the screens opened on the way; past 3 a person loses the way back
// (ux-designer: the task flow, fewest steps).
void WalkRoutes()
{
    std::map<std::string, Reach> seen;
    std::vector<std::string> queue = {""};
    seen[""] = {{}, 0, "Home"};
    for (size_t q = 0; q < queue.size(); ++q) {
        const Reach here = seen[queue[q]];
        router::PopToRoot();
        Settle();
        for (const std::string &id : here.path) router::PushRoute(id.c_str());
        Settle();
        const std::vector<lv_obj_t *> walk = Focusables();
        for (size_t k = 0; k < walk.size(); ++k) {
            const char *route = RouteOf(walk[k]);
            if (route == nullptr || seen.count(route)) continue;
            Reach next = here;
            next.path.push_back(route);
            // The ring, then DOWN k times or UP round the end (the walk wraps, router.cpp), then OK.
            next.presses = here.presses + 1 + static_cast<int>(std::min(k, walk.size() - k)) + 1;
            next.trail = here.trail + " > " + Name(walk[k]);
            seen[route] = next;
            queue.push_back(route);
        }
    }
    WriteFlow(seen);
}

// Every hardware-test screen, opened by its route over home, as a home row or Display's button opens it.
void AuditScreens()
{
    for (const Route &r : kScreens) {
        router::PopToRoot();
        Settle();
        if (router::PushRoute(r.id) != ESP_OK) {
            std::printf("screen audit: %s did not open\n", r.id);
            continue;
        }
        Screen(r.name, r.specimen);
    }
}

// The status line under a screen's title, showing news as Storage says it when no card is in (router::Say).
void AuditStatusLine()
{
    router::PopToRoot();
    Settle();
    if (router::PushRoute("hw.storage") != ESP_OK) {
        std::printf("screen audit: hw.storage did not open for the status line\n");
        return;
    }
    Settle();
    if (router::Say(router::StatusKind::kError, "No card. Insert one to read it.") == 0) {
        std::printf("screen audit: the status line said nothing\n");
        return;
    }
    Screen("status line");
}

// The kit's overlays: the keyboard, the dialog and the select sheet, each opened from the kit page as OK opens it.
void AuditOverlays()
{
    struct Overlay { const char *control, *name; };
    static const Overlay kOverlays[] = {{"Password", "keyboard"}, {"Delete", "dialog"}, {"Remind me", "select sheet"}};
    for (const Overlay &o : kOverlays) {
        router::PopToRoot();
        Settle();
        router::PushRoute("hw.display.kit");
        Settle();
        if (!Press(o.control)) {
            std::printf("screen audit: the kit's %s did not open\n", o.name);
            continue;
        }
        Measure(o.name, false);
        CheckRings(o.name, nullptr);
        router::DismissOverlays();
        Settle();
    }
}

// --- The fixture: the measures themselves, on two labels set at known gaps ---------------------------------------------------

// The rows the glass inks in a box, drawn with only `keep` showing of `labels`.
Box InkAlone(lv_obj_t *keep, const std::vector<lv_obj_t *> &labels)
{
    for (lv_obj_t *l : labels) {
        if (l != keep) lv_obj_add_flag(l, LV_OBJ_FLAG_HIDDEN);
    }
    const std::vector<uint8_t> glass = Render();
    for (lv_obj_t *l : labels) lv_obj_remove_flag(l, LV_OBJ_FLAG_HIDDEN);
    return InkBox(glass, nullptr, Coords(keep));
}

constexpr int kFixtureGaps[2] = {dt::kSpacingSm, dt::kSpacingLg};

// On a screen of paper, a column of two cards, each a label's line over a caption's, `kFixtureGaps[k]` px apart by font box:
// the labels, title then caption, card by card; none when the faces are missing.
std::vector<lv_obj_t *> BuildFixture(lv_obj_t *scr)
{
    const lv_font_t *title = ui::Bitcount(dt::kTypographyLabel.size_px, dt::kTypographyLabel.weight);
    const lv_font_t *body = ui::Bitcount(dt::kTypographyCaption.size_px, dt::kTypographyCaption.weight);
    std::vector<lv_obj_t *> labels;
    if (title == nullptr || body == nullptr) return labels;
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);  // LVGL's own default theme is none here: paper, as a screen sets
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_t *column = lv_obj_create(scr);
    lv_obj_set_size(column, kW, kH);
    lv_obj_remove_flag(column, LV_OBJ_FLAG_SCROLLABLE);
    for (int k = 0; k < 2; ++k) {
        lv_obj_t *card = lv_obj_create(column);
        lv_obj_set_pos(card, 40, 100 + 300 * k);
        lv_obj_set_size(card, 400, 200);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *a = lv_label_create(card), *b = lv_label_create(card);
        lv_obj_set_style_text_font(a, title, 0);
        lv_obj_set_style_text_font(b, body, 0);
        lv_label_set_text(a, "Hello there");
        lv_label_set_text(b, "A line below it");
        lv_obj_set_pos(a, 0, 0);
        lv_obj_set_pos(b, 0, lv_font_get_line_height(title) + kFixtureGaps[k]);
        labels.push_back(a);
        labels.push_back(b);
    }
    return labels;
}

// The fixture's labels measured as every view is: the audit's font boxes must stand exactly the set gap apart, its ink gaps
// must match each label drawn alone, and its pair metric must carry the worst font-box gap.
int CheckFixture()
{
    lv_obj_t *before = lv_screen_active();
    lv_obj_t *scr = lv_obj_create(nullptr);
    lv_screen_load(scr);
    const std::vector<lv_obj_t *> labels = BuildFixture(scr);
    if (labels.empty()) {
        std::printf("screen audit fixture: FAIL no Bitcount face for label or caption\n");
        return 1;
    }
    const int *gaps = kFixtureGaps;
    Settle();
    int bad = 0;
    auto check = [&](bool ok, const std::string &what) {
        if (!ok) { ++bad; std::printf("screen audit fixture: FAIL %s\n", what.c_str()); }
    };
    View v;
    v.name = "fixture";
    const size_t metrics = s_metrics.size(), pairs = s_pairs_seen.size();
    Capture(v);
    for (int k = 0; k < 2; ++k) {
        const Node &a = v.nodes[labels[2 * k]], &b = v.nodes[labels[2 * k + 1]];
        const Box ink_a = InkAlone(labels[2 * k], labels), ink_b = InkAlone(labels[2 * k + 1], labels);
        const std::string at = "the pair " + std::to_string(gaps[k]) + " px apart";
        check(a.text && b.text, at + ": both labels read as text");
        check(GapBetween(a.font, b.font) == gaps[k], at + ": font-box gap " + std::to_string(GapBetween(a.font, b.font)));
        check(GapBetween(a.ink, b.ink) == GapBetween(ink_a, ink_b),
              at + ": ink gap " + std::to_string(GapBetween(a.ink, b.ink)) + ", drawn alone " + std::to_string(GapBetween(ink_a, ink_b)));
        check(a.ink.y1 >= a.font.y1 && a.ink.y2 <= a.font.y2 && b.ink.y1 >= b.font.y1, at + ": the ink inside its font box");
    }
    CheckPairs(v);
    std::set<double> seen;
    for (size_t i = metrics; i < s_metrics.size(); ++i) {
        if (s_metrics[i].name.rfind("pair gap by font box", 0) == 0) seen.insert(s_metrics[i].value);
    }
    check(seen.count(dt::kSpacingLg) == 1, "the pair metric carries the worst font-box gap, " + std::to_string(dt::kSpacingLg) + " px");
    s_metrics.resize(metrics);
    s_pairs_seen.resize(pairs);
    lv_screen_load(before);
    lv_obj_delete(scr);
    std::printf("screen audit fixture: %s\n", bad ? "FAIL" : "font boxes and ink at their known gaps");
    return bad;
}

// --- The ring fixture: rings cut a known amount on each side, let out, and drawn two at once ------------------------------------

// A button with the one ring (ui::RingOutside), 160 x 64 at x, y in `parent`, out of the group: the fixture sets its states.
lv_obj_t *RingFixtureButton(lv_obj_t *parent, int x, int y)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_group_remove_obj(b);
    lv_obj_remove_state(b, static_cast<lv_state_t>(LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY));
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, 160, dt::kSpacingTarget);
    ui::RingOutside(b);
    return b;
}

// A box 400 x 200 at 40, 300 on the screen, no padding and no scroll: it cuts its children at its edge unless it lets them out.
lv_obj_t *RingFixtureBox(lv_obj_t *scr)
{
    lv_obj_t *box = lv_obj_create(scr);
    lv_obj_remove_style_all(box);
    lv_obj_set_pos(box, 40, 300);
    lv_obj_set_size(box, 400, 200);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

lv_obj_t *RingFixtureScreen()
{
    lv_obj_t *scr = lv_obj_create(nullptr);
    lv_screen_load(scr);
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    return scr;
}

// The ring's measures on known layouts, each on a screen of its own: a button in a clipping box, placed so the box cuts its
// ring a known amount on one side; the same in a box that lets it out (ui::LetRingOut), whole; in a box set overflow visible
// with no ext_draw_size, still cut; a button open for editing (ui::RingEditing, its 6 px stroke), read by EditingOutline. Each
// is read by the measure and drawn so on the glass: the ring's changed pixels end where the measure says. Then two buttons
// ringed at once read two rings, and one alone one.
int CheckRingFixture()
{
    lv_obj_t *before = lv_screen_active();
    int bad = 0;
    auto check = [&](bool ok, const std::string &what) {
        if (!ok) { ++bad; std::printf("screen audit ring fixture: FAIL %s\n", what.c_str()); }
    };
    constexpr int kReach = dt::kSpacingFocusOffset + dt::kSpacingStrokeFocus;   // 8
    constexpr int kEditReach = dt::kSpacingFocusOffset + dt::kSpacingStrokeEdit;  // 10
    constexpr int kRight = 400 - 160, kBottom = 200 - dt::kSpacingTarget, kMid = (200 - dt::kSpacingTarget) / 2;
    enum class Box_ { kClips, kLetOut, kOverflowOnly };
    struct Case { const char *what; Box_ box; bool editing; int x, y; int cut[4]; };
    const Case cases[] = {
        {"cut at the bottom", Box_::kClips, false, 120, kBottom, {0, 0, 0, kReach}},
        {"cut on the left", Box_::kClips, false, 3, kMid, {kReach - 3, 0, 0, 0}},
        {"cut at the top", Box_::kClips, false, 120, 0, {0, kReach, 0, 0}},
        {"cut on the right", Box_::kClips, false, kRight, kMid, {0, 0, kReach, 0}},
        {"let out", Box_::kLetOut, false, 120, kBottom, {0, 0, 0, 0}},
        {"overflow visible with no ext_draw_size", Box_::kOverflowOnly, false, 120, kBottom, {0, 0, 0, kReach}},
        {"open for editing, cut on the right", Box_::kClips, true, kRight - 3, kMid, {0, 0, kEditReach - 3, 0}},
    };
    for (const Case &t : cases) {
        lv_obj_t *scr = RingFixtureScreen();
        lv_obj_t *box = RingFixtureBox(scr);
        if (t.box == Box_::kLetOut) ui::LetRingOut(box);
        if (t.box == Box_::kOverflowOnly) lv_obj_set_overflow_visible(box, true);
        lv_obj_t *button = RingFixtureButton(box, t.x, t.y);
        if (t.editing) ui::RingEditing(button);
        Settle();
        const std::vector<uint8_t> plain = Render();
        lv_obj_add_state(button, t.editing ? static_cast<lv_state_t>(LV_STATE_FOCUS_KEY | LV_STATE_EDITED) : LV_STATE_FOCUS_KEY);
        lv_obj_update_layout(scr);
        const std::vector<uint8_t> ringed = Render();
        const std::string at = std::string(t.what) + ": ";
        int width = 0, pad = 0, edit_width = 0, edit_pad = 0;
        const bool drawn = Outline(button, &width, &pad);
        check(drawn && width + pad == (t.editing ? kEditReach : kReach), at + "an outline " + std::to_string(width) + " + " + std::to_string(pad));
        if (t.editing) {
            check(EditingOutline(button, &edit_width, &edit_pad) && edit_width == width && edit_pad == pad,
                  at + "the editing outline read " + std::to_string(edit_width) + " + " + std::to_string(edit_pad));
        }
        const RingCut c = t.editing ? CutOf(button, edit_width, edit_pad) : CutOf(button, width, pad);
        std::string read, want;
        bool same = true;
        for (int k = 0; k < 4; ++k) {
            same = same && c.px[k] == t.cut[k];
            read += (k ? ", " : "") + std::to_string(c.px[k]);
            want += (k ? ", " : "") + std::to_string(t.cut[k]);
        }
        check(same, at + "cut " + read + " px (left, top, right, bottom), not " + want);
        const Box ink = InkBox(ringed, &plain, {0, 0, kW - 1, kH - 1});
        const Box drawn_at{c.ring.x1 + t.cut[0], c.ring.y1 + t.cut[1], c.ring.x2 - t.cut[2], c.ring.y2 - t.cut[3]};
        check(ink.x1 == drawn_at.x1 && ink.y1 == drawn_at.y1 && ink.x2 == drawn_at.x2 && ink.y2 == drawn_at.y2,
              at + "the glass draws the ring at " + std::to_string(ink.x1) + ", " + std::to_string(ink.y1) + " to " + std::to_string(ink.x2) +
                  ", " + std::to_string(ink.y2) + "; the measure " + std::to_string(drawn_at.x1) + ", " + std::to_string(drawn_at.y1) + " to " +
                  std::to_string(drawn_at.x2) + ", " + std::to_string(drawn_at.y2));
        check(RingsDrawn().size() == 1, at + std::to_string(RingsDrawn().size()) + " rings drawn at once");
        lv_screen_load(before);
        lv_obj_delete(scr);
    }
    // Two rings at once: both buttons' states set here, then one taken off.
    lv_obj_t *scr = RingFixtureScreen();
    lv_obj_t *a = RingFixtureButton(scr, 40, 100), *b = RingFixtureButton(scr, 40, 300);
    lv_obj_add_state(a, LV_STATE_FOCUS_KEY);
    lv_obj_add_state(b, LV_STATE_FOCUS_KEY);
    lv_obj_update_layout(scr);
    check(RingsDrawn().size() == 2, "two ringed: " + std::to_string(RingsDrawn().size()) + " rings drawn at once");
    lv_obj_remove_state(b, LV_STATE_FOCUS_KEY);
    lv_obj_update_layout(scr);
    check(RingsDrawn().size() == 1, "one ringed: " + std::to_string(RingsDrawn().size()) + " rings drawn at once");
    lv_screen_load(before);
    lv_obj_delete(scr);
    std::printf("screen audit ring fixture: %s\n", bad ? "FAIL" : "rings cut a known amount on each side, open for editing and in a box overflow visible without reach; one let out whole; two rings at once");
    return bad;
}

// --- The disabled fixture: a reason far from its control, found only when the control names it ----------------------

// A disabled action button low on a screen of paper and a caption far above it, linked as its reason or not: the reason
// metric reads 0 linked and 1 unlinked.
int CheckDisabledFixture()
{
    lv_obj_t *before = lv_screen_active();
    int bad = 0;
    for (const bool linked : {true, false}) {
        lv_obj_t *scr = RingFixtureScreen();
        lv_obj_t *words = lv_label_create(scr);
        const auto &caption = dt::kTypographyCaption;
        lv_obj_set_style_text_font(words, ui::Bitcount(caption.size_px, caption.weight), 0);
        lv_label_set_text(words, "3 of 8 characters needed");
        lv_obj_set_pos(words, 40, 100);
        lv_obj_t *button = widgets::MakeButton(scr, "Join", widgets::ButtonVariant::kAction, 200);
        lv_obj_set_pos(button, 40, 600);
        lv_obj_add_state(button, LV_STATE_DISABLED);
        if (linked) widgets::LinkDisabledReason(button, words);
        Settle();
        View v;
        v.name = "fixture";
        const size_t metrics = s_metrics.size();
        Capture(v);
        CheckDisabled(v);
        const double silent = s_metrics.size() > metrics ? s_metrics.back().value : -1;
        s_metrics.resize(metrics);
        if (silent != (linked ? 0 : 1)) {
            ++bad;
            std::printf("screen audit disabled fixture: FAIL %s reason 500 px above: %g silent\n",
                        linked ? "a linked" : "an unlinked", silent);
        }
        lv_screen_load(before);
        lv_obj_delete(scr);
    }
    // The link: a second call replaces the reason, and the reason's deletion clears it, either going first.
    lv_obj_t *scr = RingFixtureScreen();
    lv_obj_t *control = lv_obj_create(scr), *one = lv_label_create(scr), *two = lv_label_create(scr);
    widgets::LinkDisabledReason(control, one);
    widgets::LinkDisabledReason(control, two);
    const bool replaced = widgets::DisabledReason(control) == two;
    lv_obj_delete(two);
    const bool cleared = widgets::DisabledReason(control) == nullptr;
    widgets::LinkDisabledReason(control, one);
    lv_obj_delete(control);
    lv_obj_delete(one);  // its unhooked link is not called on a freed cell
    if (!replaced || !cleared) {
        ++bad;
        std::printf("screen audit disabled fixture: FAIL the link %s\n",
                    replaced ? "outlives its reason" : "was not replaced");
    }
    lv_screen_load(before);
    lv_obj_delete(scr);
    std::printf("screen audit disabled fixture: %s\n",
                bad ? "FAIL" : "a reason far above is told when linked, silent when not");
    return bad;
}

// LVGL on the host: its heap grown as the firmware's PSRAM grows it, a glass-sized display read as the glass reads it, and
// a keypad in a group, as the buttons are.
bool StartLvgl()
{
    lv_init();
    // LVGL's own defaults give it a 64 KB heap, and no pool it adds may be larger; the kit page alone needs megabytes, as the
    // firmware's PSRAM gives it: 256 more pools of 60 KB.
    constexpr size_t kPool = 60 * 1024, kPools = 256;
    alignas(16) static uint8_t heap[kPool * kPools];
    for (size_t i = 0; i < kPools; ++i) {
        if (lv_mem_add_pool(heap + i * kPool, kPool) == nullptr) {
            std::printf("screen audit: LVGL took no more memory\n");
            return false;
        }
    }
    lv_tick_set_cb(Ticks);
    lv_display_t *display = lv_display_create(kW, kH);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_L8);
    lv_display_set_buffers(display, s_l8, nullptr, sizeof s_l8, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(display, Flush);
    lv_display_set_theme(display, nullptr);
    s_keys = lv_indev_create();
    lv_indev_set_type(s_keys, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_keys, [](lv_indev_t *, lv_indev_data_t *data) { data->state = LV_INDEV_STATE_RELEASED; });
    s_group = lv_group_create();
    lv_group_set_default(s_group);
    lv_indev_set_group(s_keys, s_group);
    return true;
}

// The folders the audit writes, with last run's views taken away, as this run may not have them all.
void PrepareOutput()
{
    mkdir("build/host/audit", 0755);
    mkdir("build/host/audit/rings", 0755);
    for (const std::string folder : {"build/host/audit/", "build/host/audit/rings/"}) {
        if (DIR *dir = opendir(folder.c_str())) {
            while (const dirent *e = readdir(dir)) {
                const std::string file = e->d_name;
                if (file.size() > 4 && file.compare(file.size() - 4, 4, ".png") == 0) {
                    std::remove((folder + file).c_str());
                }
            }
            closedir(dir);
        }
    }
    mkdir(".cache", 0755);
    mkdir(".cache/harness", 0755);
    mkdir(".cache/harness/proxies", 0755);
}

}  // namespace

int main()
{
    if (!StartLvgl()) return 1;
    if (ui::Start() != ESP_OK || router::Init() != ESP_OK) {
        std::printf("screen audit: ui or the router did not start\n");
        return 1;
    }
    MapFaces();
    FindOffsetCb();
    // A measure that misreads a known layout would misread every screen.
    if (CheckFixture() != 0 || CheckRingFixture() != 0 || CheckDisabledFixture() != 0) return 1;
    PrepareOutput();

    // Home and the routes, as app_shell builds them.
    hw_test::Install();
    app_hooks::Seal();
    app_hooks::RunBoot(app_hooks::Phase::kBoardUp);
    if (router::SetRoot(router::MakeScreen(screen_home::Build)) != ESP_OK) {
        std::printf("screen audit: home did not build\n");
        return 1;
    }
    Screen("home");
    AuditScreens();
    AuditStatusLine();
    AuditOverlays();
    router::PopToRoot();
    Settle();

    CheckAcross();
    WriteJson(".cache/harness/proxies/ui-designer.json", "ui-designer", s_metrics);
    WalkRoutes();
    int outside = 0;
    for (const Metric &m : s_metrics) outside += !m.ok;
    std::printf("screen audit: %zu views, %zu metrics, %d outside their range (reported, never failed) — "
                ".cache/harness/proxies/ui-designer.json, build/host/audit/\n",
                s_views.size(), s_metrics.size(), outside);
    return 0;
}
