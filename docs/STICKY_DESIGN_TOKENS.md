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
| 3 | [Type](#3-type) | The Inter files, each role compiled at one bit, glyph ranges |
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

- **Paint `scale.primary.1` or `scale.primary.2` where a partial refresh must keep it** — use the dither (§4).
- **Animate** — no timer drives a redraw faster than §5's limits.
- **Draw a stroke under 2 px, a weight under 600, or words under 22 px.**
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
| exempt | Decoration only — never under words, never an edge, never a state | Not paired |

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
| `colors.band` | exempt | `#AAAAAA` | `#FFFFFF` | Decoration on a full-refresh frame |

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
- **`colors.text-secondary` on a fill has no pair** — 2.82 : 1 on black; words on a fill are `colors.text-on-fill`.

## 3. Type

- **The face is Inter**, the SemiBold (600) and Bold (700) cuts, in the optical sizes nearest each role `[?]`.
- **Each role is compiled once per size at one bit a pixel** — `hero` 165, `display` 55, `headline` 46, `body` 32,
  `label` 26, `caption` 22 px — and a two-bit copy only for a role drawn on a full-refresh frame alone.
- **A role's line height is its line box** — 168, 64, 54, 42, 34 and 30 px; a block of text is its line count times
  the line box, so the layout reserves its height before the words arrive.
- **The glyph ranges** cover every character FolloUp's strings use; which ranges is open (§8).
- **Changing figures are compiled tabular** — Inter's `tnum` — for the clock, the battery, a recording's timer and a
  count.
- **Letter spacing rounds to whole panel pixels at each size** — `display`'s −0.01 em is −1 px, `hero`'s −0.04 em is
  −7 px `[D]`.

## 4. Space, strokes and shapes

- **Every spacing, stroke and radius is a whole number of panel pixels**, as DESIGN.md's front matter gives it — no
  scaling at run time.
- **`spacing.target`, 64 px, is a hit area**: a control drawn smaller pads its touch box to it.
- **The dither is a one-pixel checkerboard of levels 0 and 3**, aligned to the panel's pixel grid, so it is the same
  pattern on every frame and survives a partial refresh `[D]`.
- **The overlay's offset is `spacing.overlay-offset`, a solid 6 px of level 0,** on the right and bottom edges,
  drawn as two rectangles, never a blur.
- **A list row's padding is two values** — `spacing.sm` above and below, `spacing.md` either side; DESIGN.md's
  front matter carries the vertical one, since a component takes one padding token: 8 + 34 + 30 + 8 = 80 px `[D]`.
- **The ring inside a control** sits on the edge of one with no outline, and `spacing.focus-offset` inside an outlined
  one — 3 + 4 + 4 = 11 px in from each edge `[D]`; a 64 px button keeps 42 px inside the ring for a 34 px line box, and
  a footer button `(432 − 2 × 8) / 3` = 138 px wide keeps 116 px for its label `[D]`.

## 5. Refresh and feedback

| Token | Value | What the code does |
| --- | --- | --- |
| `kRefreshPartialLimit` | 20 | Counts partial refreshes; the next update after 20 is drawn full, and a full one resets the count — proven on the SSD1677, Seeed's practice on the SSD2677 `[HW]` `[?]` |
| `kFeedbackTickMs` | 100 ms | The buzzer tick sounds as the input watcher reads a press — one poll, 10 ms (`kWatchMs`), after it — even during a refresh |
| `kWaitIndicatorDelayMs` | 1000 ms | Declared for DESIGN.md's rule — a wait's indicator drawn only once the wait has run this long, `motion.wait-indicator`; no screen has a wait indicator yet |
| `kSelfUpdateMinMs` | 5000 ms | The period of the hardware-test screens' self-updates (Sensors, Battery and power) — DESIGN.md's floor between two self-driven redraws of an area, `motion.self-update`, which no code enforces |
| `kClockTickMs` | 60000 ms | Declared for DESIGN.md's clock redraw, `motion.clock-tick`; the firmware has no clock yet |

- **A full refresh is drawn for**: the first frame after boot or wake; a full a screen asks for (`RequestFull`, the
  hardware-test screens'); the update after `kRefreshPartialLimit` partials; the one-bit frame after a greyscale
  refresh; on the V1.0, the pause of 2 s after 150 frames by scans; and, as a recovery, a partial that fails — drawn
  again in full at once — and the next frame after a failed refresh, settle or standby, since the glass is then
  unknown. In order, with the code that sets each:
  [STICKY_EPAPER_WAVEFORMS §3.4](STICKY_EPAPER_WAVEFORMS.md#34-which-refresh-a-frame-gets). When a greyscale full
  refresh is drawn is open (§8).
- **DESIGN.md also keeps a full refresh for closing a large overlay and for the frame left before sleep** (DESIGN →
  Motion → Nothing animates) — no screen in the firmware has an overlay or that last frame yet, so no code sets them.
- **A new screen on the V1.0 is two partials, through white**, not a full one — 1.0 to 1.25 s against a full's 1.6 s,
  and counted as two toward `kRefreshPartialLimit`, so one that would pass it is drawn full. Ten page changes, then a
  full refresh, left no trace on the glass over a run of 80 `[SEEN]`.
- **Two full refreshes back to back are DESIGN.md's to forbid, and not yet enforced** — DESIGN → Do's and Don'ts asks
  that a second request within one full refresh's time waits, never dropped, and the frame before sleep waits its
  turn; `Send` has no minimum interval between full refreshes yet (§8).
- **Input during a refresh is kept** (DESIGN → Motion → Press) — a task of its own, above LVGL's, reads UP, DOWN, OK
  and the touch panel every 10 ms, ticks on each press, and queues each press and release; LVGL replays the queue when
  the refresh ends, so a short press inside a refresh is never lost and moves focus once. An OK pressed before the frame
  the last OK caused has been drawn is dropped; how a queued OK lands after queued moves is open (§8). A press or a
  move is queued only with room left for its release, so a release is never lost; holding OK for 1 s goes back,
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
- **A type role is a struct of its size, its line box and its weight**; `ui::Font(role)` gives the font LVGL
  draws it in. The firmware renders Inter through LVGL's Tiny TTF at each size for now, and the flush thresholds it
  to one bit on a partial refresh; compiling each role at one bit (§3) is open (§8).

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
| Compiling each type role at one bit a pixel, in place of Tiny TTF's anti-aliased glyphs | Open | The glyph ranges below, and how the Tiny TTF text reads on the glass |
| A script that compares DESIGN.md's front matter with §2's tables | Open | The token header, which a third copy would make worth checking |
| Inter's optical sizes per role, and the glyph ranges every string needs | Open | The first strings in the firmware, compiled and drawn on the glass |
| The glass's real white and black, against the levels' nominal 21 : 1 | Open | A reading with a light meter on a V1.0 and a V2.0 board |
| Whether the SSD2677 holds the 20-partials rule without ghosting, and its refresh times | Open | STICKY_HARDWARE_SPECS's open items, settled on a V2.0 board |
| When a greyscale full refresh is drawn, and whether grey roles show on screens that take input | Open | The person, with the driver and the first screens |
| Each type role drawn at its line box — `ui::LineBox()` is not applied, so Tiny TTF's 1.21 em draws body at about 38 px, not 42 `[?]` | Open | The person's choice, after the hardware-test screens are checked for fit on the glass |
| The first button press showing the focus ring and moving nothing (DESIGN → Focus and input) — `input.cpp` has no state for a ring not yet shown | Open | The person's choice, with the focus walk on the glass |
| A guard against two full refreshes back to back (DESIGN → Do's and Don'ts) — `Send` has no minimum interval between them | Open | The person's choice, with how often Full, Speed and Four greys flash on the glass |
| How input replayed after a refresh should act — today an OK acts on the focus as the queued moves leave it, where DESIGN → Motion → Press says the frame the person saw; an ignored second OK still ticks; a double tap is replayed twice; a held UP or DOWN repeats after LVGL's long-press time. Proposed: DESIGN.md's Press lines say what the code does, or the code keeps the focus a press began on | Open | The person's choice, before a screen with a destructive action or a long list |
