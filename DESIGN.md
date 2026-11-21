---
version: alpha
name: ALXV Labs FolloUp
description: FolloUp's design system for e-ink — a voice-first capture companion on the reTerminal Sticky's 3.97-inch black-and-white e-paper glass, read at a glance and usable by anyone.
colors:
  black: "{colors.scale.primary.0}"
  white: "{colors.scale.primary.3}"
  scale:
    primary:  # the panel's four greys — the system's one palette
      0: "#000000"  # level 0 — black on every refresh
      1: "#555555"  # level 1 — dark grey; a UI fill is its dither, text and lines draw black
      2: "#AAAAAA"  # level 2 — light grey; a UI fill is its dither, text and lines draw white
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
  band: "{colors.scale.primary.2}"  # a light-dither fill: decoration, a disabled control; black words only
typography:
  # The primitives: Bitcount Single at one bit a pixel, each size a multiple of 10 px, its line 1.2 em.
  scale-20-semibold:
    fontFamily: Bitcount Single
    fontSize: 20px
    fontWeight: 600
    lineHeight: 24px
  scale-20-bold:
    fontFamily: Bitcount Single
    fontSize: 20px
    fontWeight: 700
    lineHeight: 24px
  scale-30-semibold:
    fontFamily: Bitcount Single
    fontSize: 30px
    fontWeight: 600
    lineHeight: 36px
  scale-30-bold:
    fontFamily: Bitcount Single
    fontSize: 30px
    fontWeight: 700
    lineHeight: 36px
  scale-40-semibold:
    fontFamily: Bitcount Single
    fontSize: 40px
    fontWeight: 600
    lineHeight: 48px
  scale-40-bold:
    fontFamily: Bitcount Single
    fontSize: 40px
    fontWeight: 700
    lineHeight: 48px
  scale-50-semibold:
    fontFamily: Bitcount Single
    fontSize: 50px
    fontWeight: 600
    lineHeight: 60px
  scale-50-bold:
    fontFamily: Bitcount Single
    fontSize: 50px
    fontWeight: 700
    lineHeight: 60px
  scale-60-semibold:
    fontFamily: Bitcount Single
    fontSize: 60px
    fontWeight: 600
    lineHeight: 72px
  scale-60-bold:
    fontFamily: Bitcount Single
    fontSize: 60px
    fontWeight: 700
    lineHeight: 72px
  scale-70-semibold:
    fontFamily: Bitcount Single
    fontSize: 70px
    fontWeight: 600
    lineHeight: 84px
  scale-70-bold:
    fontFamily: Bitcount Single
    fontSize: 70px
    fontWeight: 700
    lineHeight: 84px
  scale-80-semibold:
    fontFamily: Bitcount Single
    fontSize: 80px
    fontWeight: 600
    lineHeight: 96px
  scale-80-bold:
    fontFamily: Bitcount Single
    fontSize: 80px
    fontWeight: 700
    lineHeight: 96px
  scale-100-semibold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 100px
    fontWeight: 600
    lineHeight: 120px
  scale-100-bold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 100px
    fontWeight: 700
    lineHeight: 120px
  scale-120-semibold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 120px
    fontWeight: 600
    lineHeight: 144px
  scale-120-bold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 120px
    fontWeight: 700
    lineHeight: 144px
  scale-160-semibold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 160px
    fontWeight: 600
    lineHeight: 192px
  scale-160-bold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 160px
    fontWeight: 700
    lineHeight: 192px
  # The roles: each is one primitive, named beside it.
  hero:  # = typography.scale-60-bold
    fontFamily: Bitcount Single
    fontSize: 60px
    fontWeight: 700
    lineHeight: 72px
  display:  # = typography.scale-50-bold
    fontFamily: Bitcount Single
    fontSize: 50px
    fontWeight: 700
    lineHeight: 60px
  headline:  # = typography.scale-40-bold
    fontFamily: Bitcount Single
    fontSize: 40px
    fontWeight: 700
    lineHeight: 48px
  body:  # = typography.scale-30-semibold
    fontFamily: Bitcount Single
    fontSize: 30px
    fontWeight: 600
    lineHeight: 36px
  label:  # = typography.scale-30-bold
    fontFamily: Bitcount Single
    fontSize: 30px
    fontWeight: 700
    lineHeight: 36px
  caption:  # = typography.scale-20-semibold
    fontFamily: Bitcount Single
    fontSize: 20px
    fontWeight: 600
    lineHeight: 24px
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
    height: "{spacing.stroke.hairline}"
  focus-ring:
    backgroundColor: "{colors.focus-ring}"
    width: "{spacing.stroke.focus}"
  focus-ring-on-fill:
    backgroundColor: "{colors.focus-ring-on-fill}"
    width: "{spacing.stroke.focus}"
  card-outline:
    backgroundColor: "{colors.outline}"
    width: "{spacing.stroke.default}"
  band:
    backgroundColor: "{colors.band}"
    height: "{spacing.sm}"
  lock-clock:  # its own type size is open; hero until it is chosen
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
   and light grey white — except a grey fill, which is drawn as its dither.
3. **Still, and changed sparingly.** The glass changes only when something changed, in as few refreshes and
   over as small an area as the change needs. A full refresh flashes the whole glass for 1 s on a V2.0 and 1.6 s on a
   V1.0; partial refreshes leave traces that build up, and every refresh costs battery.
4. **Glanceable and calm.** A screen is read in a glance from where the device hangs, and stays true with no power:
   one primary item, the largest type that fits, capture one action away — the AI button or a Record button — and a
   designed last frame, because it stays on the glass while the device sleeps.
5. **One system.** Every grey, size, space, stroke, radius and timing is a token here; a value that is not here is
   an exception, argued in DESIGN → Exceptions.

## Colors

### The four levels

- **The glass has four greys and no colour**: `scale.primary` 0 to 3, black to white — the system's one palette.
  Black and white carry the system; the two greys between are dithers on the UI and true greys in images.
- **Only a greyscale full refresh draws all four; every other refresh draws two** — levels 0 and 1 become black, 2
  and 3 white, except a grey fill, which is its dither. Most frames are partial, so a screen is designed in its
  partial-refresh form first.
- **The two greys are for images only** — a photo, an illustration or artwork, drawn with a greyscale full refresh on
  a screen left still; never a UI element, which the next partial refresh would collapse.
- **Every grey on a UI element is a dither** — a fixed black-and-white pattern locked to the glass's pixels, which
  every refresh keeps: `scale.primary.1` as one white pixel in four, `scale.primary.2` as three in four.
- **Text and lines are never dithered** — a grey text or line role draws solid: level 1 black, level 2 white.

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
- **`band`** — light grey as a dither fill: decoration and a disabled control's inside, on every refresh. Words on it
  are `text-primary`, black, only; never an edge, never the only sign of a state.

### Contrast floors

**Text meets 4.5 : 1 and a non-text part 3 : 1, on a full refresh and on a partial one.**

| Pair | Full refresh | Partial refresh |
| --- | --- | --- |
| Black on white — text, outlines, the focus ring | 21 : 1 | 21 : 1 |
| Dark grey on white — supporting text, dividers, on a greyscale frame only | 7.5 : 1 | 21 : 1 |
| White on black — words on a chosen row or the action button, the ring on a chosen row | 21 : 1 | 21 : 1 |
| Dark grey on black | 2.8 : 1 — never painted | 1 : 1 |

- **These are the levels' ratios, not the glass's** — e-paper's white is paper grey and its black is not ink black,
  so each ratio is an upper bound on the device. That is why words are only ever black on white or white on black.
- **Every pair, with its proof on both refreshes, is in STICKY_DESIGN_TOKENS §2.**

## Typography

### Face

- **Bitcount Single, compiled into the firmware** — a pixel font, each letter built from square cells, under the SIL
  Open Font License; at one bit a pixel its strokes are the glass's own pixels.
- **Weights 600 and 700 only.** A lighter weight breaks up at one bit a pixel; hierarchy comes from size and weight,
  never from grey alone. Below 50 px the two weights look alike, so size carries the hierarchy there.
- **Every size is a multiple of 10 px** — the font's cell is a tenth of the em, so each cell is whole pixels.
- **Glyphs are drawn at one bit a pixel** on every refresh, so a stroke is solid black, never a grey edge that
  thresholds into a ragged one.

### Scale and roles

At 235 ppi a pixel is 0.108 mm and a point is 3.26 px.

- **The scale is the primitives** — 20 to 80 px in steps of 10, and 100, 120 and 160 px for figures, each at 600 and
  700, its lines 1.2 em apart (`typography.scale-30-semibold` and its siblings). A screen takes a role, never a
  primitive; only the hardware test's type scale page shows the primitives themselves (DESIGN → Exceptions).
- **A role is one primitive:**

| Role | Primitive | Size | Use |
| --- | --- | --- | --- |
| `hero` | `scale-60-bold` | 60 px, 18.4 pt | A figure or a word taken in at a glance — never the clock, which has a size of its own |
| `display` | `scale-50-bold` | 50 px, 15.3 pt | One per screen at most: a sticky's words |
| `headline` | `scale-40-bold` | 40 px, 12.3 pt | A screen's title |
| `body` | `scale-30-semibold` | 30 px, 9.2 pt | Everything people read: a transcript, a summary, a note |
| `label` | `scale-30-bold` | 30 px, 9.2 pt | Buttons, hints, tags, a row's title — `body`'s size, in bold |
| `caption` | `scale-20-semibold` | 20 px, 6.1 pt | The status bar and timestamps — the smallest words on the glass |

### Reading

- **24 characters of `body` a line** across the content width — wrap, never truncate, words people read.
- **Truncate only a label whose full words are one press away.**
- **`hero` and `display` are large text** for contrast — WCAG's 14 pt bold.
- **Figures keep their width** — every character is the same width — so a changing number, the clock, the battery, a
  recording's timer, "3 of 5", redraws the same area.

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
- **An overlay — a dialog, the keyboard, a select list — stands on a `spacing.stroke.strong` outline with a solid
  `spacing.overlay-offset` in black**, so it reads as lifted off the page on either refresh.

### Dither

- **A dither is the only grey a UI element shows** — a band, a progress bar's empty part inside a black outline with
  its step in words, a disabled control's inside within its solid outline with the reason in words. Both patterns
  read as two distinct greys on the V1.0's glass, close to the true greys.
- **Words sit only on the light dither, in black** — 16 : 1 against its average; never on the dark dither, where
  white words average 3.5 : 1 and their edges break up.
- **Fills are square and opaque** — a rounded corner or a translucent fill blends to other greys and draws solid.

## Shapes

### Strokes

- **Nothing thinner than `spacing.stroke.hairline`, 2 px** — a one-pixel line breaks up and ghosts on e-paper.
- **`spacing.stroke.default`, 3 px, outlines controls and cards; `spacing.stroke.focus` and
  `spacing.stroke.strong`, 4 px, the focus ring and overlays.**

### Corners

- **Small and few** — `rounded.sm` for stickies, tags and cards, `rounded.md` for buttons and fields, `rounded.full`
  for toggles, chips and pagination dots.
- **A nested corner is the outer radius less the gap between them**, never the same radius twice.

## Motion

### Nothing animates

- **Every motion token is a step** — `steps(1, start)`, no duration: a change is the next frame. No slide, fade, scale,
  spinner, blinking cursor or marquee; e-paper cannot show them, and trying costs refreshes.
- **The refresh is the motion.** A press and a selection land in one partial refresh, and so does a change of screen
  on the V2.0; on the V1.0 a change of screen is two, through white, 1 to 1.25 s, so the old page leaves no ghost. A full
  refresh — the whole glass flashing — is kept for the first frame after waking, the update after twenty partial
  refreshes, closing a large overlay, and the frame left on the glass before sleep.
- **Grey on screens that take input is a dither** — the true greys of a greyscale full refresh are for images on a
  still screen, and collapse to black or white at the next partial refresh (Colors → The four levels).
- **A refresh that must wait, waits** — it is never dropped, and it shows the latest state when it runs.

### Press

- **The buzzer answers first** — a short tick within 100 ms of every press, before the glass can change; a
  press with no tick reads as missed.
- **A press is answered by its result**, never by a pressed state: the tick, then the next frame; an ignored press
  gets the tick alone. Only a hold in progress — the AI button recording — is drawn while it lasts.
- **Every press ticks, even during a refresh.** UP and DOWN move focus to the latest place; an OK pressed before the
  frame lands waits and acts where the presses before it left focus, and a second OK in that time ticks but is
  ignored. On touch, a double tap during a refresh is two taps.

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
- **A button's words fit it on one line** — at `label`'s 18 px a character, a full-width button holds 22, a half
  button 10 and a third 6; a longer name is shortened, never cut off or wrapped.
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
  `spacing.stroke.hairline`.
- **Status line** — news that asks nothing, until the next change of screen.
- **Wait indicator** — still words and a segmented progress bar, stepping no faster than every 5 s.
- **Recording indicator** — "Recording" and its time, with a distinct tone at start and stop.

### States

- **Focused** — the `spacing.stroke.focus` ring, always. Around a control with room, it sits `spacing.focus-offset`
  outside it. Inside a full-width row, a chip or a footer button, so it never touches a neighbour or the glass's
  edge, it sits on the edge of a control with no outline, and `spacing.focus-offset` inside an outlined one — 11 px
  in from its edge.
- **Chosen** — inverted: `surface-chosen` with `text-on-fill`. A focused, chosen row or chip is inverted with the ring
  in `focus-ring-on-fill`.
- **Disabled** — the light dither inside the control's solid outline, its label in black and the reason in words;
  prefer explaining over disabling.
- **Busy** — the label says what it is doing.
- **Done** — in words ("Done"), never a grey.

## Do's and Don'ts

- Do design every screen in its partial-refresh form first, and check it reads in black and white alone.
- Do show focus with the ring and what is chosen with an inversion — never one for the other, never a grey alone.
- Do give every touch target 64 px, and every action a path by the buttons.
- Do let the buzzer answer a press before the glass does.
- Don't animate anything — no slides, fades, spinners or blinking cursors.
- Don't put words on the dark dither, or white words on any dither; don't rely on a 1 px line.
- Don't use a weight under 600, or a size under `caption`.
- Don't time anything to leave.
- Don't trigger two full refreshes back to back — one full refresh's own time apart at the least.

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

A value off these tokens is argued here — what, where and why — before a screen uses it.

- **The hardware-test demo** (`components/hw_test`) — a bench tool for V1.0 and V2.0 boards, not a FolloUp screen.
  - **What:** its fixtures' sizes (the checker, the grey bands, the touch marks and dot, the microphone bar and its
    scale, the benchmark's track and its step, the footer's Back), a test's absolute grey levels (the checker's dots,
    the words on each grey band), its timers and lengths (stats shown after a refresh, the benchmark's frames, a
    second of recording, leaving a screen, the page walk's 3 s on each page), the refresh benchmark's bar, which
    animates as fast as the glass allows, the page walk, which changes screen by itself from the console, and the type
    scale page, which draws every primitive rather than a role, scrolls by pixel — no animation, momentum or
    overscroll — and makes each sample a focus stop so UP and DOWN reach it.
  - **Why:** each measures the glass, the touch or a sensor — a fixture sized to the test, not to reading — the
    benchmark exists to drive the glass as fast as it goes, the page walk to change screen many times over for a
    ghosting run, and the type scale page to show the whole scale for choosing the roles, more than one page holds, so
    Nothing animates, Nothing leaves on a timer, A screen takes a role and Content turns by page cannot hold there.
  - **Where it stops:** a size that matches a token uses the token; text, colours by role, focus and hit areas follow
    this file; nothing outside `components/hw_test` uses the exception.

## Open items

- **The frame left on the glass while the device sleeps** — the stickies, the lock screen or a blank glass — decided
  with the designs.
- **The viewing distance** the sizes are set for — `hero`'s 60 px figures read at about 35 cm and `display` at about
  29 cm.
- **The clock's own size** — the lock screen's clock takes a type size of its own, likely one of the 100, 120 and
  160 px figure sizes; `lock-clock` takes `hero` until it is chosen.
- **The AI button** — its name on the glass (AI or OK), a hold's length, and how a hold, a wake and power-off differ.
- **The keyboard's layout** — six 64 px keys a row.
- **The dither over many partials** — whether it ghosts before the full refresh clears it.
- **The clock's cost** — a minute tick makes a full refresh every twenty minutes on a still screen — and whether the
  lock screen's clock is 24-hour or shows AM and PM in the status bar.
- **A spoken mode** — none is possible on this hardware; who that leaves out, and what would serve them.
- **A press answered without sound** — how a muted device, or a person who cannot hear the tick, knows a press landed
  before the next frame does.
- **The glass's real contrast** — its white and black measured against the nominal 21 : 1.
- **Whether a 2 px hairline and `caption` at 20 px read from a metre away** — on a V1.0 and a V2.0 board.
- **How many times a full refresh flashes**, against WCAG's three a second.
- **Press-to-start and press-to-stop recording** beside the hold.
- **A larger text setting**, and the glyph ranges every string needs.
- **The buzzer's tones** for a tick, an ignored press, a success, a failure, a wait, and a recording's start and stop.
