---
version: alpha
name: ALXV Labs FolloUp
description: FolloUp's design system for e-ink — a voice-first capture companion on the reTerminal Sticky's 3.97-inch black-and-white e-paper glass, read at a glance and usable by anyone.
colors:
  black: "#000000"
  white: "#FFFFFF"
  scale:
    primary:  # the panel's four greys — the system's one palette
      0: "#000000"  # level 0 — black on every refresh
      1: "#555555"  # level 1 — dark grey; black after a partial refresh
      2: "#AAAAAA"  # level 2 — light grey; white after a partial refresh
      3: "#FFFFFF"  # level 3 — white, the glass
  background: "{colors.white}"  # the glass behind everything
  surface-action: "{colors.black}"  # the one action a screen is for
  surface-chosen: "{colors.black}"  # a chosen row, chip or option — never focus
  surface-on: "{colors.black}"  # a toggle that is on
  text-primary: "{colors.black}"  # every word that must read
  text-secondary: "{colors.scale.primary.1}"  # supporting words and timestamps; black after a partial refresh
  text-on-fill: "{colors.white}"  # words on any black surface
  outline: "{colors.black}"  # control and card edges
  divider: "{colors.scale.primary.1}"  # the rule between rows; black after a partial refresh
  focus-ring: "{colors.black}"  # where the buttons' focus is
  focus-ring-on-fill: "{colors.white}"  # the focus ring on a chosen, black row or chip
  band: "{colors.scale.primary.2}"  # decoration on a full-refresh frame only; white after a partial refresh
typography:
  hero:
    fontFamily: Inter
    fontSize: 165px
    fontWeight: 700
    lineHeight: 168px
    letterSpacing: -0.04em
  display:
    fontFamily: Inter
    fontSize: 55px
    fontWeight: 700
    lineHeight: 64px
    letterSpacing: -0.01em
  headline:
    fontFamily: Inter
    fontSize: 46px
    fontWeight: 700
    lineHeight: 54px
  body:
    fontFamily: Inter
    fontSize: 32px
    fontWeight: 600
    lineHeight: 42px
  label:
    fontFamily: Inter
    fontSize: 26px
    fontWeight: 700
    lineHeight: 34px
  caption:
    fontFamily: Inter
    fontSize: 22px
    fontWeight: 600
    lineHeight: 30px
rounded:
  none: 0px
  sm: 4px
  md: 8px
  full: 9999px
spacing:
  xs: 4px
  sm: 8px
  md: 16px
  lg: 24px
  xl: 32px
  2xl: 48px
  edge: 24px
  focus-offset: 4px
  target: 64px
  content: 432px
  status-bar: 44px
  footer: 80px
  list-row-height: 80px
  toggle-height: 44px
  overlay-offset: 6px
stroke:
  hairline: 2px
  default: 3px
  focus: 4px
  strong: 4px
motion:
  state-change:
    duration: 0ms
    delay: 0ms
    timingFunction: steps(1, start)
  surface-enter:
    duration: 0ms
    delay: 0ms
    timingFunction: steps(1, start)
  surface-exit:
    duration: 0ms
    delay: 0ms
    timingFunction: steps(1, start)
  wait-indicator:
    duration: 0ms
    delay: 1000ms
    timingFunction: steps(1, start)
  self-update:
    duration: 0ms
    delay: 5000ms
    timingFunction: steps(1, start)
  clock-tick:
    duration: 0ms
    delay: 60000ms
    timingFunction: steps(1, start)
components:
  status-bar:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.caption}"
    height: "{spacing.status-bar}"
  footer:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    height: "{spacing.footer}"
  button-action:
    backgroundColor: "{colors.surface-action}"
    textColor: "{colors.text-on-fill}"
    typography: "{typography.label}"
    rounded: "{rounded.md}"
    height: "{spacing.target}"
    padding: "{spacing.md}"
  button-outline:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.md}"
    height: "{spacing.target}"
    padding: "{spacing.md}"
  list-row:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    height: "{spacing.list-row-height}"
    padding: "{spacing.sm}"
  list-row-supporting:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-secondary}"
    typography: "{typography.caption}"
  list-row-chosen:
    backgroundColor: "{colors.surface-chosen}"
    textColor: "{colors.text-on-fill}"
  sticky:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.display}"
    rounded: "{rounded.sm}"
    padding: "{spacing.lg}"
  tag:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.sm}"
    padding: "{spacing.sm}"
  timeline-chip:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.full}"
    height: "{spacing.target}"
  timeline-chip-chosen:
    backgroundColor: "{colors.surface-chosen}"
    textColor: "{colors.text-on-fill}"
  toggle:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    rounded: "{rounded.full}"
    height: "{spacing.toggle-height}"
  toggle-on:
    backgroundColor: "{colors.surface-on}"
    textColor: "{colors.text-on-fill}"
  dialog:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.body}"
    rounded: "{rounded.md}"
    padding: "{spacing.lg}"
  status-line:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
  divider:
    backgroundColor: "{colors.divider}"
    height: "{stroke.hairline}"
  focus-ring:
    backgroundColor: "{colors.focus-ring}"
    width: "{stroke.focus}"
  focus-ring-on-fill:
    backgroundColor: "{colors.focus-ring-on-fill}"
    width: "{stroke.focus}"
  card-outline:
    backgroundColor: "{colors.outline}"
    width: "{stroke.default}"
  band:
    backgroundColor: "{colors.band}"
    height: "{spacing.sm}"
  lock-clock:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.hero}"
---

# ALXV Labs FolloUp — Design

FolloUp's design system: the principles behind its screens, and the greys, type, spacing, shapes, motion, words and
components they are built from. FolloUp is a voice-first place to catch an idea, a to-do or a note at the moment it
arrives — Gemini transcribes and summarises it, the MicroSD card keeps it, and the ones that matter stay in view as
stickies on an always-on e-paper glass. It is designed for e-ink first, the reTerminal Sticky's 3.97-inch
black-and-white panel with three buttons, a touch panel and a buzzer, and for everyone who will use it. How the
firmware implements the system is [STICKY_DESIGN_TOKENS](docs/STICKY_DESIGN_TOKENS.md); the glass and its refresh
modes are [STICKY_HARDWARE_SPECS](docs/STICKY_HARDWARE_SPECS.md).

## Overview

### Brand and character

- **A quiet reminder, not another screen.** FolloUp lives on a fridge, a desk or a wall and holds what matters next;
  it never asks to be checked, and it never blinks for attention.
- **Ink on paper.** Black type on a white glass, cut with clean lines and solid shapes — a device that looks printed,
  not lit, and reads the same in sunlight as on a dim desk.
- **Plain and kind.** Short, warm, exact words; one thing at a time; nothing that hurries the person.

### Principles

When two pull against each other, the earlier wins — between options that both meet DESIGN → Accessibility.

1. **Accessible by default.** Every screen works by the UP, DOWN and OK buttons alone, tells every outcome in words
   and in sound, and is sized for the glass, not a laptop. No screen reader runs here, the buttons work with wet
   hands, gloves or a tremor where touch does not, and a size chosen on a laptop is the wrong size on a 3.97-inch
   panel.
2. **Black and white first.** Every screen reads in pure black and white, and grey only adds to it. The panel shows
   four levels only on a greyscale full refresh; every other refresh is one bit a pixel, which turns dark grey black
   and light grey white.
3. **Still, and changed sparingly.** The glass changes only when something changed, in one refresh, over as small an
   area as the change needs. A full refresh flashes the whole glass for about 1.65 s, partial refreshes leave traces
   that build up, and every refresh costs battery.
4. **Glanceable and calm.** A screen is read in a glance from where the device hangs, and stays true with no power:
   one primary item, the largest type that fits, capture one action away — the AI button or a Record button — and a
   designed last frame, because it stays on the glass while the device sleeps.
5. **One system.** Every grey, size, space, stroke, radius and timing is a token here; a value that is not here is
   an exception, argued in DESIGN → Exceptions.

## Colors

### The four levels

- **The glass has four greys and no colour**: `scale.primary` 0 to 3, black to white — the system's one palette.
  Black and white carry the system; the two greys between are seasoning.
- **Only a greyscale full refresh draws all four; every other refresh draws two** — levels 0 and 1 become black, 2
  and 3 white. Most frames are partial, so a screen is designed in its partial-refresh form first.
- **A grey that must survive a partial refresh is a dither** — a one-pixel checkerboard of black and white, never
  `scale.primary.1` or `scale.primary.2`.

### Roles

- **`background`** — white: the glass behind everything.
- **`text-primary`, `outline`, `focus-ring`** — black: every word that must read, every edge, where the buttons'
  focus is.
- **`surface-action`, `surface-chosen`, `surface-on`** — black: the one action a screen is for, a chosen row, chip or
  option, a toggle that is on. Inversion shows what is chosen or on, never focus, which is always the ring; each job
  keeps its own role, so one can change without the others.
- **`focus-ring-on-fill`** — white: the focus ring on a chosen, black row or chip, so focus and chosen show at once.
- **`text-on-fill`** — white: words on any black surface.
- **`text-secondary`, `divider`** — dark grey: supporting words, timestamps, the rule between rows. Black after a
  partial refresh, so never the only difference between two things.
- **`band`** — light grey: decoration on a full-refresh frame only. White after a partial refresh, so never under
  words, never an edge, never a state.

### Contrast floors

**Text meets 4.5 : 1 and a non-text part 3 : 1, on a full refresh and on a partial one.**

| Pair | Full refresh | Partial refresh |
| --- | --- | --- |
| Black on white — text, outlines, the focus ring | 21 : 1 | 21 : 1 |
| Dark grey on white — supporting text, dividers | 7.5 : 1 | 21 : 1 |
| White on black — words on a chosen row or the action button, the ring on a chosen row | 21 : 1 | 21 : 1 |
| Dark grey on black | 2.8 : 1 — never painted | 1 : 1 |

- **These are the levels' ratios, not the glass's** — e-paper's white is paper grey and its black is not ink black,
  so each ratio is an upper bound on the device. That is why words are only ever black on white or white on black.
- **Every pair, with its proof on both refreshes, is in STICKY_DESIGN_TOKENS §2.**

## Typography

### Face

- **Inter, compiled into the firmware** — open shapes and clear figures, under the SIL Open Font License.
- **Weights 600 and 700 only.** A regular weight breaks up at one bit a pixel; hierarchy comes from size and weight,
  never from grey alone.
- **Glyphs are drawn at one bit a pixel** for anything a partial refresh draws, so a stroke is solid black, never a
  grey edge that thresholds into a ragged one.

### Roles

At 235 ppi a pixel is 0.108 mm and a point is 3.26 px.

| Role | Size | Use |
| --- | --- | --- |
| `hero` | 165 px, 50.6 pt | The lock screen's clock — digits only; its weekday is `headline` |
| `display` | 55 px, 16.9 pt | One per screen at most: a sticky's words |
| `headline` | 46 px, 14.1 pt | A screen's title |
| `body` | 32 px, 9.8 pt | Everything people read: a transcript, a summary, a note |
| `label` | 26 px, 8.0 pt | Buttons, hints, tags, a row's title |
| `caption` | 22 px, 6.7 pt | The status bar and timestamps — the smallest words on the glass |

### Reading

- **About 26 characters of `body` a line** across the content width — wrap, never truncate, words people read.
- **Truncate only a label whose full words are one press away.**
- **`headline`, `display` and `hero` are large text** for contrast — WCAG's 18 pt regular or 14 pt bold.
- **Figures that change use Inter's tabular figures** — the clock, the battery, a recording's timer, "3 of 5" — so a
  number keeps its width and the area a refresh redraws stays the same.

## Layout

### The one glass

- **480 × 800 portrait, one to one** — the 3.97-inch 800 × 480 panel turned on its side, at 235 ppi. Nothing adapts,
  because the glass never changes size; every screen keeps the same regions in the same places.
- **A mock is drawn at 480 × 800 px in both its full-refresh and its partial-refresh form**, and judged on the glass of
  a V1.0 and a V2.0 board.

### Regions

- **The status bar** — `spacing.status-bar` along the top edge: the time, battery, Wi-Fi, storage and recording state,
  on white over a hairline; charging and a low battery are said in words, since the charge light speaks only in
  colour.
- **The content** — one column inside `spacing.edge` margins, `spacing.content` wide, one primary item per screen.
- **The footer** — `spacing.footer` along the bottom edge: the screen's touch buttons, and what UP, DOWN and OK do
  here. The action button sits where OK's hint is; the buttons sit `spacing.sm` apart.

### Spacing

- **A 4 px base** — `xs` 4, `sm` 8, `md` 16, `lg` 24, `xl` 32, `2xl` 48 px. Related things sit closer than groups.
- **Every touch target is `spacing.target`, 64 px — 6.9 mm.** A control may draw smaller and pad its hit area — a
  toggle draws `spacing.toggle-height`, 44 px, and takes touches across 64.

### Paging

- **Content turns by page, never scrolls by pixel** — UP and DOWN move focus, and focus leaving the glass turns the
  page; a pixel scroll would refresh the glass on every step.
- **Focus wraps across pages** — DOWN on the last row of the last page goes to the first row of the first page, a page
  turn like any other.

## Elevation & Depth

### Flat on white

- **Depth is a line and a hard offset, never a blur or a gradient** — a blur is greys a partial refresh throws away,
  and a gradient bands into four steps.
- **An overlay — a dialog, the keyboard, a select list — stands on a `stroke.strong` outline with a solid
  `spacing.overlay-offset` in black**, so it reads as lifted off the page on either refresh.

### Dither

- **A one-pixel checkerboard is the only grey that survives a partial refresh** — a progress bar's empty part, inside
  a black outline and with its step in words, and a disabled control's outline, with the reason in words; how it reads
  on the glass is open.

## Shapes

### Strokes

- **Nothing thinner than `stroke.hairline`, 2 px** — a one-pixel line breaks up and ghosts on e-paper.
- **`stroke.default`, 3 px, outlines controls and cards; `stroke.focus` and `stroke.strong`, 4 px, the focus ring and
  overlays.**

### Corners

- **Small and few** — `rounded.sm` for stickies, tags and cards, `rounded.md` for buttons and fields, `rounded.full`
  for toggles, chips and pagination dots.
- **A nested corner is the outer radius less the gap between them**, never the same radius twice.

## Motion

### Nothing animates

- **Every motion token is a step** — `steps(1, start)`, no duration: a change is the next frame. No slide, fade, scale,
  spinner, blinking cursor or marquee; e-paper cannot show them, and trying costs refreshes.
- **The refresh is the motion.** A press, a selection and a change of screen land in one partial refresh. A full
  refresh — the whole glass flashing — is kept for the first frame after waking, the update after twenty partial
  refreshes, closing a large overlay, and the frame left on the glass before sleep.
- **When grey shows is open** — whether grey roles are drawn on screens that take input, collapsing to black or white
  on each partial refresh, or kept to screens drawn with a greyscale full refresh, is decided with the driver and the
  first screens.
- **A refresh that must wait, waits** — it is never dropped, and it shows the latest state when it runs.

### Press

- **The buzzer answers first** — a short tick within 100 ms of every accepted press, before the glass can change; a
  press with no tick reads as missed.
- **A press is answered by its result**, never by a pressed state: the tick, then the next frame. Only a hold in
  progress — the AI button recording — is drawn while it lasts.
- **Every accepted press ticks, even during a refresh.** UP and DOWN move focus to the latest place; an OK pressed
  before the frame lands waits and acts on the frame the person saw, and a second OK in that time is ignored.

### Waits and things that update on their own

- **A wait shows nothing for its first second** (`motion.wait-indicator`), then still words of what is happening.
- **Nothing updates on its own faster than every 5 s** (`motion.self-update`) — a recording's timer, a wait's progress
  — and the clock once a minute (`motion.clock-tick`).
- **Nothing leaves on a timer** — a message stays until the next change of screen.

## Voice & Content Strategy

### Voice

- **Calm, plain and warm.** Short sentences, everyday words, no exclamation marks, no jargon — the glass is read in
  passing.
- **Sentence case** everywhere: titles, buttons, hints, tags.

### Words FolloUp uses

- **Idea, To-do, Note** — the three kinds of capture; **Follow-up** — one kept in view; **Sticky** — a follow-up shown
  as a card; **Vibe check** — deciding whether an idea is still worth keeping; **Summary** — Gemini's short version.
- **Record**, never "capture" or "dictate", on a button.

### Labels and states

- **A button names what happens** — "Save", "Keep it", "Let it go", "Record" — never "OK" or "Submit". OK is the
  button's name, never its label.
- **An empty screen says what goes there and how to add it** — "No follow-ups yet. Hold AI or press Record to add
  one."
- **An error says what happened and what to do next** — "Couldn't reach Wi-Fi. Check the network in Settings."
- **A wait says what is happening** — "Transcribing…", never "Loading".

### Dates, times and numbers

- **"Today, 8:12", "Yesterday", then "Mon 17 Nov"**; the clock in the person's 12- or 24-hour setting; a percentage
  with no space before `%`.

## Components

### The kit

- **Status bar, footer** — the fixed regions (DESIGN → Layout → Regions).
- **Button** — the action button (black, one per screen, `button-action`) and the outline button (`button-outline`).
- **Toggle** — on is a black track, off an outlined one; its state also in words beside it.
- **List row** — a title and a supporting line in `spacing.list-row-height`, 80 px: `spacing.sm` above and below,
  `spacing.md` either side, divided by a hairline; a one-line row is `spacing.target`, 64 px.
- **Timeline** — date chips, then the chosen date's recordings as list rows.
- **Sticky** — one follow-up as a card: its tag and its words in `display` or `body`.
- **Carousel** — onboarding, one card at a time; the current dot filled, the others outlined.
- **Dialog** — a decision the person must make now, with one action button and Cancel; a destructive action names
  what it destroys ("Delete this note"), and its dialog opens with focus on Cancel.
- **Select list, keyboard** — the large overlays, each with Cancel or Back; the keyboard's 64 px keys fit six to a
  row, and its layout is open.
- **Icons** — a meaningful icon always has its word beside it ("Home", "Wi-Fi"); no icon stroke under
  `stroke.hairline`.
- **Status line** — news that asks nothing, until the next change of screen.
- **Wait indicator** — still words and a segmented progress bar, stepping no faster than every 5 s.
- **Recording indicator** — "Recording" and its time, with a distinct tone at start and stop.

### States

- **Focused** — the `stroke.focus` ring, always. Around a control with room, it sits `spacing.focus-offset` outside
  it. Inside a full-width row, a chip or a footer button, so it never touches a neighbour or the glass's edge, it sits
  on the edge of a control with no outline, and `spacing.focus-offset` inside an outlined one — 11 px in from its
  edge.
- **Chosen** — inverted: `surface-chosen` with `text-on-fill`. A focused, chosen row or chip is inverted with the ring
  in `focus-ring-on-fill`.
- **Disabled** — a dither outline with the label kept and the reason in words; prefer explaining over disabling.
- **Busy** — the label says what it is doing.
- **Done** — in words ("Done"), never a grey.

## Do's and Don'ts

- Do design every screen in its partial-refresh form first, and check it reads in black and white alone.
- Do show focus with the ring and what is chosen with an inversion — never one for the other, never a grey alone.
- Do give every touch target 64 px, and every action a path by the buttons.
- Do let the buzzer answer a press before the glass does.
- Don't animate anything — no slides, fades, spinners or blinking cursors.
- Don't put words on light grey, or rely on a 1 px line.
- Don't use a weight under 600, or a size under `caption`.
- Don't time anything to leave.
- Don't trigger two full refreshes back to back.

## Focus and input

### Buttons and touch

- **UP and DOWN move focus, OK acts** — on every screen, including the overlays; anything touchable is reachable this
  way.
- **The AI button records** — a hold records and its release saves; a press-to-start, press-to-stop way, the hold's
  length, and how a hold is told from OK, a wake and power-off are open (DESIGN → Open items).
- **The first button press shows the focus ring and moves nothing**; the next moves it, and focus wraps at the ends of
  a list. A screen opened by touch shows no ring.
- **An overlay takes focus when it opens and gives it back to what opened it when it closes**; every overlay closes
  with Cancel or Back.

### Hit areas

- **64 px, 6.9 mm, the floor for anything touchable** — above WCAG's 6.35 mm, the AA floor, though under the 9 mm a
  fingertip is comfortable with; a footer button, a list row, a key on the keyboard.

## Accessibility

- **WCAG 2.2 AA where it applies, and EN 301 549's closed functionality where the hardware allows** — the device has
  a buzzer and no speaker, so it cannot speak, and a person who cannot see the glass is not served today.
- **Every outcome in words and in sound** — a success, a failure, a wait and a recording's start and stop each have
  words on the glass and a distinct tone.
- **Contrast** — DESIGN → Colors → Contrast floors, on both refreshes.
- **Text size** — nothing under `caption`; a larger text setting that steps every role up one is open.
- **No time limits** — nothing leaves on a timer.
- **Flashing** — a full refresh is rare and never repeated back to back; whether one counts as more than three flashes
  a second is open.
- **Plain language** — one task per screen, and a destructive action asks first in a dialog.
- **Sleep never loses work** — the device does not sleep while an overlay, a typed entry or a recording is open, except
  on a low battery, when it says so in words, keeps what is open, and then sleeps.

## Exceptions

- **None in use.** A value off these tokens is argued here — what, where and why — before a screen uses it.

## Open items

- **The frame left on the glass while the device sleeps** — the stickies, the lock screen or a blank glass — decided
  with the designs.
- **When grey shows** — on screens that take input or only on greyscale full refreshes — decided with the driver and
  the first screens.
- **The viewing distance** the sizes are set for — `display` reads at about 35 cm, `hero` at about 1 m.
- **The AI button** — its name on the glass (AI or OK), a hold's length, and how a hold, a wake and power-off differ.
- **The keyboard's layout** — six 64 px keys a row.
- **The dither on the glass** — whether a one-pixel checkerboard reads, or ghosts.
- **The clock's cost** — a minute tick makes a full refresh every twenty minutes on a still screen — and whether the
  lock screen's clock is 24-hour or shows AM and PM in the status bar.
- **A spoken mode** — none is possible on this hardware; who that leaves out, and what would serve them.
- **A press answered without sound** — how a muted device, or a person who cannot hear the tick, knows a press landed
  before the next frame does.
- **The glass's real contrast** — its white and black measured against the nominal 21 : 1.
- **Whether a 2 px hairline, Inter 600 at one bit and `caption` at 22 px read from a metre away** — on a V1.0 and a
  V2.0 board.
- **How many times a full refresh flashes**, against WCAG's three a second.
- **Press-to-start and press-to-stop recording** beside the hold.
- **A larger text setting**, and the glyph ranges every string needs.
- **The buzzer's tones** for a tick, a success, a failure, a wait, and a recording's start and stop.
