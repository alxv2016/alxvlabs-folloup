# FolloUp — Design Tokens

How the firmware implements [DESIGN.md](../DESIGN.md) on the reTerminal Sticky's e-paper glass: the greys at the
panel's two bit depths and the proof that every colour pair holds on both refreshes, the fonts and how they are
compiled, the token header and its names, the refresh policy and feedback in code, and the checks that keep code and
design in step. For anyone building or reviewing a screen or a component on the device, and for the agents that do.

**Source of truth.** The design — every value, rule and reason — is [DESIGN.md](../DESIGN.md), cited by heading path
(DESIGN → Colors → Roles); its front matter holds the values, and this doc's tables restate them for the design check.
The panel, its refresh modes and their costs are [STICKY_HARDWARE_SPECS](STICKY_HARDWARE_SPECS.md), cited `[HW]`; the
widgets' sizes come from the agent deck's UI kit, cited `[DECK]` (§9). The
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
| `[DECK]` | Read from the agent deck's widget source (a separate repository, §9) by the author on 2026-11-20 — an author's measure that a reader of this repo cannot open; a number is the deck's px, ported by its ratio |
| `[D]` | Derived: calculated from the cited values |
| `[SEEN]` | Seen on the glass by the person, on a V1.0 or V2.0 board running this repo's firmware |
| `[HOST]` | Measured on the computer, not the glass: LVGL's software renderer drawing the real widgets in the real Bitcount fonts at 480 × 800, thresholded at half, as ink to ink of sample words (a descender counts), on 2026-11-21 over 463f7f5 with the working tree, one run |
| `[?]` | Unverified — tracked in §8 |

| § | Section | Look here for |
| --- | --- | --- |
| 1 | [Rules](#1-rules) | The code's side of DESIGN.md's rules: which value, which refresh, which check |
| 2 | [Colour](#2-colour) | The levels in the framebuffer; each role's kind and its value on both refreshes; the contrast proof |
| 3 | [Type](#3-type) | Bitcount Single at one bit, the scale's primitives, the roles, line boxes, glyph ranges |
| 4 | [Space, strokes and shapes](#4-space-strokes-and-shapes) | The scales as code values; the dither; the hard offset and its elevation levels |
| 5 | [Refresh and feedback](#5-refresh-and-feedback) | The refresh policy, the partial counter and its typing-only mode, the rate limits, the buzzer tick |
| 6 | [The token header](#6-the-token-header) | Where the header lives, how a token's name becomes a symbol |
| 7 | [Enforcement](#7-enforcement) | The design check on this doc, Google's linter on DESIGN.md, what is checked by hand |
| 8 | [Open items](#8-open-items) | What is still unverified, and what settles it |
| 9 | [The ring outside and the ported widgets](#9-the-ring-outside-and-the-ported-widgets) | What the agent deck is and where its source lives; the one focus ring and the editing ring, a segment's ring in its track, every ported widget's sizes against the deck's |
| 10 | [Paging and the swipe](#10-paging-and-the-swipe) | How a list view, a text view and a horizontal pager turn a page: the pages, the swipe's thresholds and window, the lift, the ends and who owns a drag |
| 11 | [The keyboard](#11-the-keyboard) | The pages and keys, the key-by-key walk, how a press types, the typing-only partial count, and what the field component will take over |
| 12 | [The select sheet](#12-the-select-sheet) | The bottom sheet a dropdown opens: its height, how many rows a page, and the overlay that takes and gives back focus |

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
| `colors.scale.sparse.sixteenth` | `#F8F8F8` | `#F8F8F8` | One black pixel in sixteen, as a dither on every refresh — no panel level; the words on a disabled control's face (§4) |
| `colors.scale.sparse.eighth` | `#F0F0F0` | `#F0F0F0` | One black pixel in eight — a disabled row's name on the white page, and elevation 1's offset (§4) |

| Kind | Means | Paired in §2.2 |
| --- | --- | --- |
| surface | A background words are painted on | As the background for every text and non-text role |
| text | Words, or an icon that carries meaning | On every surface, floor 4.5 |
| non-text | Edges, dividers, the focus ring | On every surface, floor 3 |
| fill | A solid background for a control or a state | As the background for every on-fill role |
| on-fill | Words on a fill | On every fill, floor 4.5 |
| exempt | Decoration, a disabled control's face and its words — never an edge, never the only sign of a state — and a mark that sits on one surface only, paired by hand in §2.2 with that surface: `thumb` on `band`, `thumb-on` and `knob` on `surface-on`, and a checkbox's check or a radio's dot, which are `thumb-on` | Not paired by the kinds |

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
| `colors.surface-on` | fill | `#000000` | `#000000` | A toggle that is on, a slider's fill, a scroll indicator's thumb |
| `colors.text-on-fill` | on-fill | `#FFFFFF` | `#FFFFFF` | Words on any fill |
| `colors.band` | exempt | `#AAAAAA` | `#FFFFFF` | A light-dither fill on every refresh (§4) — the partial column is its solid value, which a fill never draws: a toggle's, segment's, slider's, checkbox's or radio's track, a chip's, card's or tonal button's fill, a progress bar's track, a scroll indicator's pieces, decoration, a disabled control's face |
| `colors.thumb` | exempt | `#000000` | `#000000` | A toggle's thumb on its off track, the light dither — 16 : 1 against the dither's average; paired with `colors.band` in §2.2 |
| `colors.thumb-on` | exempt | `#FFFFFF` | `#FFFFFF` | A toggle's thumb on its black, on track; paired with `colors.surface-on` |
| `colors.knob` | exempt | `#FFFFFF` | `#FFFFFF` | A range slider's knob bar on its black fill; paired with `colors.surface-on` |
| `colors.shadow-1` | exempt | `#F0F0F0` | `#F0F0F0` | Elevation 1's offset, a raised surface's: one black pixel in eight on every refresh (§4.1) — decoration, with no words or marks on it; no surface uses it yet |
| `colors.shadow-2` | exempt | `#AAAAAA` | `#FFFFFF` | Elevation 2's offset, a bottom sheet's: the light dither on every refresh — the partial column is its solid value, which a fill never draws |
| `colors.shadow-3` | exempt | `#555555` | `#000000` | Elevation 3's offset, a dialog's: the dark dither on every refresh — the partial column is its solid value, which a fill never draws |
| `colors.keyline` | exempt | `#FFFFFF` | `#FFFFFF` | The white line round an offset's outer edge, `spacing.stroke.hairline` wide: invisible over white, an edge over a dither (§4.1) |
| `colors.text-disabled` | exempt | `#F8F8F8` | `#F8F8F8` | Words, check, dot and glyph on a disabled control's light-dither face: the sparse dither of one black pixel in sixteen on every refresh (§4) — the value is the pattern's average, in linear light |
| `colors.text-disabled-page` | exempt | `#F0F0F0` | `#F0F0F0` | A disabled row's name and state word, a disabled slider's knob and value, a disabled mark's dot — on the white: one black pixel in eight on every refresh |

### 2.2 Contrast pairs

| Foreground | Background | Floor | Full refresh | Partial refresh |
| --- | --- | --- | --- | --- |
| `colors.text-primary` | `colors.background` | 4.5 | 21.00 | 21.00 |
| `colors.text-secondary` | `colors.background` | 4.5 | 7.46 | 21.00 |
| `colors.outline` | `colors.background` | 3 | 21.00 | 21.00 |
| `colors.divider` | `colors.background` | 3 | 7.46 | 21.00 |
| `colors.focus-ring` | `colors.background` | 3 | 21.00 | 21.00 |
| `colors.thumb` | `colors.band` | 3 | 9.04 | 21.00 |
| `colors.text-on-fill` | `colors.surface-action` | 4.5 | 21.00 | 21.00 |
| `colors.text-on-fill` | `colors.surface-chosen` | 4.5 | 21.00 | 21.00 |
| `colors.text-on-fill` | `colors.surface-on` | 4.5 | 21.00 | 21.00 |
| `colors.thumb-on` | `colors.surface-on` | 4.5 | 21.00 | 21.00 |
| `colors.knob` | `colors.surface-on` | 4.5 | 21.00 | 21.00 |

- **The ratios are the levels', computed from their hex values** — the glass's own white and black sit closer, so each
  ratio is an upper bound on the device `[?]`.
- **A grey fill draws as a dither, not the partial-refresh column** — the tables give the solid value a text or line
  role draws; a fill of level 1 or 2 is its pattern (§4), averaging `#898989` and `#E0E0E0` in linear light `[D]`.
  Black words on the light dither average 16 : 1; white words on the dark dither 3.5 : 1, under the floor — so words
  sit only on the light dither, in black (DESIGN → Elevation & Depth → Dither) `[D]`.
- **The thumb's pair is on its own surface, `band`** — full refresh 9.04 : 1 against the solid level 2, partial 21 : 1
  against white, and 16 : 1 against the dither's average `[D]`; it never sits on the white glass, so no pair is filed
  there. The same for the white thumb and the knob, which sit on `surface-on` alone.
- **An offset is decoration and exempt** — `shadow-1`, `shadow-2` and `shadow-3` carry no words and no state, so no pair is filed
  for them, as for `band`. Their averages in linear light against the white page are 1.14 : 1, 1.31 : 1 and 3.5 : 1; level 2
  over the light dither is the same pattern as its ground (1.00 : 1) and the eighth over it 1.16 : 1, which is what the keyline
  is for. A raised surface's boundary is its `spacing.stroke.strong` outline, black on white, the table's 21 : 1 pair `[D]`.
- **A track's own edge is not a 3 : 1 boundary** — the light dither averages 0.75 white in linear light, 1.3 : 1
  against the white glass, so what identifies a toggle, a segment or a slider is a part that is: the black thumb
  (16 : 1 on the dither), the black fill and its white knob, the black chosen item, the ring when focused, and the
  state or the value in words `[D]`.
- **`colors.text-secondary` on a fill has no pair** — 2.82 : 1 on black; words on a fill are `colors.text-on-fill`.
- **A disabled control's words have no pair, by exemption** — WCAG 1.4.3 excuses an inactive component, and the person's
  brief is that it should look inert ("disabled elements are exempted from accessibility contrast"). The ratios are the
  patterns' averages in linear light `[D]`: `colors.text-disabled` (fifteen white in sixteen) on the light-dither face
  (three in four) is 1.23 : 1; `colors.text-disabled-page` (seven in eight) on white is 1.14 : 1; the face against white
  1.31 : 1. They are held by nothing but legibility on the glass `[?]` and by the reason in solid `caption`,
  `text-secondary` — a pair under the floor is never filed in the pairs table, so the design check does not read it.
- **A checkbox's check and a radio's dot are `colors.thumb-on` on `colors.surface-on`** — the table's pair, 21 : 1; an
  off box has no black part, so it keeps a `spacing.stroke.default` `colors.outline` edge, 21 : 1 on the glass, rather
  than rely on the dither alone, 1.3 : 1.

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
  under `typography`, so DESIGN.md writes each role's values out with its primitive named beside it. `caption-strong` is `scale-20-bold`
  (`kTypographyCaptionStrong`): a Wi-Fi network's name, a choice card's name and a select sheet's options, 12 px a character.
  `caption-loose` is
  `caption`'s primitive on a 30 px line (`kTypographyCaptionLoose`), a text view's: the deck's 24 px line on 16 px type
  at its 1.5 em `[DECK]`, as 1.5 em of 20 px.
- **A role's line height is its line pitch** — 72, 60, 48, 36, 36 and 24 px, and 30 for `caption-loose`. `ui::NewLabel` and the screen's style
  set LVGL's line space to the pitch less the compiled font's height, so the lines of a block step by the pitch; a
  single line is the compiled font's height, about 0.9 em — 19 px for `caption` at 20 px, 26 for `body` and `label` at 30,
  52 at 60 — and a block of n lines is n − 1 pitches and one font height `[D]`. A widget's name over a detail is made whole line boxes instead (§9); a free label stays the font's height.
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
### 4.1 The dithers

- **A grey fill is a dither** — `ui::Fill(level)` gives levels 1 and 2 their fill codes (`FillCode`, in
  `glass_rules.h`, with `OneBitWhite`, the pattern each draws; `tests/host` checks both) in LVGL's L8 frame,
  `lvgl_display::kDitherDark` (`0x56`) and `kDitherLight` (`0xAB`), one step above the plain greys, and the one-bit
  packing (`PackMono`) draws them as fixed patterns locked to the panel's pixel grid: level 1 one white pixel in four,
  level 2 three in four. Every refresh keeps them; a greyscale refresh reads the codes as the true greys. Text and
  lines take `ui::Level`, so their greys threshold to solid black or white; an anti-aliased edge pixel thresholds
  too, unless its blend lands exactly on a code, when it draws as the dither — rare, and why words sit only on the
  light dither. Both patterns read as distinct greys, and alike, on the V1.0's and the V2.0's glass `[SEEN]`.
- **Two sparse dithers are the one text drawn as a pattern** — a disabled control's words (DESIGN → Components → States →
  Disabled). `ui::Fill` gives levels 4 and 5 of the token header (`kColorTextDisabled`, `kColorTextDisabledPage`) their codes,
  `kDitherSixteenth` (`0xC1`) and `kDitherEighth` (`0xC2`), and `OneBitWhite` takes the pixel's column and row on the
  panel, not their parity: the sixteenth is black at every fourth column on every fourth row (one pixel in sixteen), the
  eighth at every fourth column on every second row (one in eight), each black pixel one of the light dither's — so
  words on the face are the face with its dots taken out, and the pattern never moves with what is drawn over it.
  The codes sit in the top quarter of the grey range, mid-band, so a greyscale refresh reads both as white (images only,
  where no disabled control stands) and an anti-aliased edge rarely lands on one. The one-bit font draws every stroke
  pixel as the text colour, so a glyph keeps the pattern with no grey edge; `tests/host/disabled_words_lvgl_test.cpp` renders
  `label` and `caption` in the real Bitcount fonts, and the A1 check and speaker-off marks recoloured, on LVGL's L8 frame
  and checks that the only values are the ground's and the code, and the code is on exactly the mark's ink `[HOST]`.
  Nothing of this is seen on the glass `[?]` (§8).
- **A raised surface's offset is its own shape, `spacing.overlay-offset`, 6 px, away — in the dither of its elevation level,
  with a white keyline round it** — drawn by `ui::AddOffset` (`components/ui/ui.cpp`) before the box, which covers all of it
  but the exposed edge: a hard offset, never a blur and never solid black. `kLevels[]` there is the one table of the scale
  (`design_tokens.h`'s `kElevation0` to `kElevation3`): level 0 none; 1 the eighth (`kColorShadow1`, `kDitherEighth`); 2 the light
  dither (`kColorShadow2`, `kDitherLight`); 3 the dark dither (`kColorShadow3`, `kDitherDark`). The keyline is a white rectangle one
  `kSpacingStrokeHairline` larger on every side, drawn under the offset with its radius grown by the same, so a white line rings
  its outer side; the reach, offset and keyline together, is 8 px, which the box's parents let out (`ui::LetRingOut` allows 10 px, the 6 px editing ring's
  reach beside the 8 px of the ring and the offset here).
  The side is the surface's: down and to the right for a floating box (the dialog, `OffsetSide::kBottomRight`), up from the top
  for a sheet that hangs off the glass's bottom and sides (`OffsetSide::kTop`). Each level draws its own pattern at the
  panel's pixel grid, which every one-bit refresh keeps; `tests/host/elevation_lvgl_test.cpp` renders each level on LVGL and
  reads the band, the keyline over a black ground and the untouched box `[HOST]`.
### 4.2 Rows, buttons and pages

- **A list row's padding is two values** — `spacing.sm` above and below, `spacing.md` either side; DESIGN.md's
  front matter carries the vertical one, since a component takes one padding token. Its title and supporting line
  are one line each, 26 px of `label` and 19 of `caption`, so 8 + 26 + 19 + 8 = 61 px, centred in the 80 px row `[D]`.
  Rows are `spacing.md` apart with no rule between: a ring reaches 8 px, so a rule in a 16 px gap would touch it.
- **The ring is outside every control (§9)**, so a button's label has the whole box: a half button `(432 − 16) / 2` =
  208 px wide, 16 px from its neighbour, keeps 202 px inside its 3 px edge for 10 characters of `label` at 18 px a
  character, 180 px; a full-width button keeps 426 px inside its edge for 22 characters, 396 px, 15 px clear a side; a footer button `(432 − 2 × 16) / 3` = 133 px keeps 127 px for 6 characters, 108 px `[D]` (DESIGN →
  Voice & Content Strategy → Labels and states). **The padding is `spacing.md` a side**, which leaves 170 px in the half
  button (208 − 2 × 3 − 2 × 16) and 95 px in the footer button, less than the words. The label is as wide as its words and
  centred, so it sits over the padding and is clipped by nothing — LVGL clips a child to its parent's whole box, not its
  padded content — and never reaches the edge: 11 px from the half button's 3 px edge, and 9 to 10 px from the footer
  button's (127 − 108 = 19, shared by the two sides in whole pixels) `[D]`. A longer label would reach the edge, which is
  why a button's name is shortened, never cut off (the citation above).
- **A page's rows, with a footer: eight 64 px one-line rows (8 × 80 − 16 = 624), six two-line rows of 80 (6 × 96 − 16 =
  560); without one, seven (7 × 96 − 16 = 656)** — the footer is `spacing.footer` 96 `[D]`.

## 5. Refresh and feedback

| Token | Value | What the code does |
| --- | --- | --- |
| `kRefreshPartialLimit` | 20 | Counts partial refreshes; the next update after 20 is drawn full, and a full one resets the count — proven on the SSD1677, and on the SSD2677 no ghost seen before it `[HW]` `[SEEN]`; while the keyboard is open only the frames a typing press or a page switch caused count, and `kUncountedPartialLimit` (60) caps the rest (below) |
| `kFeedbackTickMs` | 100 ms | The buzzer tick sounds as the input watcher reads a press — one poll, 10 ms (`kWatchMs`), after it — even during a refresh: `devices::Tick` plays `sound.tick` (`kSoundTick`), 30 ms of the full square at 4 kHz, through the buzzer's note player, whose task runs above the watcher and cuts whatever sound is playing, so a tick never waits behind another sound ([STICKY_HARDWARE_SPECS §4.2](STICKY_HARDWARE_SPECS.md#42-devices)) |
| `kSoundTick` | 4 kHz, the full square, 30 ms | `sound.tick`: every press and every repeated step — `devices::Tick`, which `lvgl_display::SetPressFeedback` runs |
| `kSoundAtEnd` | 2637 Hz twice, 25 ms each, 45 ms apart | `sound.at-end`: a step that can go no further — `lvgl_display::PlayCue(Cue::kAtEnd)` from the list view, the text view, the horizontal pager and the range slider (below) |
| `kSoundError` | 4186, 3322, then 2794 sliding to 2093 Hz; 600 ms | `sound.error`: that did not work — `devices::Play(sound::Word::kError)`, and the status line's error news (`Cue::kError`) |
| `kSoundSuccess` | 3136 then 4186 Hz on the 25 % pulse; 260 ms | `sound.success`: only once the action succeeded — `devices::Play(sound::Word::kSuccess)`, and the status line's success news (`Cue::kSuccess`) |
| `kSoundRecordingStart` | 3520, then 5274 Hz on the 25 % pulse; 235 ms | `sound.recording-start` — before the recording, never in it |
| `kSoundRecordingStop` | 5274, then 3520 Hz on the 25 % pulse; 235 ms | `sound.recording-stop` |
| `kSoundBandLowHz`, `kSoundBandHighHz` | 2000, 6000 Hz | `sound.band-low`, `sound.band-high`: every pitch between, round the buzzer's 4 kHz resonance; `tests/host/sound_rules_test.cpp` holds the six to it `[HOST]` |
| `kSoundTickMaxMs`, `kSoundMaxMs` | 40, 600 ms | `sound.tick-max`, `sound.max`: a tick's longest and any other sound's, held by the same test |
| `kWaitIndicatorDelayMs` | 1000 ms | Declared for DESIGN.md's rule — a wait's indicator drawn only once the wait has run this long, `motion.wait-indicator`; no screen has a wait indicator yet |
| `kSelfUpdateMinMs` | 5000 ms | The period of the hardware-test screens' self-updates (Sensors, Battery and power) — DESIGN.md's floor between two self-driven redraws of an area, `motion.self-update`, which no code enforces |
| `kClockTickMs` | 60000 ms | Declared for DESIGN.md's clock redraw, `motion.clock-tick`; the firmware has no clock yet |
| `kMotionKeyRepeatDelayMs` | 600 ms | How long UP or DOWN is held before it steps again, `motion.key-repeat-delay` — `kKeyRepeatDelayUs` in `key_rules.h`, which also lets a step that drew nothing be followed this long after it |
| `kMotionKeyRepeatPeriodMs` | 500 ms | How often a held UP or DOWN steps after that, `motion.key-repeat-period` — `kKeyRepeatPeriodUs` in `key_rules.h` |

- **The six sound words are DESIGN.md's `sound.*` tokens, and `devices` plays them by word** (`sounds.h`: `kSounds`, in
  `sound::Word`'s order, `devices::Play(Word)`); the hardware test's Buttons and buzzer screen has a row for each, and the
  console's `sound <name>` plays one by its token's name (`tick`, `at-end`, `error`, `success`, `recording-start`,
  `recording-stop`). The person chose each by ear on a V1.0 from candidates in the agent deck's style.
- **A widget asks the at-end sound by cue, never the buzzer** — `lvgl_display::PlayCue(Cue::kAtEnd)`, which the shell
  sets to `devices::Play(Word::kAtEnd)` (`SetCuePlayer`). The list view and the horizontal pager ask it when a swipe would
  turn past the last or the first page; the text view, open for reading, when UP or DOWN or a swipe would; the range
  slider, open for editing, when a step would pass its end — each repeated step of a held key too. A key's tick has
  sounded already, from the input watcher; the at-end sound follows within one read of the keypad and cuts it, so the
  person hears the tick's start and then the double tick. A swipe's tick sounded at the contact; its at-end sounds at the
  lift. UP and DOWN that walk a list's rows leave it at its end for the next control, as the group wraps, and ask nothing.
  `tests/host/slide_off_lvgl_test.cpp` and `swipe_lvgl_test.cpp` count the cue `[HOST]`.
- **Outcomes sound where they happen** — the hardware test's Microphone screen plays `sound.recording-start` at the press,
  lets the microphone settle at least as long as the sound has left, so the recording never holds it, and closes on
  `sound.recording-stop`, or `sound.error` when the read failed; its Storage screen's Read card says its outcome in the
  status line — "Card read." sounds `sound.success`, "No card. Insert one to read it." and the mount failure sound
  `sound.error` — while its label keeps the facts (the next bullet). The kit page's keyboard and dialog
  are samples with no real outcome — Join with too short a password is a disabled action, which gets the tick alone, and
  a destructive action confirmed gets the press's tick (DESIGN → Motion → Sound) — so they sound none.
- **The status line sounds by cue and refreshes the band alone** — `router::Say` asks `lvgl_display::PlayCue(Cue::kSuccess)`
  or `Cue::kError` just before it sets the band's words, and the shell's cue player (`app_shell`) maps them to
  `devices::Play(Word::kSuccess)` and `kError`; info asks nothing. Held under an overlay, the news sounds when it is drawn,
  as the overlay closes. The band is a fixed 80 px zone (`spacing.list-row-height`, y 44 to 123), so a line shown, replaced
  or retracted is one partial refresh of that zone, 480 × 80, and nothing below it changes — checked on the render, where every
  pixel from y 124 down is identical empty, with one line and with two. Each counts toward `kRefreshPartialLimit`; a line
  that leaves with its screen costs no refresh of its own. The same words again are no refresh, but success and error sound again. The lower
  rule, a 2 px hairline drawn only while news shows, is the thinnest feature on the glass and the one thing a retraction can
  leave a faint trace of; judged on a V1.0 and a V2.0, and an always-drawn rule is the way out `[?]`.
  `tests/host/status_rules_test.cpp` and `status_line_lvgl_test.cpp` hold the rules and the layout `[HOST]`.
- **A full refresh is drawn for**: the first frame after boot or wake; a full a screen asks for (`RequestFull`, the
  hardware-test screens'); the update after `kRefreshPartialLimit` partials; the one-bit frame after a greyscale
  refresh; on the V1.0, the pause of 2 s after 150 frames by scans; and, as a recovery, a partial that fails — drawn
  again in full at once — and the next frame after a failed refresh, settle or standby, since the glass is then
  unknown. In order, with the code that sets each:
  [STICKY_EPAPER_WAVEFORMS §3.4](STICKY_EPAPER_WAVEFORMS.md#34-which-refresh-a-frame-gets). A greyscale full refresh is
  for a still screen showing an image (DESIGN → Colors → The four levels); no product screen has one yet, and the
  hardware-test Four greys screen draws one as a fixture.
- **DESIGN.md also keeps a full refresh for closing a full-glass overlay and for the frame left before sleep** (DESIGN →
  Motion → Nothing animates): the router sets one as a full-glass overlay closes — the keyboard is the one —
  (`router::CloseOverlay` calls `lvgl_display::RequestFull`), and no screen has that last frame yet. A bottom sheet and a dialog
  are part of the page and open and close on a partial, and the router asks no full refresh for them; the ghost the sheet's inverted
  chosen row may leave after its close is judged on the glass `[?]`.
- **While the keyboard is open only the presses that type, and a page switch, count toward the twenty** (the person's decisions): the keyboard sets
  `lvgl_display::SetPartialCounting(PartialCounting::kTypingOnly)` as it opens and `kEvery` as it closes, and calls
  `lvgl_display::NoteTyping()` in each press that changes the text — a character, Space, Delete on a non-empty text, and the field's clear cross, which changes the text as Delete does — and in each page
  switch (Shift, 123, ABC, #+=), which counts as one partial, since the redraw is on the same screen. `Send` reads
  the flag once per frame (`s_typed.exchange(false)`), so a frame that coalesces a ring move and a typing press counts, and one
  of ring moves counts nothing toward the twenty, on a V2.0 and for the V1.0's two-partial page change alike. The arithmetic is `PartialsCounted` and `PartialsOverLimit` in `glass_rules.h`: a frame that counts none is
  never forced full by the twenty, so the 21st typed character is drawn full. The moves that count none are not free: they add
  to a count of their own, `s_uncounted` in `display.cpp`, and the frame that would take it past `kUncountedPartialLimit` in
  `glass_rules.h`, 60 (the person's decision; a start, to be tuned on the glass `[?]`), is drawn full, which resets both counts —
  the same `PartialsOverLimit` arithmetic, so the 61st ring move is the first full. `tests/host/glass_rules_test.cpp` runs
  both rules against a counter `[HOST]`. What the moves leave is the dither and the black rings, which build up between
  those fulls `[?]`: a gate 3 look at the glass after a long entry sets the 60. Opening the keyboard's screen is not a typing
  press; its close is a full refresh.
  **On the V1.0 the ceiling is qualified.** Its default preset is `Scan 3 x 10` (`kDefaultTuning1677`: `scan_frames` 3, `scan_budget` 10), so a frame that
  changes at most 15 % of the glass (`kScanMaxChanged`) — a ring move, a typed character — is drawn by scans, and `Send` returns
  from `SendByScans` before either count: it adds to neither `s_stats.partials` nor `s_uncounted`, a typing press's frame included.
  The bound on a long walk there is the scan rule's own: after `kScanFramesBeforeFull` (150) scan frames the next pause of
  `kFullAfterPauseMs` (2 s) draws a full refresh (the scan bullet below). The 60 holds on a V2.0 and for the V1.0's frames that are
  partials — a page switch, which changes more than the ring does, or a board set to a preset without scans.
- **A new screen on the V1.0 is two partials, through white**, not a full one — 1.0 to 1.25 s against a full's 1.6 s,
  and counted as two toward `kRefreshPartialLimit`, so one that would pass it is drawn full. Ten page changes, then a
  full refresh, left no trace on the glass over a run of 80 `[SEEN]`.
- **Two full refreshes never run back to back** (DESIGN → Do's and Don'ts) — a full or greyscale refresh asked for
  sooner after the last one ended than that one took waits out the rest in `Send` (`WaitAfterFlash`; its arithmetic is
  `FlashWaitMs` in `glass_rules.h`, which `tests/host` checks), then runs with the frame LVGL drew; it is never
  dropped (DESIGN → Motion → Nothing animates). The gap is the last full or grey refresh's own measured time — after a
  full, about 1 s on a V2.0 and 1.6 s on a V1.0; after a grey, the grey's time, 1.25 s on a V2.0 and 1.31 s on a V1.0
  (STICKY_EPAPER_WAVEFORMS §4) — so it follows each controller and the temperature without a constant. Leaving Four
  greys straight after it lands held the full 546–607 ms on a V2.0 and 587–852 ms on a V1.0, and two Full presses held
  the second 1443–1523 ms on a V1.0 `[SEEN]`. A press during the wait still ticks and is replayed after it, in the
  frame after the held one. A failed refresh sets no gap, since it flashed nothing, so a retry never waits on a busy
  timeout; the full that redraws a failed partial runs at once, a recovery, rare, and the one exception.
- **Input during a refresh is kept** (DESIGN → Motion → Press) — a task of its own, above LVGL's, reads UP, DOWN, OK
  and the touch panel every 10 ms, ticks on each press, and queues each press and release; LVGL replays the queue when
  the refresh ends, so a short press inside a refresh is never lost and moves focus once. An OK pressed before the frame
  the last OK caused has been drawn is ignored, though it ticks; a queued OK acts where the moves queued before it
  left focus, and a double tap is replayed as two taps (the person's choice, from the glass). A press
  or a move is queued only with room left for its release, so a release is never lost; holding OK for 1 s runs the escape
  chain ([DESIGN → Focus and input](../DESIGN.md#focus-and-input)), once a hold, whether or not LVGL saw the hold begin (`OkHold` in `key_rules.h`). The tick sounded at once during a full refresh `[SEEN]`.
- **A held UP or DOWN steps again by `KeyRepeat` in `key_rules.h`, at the glass's pace** (DESIGN → Focus and input; the
  person's decisions: 600 ms, then every 500 ms, and every step ticks). LVGL's own repeat is off —
  `lv_indev_set_long_press_time` and `lv_indev_set_long_press_repeat_time` at `UINT16_MAX` on the keypad — since it stepped
  every 100 ms after 400 ms and turned a control's editing off on each repeat. When the queue is empty and the key is still
  down by the watcher's word too (`s_step_held`), the keypad read asks `KeyRepeat::Step`: a step is due once the key has been
  held `kMotionKeyRepeatDelayMs`, `kMotionKeyRepeatPeriodMs` after the last step, with the SSD1677's scans no longer driving
  the last frame (`RefreshBusy`) and the last step's frame landed (`LastRefreshEndUs` after it) — or, for a step that drew
  nothing, `kMotionKeyRepeatDelayMs` after it. A frame being drawn needs no test of its own: the flush runs on LVGL's task,
  the task that reads the keys, so no read runs during one, and the landed-frame test is what holds a step back until its
  frame is on the glass. The read then sends a release and a press, the press
  through `KeySeen` and the tick, and mapped to the editing keys when a control is open for editing. The release stops it
  at once and nothing is queued; OK never repeats (`KeyRepeats`). The last step is timed from when LVGL acted on it, not from
  when the watcher saw the press, so a press replayed after a refresh waits for its own frame. `tests/host/key_rules_test.cpp`
  checks the delay, the period, both gates, the no-frame wait, the release and one escape per hold `[HOST]`.
- **With no ring on the glass the first press only rings** — the rule is [DESIGN → Focus and input](../DESIGN.md#focus-and-input), the mechanics the router's key read hook
  ([APP_ARCHITECTURE §7](APP_ARCHITECTURE.md#7-screens-routes-and-focus)). On the glass: the press ticks at once, as every press does, and its frame is the ring's partial. `ok_gate` in
  `ReadKeys`, which drops an OK pressed before the last OK's frame has landed, treats that frame as the OK's, so an OK pressed
  once the ring is on the glass acts. A held UP or DOWN whose press only rang steps at `kMotionKeyRepeatDelayMs`.
- **An open overlay holds the screen under it, by DESIGN.md's rule** — what changes underneath is kept in state and
  drawn when the overlay closes, so the overlay's own refresh is never queued behind the page's. The select sheet, the dialog
  and the keyboard are overlays on the router's stack (APP_ARCHITECTURE §7): the screen under one hears `OnCovered(true)` as the
  first opens and `OnCovered(false)` as the last closes, and the hardware-test screens that update themselves (Sensors, Battery
  and power) pause their timer meanwhile and, uncovered, read at once (`lv_timer_ready`).
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
- **A component reads the header through two layers, as the agent deck's do (§9)** — `components/ui` is the theme (fonts,
  styles, `Level`, `Fill`, the one focus ring by `RingOutside`, `RingEditing` and `LetRingOut`), and `components/widgets`
  builds DESIGN.md's kit from it; a widget's sizes are its tokens (`kSpacingTarget`, `kSpacingToggleWidth`, `kSpacingMark`),
  never a literal, so a widget sized for 235 ppi keeps the size the kit gives it in millimetres.

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
| A focus ring's outer corner: the host render measures radius 16 round every rounded control (a value row, a radio row, a choice card, a facts page), where DESIGN → Shapes gives the control's radius plus the gap out to it, 12 + 4 + 4 = 20 — whether LVGL's outline radius counts the offset and the stroke | Open | The code made to draw 20, or the rule restated as drawn, by the person's choice |
| Generating the token header's symbols from DESIGN.md's front matter, instead of by hand | Open | A second change to the tokens, which makes a generator worth it |
| A script that compares DESIGN.md's front matter with §2's tables | Open | The token header, which a third copy would make worth checking |
| The glyph ranges beyond ASCII and · ° × – — ’ • … — a name with an accent, a currency — and what a character outside them draws: LVGL's placeholder box is on (`CONFIG_LV_USE_FONT_PLACEHOLDER`) | Open | An out-of-range character drawn on the glass; the first strings that need more |
| The glass's real white and black, against the levels' nominal 21 : 1 | Open | A reading with a light meter on a V1.0 and a V2.0 board |
| Whether a disabled control's words, in the two sparse dithers, read on a V1.0 and a V2.0 (1.23 : 1 and 1.14 : 1 against their ground), and whether a disabled checkbox's check and speaker-off glyph are legible at all | Open | The kit page's disabled samples, seen on both boards |
| Whether an offset reads on the glass: elevation 2's light dither over a dithered field, the eighth (level 1, no surface yet), and the 2 px keyline, on a V1.0 and a V2.0 (§4.1) | Open | Open and close a sheet ten times over the disabled Alert field and look at the offset and where it was |
| Whether the sheet's close on a partial leaves a ghost where its inverted chosen row stood, and whether the keyboard's ring moves, uncounted, leave one (§5) | Open | The kit page's dropdown and keyboard samples walked ten times on a V1.0 |
| The unlit bars of the signal glyph, 3 to 4 px arcs in a dither, read as faint grey strokes on the glass (DESIGN → Exceptions) | Open | The kit page's network rows seen on both boards; the way out is a 2 px outline |
| The text field's eye and clear button, square inside the field's `rounded.xl` corner, and its ring's white gap to the outline, read on the glass (§11.4) | Open | The kit page's text and password inputs seen on both boards |
| Whether the at-end sound cutting a key's tick, and the outcome sounds, read on the board — heard through a refresh in flight | Open | The Buttons and buzzer screen's rows, a list, a slider and the text view walked to their ends on a V1.0 and a V2.0 |
| Whether the status line's lower rule, a 2 px hairline drawn only while news shows, leaves a faint trace in the band when news is retracted, and whether it is noticed as plain words with no box (§5) | Open | The hardware test's Storage screen: Read card with no card, then with one, ten times on a V1.0 and a V2.0 |
| The swipe's thresholds, 64 px, and a flick of 24 px with 12 px in the last 100 ms (§10) — proposed, never judged under a finger | Open | A device walk on the glass: a slow long swipe, a flick, a short flick under the limit, a pull-back, a rest before the lift |

## 9. The ring outside and the ported widgets

- **The agent deck** is the author's other firmware — a 480 × 320 landscape status panel for coding agents on a 166 ppi
  glass — whose UI kit FolloUp ports; this doc owns the ported sizes, and the rest link here. Its source is the
  separate `claude-agent-deck-3` repository, which this repo does not hold and a reader of it cannot open:
  `components/widgets/` (each widget's code), `components/theme/` (its tokens) and `components/icons/source/` (the
  artwork), read by the author on 2026-11-20, with the author's own reference pictures of every widget from the deck's
  simulator, kept on the author's machine. A widget is ported by its shapes, proportions and
  layout, never redrawn: the deck's px × 1.416 (235 / 166), rounded to a token, its tones mapped to black, white and the
  light dither in the order of their strength, its motion left out. Every number in the table below is `[DECK]` in its
  second column and `[D]` in the third and fourth.
- **The one focus ring is outside every control**, `spacing.focus-offset` clear of its outer edge — its outline
  included — with white between, the outline's radius the control's plus its pad and width (LVGL's `outline_pad`):
  a button, an icon button, a chip, a row, a card, a text view, a toggle, a checkbox, a radio and a range slider, black on the white glass, whether the
  control is chosen or not. It reaches 8 px, so stops sit `spacing.md` apart and each container above one lets it out
  (`ui::LetRingOut`, an `ext_draw_size` of the larger reach) or pads it: the list view's band is 8 px wider and taller
  than its rows all round, which is the ring's reach, and the footer's 16 px over and under its buttons keep it 8 px
  from the glass's edge `[D]`. A control open for editing — a range slider, a roller, a text view being read — draws it at
  `spacing.stroke.edit`, 6 px, so it reaches 10 px, 14 px from the glass's edge on a 24 px margin, and a name 16 px
  above a slider leaves 6 px of line box and 9 of glyph clear of it `[D]`. A segment's ring is round the focused item
  inside its track: the track's `spacing.sm` inset is the ring's reach (4 + 4), so the ring ends on the track's edge and
  a neighbour's gap, 8 px, holds it, the track's `rounded.xl` 16 concentric with the item's `rounded.md` 8 and the
  ring `[D]`. A chosen control keeps the black ring, with 4 px of white between.
- **The deck's one base radius, 10, is `rounded.lg` 12 everywhere it appears** — buttons, chips, rows, cards, a text
  view's ring, a range slider's track; its small radius 5, a checkbox's mark, is `rounded.md` 8; its pills — the
  toggle, the progress and step pieces, the scroll indicator's, the page dots, the radio mark — are `rounded.full`.
- **Every ported widget, the deck's number → scaled ×1.416 → FolloUp:**

| Widget | Deck px `[DECK]` | Scaled `[D]` | FolloUp px | Token |
| --- | --- | --- | --- | --- |
| Toggle | track 66 × 30, thumb 36 × 24 inset 3 | 93 × 42, 51 × 34, 4 | 96 × 44, 52 × 36 inset 4 | `toggle-width`, `toggle-height`, `toggle-thumb-width`, `xs` |
| Segment | track 252 × 48 → 357 × 68, items inset and apart 6 | 8.5 | 64 high, items 48, 8 round and between | `target`, `sm`, `rounded.md` / `xl` |
| Range slider | 36 high, fill 30, knob 6 × 24 | 51, 42, 8.5 × 34 | 52, 44, 8 × 36 | `slider-height`, `xs`, `sm` |
| Standard button | 150 × 44, edge 2 reserved on every variant, words 12 in | 212 × 62, 2.8, 17 | 212 × 64, edge 3, padding 16 | `button-width`, `target`, `stroke.default`, `md` |
| Chip, compact button | 48 high, pad 18, edge 3 (chosen) | 68, 25, 4.2 | 64 high, pad 24 (a compact button 24 and its 3 px edge), edge 4 on a chip | `target`, `lg`, `stroke.strong` |
| Progress bar | stroke 9 (5-hour 12), gap 6 | 13, 17, 8.5 | 12 (16), gap 8 | `progress`, `progress-lead`, `sm` |
| Step bar | stroke 9, gap 8 | 13, 11 | 12, gap 12 | `progress`, `step-gap` |
| Page dots | dot 6, current 20 × 6, gap 6 | 8.5, 28 | 8, 28 × 8, gap 8 | `page-dot`, `page-pill`, `sm` |
| Value row | 36 (48 in a list), words 12 in | 51 / 68, 17 | 64, words 16 in | `target`, `md` |
| Detail row | name 16 over value 18, 6 pad, 55 | 78 | 76 (8 + 24 + 36 + 8) | `sm`, `caption`, `label` |
| Radio row | 60 high, words 14 in, mark 30 (edge 3, dot 18) | 85, 20, 42.5 | 80 at least (4 + 36 n + 4: 80, 80, 116, 152 for one to four lines), 20, 42 (edge 4, dot 26) | `list-row-height`, `card-inset`, `mark`, `mark-dot` |
| Choice card | min mark + 24, words 14 in, gap 12 | 76, 20, 17 | 74 (`mark` + 2 × `md`; 80 with a name over a detail, 24 + 24 + 2 × `md`), 20, gap 16 | `mark`, `card-inset`, `md` |
| Roller | — (the person's words: a stepped picker, FolloUp's own) | — | three rows of 64, as wide as its widest value and 24 either side (84 for two figures), outline 2, open band 72 × 64 | `target`, `lg`, `stroke.hairline`, `xs` |
| Carousel | — (BYTE-90's carousel slid its cards; FolloUp turns them in place) | — | a card 432 wide filling the height, footer slots 133 wide and 16 apart | `content`, `card-inset`, `stroke.default`, `md` |
| Icon button | tile 54, glyph 30 (56 %) | 76, 42 | 64 (the hit-area floor, drawn and hit alike), glyph art 40 (62 %) | `target`, `icon` |
| Glyph beside words | icon 24, gap 9 | 34, 13 | art 40 (ink 18 to 33), 16 of white from its ink to the first letter | `icon`, `md` |
| Checkbox, radio | — (the person's words: like the switch) | — | 44, `rounded.md` or full, edge 3 off, dot 16 | `check-size`, `check-dot`, `stroke.default` |
| Scroll indicator | lane 6 × 176, gap 6, 7 from the glass | 8.5 × 249 | 8 × 248, gap 8, x 467 | `scroll-thumb`, `scroll-length`, `scroll-x`, `sm` |
| Status block | mark 60, column gap 16, heading 22 over 16 | 85, 23 | mark 85, gap 24; a block with no mark leaves the slot empty, its words `spacing.lg` in (x 48); heading `label` 30 over `caption` | `status-mark`, `lg`, `sm` |

  The deck's type sizes — 16, 18, 22 — are `caption` 20, `label` 30, `label` 30: the scale has no 25. Its names are
  `label_xs` and `label_sm`; its medium and bold are weights 600 and 700.
- **A name over a detail stands on line boxes** (a toggle row's name over its reason, a status block's heading over its line,
  a home row's title, a detail row, a choice card): `ui::FitLineBox` pads a label above and below by its role's pitch less
  the compiled font's height, half each with the odd pixel above, so a one-line `label` is 36 px, a `caption` 24 and a
  block of n lines n pitches; a dotted label is one line box tall. A detail row's name and value and a radio row's words wrap on
  the same boxes (`widgets::detail::WrapWords`, shared with the choice card, takes no colour of its own so a disabled row's
  `text-disabled` reaches it): a radio row is 4 + 36 n + 4 high, at least 80 — 80, 80, 116, 152 for one to four lines — and a detail row
  8 + 24 a + 36 b + 8; the host render drew every wrapped frame pixel for pixel as the draft did `[HOST]`. `widgets::detail::Line` and `Words` and `ui::NewRow`
  use it, and `tests/host/fit_line_box_lvgl_test.cpp` checks it on LVGL's real label code for `label`, `caption` and
  `caption-loose` — odd and even spare pixels, dotted and wrapped `[D]`. The white between a name and its line is the spare
  room under the name and over the detail, plus any gap: `label` over `caption` 5 + 3 px, a card's bold `caption` over its
  `caption` 3 + 3. A toggle row takes `spacing.xs` between its name and its reason, only while the reason shows, so the row
  is 64 px with it, and there is no gap when it hides. On the host render `[HOST]`, a home row's title and
  detail stand 8 px of white apart, a card's name and detail 6, a status block's heading and its line 22, a disabled toggle
  row's name and reason 12 and a detail row's 13. Heights: a row is 80, a toggle row 64 — its name, state word and toggle centred in it, the toggle 10 px above and below on the host render `[HOST]` — a choice card with a detail 80, a
  detail row 76 (8 + 24 + 36 + 8) and a status block's text 68. A free label on a screen — `ui::NewLabel` alone —
  stays the font's height.
- **The scroll indicator is the deck's lane**, a `spacing.scroll-thumb` wide stroke of separate round-capped pieces — a
  short track piece in the light dither, `spacing.sm` of white, the black thumb, `spacing.sm` of white, the rest of the
  track — `spacing.scroll-length` long at most, its left edge `spacing.scroll-x` on the glass: 1 px clear of the 6 px
  editing ring round a full-width view (24 + 432 = 456, + 4 + 6 = 466), 5 px from the glass's edge. The thumb
  (`surface-on`) is the view's share of the length, never under a dot. Where it stands is DESIGN → Components → The kit's rule;
  the hold at the ends is the deck's `draw()` `[DECK]`, 16 px here (a gap and a dot), and the even spread starts past 15
  pages at 248 px. `widgets::detail::ThumbAt` (`src/internal/scroll_rules.h`) is the rule, and
  `tests/host/scroll_test.cpp` checks it for 1 to 60 pages `[D]`; everything fitting hides the indicator. **Where it is centred:** on the glass's height
  (400), the same on every page, for a list that fills the content column — Home's — and on
  the view itself for a view in a page that holds more, the kit page's, where the glass's middle is not the view's;
  `widgets::IndicatorCentre` says which. The lane stands 19 px past its host's right edge, more than a ring reaches, so
  the host's `ext_draw_size` is widened to it: LVGL clips a child to its parent's area plus that size, and the lane would
  not draw at all `[D]`.
- **The three status marks and the choice card's check are the deck's artwork at one bit** — `components/widgets/marks/make_marks.py` box-averages
  the 128 × 128 art, thresholds it at half and packs it as LVGL's A1, so a mark draws solid in the ring or the box that
  holds it: a check 26 × 18 in a checkbox's mark, the status block's check, clock and link-off 57 × 41, 73 × 73 and
  73 × 68. FolloUp has no icon set; the person's decision was the status block's three marks, the choice card's check is a fourth, and these four are the widgets component's alone, each beside its word (DESIGN →
  Components → Icons).
- **The sixteen glyphs are BYTE-90's art at one bit** — the ten below on the 40 px art box, and six more cut to a box each from the
  same PNGs (`make_marks.py`'s `crop`, no trim, so the three signal glyphs register over one another): the signal in three
  strengths, 40 × 30 each, cut from their shared ink box (3, 21) to (125, 111) of the 128 canvas; the padlock 19 × 24; a
  check 28 × 20 for a row's facts line; and the dropdown's chevron 14 × 28. Each equals the approved draft's pixels byte for byte
  `[D]`. Every stroke is 3 to 4 px (the padlock 2 to 3), none under `spacing.stroke.hairline`. The ten:
- **The ten glyphs are BYTE-90's art at one bit on the 40 px art box** (`spacing.icon`), made by the same script from
  `byte90-series3/assets/icons` (a second repository this repo does not hold): an arrow 27 × 26 ink, a cross
  22 × 22, play 18 × 22, pause 24 × 26, stop 22 × 22, the speaker 32 × 28 and with a slash 33 × 34, a check 26 × 20, the
  microphone 24 × 33, a refresh 28 × 28 — `Glyph` and `IconAction` in `include/widgets/glyph.h`, and a glyph outside the
  closed set of DESIGN → Exceptions → The icon button does not compile as an `IconAction`. Every ink pixel but a few diagonal
  join pixels is in a 2 × 2 block, so no stroke is under `spacing.stroke.hairline` `[D]`. A button's glyph is a child
  image recoloured to its words' colour (`detail::NewMark`), and from the disabled state it takes `text-disabled`, which
  the button passes down (`LV_OBJ_FLAG_STATE_TRICKLE`); the ink to the first letter is 15 px of white on the host render, the
  font's glyph offset taking one of the 16 `[HOST]`. A half button holds 7 characters beside a glyph (28 + 16 + 7 × 18 = 170)
  and `Refresh`, the widest, fits.
- **A disabled control is one rule built in one place** — `detail::DitherWhenDisabled` (`src/internal/base.cpp`): the face
  the light dither, the reserved edge transparent (its width kept, so nothing moves) and the words `text-disabled`, which a
  child label inherits; a rim object (`DisabledRim`) is gone. The black parts are white parts knocked out of the face — a
  toggle's thumb on either end, a chosen segment item, a slider's fill — and what stands on a white part is
  `text-disabled-page`: a chosen segment item's word, a slider's knob and its value while it fits in the fill (on the
  track it is `text-disabled`), and, as a build decision the draft left open, a radio row's or choice card's dot or check on
  its white mark, which has no edge. A row's own name and state word are `text-disabled-page` (a setting row sets each
  word's colour itself, since a label's own colour outranks its parent's disabled style); the reason stays solid `caption`.
- **A focused control comes into view at once, by the theme (`ui::ShowWhenFocused`)** — LVGL's own scroll on focus
  animates, which the glass would draw frame by frame, so `NewButton`, a tappable `NewRow` and every widget's
  `JoinGroup` take `LV_OBJ_FLAG_SCROLL_ON_FOCUS` off and scroll the pages round the control without animation, then the
  nearest page that scrolls a little further, so the ring (4 + 6 px, open for editing) is in view too. A list view's rows
  name their list, so the page behind brings the whole list into view and the list turns its own pages: a row never
  scrolls the band `[D]`.
- **BYTE-90's network row, dialog, dropdown and pager are built from the approved drafts**, each from the repo's own rows and
  buttons, in tokens: the **network row** (`widgets::MakeNetworkRow`) is the home row's shell — 80 px, `rounded.lg`, 16 either side
  and 8 above and below — with the name in `caption-strong` 344 px wide (the row's 400 inside its padding less the 40 px signal
  glyph and a 16 px gap: 28 characters, then three stops), the facts line in `caption` 24 px high with `spacing.sm` between a
  glyph and its word and `spacing.lg` before the second fact, and the signal glyph — a 40 × 30 canvas, alone and with no word
  (DESIGN → Exceptions → The signal glyph without a word), centred on the row's height — floating at the right end; the unlit bars are the strongest glyph recoloured `ui::Fill(band)` (the dark dither on the
  inverted row) under the lit glyph on the same canvas. The **dialog** (`MakeDialog`): outline 4, `rounded.md`, `spacing.lg` inside,
  `label` title and `body` message, two half buttons 180 px wide `spacing.md` apart. The **dropdown** (`MakeDropdown`) is a value row
  with the chevron `spacing.md` after the value, the reason under it a page gap while disabled. The **horizontal pager**
  (`MakeHorizontalPager`): a band 448 × 80 round a strip of 64 px items, whole items to 432 px a page, the next page a full view
  (448 px) on so no item peeks at the edge, and "1 of 2" alone, in `caption`, `text-secondary`, 20 px, left-aligned at the strip's left edge, 16 px under the chips — no page dots;
  its host test checks the words, their place and that nothing else stands there `[HOST]`. The host renders beside each draft are in the task's scratchpad, not the repo.
- **The roller is built from the pure rules in `roller_rules.h`** (`widgets::MakeRoller`, `MakeNumberRoller`; its behaviour is DESIGN → Roller):
  a white face `rounded.lg` 84 × 192 for two figures, three labels each centred in its 64 px row, the hairline outline a floating child
  over the face so the rows are placed from its top, and the open band, `surface-chosen`, `rounded.md`, 72 × 64 at x 6, behind the middle
  row while the group edits. A step changes three labels' words and nothing else, 232 px on the host render `[HOST]`. Mechanics that a
  change must keep: a press does not focus it (`LV_OBJ_FLAG_CLICK_FOCUSABLE` is off — LVGL's click focus ends an edit even on the control
  already focused, which would close it under a tap on a row), and its arrow scroll is off (`LV_OBJ_FLAG_SCROLL_WITH_ARROW`, which would
  scroll the rows a quarter of their height on every step); a transparent part wider than the roller gives the swipe's claim somewhere
  to scroll; the group's `LV_STATE_EDITED` is the one truth that opens it, shows the band and the footer's bar, and closing it reports one
  change when the value is not the one it opened on. The footer's bar (`MakeRollerFooter`) is bound with `SetRollerFooter`, hides only
  the footer's parts that were showing and shows those again, and forgets a roller that is deleted while open. `tests/host/roller_rules_test.cpp`
  and `roller_lvgl_test.cpp` hold the rules and the widget on LVGL's real input, and the Clock screen's set (`hw_test/clock_rules.h`) `[HOST]`.
- **The carousel is one card turned in place** (`widgets::MakeCarousel`; its behaviour is DESIGN → Carousel): the card is 432 × 588 on the
  Welcome screen — the content column's width and the rest of its height — its words 20 px in from the card's edge (17 inside the outline);
  under it "2 of 4" in `caption` and the page dots `spacing.md` after, and the footer's three slots, each 133 wide and `spacing.md` apart, at
  x 24, 173 and 322 — Back in a slot of its own, so hiding it on the first card leaves it empty. A turn sets the card's two labels, the words,
  the dots, Next's label and Back's visibility in one LVGL frame, so a V2.0 draws one partial; hiding Back hands the ring to Next
  (`router::SetHiddenHandingFocus`). The swipe is the pages' horizontal claim on the card, which has an invisible part wider than itself for
  the drag to go to. Holding OK leaves through `router::AttachEscape`, which the screen's `OnCreate` calls and the router runs after the
  overlays and before going back, so it works at the root. The host render of the built screen is pixel for pixel the draft redrawn with Skip
  outlined `[HOST]`. `tests/host/carousel_lvgl_test.cpp` drives it with a scripted finger and the buttons, `carousel_escape_lvgl_test.cpp` holds OK at
  depth 1 and 2 `[HOST]`. The hardware test's "Welcome" screen shows it with four cards of illustrative copy.
- **A row's content is centred on its height** — one line or more (DESIGN → Components → The kit). LVGL sizes a flex track to its
  tallest child, so a row's `min_height` alone pads below its content; the pattern that holds is a content column that takes the
  row's height and centres its lines (`setting_row.cpp`, `select_sheet.cpp`), and a row with a fixed height centres its track
  (`value_row.cpp`, `ui::NewRow`). `tests/host/row_centring_lvgl_test.cpp` builds every row kind with one line of content and with
  more on LVGL's real layout and fonts and asserts the content block's centre is within 1 px of the row's: toggle, toggle icon,
  checkbox and radio button rows (one line, two, disabled with a reason), value rows, the dropdown, home rows, radio rows, choice
  cards, radio rows of one to four lines and detail rows of one to three, a list view's band cut to its page's rows, network rows (in a list view too) and the select sheet's options (one line, two, three) `[HOST]`. Every row measured an offset of 0 px on
  2026-11-21, and an option row whose column lost its minimum height measured −12 px — the test sees it.
- **A page with no row in the focus group has a stop of its own** (`list_view.cpp`'s `BuildStops`, called by `UpdateListView`; DESIGN →
  Components → The kit). The stop is a `detail::Bare` child of the list's frame — not of the band, so a row's index is still its place in
  the pages — with `rounded.lg` corners and `ui::RingOutside`, joined to the group by `detail::JoinGroup(stop, frame)`, so the page
  brings the view and the ring into view as a row's does; it draws nothing at rest. It is never hidden, since LVGL's focus step skips a
  hidden object: while its page shows it has the box of the page's rows (`ShowStops`, from the first row's top to the last row's bottom,
  in the frame's own coordinates, 432 × 480 for five facts of the About screen's and 432 × 352 for four) and every other stop is 0 × 0, so
  none of it is ever on the glass and the router's "first control in view" never lands on it. Its `LV_EVENT_FOCUSED` turns to its page
  (`OnStopFocused`), as a row's does. It takes no touch (`Bare` clears `LV_OBJ_FLAG_CLICKABLE`: a swipe goes to the band as before), OK sends
  it a click nothing hears, and `LV_OBJ_FLAG_SCROLL_WITH_ARROW` is off on it and on the band. LVGL has a swap and no insert, so
  `PlaceStop` adds it at the end of the group and swaps it down to its page's place — before the first row of a later page that takes
  focus, else after the last row or stop before it. A second `UpdateListView` makes the stops anew — the old ones leave `State::stops`
  before they are deleted, since deleting a focused one refocuses another whose handler reads it — and a list of facts only puts them back
  where its first old stop stood; the first call of such a list has nothing to stand by and adds them at the end, so it is made before the
  footer's Back. A swipe moves the focus from a row or stop of the list to the new page's first row that takes focus, or its stop
  (`FocusOnPage`). `PageWords::kUnder` adds a `caption` `text-secondary` label to the frame at `spacing.md` under the view's whole height and
  makes the frame that line taller, only while there is more than one page; `ListView::words` is nullptr when not asked. The About
  screen's host render beside the draft is pixel for pixel the same on all fifteen frames `[HOST]`; the real About, twelve facts, is two pages of six, 536 px each. `tests/host/facts_page_lvgl_test.cpp`
  walks a facts screen of two pages and a mixed list of three by UP and DOWN, round the wrap, and checks the box, the ring's width, reach and
  colour against a button's, OK doing nothing, the words, that LVGL's arrow scroll stays off (the same key does scroll the band with the flag put back),
  and a second `UpdateListView` after the footer and with a stop focused.
- **A radio row's and choice card's group is the siblings that share a parent** (`choice_card.cpp`'s `detail::MakeChoice`, shared by
  `radio_row.cpp`): the one `LV_EVENT_CLICKED` listener, added first so a caller's own sees the group settled, carries a tag as its
  user data — one-of-a-set or any-number — and a sibling is told by finding that listener, so the object's own user data stays the
  caller's; it chooses it: a disabled one is ignored, the chosen
  one sends nothing, another clears the group's other tagged children and then sends `LV_EVENT_VALUE_CHANGED` on itself; a card of any
  number flips and sends each time. `SetRadioChosen` and `SetChoiceChosen` set the state, clearing the group when they choose, and send
  nothing. The hardware test's hand-clearing of both is gone; its Reminders screen listens for the change and says the choice in the
  status line (`router::Say`). `tests/host/choice_group_lvgl_test.cpp` drives OK and a tap through the real input.
- **The kit page shows every widget on the glass** (`UI kit` on the hardware-test home): buttons, buttons with a glyph,
  icon buttons, toggle rows, toggle icon rows, checkbox rows, radio button rows, rows, chips, segments, progress, step and
  page dots, radio rows, choice cards, sliders, a paged list view and text view, Wi-Fi network rows, the failed-join dialog,
  dropdowns that open the select sheet (three options, eight with a long one, and a disabled one), rollers (Count stops, Hour goes round, with the footer's Cancel while one is open), a horizontal pager of chips (the carousel has its own Welcome screen),
  the keyboard opened from two buttons, status blocks — each with a disabled sample where it can be disabled. `tests/` has no check of the widgets, only of the dither's words (§4) `[D]`; the host render of
  each beside the draft is the gate-2 picture.

## 10. Paging and the swipe

### 10.1 Pages

- **A list's pages are whole rows that fit its view** (`list_view.cpp`'s `Repage`): the band's view is `rows_per_page` rows of the
  nominal height and the gaps between them, and each page takes the rows that fit it, the next page starting with the next row.
  Rows of the nominal height make `rows_per_page` of them a page, as always; a row that grew with its words (a select sheet's
  third line) takes the room of more than one and a page holds fewer. The room under the last page's last row is what its first
  row's top and the view leave, so every page is as tall as the first.
- **A horizontal pager's pages are whole items that fit `spacing.content`** (`horizontal_pager.cpp`): the band is
  `spacing.content` + 2 × `spacing.sm` wide, each page starts a band's width after the last one's start (a margin on its first
  item) so no item of the next page shows at the edge, and a turn scrolls the band to `page` × 448 px at once.

- **A list view and a text view hold whole pages and turn them whole** (DESIGN → Layout → Paging): the band is a
  scrollable LVGL object with room added under its last page, so every page is as tall as the first and nothing ever
  scrolls back to fill the glass; a page turn is a jump of the band's scroll to the page's first row or line, drawn in
  the next refresh. A list's page is `rows_per_page` rows, `spacing.md` apart; a text view's is its `lines` of
  `caption-loose` less one, the last line of a page the first of the next.
- **A text view's words stand `spacing.sm` inside its view on every side.** The view is `lines` × 30 + 2 × 8 px tall and
  the full 432 px wide; the band that holds and clips the words is the inside of that, 416 px wide and `lines` lines
  tall, at an 8 px inset — a band that is the view itself would show a part of the next line in the inset. The band is
  the focus stop and draws the ring `spacing.focus-offset` + 8 px out from its own edge (`ui::RingOutside`'s `inset`), its
  corners `rounded.lg` less 8, so the ring stands where it would round the view, the focus ring's radius and weight
  unchanged: 4 px, or 6 px open for reading. The first glyph is 8 px inside the view, 12 px from the ring's inner edge and
  16 px from its outer edge when focused (18 open) `[D]`. At 416 px of 12 px characters a line holds 34 characters, and
  the content's 660 px hold 21 lines of 30 px with the two insets, 20 new lines to a page; the view's box and the ring
  beside it are brought into view as one (`ui::ShowWhenFocused`'s `show`), and the scroll indicator stands 1 px clear of
  the 6 px ring `[D]`.

- **A horizontal swipe is the same decision with the axis swapped** — `ClaimSwipe(band, turn, SwipeAxis::kHorizontal)` reads the
  band's `scroll_x`: a finger moving left is positive, so it turns forward, and the pure rule in `swipe_rules.h` is unchanged and
  tested by the same `swipe_test.cpp`. Only the band's own axis is taken (`LV_OBJ_FLAG_SCROLL_CHAIN_HOR` off): a vertical drag that
  starts on the strip chains to the page behind, which is the page's.

### 10.2 The swipe's decision

- **A swipe is read on lift, by `widgets::detail::ClaimSwipe`** — the band is made the object a vertical drag claims, so
  the page behind never scrolls by a pixel (no scroll chaining, no momentum, no gesture bubbling; the touch check's
  `touch-exception` marks that line). LVGL reads a drag after its own `LV_INDEV_DEF_SCROLL_LIMIT`, 10 px. Each move is
  logged with its time and the net travel so far, finger up positive, and the band is put back where the page keeps it at
  once, so nothing moves under the finger and the page lands in one refresh after the lift. LVGL's `LV_EVENT_GESTURE`
  is not used: it does not fire while another object scrolls, and the kit page scrolls by pixel beneath its views.
- **The decision is `PageTurn` in `components/widgets/src/internal/swipe_rules.h`**, a pure function tested on the computer
  by `tests/host/swipe_test.cpp` (`bash tests/host/run.sh`). The thresholds are in the token header, proposed and to be
  judged under a finger on the glass `[?]` (§8):

  | Rule | Value | Token |
  | --- | --- | --- |
  | A swipe commits by distance | the net travel is at least 64 px, one target, however slowly | `kTouchSwipeDistance`, from `spacing.target` |
  | or by a flick | the net travel is at least 24 px **and** the finger moved at least 12 px the same way in the last 100 ms before the lift | `kTouchFlickDistance`, `kTouchFlickMovement`, `kTouchFlickWindowMs` |
  | The window slides | it is the movement between the position 100 ms before the lift and the position at it, from a ring of the latest 32 moves (time, net travel); a move exactly 100 ms old is inside | `Drag`, `MovedWithin` |
  | A finger that rested places | nothing moved in the window, so there is no flick, however fast the drag was before it | |
  | A pull-back places | the net travel counts moves back, so a drag pulled back under 64 px turns nothing, and a flick the other way is not this swipe's | |
  | One swipe, one page | however far or fast; up is +1, the next page, and down is -1 | |

### 10.3 The lift, the ends and who owns a drag

- **The lift is `LV_EVENT_SCROLL_THROW_BEGIN`, not the end of the scroll** — LVGL sends it to the scrolled band as the
  finger leaves, inside the touch read, so the window ends at the lift. `LV_EVENT_SCROLL_END` comes later, from the throw's
  animation callback, after whatever the task was doing — a refresh of a second or more — and would read a flick as a finger
  that rested. The drag's moves are read only while `lv_indev_active()` holds this band as its scroll object, so a key's or
  a focus's scroll, and the scroll LVGL makes from that animation callback, where there is no active input device, are
  never a drag `[D]`.
- **The swipe's own scrolls are left alone** — the put-back under the finger and the page turn are made with
  `lv_obj_scroll_to_y`, which sends the band `LV_EVENT_SCROLL_BEGIN`, `LV_EVENT_SCROLL` and `LV_EVENT_SCROLL_END`, the
  same events a finger's drag makes, while the finger still holds the band. A `settling` flag, set around each of them,
  has the three handlers return at once; without it the put-back began a drag, which put the band back, and so on until the
  stack ran out. `tests/host/swipe_lvgl_test.cpp` drives LVGL's real input and scroll code with a scripted finger and
  checks it: scroll events never more than two deep, a 64 px drag (a finger's 73: LVGL's scroll limit keeps 9 of its first
  px) turns exactly one page and 63 does not, the page stays turned, the next swipe starts from it, a flick turns, a scroll
  with no finger is left where it went (`bash tests/host/run.sh` builds LVGL from `managed_components` once, into
  `build/host/lvgl`) `[D]`.
- **A drag that began on a disabled child is read at the end of the scroll** — LVGL sends no release and no
  `LV_EVENT_SCROLL_THROW_BEGIN` for a disabled object, only the `LV_EVENT_SCROLL_END` its throw ends with, which carries the
  input device (LVGL's own scrolls carry none). The drag is decided there, once: its distance is exact, and a flick at its
  end may read as a finger that rested if the task was busy since the lift. The test drives it too `[D]`.
- **The ends and the refresh** — at an end nothing turns, nothing wraps and nothing resists (the buttons wrap), and the tick
  sounded at the press is the answer. A swipe made while a refresh holds LVGL's task is read when the task runs again;
  what the glass then does is for the device walk `[?]`.
- **Who owns a drag** — the band owns a vertical drag that starts on it, on a row or between rows: the tap is cancelled
  and the page turns on lift. A slider has no drag of its own: a vertical drag that starts on one is the page's, and the
  slider's release after it sets nothing (below); a horizontal drag on a slider is its value, landing on lift — on the track,
  its hit area or within `spacing.md` of them. A lift farther off is slide-off: the router cancels the press
  (`router::CancelOnSlideOff`, [APP_ARCHITECTURE §7](APP_ARCHITECTURE.md#7-screens-routes-and-focus)), the slider hears `LV_EVENT_PRESS_LOST` and keeps the value it had when the finger went
  down (saved on `LV_EVENT_PRESSED`). `tests/host/slide_off_lvgl_test.cpp` checks both `[HOST]`. The page
  behind a view — the kit page, which scrolls by pixel beneath them — never takes a drag that starts on a view, and takes
  any other.
- **A slider's release is not a tap after a drag** — a touch that scrolled anything leaves the track's value alone
  (`lv_indev_get_scroll_obj`), so a vertical drag that began on a slider never sets it.

## 11. The keyboard

### 11.1 Pages and keys

- **Pages and keys are data** (`components/widgets/src/internal/keyboard_pages.h`): six rows of six slots on four pages — letters in the
  order of the alphabet, capitals, digits with common punctuation (123) and the rest of the punctuation (#+=) — each key one slot
  or two joined; a row with no keys (the symbols page's fourth) draws nothing and holds no stop.
  A slot is `spacing.target`, 64 px, and the gap `spacing.key-gap`, 12 px: six slots are 444 px across, 18 px from each side of the
  glass, and six rows 444 px down, from y 128 under the status bar's 44 (the field's slot 16 down, the count line 88). The bottom
  row is three keys of two slots each, 140 px — Shift (the word alone, no glyph), 123 (ABC beside the letters; #+= and 123 on the
  symbol pages' first slot) and Space — so each label, at `label`'s 18 px a character, stands centred with 25 px either side
  (Shift, Space) or 43 px (123, ABC, #+=); the three stand in slots 0, 2 and 4 on every page, so a page switch keeps the place.
  `tests/host/keyboard_pages_test.cpp` checks the geometry, the equal bottom row, that every letter and every printable ASCII
  character is on a page, and that no page types one character twice `[HOST]`.
### 11.2 The walk

- **A key is the focus stop, a row is not** (the person's decision: no row level): `widgets::OpenKeyboard` makes each key a
  plain object with `ui::RingOutside`'s one ring, `spacing.stroke.focus` 4 px and `spacing.focus-offset` out, so a key's ring reaches
  8 px and leaves 4 px of the 12 px `spacing.key-gap` clear; no key is ever `LV_STATE_EDITED`, and the group is never put to editing, so
  `input.cpp` sends UP and DOWN as focus moves. `BuildWalk` puts the stops in the group in the order of the glass: the field, its
  button, every key row by row from the left, Cancel, the action. A page switch makes the keys again, so it makes the walk again
  and puts the focus back on the key of the same row at the nearest slot — the place is kept; the old keys first leave the
  keyboard's own group (`lv_group_remove_all_objs` on its overlay's group), so none is deleted while focused.
- **Hold OK** runs the escape chain — what it does is [DESIGN → Focus and input](../DESIGN.md#focus-and-input), how is the `Escape` row of [APP_ARCHITECTURE §7](APP_ARCHITECTURE.md#7-screens-routes-and-focus) — and never types; the
  keyboard's part is its overlay `escape`, which cancels at once, with no row to leave, and answers Cancel.
### 11.3 Typing and the field

- **A press types once however long it is held**: the keypad read sends a key on release and a hold goes to the escape chain (§11.2), so a
  hold never repeats a character; a touch on any key types it on `LV_EVENT_CLICKED`, the release on the key, and a finger that slides off, past the key's hit area and `spacing.md`, gets the router's slide-off and sends
  `LV_EVENT_PRESS_LOST`, which cancels it (WCAG 2.5.2); the tick still sounds on contact, in the input task. Shift, 123, ABC and #+=
  act the same way. Delete removes one character; a press past `max_length` ticks and types
  nothing.
- **The text is the field component's** (the person's decision): the keyboard keeps none, and calls `KeyboardTarget` — `append`,
  `erase` and `text` — for a character, Delete and what has been typed; a slot `spacing.content` × `spacing.target` stands at
  the field's place for it, `SetKeyboardTarget` sets the target once the field is made in the slot, and `KeyboardTextChanged`
  tells the keyboard the text changed another way (§11.4). The count line under the slot reads the text
  ("3 of 8 characters needed", then "9 characters"), the action is disabled (the one disabled look) until `min_length`. The 88 px between the
  last key row and the footer is white (DESIGN → Components → The kit: no hint line), which keeps the keys' rings clear of the
  footer's.
- **The typing-only partial count is §5's**; the keyboard is a full-glass overlay (APP_ARCHITECTURE §7): `OpenKeyboard` makes its
  screen, opens it on the router's overlay stack — which loads it, a page change, two partials through white on the V1.0 —
  and builds the keys into the overlay's group. Closing it — the action, Cancel, holding OK — sets the counting back to every
  partial, closes the overlay, which brings back the page it opened over as one full refresh and focus on what opened it,
  ringed if it was, deletes its screen after the event, and then answers `done`. The page under it is kept, not built again.

### 11.4 The field

- **`widgets::MakeTextInput` and `MakePasswordInput`** (`components/widgets/src/text_input.cpp`; the person's decisions on `input_mock2`):
  a `lv_button` frame `spacing.content` × `spacing.target`, white, a `spacing.stroke.default` outline, `rounded.xl` (16; the base 12
  would leave the button's corner and its ring off the frame's corner centre), the words in `label` 16 px in from the edge
  (`spacing.md`), a placeholder in `caption` 8 px (`spacing.sm`) after a still caret 3 × 36. The typed area runs from 16 px in to
  the button's hit square, 352 px: `(352 − 3) / 18` is **19 characters**, and a longer value shows its last 19, the caret in view.
  Disabled is `DitherWhenDisabled` and the words' own `text-disabled`; the trickle flag passes the state to the row and the labels,
  since a colour a child sets for itself beats its parent's disabled style.
- **The button is the kit's icon button at another size**: `detail::MakeIconButtonSized(parent, action, variant, size, radius, words)`
  in `icon_button.cpp`, which `MakeIconButton` calls with `spacing.target` and `rounded.lg`; the field calls it with ghost, `spacing.xl` (32)
  and ``rounded.xl` − kSpacingMd` = 0 (square), so its face, ring (RingOutside: 4 px, `spacing.focus-offset` out, so 8 px of reach, the
  ring's outer radius 8 on the same corner centre as the frame's), disabled look and group join are the icon button's own. It stands 16 px
  from the frame's right, top and bottom edges (13 px inside the 3 px outline); `lv_obj_set_ext_click_area(button, (spacing.target − size) / 2)` makes
  its hit the 64 × 64 square at the frame's right end, the frame's edges its own `[HOST]`: `tests/host/text_input_lvgl_test.cpp` checks
  the three distances and the corner points of the square on LVGL's real layout. `IconAction` gains `kClear` (the cross, `glyph_close`),
  `kEyeOpen` and `kEyeSlashed`; the eye pair, `glyph_eye_on` and `glyph_eye_off`, is cut by `make_marks.py` from BYTE-90's `visible-on` and
  `visible-off` with one shared crop (2, 12, 126, 120), 30 × 26, no trim, so the pair registers; the eye shows the action.
- **The password mask and the reveal**: the field keeps the real text and draws `•` (U+2022, in the font) for each character but the last,
  which is in the clear until `lv_timer` fires after `kMotionPasswordRevealDelayMs` (`motion.password-reveal`, 1500 ms, LVGL's own
  password show time). The next typing press (`InputAppend`) masks it as part of its own redraw and starts the time again; Delete, the
  eye, `SetInputText` and the field going away end it; the timer is a one-shot LVGL frees itself, stopped in the frame's delete event
  `[HOST]`. The hide is no press and never calls `NoteTyping`, so it counts none toward `kRefreshPartialLimit` (§5; it adds one to the
  uncounted ceiling), and the V2.0 draws two partials over the text per character that count one `[?]`: a gate 3 look at the field's
  ghost after a long password.
### 11.5 Wiping a typed password

- **A typed password is wiped from every working copy the field keeps** (SECRETS §5): the text is a fixed buffer of
  `kInputMaxText` (64) bytes in the field's state, never a `std::string`, which reallocates as it grows and leaves its old buffer, the
  password in it, unwiped; a buffer that never moves is wiped where it stands. `detail::SecureZero` (`internal/secure_zero.h`) is
  `mbedtls_platform_zeroize`, which writes through a volatile pointer so the compiler cannot drop it as a dead store — a plain `memset`
  before a free can be removed — and mbedTLS is a private requirement of the widgets component. It wipes the whole buffer on the cross,
  `SetInputText` and the field going away (before `delete`), and the one byte Delete frees; the visible text the field builds for the
  label (bullets, or the clear characters while shown) is a stack buffer wiped as soon as the label has copied it. **LVGL's label keeps its
  own copy of the visible text**, and frees the old copy when the text changes, leaving its bytes in the freed block; the field wipes
  that copy in place, through the pointer LVGL gives, before it sets new text and before the label goes. While the text is masked
  that copy is bullets, so only the revealed state — the eye pressed, or the last typed character in the clear — leaves the password in
  LVGL's buffer, until the next redraw, the reveal time's end or the eye. As far as LVGL allows: the copy `lv_label_set_text` makes
  itself is wiped only on the next change, and a block the allocator has moved is out of reach. `tests/host/text_input_lvgl_test.cpp`
  checks that each of those paths zeroes the buffer, and that the wipe began over the password, through a spy on the stand-in for
  `mbedtls_platform_zeroize` (`tests/host/stubs/mbedtls/platform_util.h`) `[HOST]`.

### 11.6 The field in the walk

- **The walk**: the frame is a stop (`Focusable`) on a page and in the keyboard alike, and its button the next. In the keyboard
  `KeyboardJoinField(keyboard, field.frame, field.button)` rebuilds the group so the walk is the field, the button, every key,
  Cancel, the action, and round again (it opens on the first key: DESIGN → Focus and input); the frame has no handler there, so OK on it
  and a tap on it do the tick and nothing else. The group skips a hidden button, so the cross going leaves the walk, and its
  handler moves focus to the next stop at once (`lv_group_focus_next`). LVGL puts every button-class object in the default group as
  it is made, so a frame and a button join it by themselves and the walk is built after. **Never both rings**: the frame had
  `LV_OBJ_FLAG_STATE_TRICKLE` for the disabled look, which passes every state a parent takes to its children, `LV_STATE_FOCUSED` and
  `LV_STATE_FOCUS_KEY` included, so a focused field drew the button's ring too; the flag is gone, and disabled is passed by hand
  to the words and the button (`SetInputEnabled`). `tests/host/text_input_lvgl_test.cpp` checks, on a page and in the keyboard and for
  both inputs, that the field focused rings the field alone and the next move rings the button alone, the walk's order, the
  wrap both ways, the place kept by a page switch, that joining the field leaves the group's editing off, and that holding OK cancels
  `[HOST]`, with a check that no label on the keyboard's screen is a hint at any stop. The field keeps its action words for
  `IconButtonWords`, which a screen with a footer hint reads; the keyboard reads none. The field sends `LV_EVENT_VALUE_CHANGED` after the eye or the cross, and the caller tells the
  keyboard (`KeyboardTextChanged`).

## 12. The select sheet

- **The sheet is the dropdown's open hook**: `widgets::MakeDropdown` installs `OpenSelectSheet` as the field's opener, so OK or a tap
  on the field opens the sheet; `SetDropdownOpener` swaps in another. A choice is `ChooseDropdown`, which sends
  `LV_EVENT_VALUE_CHANGED` once and only for a change.
- **Its height is a token sum** (`components/widgets/src/internal/sheet_rules.h`): 4 outline, 16, the title's 36 line box, 16, the
  rows' band (64 n + 16 (n − 1)), 16 and the foot's 96 — 488 px for four rows (top at y 312), 408 for three, 328 for two. A list of
  fewer options than a page draws only its rows. It is `spacing.content` + 2 × `spacing.edge` wide plus 4 px each side
  and a corner radius taller than the glass below it, so only its top outline and its `rounded.2xl` corners show, and the offset
  (elevation 2, §4.1) rises from the top.
- **Four rows a page, or three when four would cover the field**: the field's bottom plus the ring's 8, 8 of white and the
  offset's 6 must clear the sheet's top, so a field ending at y 290 or higher takes four, one ending between 291 and 370 three, and
  a lower one three too (never fewer: two would turn the page too often) — the title names the field. `RowsPerPage` is the rule,
  `tests/host/sheet_rules_test.cpp` checks it at both edges `[HOST]`.
- **A row is `caption-strong` in 364 px** (the row's 400 inside its padding less the check's 28 and 8): 30 characters a line, wrapping
  to a second line and a third, each on a 24 px line box. The words column takes the row's minimum, 64 px, and centres its lines, so a
  row is 64 px for one or two lines (2 × 24 and 8 above and below) and 88 for three (§9's centring rule and test). The chosen row is
  inverted with the check at its right end, white, 28 × 20, and no word (DESIGN → Exceptions); a page holds the whole rows that
  fit, so a three-line option can cost a page a row (§10.1).
- **It is an overlay on the router's stack** (APP_ARCHITECTURE §7): opening calls `router::OpenOverlay` before a row is made, so
  the rows and Cancel are its own group's only stops, and focuses the chosen row; the field under it keeps the page's focus with
  its ring off. Closing calls `router::CloseOverlay` — the page's group bound again, the field's ring back if it showed — then
  deletes the sheet's objects and reports a choice. Its overlay `escape` is Cancel, which holding OK reaches through the escape
  chain ([DESIGN → Focus and input](../DESIGN.md#focus-and-input); [APP_ARCHITECTURE §7](APP_ARCHITECTURE.md#7-screens-routes-and-focus)). A screen deleted while the sheet is open
  takes the sheet's objects with it, and their delete event closes the overlay first. A full-screen transparent catcher under the
  sheet is clickable and does nothing, so a touch above the sheet is eaten and a drag does not reach the page behind.
- **A dialog opens over it the same way** (`widgets::OpenDialog`): its two buttons are the only stops, and closing it gives the
  buttons back to the sheet's chosen row, ringed if it was.
- **The kit page** shows a dropdown of three options, one of eight whose last option is two lines, and a disabled one; opening
  the sheet and walking it is a gate 3 look for the scrolled page behind and the partial refreshes.

