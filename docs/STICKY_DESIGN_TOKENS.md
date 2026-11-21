# FolloUp — Design Tokens

How the firmware implements [DESIGN.md](../DESIGN.md) on the reTerminal Sticky's e-paper glass: the greys at the
panel's two bit depths and the proof that every colour pair holds on both refreshes, the fonts and how they are
compiled, the token header and its names, the refresh policy and feedback in code, and the checks that keep code and
design in step. For anyone building or reviewing a screen or a component on the device, and for the agents that do.

**Source of truth.** The design — every value, rule and reason — is [DESIGN.md](../DESIGN.md), cited by heading path
(DESIGN → Colors → Roles); its front matter holds the values, and this doc's tables restate them for the design check.
The panel, its refresh modes and their costs are [STICKY_HARDWARE_SPECS](STICKY_HARDWARE_SPECS.md), cited `[HW]`. The
symbols are the firmware's token header (§6). Where this doc and the code disagree, find which is intended and fix the
other; how a screen looks on the glass beats both.

**Related.** [STICKY_HARDWARE_SPECS](STICKY_HARDWARE_SPECS.md) — the panel (§2, §4.5), sleep (§6), the buzzer (§4.2).
Steps — bring-up, display work, the pin check: the `sticky-hardware` skill
([.agents/skills/sticky-hardware/SKILL.md](../.agents/skills/sticky-hardware/SKILL.md)).

**Stability.** Code cites this doc as `STICKY_DESIGN_TOKENS §n` for how the code does something, and DESIGN.md by
heading path for why. Do not renumber; add new material under the next free number.

| Tag | Meaning |
| --- | --- |
| `[HW]` | From STICKY_HARDWARE_SPECS |
| `[D]` | Derived: calculated from the cited values |
| `[SEEN]` | Seen on the glass by the person, on a V1.0 or V2.0 board running this repo's firmware |
| `[?]` | Unverified — tracked in §8 |

| § | Section | Look here for |
| --- | --- | --- |
| 1 | [Rules](#1-rules) | The code's side of DESIGN.md's rules: which value, which refresh, which check |
| 2 | [Colour](#2-colour) | The levels in the framebuffer; each role's kind and its value on both refreshes; the contrast proof |
| 3 | [Type](#3-type) | Bitcount Single at one bit, the scale's primitives, the roles, line boxes, glyph ranges |
| 4 | [Space, strokes and shapes](#4-space-strokes-and-shapes) | The scales as code values; the dither; the hard offset |
| 5 | [Refresh and feedback](#5-refresh-and-feedback) | The refresh policy, the partial counter, the rate limits, the buzzer tick |
| 6 | [The token header](#6-the-token-header) | Where the header lives, how a token's name becomes a symbol |
| 7 | [Enforcement](#7-enforcement) | The design check on this doc, Google's linter on DESIGN.md, what is checked by hand |
| 8 | [Open items](#8-open-items) | What is still unverified, and what settles it |

---

## 1. Rules

### Do

1. **Take every value from the token header, by role** — `kColorTextPrimary`, never `0x00` or a level number (§6).
2. **Draw every frame as if a partial refresh will show it** — the roles' partial-refresh values in §2.1 are how the
   glass looks most of the time.
3. **Compile every glyph a partial refresh draws at one bit a pixel** (§3).
4. **Count partial refreshes and draw the update after twenty as a full one** (§5).
5. **Fire the buzzer tick from the input handler, before the refresh is queued** (§5).
6. **Change DESIGN.md's front matter and this doc's tables in the same change**, then the header (§7).

### Never

- **Paint `scale.primary.1` or `scale.primary.2` solid on a UI element** — fill it with `ui::Fill`, the dither (§4);
  the solid greys are for images drawn with a greyscale refresh (DESIGN → Colors → The four levels).
- **Animate** — no timer drives a redraw faster than §5's limits.
- **Draw a stroke under 2 px, a weight under 600, or words under `caption`, 20 px.**
- **Use a value the header does not have** — decide it in DESIGN.md first.

## 2. Colour

### 2.1 Roles

The panel stores two bits a pixel for a greyscale full refresh, levels 0 to 3; a partial or mono full refresh keeps
one, and a level under 2 becomes black, 2 or more white `[HW]`. The tables below restate DESIGN.md's colours with
their value on each refresh — the design check reads them (§7).

| Token | Full refresh | Partial refresh | Use |
| --- | --- | --- | --- |
| `colors.scale.primary.0` | `#000000` | `#000000` | Level 0 |
| `colors.scale.primary.1` | `#555555` | `#000000` | Level 1 |
| `colors.scale.primary.2` | `#AAAAAA` | `#FFFFFF` | Level 2 |
| `colors.scale.primary.3` | `#FFFFFF` | `#FFFFFF` | Level 3 |

| Kind | Means | Paired in §2.2 |
| --- | --- | --- |
| surface | A background words are painted on | As the background for every text and non-text role |
| text | Words, or an icon that carries meaning | On every surface, floor 4.5 |
| non-text | Edges, dividers, the focus ring | On every surface, floor 3 |
| fill | A solid background for a control or a state | As the background for every on-fill role |
| on-fill | Words on a fill | On every fill, floor 4.5 |
| exempt | Decoration, and a disabled control's inside — black words only, never an edge, never the only sign of a state | Not paired |

| Token | Kind | Full refresh | Partial refresh | Use |
| --- | --- | --- | --- | --- |
| `colors.background` | surface | `#FFFFFF` | `#FFFFFF` | The glass |
| `colors.text-primary` | text | `#000000` | `#000000` | Every word that must read |
| `colors.text-secondary` | text | `#555555` | `#000000` | Supporting words, timestamps |
| `colors.outline` | non-text | `#000000` | `#000000` | Control and card edges |
| `colors.divider` | non-text | `#555555` | `#000000` | The rule between rows |
| `colors.focus-ring` | non-text | `#000000` | `#000000` | The buttons' focus |
| `colors.surface-action` | fill | `#000000` | `#000000` | The action button |
| `colors.surface-chosen` | fill | `#000000` | `#000000` | A chosen row, chip or option — never focus |
| `colors.surface-on` | fill | `#000000` | `#000000` | A toggle that is on |
| `colors.text-on-fill` | on-fill | `#FFFFFF` | `#FFFFFF` | Words on any fill |
| `colors.focus-ring-on-fill` | on-fill | `#FFFFFF` | `#FFFFFF` | The focus ring on a chosen row or chip |
| `colors.band` | exempt | `#AAAAAA` | `#FFFFFF` | A light-dither fill on every refresh (§4) — the partial column is its solid value, which a fill never draws: decoration, a disabled control's inside |

### 2.2 Contrast pairs

| Foreground | Background | Floor | Full refresh | Partial refresh |
| --- | --- | --- | --- | --- |
| `colors.text-primary` | `colors.background` | 4.5 | 21.00 | 21.00 |
| `colors.text-secondary` | `colors.background` | 4.5 | 7.46 | 21.00 |
| `colors.outline` | `colors.background` | 3 | 21.00 | 21.00 |
| `colors.divider` | `colors.background` | 3 | 7.46 | 21.00 |
| `colors.focus-ring` | `colors.background` | 3 | 21.00 | 21.00 |
| `colors.text-on-fill` | `colors.surface-action` | 4.5 | 21.00 | 21.00 |
| `colors.text-on-fill` | `colors.surface-chosen` | 4.5 | 21.00 | 21.00 |
| `colors.text-on-fill` | `colors.surface-on` | 4.5 | 21.00 | 21.00 |
| `colors.focus-ring-on-fill` | `colors.surface-action` | 4.5 | 21.00 | 21.00 |
| `colors.focus-ring-on-fill` | `colors.surface-chosen` | 4.5 | 21.00 | 21.00 |
| `colors.focus-ring-on-fill` | `colors.surface-on` | 4.5 | 21.00 | 21.00 |

- **The ratios are the levels', computed from their hex values** — the glass's own white and black sit closer, so each
  ratio is an upper bound on the device `[?]`.
- **A grey fill draws as a dither, not the partial-refresh column** — the tables give the solid value a text or line
  role draws; a fill of level 1 or 2 is its pattern (§4), averaging `#898989` and `#E0E0E0` in linear light `[D]`.
  Black words on the light dither average 16 : 1; white words on the dark dither 3.5 : 1, under the floor — so words
  sit only on the light dither, in black (DESIGN → Elevation & Depth → Dither) `[D]`.
- **`colors.text-secondary` on a fill has no pair** — 2.82 : 1 on black; words on a fill are `colors.text-on-fill`.

## 3. Type

- **The face is Bitcount Single**, a pixel font, at weights 600 and 700 — the upright SemiBold and Bold
  instances of its variable font (`wght` 600 and 700, `ELXP` 0, `ELSH` 0, `slnt` 0, `CRSV` 0) from
  petrvanblokland/TYPETR-Bitcount, under the SIL Open Font License (`components/ui/fonts/bitcount/OFL.txt`).
- **Every size is compiled at one bit a pixel** by `lv_font_conv` 1.5.3 (`components/ui/fonts/bitcount/make_fonts.sh`),
  so a stroke is solid black on every refresh; no font engine runs on the device.
- **Bitcount draws on a 100-unit cell, a tenth of the em, so every size is a multiple of 10 px** — each cell then falls
  on whole panel pixels, 2 px at 20 px and 6 px at 60 px; a size between draws its cells unevenly. Weight widens each
  cell's square by 20 units at 600 and 30 at 700, so the two weights look alike below about 50 px `[D]`.
- **The scale is the primitives, `typography.scale-<size>-<semibold|bold>`** — 20, 30, 40, 50, 60, 70 and 80 px with
  every character the strings use, and 100, 120 and 160 px with figures only, each at both weights, its line pitch
  1.2 em (the font's ascent and descent) — `kTypographyScale30Semibold` and its siblings in `design_tokens.h`.
- **A role is one primitive** — `hero` `scale-60-bold`, `display`
  `scale-50-bold`, `headline` `scale-40-bold`, `body` `scale-30-semibold`, `label` `scale-30-bold`, `caption`
  `scale-20-semibold`; in the header, `kTypographyBody = kTypographyScale30Semibold`. Google's linter allows no group
  under `typography`, so DESIGN.md writes each role's values out with its primitive named beside it.
- **A role's line height is its line pitch** — 72, 60, 48, 36, 36 and 24 px. `ui::NewLabel` and the screen's style
  set LVGL's line space to the pitch less the compiled font's height, so the lines of a block step by the pitch; a
  single line is the compiled font's height, about 0.9 em — 19 px for `caption` at 20 px, 26 for `body` and `label` at 30,
  52 at 60 — and a block of n lines is n − 1 pitches and one font height `[D]`.
- **The glyph ranges** are ASCII and · ° × – — ’ • … at 20 to 80 px, and the space, the figures and the colon at
  100 px and up; Bitcount has no arrows, so no string uses one. What a character outside them draws on the glass is
  open (§8).
- **Every character is 0.6 em wide** — the face is monospaced — so changing figures keep their width without a
  tabular setting, and `body` fits 24 characters across `spacing.content` `[D]`.
- **No letter spacing** — it would break the cells off the pixel grid.

## 4. Space, strokes and shapes

- **Every spacing, stroke and radius is a whole number of panel pixels**, as DESIGN.md's front matter gives it — no
  scaling at run time.
- **`spacing.target`, 64 px, is a hit area**: a control drawn smaller pads its touch box to it.
- **A grey fill is a dither** — `ui::Fill(level)` gives levels 1 and 2 their fill codes in LVGL's L8 frame,
  `lvgl_display::kDitherDark` (`0x56`) and `kDitherLight` (`0xAB`), one step above the plain greys, and the one-bit
  packing (`PackMono`) draws them as fixed patterns locked to the panel's pixel grid: level 1 one white pixel in four,
  level 2 three in four. Every refresh keeps them; a greyscale refresh reads the codes as the true greys. Text and
  lines take `ui::Level`, so their greys threshold to solid black or white; an anti-aliased edge pixel thresholds
  too, unless its blend lands exactly on a code, when it draws as the dither — rare, and why words sit only on the
  light dither. Both patterns read as distinct greys, and alike, on the V1.0's and the V2.0's glass `[SEEN]`.
- **The overlay's offset is `spacing.overlay-offset`, a solid 6 px of level 0,** on the right and bottom edges,
  drawn as two rectangles, never a blur.
- **A list row's padding is two values** — `spacing.sm` above and below, `spacing.md` either side; DESIGN.md's
  front matter carries the vertical one, since a component takes one padding token. Its title and supporting line
  are one line each, 26 px of `label` and 19 of `caption`, so 8 + 26 + 19 + 8 = 61 px, centred in the 80 px row `[D]`.
- **The ring inside a control** sits on the edge of one with no outline, and `spacing.focus-offset` inside an outlined
  one — 3 + 4 + 4 = 11 px in from each edge `[D]`; a 64 px button keeps 42 px inside the ring for a 26 px line, and
  a footer button `(432 − 2 × 8) / 3` = 138 px wide keeps 116 px for its label, 6 characters of `label` at 18 px a
  character, and a half-width button 212 px wide keeps 190 px, 10 characters `[D]`. The budget is the room clear of the
  ring: a 10-character label runs 3 px into the button's `spacing.md` padding on each side, inside its edge and clear
  of the ring (DESIGN → Voice & Content Strategy → Labels and states).

## 5. Refresh and feedback

| Token | Value | What the code does |
| --- | --- | --- |
| `kRefreshPartialLimit` | 20 | Counts partial refreshes; the next update after 20 is drawn full, and a full one resets the count — proven on the SSD1677, and on the SSD2677 no ghost seen before it `[HW]` `[SEEN]` |
| `kFeedbackTickMs` | 100 ms | The buzzer tick sounds as the input watcher reads a press — one poll, 10 ms (`kWatchMs`), after it — even during a refresh |
| `kWaitIndicatorDelayMs` | 1000 ms | Declared for DESIGN.md's rule — a wait's indicator drawn only once the wait has run this long, `motion.wait-indicator`; no screen has a wait indicator yet |
| `kSelfUpdateMinMs` | 5000 ms | The period of the hardware-test screens' self-updates (Sensors, Battery and power) — DESIGN.md's floor between two self-driven redraws of an area, `motion.self-update`, which no code enforces |
| `kClockTickMs` | 60000 ms | Declared for DESIGN.md's clock redraw, `motion.clock-tick`; the firmware has no clock yet |

- **A full refresh is drawn for**: the first frame after boot or wake; a full a screen asks for (`RequestFull`, the
  hardware-test screens'); the update after `kRefreshPartialLimit` partials; the one-bit frame after a greyscale
  refresh; on the V1.0, the pause of 2 s after 150 frames by scans; and, as a recovery, a partial that fails — drawn
  again in full at once — and the next frame after a failed refresh, settle or standby, since the glass is then
  unknown. In order, with the code that sets each:
  [STICKY_EPAPER_WAVEFORMS §3.4](STICKY_EPAPER_WAVEFORMS.md#34-which-refresh-a-frame-gets). A greyscale full refresh is
  for a still screen showing an image (DESIGN → Colors → The four levels); no product screen has one yet, and the
  hardware-test Four greys screen draws one as a fixture.
- **DESIGN.md also keeps a full refresh for closing a large overlay and for the frame left before sleep** (DESIGN →
  Motion → Nothing animates) — no screen in the firmware has an overlay or that last frame yet, so no code sets them.
- **A new screen on the V1.0 is two partials, through white**, not a full one — 1.0 to 1.25 s against a full's 1.6 s,
  and counted as two toward `kRefreshPartialLimit`, so one that would pass it is drawn full. Ten page changes, then a
  full refresh, left no trace on the glass over a run of 80 `[SEEN]`.
- **Two full refreshes never run back to back** (DESIGN → Do's and Don'ts) — a full or greyscale refresh asked for
  sooner after the last one ended than that one took waits out the rest in `Send` (`WaitAfterFlash`), then runs with
  the frame LVGL drew; it is never dropped (DESIGN → Motion → Nothing animates). The gap is the last full or grey
  refresh's own measured time — after a full, about 1 s on a V2.0 and 1.6 s on a V1.0; after a grey, the grey's
  time, 1.25 s on a V2.0 (STICKY_EPAPER_WAVEFORMS §4) — so it follows each controller and the temperature without a
  constant. Leaving Four greys straight after it lands held the full 546–607 ms on a V2.0 `[SEEN]`. A press during the
  wait still ticks and is replayed after it, in the frame after the held one. A failed refresh sets no
  gap, since it flashed nothing, so a retry never waits on a busy timeout; the full that redraws a failed partial runs
  at once, a recovery, rare, and the one exception.
- **Input during a refresh is kept** (DESIGN → Motion → Press) — a task of its own, above LVGL's, reads UP, DOWN, OK
  and the touch panel every 10 ms, ticks on each press, and queues each press and release; LVGL replays the queue when
  the refresh ends, so a short press inside a refresh is never lost and moves focus once. An OK pressed before the frame
  the last OK caused has been drawn is ignored, though it ticks; a queued OK acts where the moves queued before it
  left focus, and a double tap is replayed as two taps (the person's choice, from the glass); a held UP or DOWN
  repeats its move after LVGL's long-press time, `LV_INDEV_DEF_LONG_PRESS_TIME` (400 ms), each repeat a frame. A press
  or a move is queued only with room left for its release, so a release is never lost; holding OK for 1 s goes back,
  whether or not LVGL saw the hold begin. The tick sounded at once during a full refresh `[SEEN]`.
- **An open overlay holds the screen under it, by DESIGN.md's rule** — what changes underneath is kept in state and
  drawn when the overlay closes, so the overlay's own refresh is never queued behind the page's; no overlay exists in
  the firmware yet.
- **Which refresh a frame gets is the display's policy** — a refresh by the controller's own waveform, or, on the
  SSD1677, scans for a small change; scans never count toward `kRefreshPartialLimit`, and after 150 scan frames the next
  pause of 2 s gets the full refresh ([STICKY_EPAPER_WAVEFORMS §3.4](STICKY_EPAPER_WAVEFORMS.md#34-which-refresh-a-frame-gets)).

## 6. The token header

- **One C++ header carries every token** — `components/design_tokens/include/design_tokens.h`, kept equal to DESIGN.md's
  front matter by hand and named in `TOKEN_FILES` in `.agents/project.py`.
- **A token's path becomes its symbol**: `colors.text-primary` → `kColorTextPrimary`, `spacing.target` →
  `kSpacingTarget`, `spacing.stroke.focus` → `kSpacingStrokeFocus`, `rounded.sm` → `kRoundedSm`.
- **A colour is stored as its level, 0 to 3**, with the hex in a comment — `kColorTextSecondary = 1;  // #555555`.
- **A type token is a struct of its size, its line box and its weight**, a role being one primitive by name;
  `ui::Font(role)` gives the compiled font LVGL draws it in, and `ui::Bitcount(size, weight)` any primitive's (§3).

## 7. Enforcement

- **The design check runs on this doc** — `python3 .agents/skills/design-system/scripts/design_check.py --file
  docs/STICKY_DESIGN_TOKENS.md` proves every pair in §2.2 meets its floor on both refreshes, every pair the kinds call
  for exists, and every ratio is true; `verify.py run` runs it when DESIGN.md or this doc changes.
- **Google's linter runs on DESIGN.md** — `npx @google/design.md lint DESIGN.md` checks its token references, its
  sections and their order, and each component's contrast; it fetches the package from npm, so the guard asks first.
- **DESIGN.md's front matter and §2's tables agree by hand** — a script to compare them is open (§8).

## 8. Open items

| Item | Status | Unblocked by |
| --- | --- | --- |
| Generating the token header's symbols from DESIGN.md's front matter, instead of by hand | Open | A second change to the tokens, which makes a generator worth it |
| A script that compares DESIGN.md's front matter with §2's tables | Open | The token header, which a third copy would make worth checking |
| The glyph ranges beyond ASCII and · ° × – — ’ • … — a name with an accent, a currency — and what a character outside them draws: LVGL's placeholder box is on (`CONFIG_LV_USE_FONT_PLACEHOLDER`) | Open | An out-of-range character drawn on the glass; the first strings that need more |
| The glass's real white and black, against the levels' nominal 21 : 1 | Open | A reading with a light meter on a V1.0 and a V2.0 board |
| The dither over many partials — whether it ghosts before the full refresh clears it | Open | A long page walk over a dithered fill |
| The first button press showing the focus ring and moving nothing (DESIGN → Focus and input) — `input.cpp` has no state for a ring not yet shown | Open | The person's choice, with the focus walk on the glass |
