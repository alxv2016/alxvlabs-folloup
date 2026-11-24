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
    sparse:  # two patterns lighter than level 2, drawn by a disabled control's words and, the eighth, by elevation 1's offset — no panel level, a UI fill on every refresh
      sixteenth: "#F8F8F8"  # one black pixel in sixteen, in linear light — words on a disabled control's face
      eighth: "#F0F0F0"  # one black pixel in eight, in linear light — a disabled row's name on the white page
  background: "{colors.white}"  # the glass behind everything
  surface-action: "{colors.black}"  # the one action a screen is for
  surface-chosen: "{colors.black}"  # a chosen row, chip or option — never focus
  surface-on: "{colors.black}"  # a toggle that is on, a slider's fill, a scroll indicator's thumb
  text-primary: "{colors.black}"  # every word that must read
  text-secondary: "{colors.scale.primary.1}"  # supporting words and timestamps; black after a partial refresh
  text-on-fill: "{colors.white}"  # words on any black surface
  outline: "{colors.black}"  # control and card edges
  divider: "{colors.scale.primary.1}"  # the rule between rows; black after a partial refresh
  focus-ring: "{colors.black}"  # where the buttons' focus is, outside every control, black on the white glass
  band: "{colors.scale.primary.2}"  # a light-dither fill: a control's track, a chip, a card, decoration; black words only
  thumb: "{colors.black}"  # a toggle's thumb on its off track, the light dither
  thumb-on: "{colors.white}"  # a toggle's thumb on its black, on track
  knob: "{colors.white}"  # a range slider's knob bar on its black fill
  text-disabled: "{colors.scale.sparse.sixteenth}"  # words and marks on a disabled control's face; exempt from contrast
  text-disabled-page: "{colors.scale.sparse.eighth}"  # a disabled row's name, state word and a slider's knob and value on white; exempt
  shadow-1: "{colors.scale.sparse.eighth}"  # elevation 1's offset: one black pixel in eight; no words or marks on it
  shadow-2: "{colors.scale.primary.2}"  # elevation 2's offset, a bottom sheet's: the light dither
  shadow-3: "{colors.scale.primary.1}"  # elevation 3's offset, a dialog's: the dark dither
  keyline: "{colors.white}"  # the line round an offset's outer edge, so it keeps an edge over a dithered ground
typography:
  # The primitives: Bitcount Single at one bit a pixel, each size a multiple of 10 px. Its line is 1.2 em to 40 px, then tapers,
  # each step on the 4 px grid and never under the font's own height: 56 at 50, 64 at 60, 72 at 70, 80 at 80; the figures,
  # 100 px and up, take the height of their digits.
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
  scale-30-regular:  # the only regular weight: body
    fontFamily: Bitcount Single
    fontSize: 30px
    fontWeight: 400
    lineHeight: 36px
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
    lineHeight: 56px
  scale-50-bold:
    fontFamily: Bitcount Single
    fontSize: 50px
    fontWeight: 700
    lineHeight: 56px
  scale-60-semibold:
    fontFamily: Bitcount Single
    fontSize: 60px
    fontWeight: 600
    lineHeight: 64px
  scale-60-bold:
    fontFamily: Bitcount Single
    fontSize: 60px
    fontWeight: 700
    lineHeight: 64px
  scale-70-semibold:
    fontFamily: Bitcount Single
    fontSize: 70px
    fontWeight: 600
    lineHeight: 72px
  scale-70-bold:
    fontFamily: Bitcount Single
    fontSize: 70px
    fontWeight: 700
    lineHeight: 72px
  scale-80-semibold:
    fontFamily: Bitcount Single
    fontSize: 80px
    fontWeight: 600
    lineHeight: 80px
  scale-80-bold:
    fontFamily: Bitcount Single
    fontSize: 80px
    fontWeight: 700
    lineHeight: 80px
  scale-100-semibold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 100px
    fontWeight: 600
    lineHeight: 68px
  scale-100-bold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 100px
    fontWeight: 700
    lineHeight: 68px
  scale-120-semibold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 120px
    fontWeight: 600
    lineHeight: 80px
  scale-120-bold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 120px
    fontWeight: 700
    lineHeight: 80px
  scale-160-semibold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 160px
    fontWeight: 600
    lineHeight: 108px
  scale-160-bold:  # figures only
    fontFamily: Bitcount Single
    fontSize: 160px
    fontWeight: 700
    lineHeight: 108px
  # The roles: each is one primitive, named beside it.
  hero:  # = typography.scale-60-bold
    fontFamily: Bitcount Single
    fontSize: 60px
    fontWeight: 700
    lineHeight: 64px
  display:  # = typography.scale-50-bold
    fontFamily: Bitcount Single
    fontSize: 50px
    fontWeight: 700
    lineHeight: 56px
  headline:  # = typography.scale-40-bold
    fontFamily: Bitcount Single
    fontSize: 40px
    fontWeight: 700
    lineHeight: 48px
  body:  # = typography.scale-30-regular on a 40 px line
    fontFamily: Bitcount Single
    fontSize: 30px
    fontWeight: 400
    lineHeight: 40px
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
  caption-loose:  # = typography.scale-20-semibold on a 1.5 em line: a text view's lines
    fontFamily: Bitcount Single
    fontSize: 20px
    fontWeight: 600
    lineHeight: 30px
rounded:
  none: 0px
  sm: 4px
  md: 8px
  lg: 12px  # the deck's base radius: buttons, chips, rows, cards, a text view's ring, a range slider's track (its fill's `md` and the 4 px of track around it)
  xl: 16px  # a segment's track: its items' `md` and the 8 px of track around them; a text input's frame: its square icon button 16 px in
  2xl: 32px  # a bottom sheet's top corners
  full: 9999px
spacing:
  xs: 4px
  sm: 8px
  md: 16px
  lg: 24px
  xl: 32px
  2xl: 48px
  edge: "{spacing.lg}"  # the screen's side margins
  focus-offset: 4px
  target: 64px
  content: 432px
  status-bar: 44px
  footer: 96px  # buttons 64 px high, 16 over and under, so a ring leaves 8 px to the glass's edge
  list-row-height: 80px
  toggle-height: 44px
  toggle-width: 96px
  toggle-thumb-width: 52px
  slider-height: 52px
  button-width: 212px  # the standard button; a compact one is as wide as its words
  card-inset: 20px  # a choice card's and radio row's words in from the edge
  check-size: 44px  # a checkbox's and a radio's control: the toggle's height
  check-dot: 16px  # a radio's dot on its black disc
  icon: 40px  # the art box of a glyph; its ink is 18 to 33 px
  mark: 42px
  mark-dot: 26px  # the mark less twice its edge and `xs`
  progress: 12px
  progress-lead: 16px  # the bar that runs out first
  step-gap: 12px
  page-dot: 8px
  page-pill: 28px
  scroll-thumb: 8px  # the indicator lane's width, and the shortest piece
  scroll-length: 248px
  scroll-x: 467px  # the lane's left edge on the glass, 1 px clear of the 6 px editing ring round a full-width view
  status-mark: 85px
  status-mark-slot: 73px  # the status block's mark slot: the widest compiled mark (clock, link-off), the art flush left; words stand `edge` + slot + `lg` in
  overlay-offset: 6px  # how far an offset stands from its box, at every elevation level
  key-gap: 12px  # between the keyboard's keys, across and down: a key's 4 px ring 4 px out leaves 4 px
  # The gaps by role, each a step of the scale. A pair's gap is measured between the two texts' own boxes (the font's height, no leading).
  pair-sm: "{spacing.sm}"  # a row's label over its one line
  pair-md: "{spacing.md}"  # a title over a wrapped description; a headline over body
  pair-lg: "{spacing.lg}"  # a display or a hero over body
  pair-xl: "{spacing.xl}"  # a modal's title over its message
  item: "{spacing.md}"  # row to row, button to button
  group: "{spacing.lg}"  # a dialog's message to its buttons; a group's name under a heading and over the group before it
  section: "{spacing.2xl}"  # above a section heading
  stroke:
    hairline: 2px
    default: 3px
    focus: 4px
    strong: 4px
    edit: 6px
elevation:  # how high a surface stands off the page: denser is higher, one distance for every level
  0:  # flat: on the page
    outline: "{spacing.stroke.default}"
  1:  # raised: floats over the page without taking it over; no surface yet
    outline: "{spacing.stroke.strong}"
    shadow: "{colors.shadow-1}"
    offset: "{spacing.overlay-offset}"
    keyline: "{spacing.stroke.hairline}"
  2:  # anchored: a bottom sheet, offset up from its one exposed edge
    outline: "{spacing.stroke.strong}"
    shadow: "{colors.shadow-2}"
    offset: "{spacing.overlay-offset}"
    keyline: "{spacing.stroke.hairline}"
  3:  # floating: a dialog, offset down and to the right
    outline: "{spacing.stroke.strong}"
    shadow: "{colors.shadow-3}"
    offset: "{spacing.overlay-offset}"
    keyline: "{spacing.stroke.hairline}"
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
  password-reveal:  # a masked field's last typed character shows this long, then becomes a bullet (Exceptions)
    duration: 0ms
    delay: 1500ms
    timingFunction: steps(1, start)
  key-repeat-delay:  # a held UP or DOWN steps again once it has been held this long (Focus and input)
    duration: 0ms
    delay: 600ms
    timingFunction: steps(1, start)
  key-repeat-period:  # then steps again this often while it is held, each step's frame landed first
    duration: 0ms
    delay: 500ms
    timingFunction: steps(1, start)
sound:  # the buzzer's words, each in the agent deck's note format: wave, a start and an end pitch (0 for no slide), a length, on or off; a pitch of 0 is a rest (Motion → Sound)
  band-low: 2000Hz   # every pitch at or above, round the buzzer's 4 kHz resonance
  band-high: 6000Hz  # and at or below
  tick-max: 40ms     # a tick, on every press and every repeated step
  max: 600ms         # any other sound
  tick:  # every press, and every repeated step of a held key
    - {wave: pulse-50, hz: 4000, hzEnd: 0, ms: 30, volume: 255}
  at-end:  # a step that can go no further: two ticks, lower than the press's
    - {wave: pulse-50, hz: 2637, hzEnd: 0, ms: 25, volume: 255}
    - {wave: pulse-50, hz: 0, hzEnd: 0, ms: 45, volume: 0}
    - {wave: pulse-50, hz: 2637, hzEnd: 0, ms: 25, volume: 255}
  error:  # that did not work: "womp womp", two notes down and the last sliding lower
    - {wave: pulse-50, hz: 4186, hzEnd: 0, ms: 150, volume: 255}
    - {wave: pulse-50, hz: 0, hzEnd: 0, ms: 50, volume: 0}
    - {wave: pulse-50, hz: 3322, hzEnd: 0, ms: 150, volume: 255}
    - {wave: pulse-50, hz: 0, hzEnd: 0, ms: 50, volume: 0}
    - {wave: pulse-50, hz: 2794, hzEnd: 2093, ms: 200, volume: 255}
  success:  # only once the action succeeded: a rise of a fourth
    - {wave: pulse-25, hz: 3136, hzEnd: 0, ms: 80, volume: 255}
    - {wave: pulse-25, hz: 4186, hzEnd: 0, ms: 180, volume: 255}
  recording-start:  # a bip and a higher bip a fifth up
    - {wave: pulse-25, hz: 3520, hzEnd: 0, ms: 70, volume: 255}
    - {wave: pulse-25, hz: 0, hzEnd: 0, ms: 45, volume: 0}
    - {wave: pulse-25, hz: 5274, hzEnd: 0, ms: 120, volume: 255}
  recording-stop:  # the same pair falling
    - {wave: pulse-25, hz: 5274, hzEnd: 0, ms: 70, volume: 255}
    - {wave: pulse-25, hz: 0, hzEnd: 0, ms: 45, volume: 0}
    - {wave: pulse-25, hz: 3520, hzEnd: 0, ms: 120, volume: 255}
components:
  status-bar:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
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
    rounded: "{rounded.lg}"
    height: "{spacing.target}"
    width: "{spacing.button-width}"
    padding: "{spacing.md}"
  button-outline:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.lg}"
    height: "{spacing.target}"
    width: "{spacing.button-width}"
    padding: "{spacing.md}"
  list-row:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.lg}"
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
  timeline-chip:  # a rounded box, never a pill; chosen is a black edge of `spacing.stroke.strong`, the fill unchanged
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.lg}"
    height: "{spacing.target}"
    padding: "{spacing.lg}"
  toggle:
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-primary}"
    rounded: "{rounded.full}"
    height: "{spacing.toggle-height}"
    width: "{spacing.toggle-width}"
  toggle-on:
    backgroundColor: "{colors.surface-on}"
    textColor: "{colors.text-on-fill}"
  toggle-thumb:
    backgroundColor: "{colors.thumb}"
    rounded: "{rounded.full}"
    width: "{spacing.toggle-thumb-width}"
  toggle-thumb-on:
    backgroundColor: "{colors.thumb-on}"
    rounded: "{rounded.full}"
    width: "{spacing.toggle-thumb-width}"
  checkbox:  # the toggle's look as a square: off is the light dither inside a `spacing.stroke.default` outline
    backgroundColor: "{colors.band}"
    rounded: "{rounded.md}"
    size: "{spacing.check-size}"
  checkbox-on:  # black with a white check, the toggle's thumb
    backgroundColor: "{colors.surface-on}"
    textColor: "{colors.thumb-on}"
    rounded: "{rounded.md}"
    size: "{spacing.check-size}"
  radio:
    backgroundColor: "{colors.band}"
    rounded: "{rounded.full}"
    size: "{spacing.check-size}"
  radio-on:  # black with a white dot
    backgroundColor: "{colors.surface-on}"
    textColor: "{colors.thumb-on}"
    rounded: "{rounded.full}"
    size: "{spacing.check-size}"
  radio-dot:
    backgroundColor: "{colors.thumb-on}"
    rounded: "{rounded.full}"
    size: "{spacing.check-dot}"
  icon-button:  # a square of `spacing.target`; the four button variants, and the toggle icon button black when on
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-primary}"
    rounded: "{rounded.lg}"
    size: "{spacing.target}"
  icon-button-on:
    backgroundColor: "{colors.surface-on}"
    textColor: "{colors.text-on-fill}"
    rounded: "{rounded.lg}"
    size: "{spacing.target}"
  button-tonal:
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.lg}"
    height: "{spacing.target}"
    width: "{spacing.button-width}"
    padding: "{spacing.md}"
  button-ghost:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.lg}"
    height: "{spacing.target}"
    width: "{spacing.button-width}"
    padding: "{spacing.md}"
  value-row:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.lg}"
    height: "{spacing.target}"
    padding: "{spacing.md}"
  value-row-value:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-secondary}"
    typography: "{typography.label}"
  detail-row:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-secondary}"
    typography: "{typography.caption}"
    padding: "{spacing.sm}"
  section-heading:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-secondary}"
    typography: "{typography.caption}"
  segment:
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.xl}"
    height: "{spacing.target}"
    padding: "{spacing.sm}"
  segment-chosen:
    backgroundColor: "{colors.surface-chosen}"
    textColor: "{colors.text-on-fill}"
    rounded: "{rounded.md}"
  progress-bar:  # round-capped pieces: the black fill, then the light-dither track after a gap of `spacing.sm`
    backgroundColor: "{colors.band}"
    textColor: "{colors.surface-action}"
    rounded: "{rounded.full}"
    height: "{spacing.progress}"
  page-dot:  # a hollow dot; the current page is a black pill `spacing.page-pill` wide
    backgroundColor: "{colors.background}"
    rounded: "{rounded.full}"
    size: "{spacing.page-dot}"
  page-pill:
    backgroundColor: "{colors.surface-action}"
    rounded: "{rounded.full}"
    width: "{spacing.page-pill}"
    height: "{spacing.page-dot}"
  radio-row:
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.lg}"
    height: "{spacing.list-row-height}"  # a floor: the row grows with its words, `spacing.xs` above and below them
    padding: "{spacing.card-inset}"
  choice-mark:
    backgroundColor: "{colors.background}"
    textColor: "{colors.surface-action}"
    rounded: "{rounded.full}"
    size: "{spacing.mark}"
  choice-card:
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-primary}"
    typography: "{typography.caption}"
    rounded: "{rounded.lg}"
    padding: "{spacing.card-inset}"
  text-view:  # no box: its corners shape its ring, and its words stand padding inside the view
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.caption-loose}"
    rounded: "{rounded.lg}"
    padding: "{spacing.sm}"
  range-slider:
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-primary}"
    typography: "{typography.caption}"
    rounded: "{rounded.lg}"
    height: "{spacing.slider-height}"
    padding: "{spacing.xs}"
  range-slider-fill:
    backgroundColor: "{colors.surface-on}"
    textColor: "{colors.text-on-fill}"
    typography: "{typography.caption}"
    rounded: "{rounded.md}"
  range-slider-knob:
    backgroundColor: "{colors.knob}"
    rounded: "{rounded.full}"
    width: "{spacing.sm}"
  carousel-card:  # white inside a `spacing.stroke.default` outline; never the light dither
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.body}"   # its title `display`
    rounded: "{rounded.lg}"
    padding: "{spacing.card-inset}"
  roller:  # a value in a long ordered list or range: three rows of `spacing.target`, the value between its neighbours
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.lg}"
    height: "{spacing.target}"   # one row; three make the roller
    padding: "{spacing.lg}"      # either side of its widest value
  roller-neighbour:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-secondary}"
    typography: "{typography.caption}"
  roller-open:  # the middle row inverted, as `sheet-option-chosen`
    backgroundColor: "{colors.surface-chosen}"
    textColor: "{colors.text-on-fill}"
    rounded: "{rounded.md}"
  scroll-indicator:  # the thumb; the track pieces round it are the light dither
    backgroundColor: "{colors.surface-on}"
    width: "{spacing.scroll-thumb}"
    height: "{spacing.scroll-length}"
  status-block:
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
  dialog:  # elevation 3: a `spacing.stroke.strong` outline, and an offset in the dark dither down and right
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.body}"
    rounded: "{rounded.md}"
    padding: "{spacing.lg}"
    elevation: "{elevation.3}"
  network-row:  # a Wi-Fi network: its name over its facts, its signal at the right end
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.caption}"
    rounded: "{rounded.lg}"
    height: "{spacing.list-row-height}"
    padding: "{spacing.md}"
  network-row-chosen:  # the network in use or being joined: the inversion
    backgroundColor: "{colors.surface-chosen}"
    textColor: "{colors.text-on-fill}"
  text-input:  # the frame of a text or password input; its words `label`, its placeholder `caption`
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.xl}"
    height: "{spacing.target}"
    padding: "{spacing.md}"
  text-input-disabled:  # the light dither, no outline, its words and button glyph `text-disabled`
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-disabled}"
    typography: "{typography.label}"
    rounded: "{rounded.xl}"
    height: "{spacing.target}"
  input-icon-button:  # the eye or the cross inside a field: `spacing.xl` square, square corners, hit across `spacing.target`
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    rounded: "{rounded.none}"
    size: "{spacing.xl}"
  key:  # a character key of the keyboard: the light dither, no edge, black words
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.md}"
    height: "{spacing.target}"
    width: "{spacing.target}"
  key-chrome:  # Shift, Delete, 123, ABC, #+= and Space: white inside a `spacing.stroke.default` edge
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.md}"
    height: "{spacing.target}"
  key-chosen:  # Shift while the capitals page is on
    backgroundColor: "{colors.surface-chosen}"
    textColor: "{colors.text-on-fill}"
    typography: "{typography.label}"
    rounded: "{rounded.md}"
    height: "{spacing.target}"
  dropdown:  # a value row with a select chevron beside the value
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.lg}"
    height: "{spacing.target}"
    padding: "{spacing.md}"
  sheet:  # the select list, elevation 2: full width, anchored to the glass's bottom, a strong outline and an offset up from its top edge
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.2xl}"
    padding: "{spacing.edge}"
    elevation: "{elevation.2}"
  sheet-option:  # a row of the sheet's list view, 64 px at least and growing with its words
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.body}"   # the chosen one `label`
    rounded: "{rounded.lg}"
    height: "{spacing.target}"
    padding: "{spacing.md}"
  sheet-option-chosen:
    backgroundColor: "{colors.surface-chosen}"
    textColor: "{colors.text-on-fill}"
  horizontal-pager:  # a strip of chips that turns by page, with its page in words ("1 of 2") under it — a list view's, when its screen asks, is the same words
    backgroundColor: "{colors.band}"
    textColor: "{colors.text-primary}"
    typography: "{typography.label}"
    rounded: "{rounded.lg}"
    height: "{spacing.target}"
    padding: "{spacing.md}"
  status-line:  # plain words between two rules, no box: the lower rule shows only while news does
    backgroundColor: "{colors.background}"
    textColor: "{colors.text-primary}"
    typography: "{typography.caption}"
    height: "{spacing.list-row-height}"
    padding: "{spacing.edge}"
  divider:
    backgroundColor: "{colors.divider}"
    height: "{spacing.stroke.hairline}"
  focus-ring:
    backgroundColor: "{colors.focus-ring}"
    width: "{spacing.stroke.focus}"
  focus-ring-edit:
    backgroundColor: "{colors.focus-ring}"
    width: "{spacing.stroke.edit}"
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
6. **Corners are concentric, always.** A rounded element inside a rounded parent shares the parent's corner centre,
   and so does every focus ring round a rounded element — a button inside a field, an item inside its track, a ring
   round a control: two curves at a corner never run at odds (the arithmetic is DESIGN → Shapes → Corners).

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
  every refresh keeps: `scale.primary.1` as one white pixel in four, `scale.primary.2` as three in four. A disabled
  control's words take two sparser patterns, `scale.sparse`, one black pixel in sixteen and one in eight — no panel
  level, so `scale.primary` is the panel's four greys and `scale.sparse` the two patterns beside them, for those words and, the
  eighth, for elevation 1's offset.
- **Text and lines are never dithered** — a grey text or line role draws solid: level 1 black, level 2 white — except
  a disabled control's words, `text-disabled` and `text-disabled-page`, which draw in their sparse dither, still one
  bit a pixel (DESIGN → Components → States → Disabled), and the unlit bars of a signal glyph (DESIGN → Exceptions).

### Roles

- **`background`** — white: the glass behind everything.
- **`text-primary`, `outline`, `focus-ring`** — black: every word that must read, every edge, where the buttons'
  focus is.
- **`surface-action`, `surface-chosen`, `surface-on`** — black: the one action a screen is for, a chosen segment item
  or home row, a toggle, checkbox or radio that is on, a slider's fill or a scroll indicator's thumb. Inversion shows what is chosen or on, never focus, which is
  always the ring; each job keeps its own role, so one can change without the others.
- **`text-on-fill`** — white: words on any black surface.
- **`text-secondary`, `divider`** — dark grey: supporting words, timestamps, the rule between rows. Black after a
  partial refresh, so never the only difference between two things.
- **`band`** — light grey as a dither fill: a toggle's, segment's or slider's track, a chip's, card's or tonal button's
  fill, a progress bar's track, decoration, on every refresh. Words on it are `text-primary`, black, only; never an
  edge, never the only sign of a state.
- **`thumb`, `thumb-on`, `knob`** — a toggle's thumb, black on its dither track when off and white on its black track
  when on, which a checkbox's check and a radio's dot are too; a range slider's knob bar, white on its black fill. Each is the mark that clears 3 : 1 on its own surface
  — the thumb on `band`, the other two on `surface-on` — so each flips with it.

- **`shadow-1`, `shadow-2`, `shadow-3`, `keyline`** — an offset's dither by elevation level — the eighth (one black pixel in
  eight), the light dither and the dark dither — and the white line that rings its outer edge (Elevation & Depth). No words or
  marks stand on an offset, so it needs no contrast pair; it is decoration and exempt.
- **`text-disabled`, `text-disabled-page`** — sparse dithers, one black pixel in sixteen and in eight: the words and
  marks on a disabled control's light-dither face, and a disabled row's name and state word, a disabled slider's knob
  and value, a disabled radio mark's dot on the white. Each is one step lighter than the ground it sits on, so a
  disabled control looks inert; exempt from contrast, as a disabled control is, and never the only sign of a state —
  the reason is in words.

### Contrast floors

**Text meets 4.5 : 1 and a non-text part 3 : 1, on a full refresh and on a partial one.**

| Pair | Full refresh | Partial refresh |
| --- | --- | --- |
| Black on white — text, outlines, the focus ring | 21 : 1 | 21 : 1 |
| Dark grey on white — supporting text, dividers, on a greyscale frame only | 7.5 : 1 | 21 : 1 |
| White on black — words on a chosen row or the action button | 21 : 1 | 21 : 1 |
| Dark grey on black | 2.8 : 1 — never painted | 1 : 1 |

- **These are the levels' ratios, not the glass's** — e-paper's white is paper grey and its black is not ink black,
  so each ratio is an upper bound on the device. That is why words are only ever black on white or white on black.
- **A disabled control is exempt** — its words are 1.23 : 1 on the light-dither face and 1.14 : 1 on the white (the
  patterns' averages in linear light), held by nothing but legibility on the glass and the reason in `caption`, solid
  `text-secondary`, beside or under it.
- **Every pair, with its proof on both refreshes, is in STICKY_DESIGN_TOKENS §2.**

## Typography

### Face

- **Bitcount Single, compiled into the firmware** — a pixel font, each letter built from square cells, under the SIL
  Open Font License; at one bit a pixel its strokes are the glass's own pixels.
- **Weights 600 and 700, and one 400.** A lighter weight breaks up at one bit a pixel, so the regular weight is cut at
  one size only: `body`, 30 px, whose 3 px strokes keep its gaps open at 40 px of line. Hierarchy comes from size and weight,
  never from grey alone. Below 50 px the 600 and 700 look alike, so size carries the hierarchy there, and `body`'s 400
  against `label`'s 700 is the one pair at 30 px that differs by weight.
- **Every size is a multiple of 10 px** — the font's cell is a tenth of the em, so each cell is whole pixels.
- **Glyphs are drawn at one bit a pixel** on every refresh, so a stroke is solid black, never a grey edge that
  thresholds into a ragged one — a disabled control's words are the one text in a dither (`text-disabled`), and
  still one bit a pixel: every stroke pixel is the pattern, with no grey edge.

### Scale and roles

At 235 ppi a pixel is 0.108 mm and a point is 3.26 px.

- **The scale is the primitives** — 20 to 80 px in steps of 10, and 100, 120 and 160 px for figures, each at 600 and
  700 (`typography.scale-30-semibold` and its siblings). Lines are 1.2 em apart up to 40 px and closer as the size
  grows: 56 px at 50, 64 at 60, 72 at 70 and 80 at 80, each on the 4 px grid and never under the font's own height.
  Figures sit on the height of their digits: 68 px at 100, 80 at 120 and 108 at 160. Thirty px also comes at 400,
  `typography.scale-30-regular`, and `body` sets it on a 40 px line. A screen takes a role, never a primitive. Only the
  hardware test's type scale page shows the primitives themselves (DESIGN → Exceptions).
- **A role is one primitive:**

| Role | Primitive | Size | Use |
| --- | --- | --- | --- |
| `hero` | `scale-60-bold` | 60 px, 18.4 pt | A figure or a word taken in at a glance — never the clock, which has a size of its own |
| `display` | `scale-50-bold` | 50 px, 15.3 pt | One per screen at most: a sticky's words |
| `headline` | `scale-40-bold` | 40 px, 12.3 pt | A screen's title |
| `body` | `scale-30-regular` on a 40 px line | 30 px, 9.2 pt | Everything people read: a transcript, a summary, a note; a row's state and value words; a dropdown's options; a status block's description |
| `label` | `scale-30-bold` | 30 px, 9.2 pt | The status bar's title, buttons, hints, tags, a row's title, a choice card's name, the chosen dropdown option, a field's typed text — `body`'s size, in bold |
| `caption` | `scale-20-semibold` | 20 px, 6.1 pt | Timestamps, a field's placeholder, a Wi-Fi network's name and its facts, the status band's news — the smallest words on the glass, 12 px a character |
| `caption-loose` | `scale-20-semibold` on a 30 px line | 20 px, 6.1 pt | A text view's lines: `caption` on a line 1.5 em apart, easy to follow back |

### Reading

- **24 characters of `body` a line** across the content width — wrap, never truncate, words people read.
- **Truncate only a label whose full words are one press away.**
- **`hero` and `display` are large text** for contrast — WCAG's 14 pt bold.
- **`hero` and `display` set one or two lines** — a third line is `body`.
- **Figures keep their width** — every character is the same width — so a changing number, the clock, the battery, a
  recording's timer, "3 of 5", redraws the same area.

## Layout

### The one glass

- **480 × 800 portrait, one to one** — the 3.97-inch 800 × 480 panel turned on its side, at 235 ppi. Nothing adapts,
  because the glass never changes size; every screen keeps the same regions in the same places, but for the optional status band.
- **A mock is drawn at 480 × 800 px in both its full-refresh and its partial-refresh form**, and judged on the glass of
  a V1.0 and a V2.0 board.

### Regions

- **The status bar** — `spacing.status-bar` along the top edge: the screen's title, on white over a hairline; the
  device's own state has no place on the glass yet (Open items).
- **The status band** — optional: a screen that can get news reserves `spacing.list-row-height` between its title bar and its
  content, and leaves it white until news comes, so nothing moves when it does; its words stand between the title bar's
  rule above and a `spacing.stroke.hairline` rule below that shows only with news, no box and no fill. A screen that
  cannot get news has none; it is the one region on some screens and not others, by the person's decision.
- **The content** — one column inside `spacing.edge` margins, `spacing.content` wide, one primary item per screen.
- **The footer** — `spacing.footer` along the bottom edge: the screen's action buttons, 96 px so a 64 px button has 16 over and under it and its ring leaves 8 px to the glass's edge. The action
  button sits in the footer's centre; the buttons sit `spacing.md` apart. A bottom sheet covers the footer, and its own foot,
  Cancel, stands in the footer's place. A footer holds no legend of what UP, DOWN and OK do (Accessibility); until the Help screen ships, the roller's and the dropdown sheet's footers still carry one (Open items).

### Spacing

- **A 4 px base** — `xs` 4, `sm` 8, `md` 16, `lg` 24, `xl` 32, `2xl` 48 px. Related things sit closer than groups.
- **A pair's gap is measured between the two texts' own boxes**, the font's height with no leading. It scales with the larger
  text: about 0.3 to 0.5 em, rounded to the scale. A row's label over its one line is `pair-sm`, 8. A title over a wrapped
  description, or a `headline` over `body`, is `pair-md`, 16. A `display` or `hero` over `body` is `pair-lg`, 24. A modal's title
  over its message is `pair-xl`, 32. A pair's gap is always under the gap to the next group.
- **The gaps between components are measured between line or component boxes**, never between font boxes. Row to row and
  button to button is `item`, 16. A dialog's message to its buttons is `group`, 24, and so is a group's name to the heading over
  it and to the group before it. A section heading that follows other content has `section`, 48, over it. The first heading on a
  page keeps `lg` under the page's top padding. The screen's side margins are `edge`, 24.
- **A section heading's words start at `edge`** over boxes and controls, and over the setting rows whose words stand there. They
  start `md` in over rows whose words stand 16 in: value rows, detail rows, list rows, network rows, dropdowns and status blocks.
- **Every touch target is `spacing.target`, 64 px — 6.9 mm.** A control may draw smaller and pad its hit area — a
  toggle draws `spacing.toggle-height`, 44 px, a checkbox or radio `spacing.check-size`, 44 px, and each takes touches
  across 64.
- **Focus stops sit `spacing.md` apart or more**, side by side and stacked: a ring reaches 8 px, so no ring touches a
  neighbour, and a container that holds a stop lets its ring out or pads it.

### Paging

- **Content turns by page, never scrolls by pixel** — UP and DOWN move focus, and focus leaving the glass turns the
  page; a swipe up or down turns one page, as the buttons do. A pixel scroll would refresh the glass on every step.
- **A page shows only its own rows** — a row that grew with its words takes the room of more than one, and the next page's
  first row never shows in what is left: a list's band is cut to the page's last row and the focus ring's reach under it.
- **Focus wraps across pages** — DOWN on the last row of the last page goes to the first row of the first page, a page
  turn like any other.
- **A page with nothing to focus is still a stop** — a page of facts takes UP and DOWN as a page of rows does, one press
  a page, and the ring round it shows where the person is. Focus wraps over it like any other stop.

## Elevation & Depth

### Flat on white

- **Depth is a line and a hard offset, never a blur or a gradient** — a blur is greys a partial refresh throws away,
  and a gradient bands into four steps.
- **A raised surface — a dialog, a bottom sheet — has a `spacing.stroke.strong` outline and an offset**: its own shape
  `spacing.overlay-offset` away, in the dither of its elevation level (below), never solid black, ringed on its outer side by a
  `spacing.stroke.hairline` keyline in white. Over white the keyline is invisible; over a dithered ground — a disabled field, a
  card, a chip the sheet's edge crosses — it is what keeps the offset an edge. The outline, black on white, 21 : 1, is the
  surface's boundary whatever the offset does: the offset is decoration, exempt from contrast, and never the only sign of
  anything. A full-glass overlay — the keyboard — has no edge to lift off, so it draws neither.

### Elevation

- **A level says how a surface stands over the page, never what is chosen, focused, disabled or busy**; each is a token group
  in the front matter — an outline, an offset's dither, its distance and its keyline — and a component points at its level.

| Level | Surface | Offset's dither | Black share |
| --- | --- | --- | --- |
| `elevation.0` flat | everything on the page: rows, cards, buttons, fields, chips | none; the `spacing.stroke.default` outline where the control has one | — |
| `elevation.1` raised | reserved: the first surface that floats over the page without taking it over — a popover or a menu; none exists yet | `shadow-1`, the eighth | 12.5 % |
| `elevation.2` anchored | the select list's bottom sheet, and any surface anchored to the glass's edge | `shadow-2`, the light dither | 25 % |
| `elevation.3` floating | the dialog, and any surface that interrupts | `shadow-3`, the dark dither | 75 % |

- **Denser is higher** — the offset nearest black is the one the eye reads as farthest from the page. A dialog stands over a
  sheet (a confirmation can open from inside an open sheet, and nothing but the dialog is reachable while it is up), so it takes
  the heavier offset; a sheet touches the glass on three edges, a dialog floats with four free.
- **One distance and one keyline for every level** — `spacing.overlay-offset`, 6 px, and 2 px, which together reach 8 px from the
  box, inside the 10 px a focus ring's container lets out. A distance that grew with the level would thin the lightest dither to a
  column of dots.
- **Direction belongs to the surface** — a floating box's offset falls down and to the right; a surface anchored to the glass's
  edge has one exposed edge, so its offset is its own shape moved from that edge: up from a sheet's top.
- **Where an offset cannot be told from its ground** — level 2 over the light dither is the same pattern as the ground, and the
  eighth over it is 1.16 : 1; the keyline and the outline carry the edge there, and judging them on the glass is open.

### Dither

- **A dither is the only grey a UI element shows** — a band, a toggle's, segment's or slider's track, a chip's, card's or
  tonal button's fill, a progress bar's empty part with its step in words, a scroll indicator's track, an overlay's offset, a
  disabled control's face, with no outline and the reason in words. The two patterns read as two distinct greys on the
  V1.0's glass, close to the true greys.
- **Words sit only on the light dither, in black** — 16 : 1 against its average; never on the dark dither, where
  white words average 3.5 : 1 and their edges break up. A disabled control's words are the one exception: a sparser
  dither than their ground, the sparse patterns of Colors → The four levels, lighter than the face they stand on.
- **Fills are opaque** — a translucent fill blends to other greys and draws solid; a rounded fill's edge pixels draw
  black or white, a one-bit edge.

## Shapes

### Strokes

- **Nothing thinner than `spacing.stroke.hairline`, 2 px** — a one-pixel line breaks up and ghosts on e-paper.
- **`spacing.stroke.default`, 3 px, outlines controls; `spacing.stroke.focus` and `spacing.stroke.strong`, 4 px, the
  focus ring, overlays, a chosen chip's edge and a choice mark's; `spacing.stroke.edit`, 6 px, the ring of a control open
  for editing — the one exception to the focus ring's `spacing.stroke.focus`.**

### Corners

- **Small and few** — `rounded.sm` for stickies and tags, `rounded.md` for an inset item and a checkbox,
  `rounded.lg` for buttons, chips, rows, cards, a text view's ring and a range slider's track — the deck's one base
  radius — `rounded.xl` for a segment's, `rounded.2xl` for a bottom sheet's top corners, `rounded.full` for toggles, progress pieces, a radio and its mark and pagination.
  A chip is a rounded box, never a pill.
- **A nested corner is the outer radius less the gap between them**, never the same radius twice: a slider's fill is
  `rounded.md` inside its track's `rounded.lg` and 4 px of track, a segment's item `rounded.md` inside its track's
  `rounded.xl` and 8 px of track, an icon button inside a field the field's radius less its inset — a text input's
  `rounded.xl`, 16, less the 16 px between button and edge is 0, so the button is square and its ring's outer radius
  `rounded.md` shares the frame's corner centre. A child inset by the parent's radius or more is square.
- **A ring's corner is the control's radius plus the gap out to it** — its offset and its stroke — so the ring shares
  the control's corner centre: its outer corner is 20 round a `rounded.lg` control and its inner edge 16, and round a
  square control its outer corner is 8.

## Motion

### Nothing animates

- **Every motion token is a step** — `steps(1, start)`, no duration: a change is the next frame. No slide, fade, scale,
  spinner, blinking cursor or marquee; e-paper cannot show them, and trying costs refreshes.
- **The refresh is the motion.** A press and a selection land in one partial refresh, and so does a change of screen
  on the V2.0; on the V1.0 a change of screen is two, through white, 1 to 1.25 s, so the old page leaves no ghost. A full
  refresh — the whole glass flashing — is kept for the first frame after waking, the update after twenty partial
  refreshes, closing a full-glass overlay (the keyboard), and the frame left on the glass before sleep. A bottom sheet or a
  dialog is part of the page it stands on, and opens and closes on a partial refresh; the ghost it may leave is judged on the
  glass. While the keyboard is open only the presses that type, and a page switch, count toward the twenty — the ring moving
  between rows and keys does not, but after sixty of those a full refresh is drawn.
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
- **A held UP or DOWN steps at the glass's pace** — after `motion.key-repeat-delay`, then every
  `motion.key-repeat-period`, each step drawn before the next, and each with its tick (Focus and input).

### Waits and things that update on their own

- **A wait shows nothing for its first second** (`motion.wait-indicator`), then still words of what is happening.
- **Nothing updates on its own faster than every 5 s** (`motion.self-update`) — a recording's timer, a wait's progress
  — and the clock once a minute (`motion.clock-tick`), except a masked field's last character, which hides itself after
  `motion.password-reveal`, once per character (Exceptions → A password's last character).
- **A covered screen stops updating itself** — while a sheet, a dialog or the keyboard stands over it, its own updates wait,
  and when it is uncovered it reads again at once.
- **Nothing leaves on a timer** — a message stays until the next change of screen or its caller retracts it. One exception: a masked field's last
  typed character (Exceptions → A password's last character, `motion.password-reveal`).

### Sound

- **Six words, in the 8-bit style of a game's pulse voices, and nothing else sounds** — `sound.tick`, `sound.at-end`,
  `sound.error`, `sound.success`, `sound.recording-start` and `sound.recording-stop`. A new event takes one of them
  before it gets a sound of its own.
- **Every sound is short and in the buzzer's band** — a tick at most `sound.tick-max`, any other at most `sound.max`,
  every pitch from `sound.band-low` to `sound.band-high`.
- **The tick answers every press first** (Press). A press whose step can go no further — a page turned past the last
  or the first, a slider stepped past its end — then sounds `sound.at-end`, which cuts the tick: a key's at-end follows
  its tick within one read of the buttons, a swipe's at the lift.
- **An outcome sounds as its words are set** — `sound.success` only once the action has succeeded, `sound.error` when it
  failed, never for a request merely sent; the sound comes as the words are set, so on e-ink it leads them by the
  refresh that draws them. A destructive action confirmed has the press's tick alone. News the status line says with no
  press sounds `sound.success` or `sound.error` by its kind, as its words are drawn, and info is silent; it adds no seventh
  sound.
- **A recording opens on `sound.recording-start` and closes on `sound.recording-stop`**, and the start sound is never
  in the recording; a recording that fails closes on `sound.error`.

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

- **A button names what happens in one word** — "Save", "Keep", "Discard", "Record" — never "OK" or "Submit"; OK is
  the button's name, never its label. A second word only where one would be unclear. Never restate the title or the
  object it names: under "Delete this note?" the button is "Delete". The icon button's closed set names it by its
  glyph alone (DESIGN → Exceptions).
- **A button's words fit it on one line** — at `label`'s 18 px a character, a full-width button holds 22, a half
  button 10 and a third 6 (19 and 7 beside a glyph, and a third none); a longer name is shortened, never cut off or
  wrapped.
- **An empty screen says what goes there, and how to add it only where the way is not implied** — "No follow-ups yet. Hold AI or press Record to add
  one." — the way to add is said only where it is not implied (Accessibility)
- **An error says what happened and what to do next** — "Couldn't reach Wi-Fi. Check the network in Settings."
- **A wait says what is happening** — "Transcribing…", never "Loading".
- **An ellipsis is the one character "…", never three full stops** — the fonts hold U+2026, and every word the firmware writes
  uses it ("Connecting…"). The one place three full stops stand is a word LVGL cuts to fit its line
  (`LV_LABEL_LONG_MODE_DOTS`): LVGL writes the dots itself, a full stop each, and offers no way to choose the character.

### Dates, times and numbers

- **"Today, 8:12", "Yesterday", then "Mon 17 Nov"**; the clock in the person's 12- or 24-hour setting; a percentage
  with no space before `%`.

## Components

### The kit

**The kit is the product's one set of controls.** Every control a screen shows is built by `components/widgets`.
`components/ui` holds the theme and the regions — the status bar, labels and dividers — and never a control. A control
a second screen needs moves into the kit in the same change, and every caller takes it from there. Nothing builds a
second version of a kit control. Every control the kit offers has its specimen on the kit page, or on the screen it
serves, reached and used by the keys and by touch.

- **Status bar, status band, footer** — the regions; the status band only where a screen gets news (DESIGN → Layout → Regions). The
  status bar's title is `label`; the band's news is `caption`.
- **Button** — the action button (black, one per screen, `button-action`), the outline button (`button-outline`), the
  tonal button (the light dither, black words, `button-tonal`) for a second action that is not Cancel, and the ghost
  button (words alone, `button-ghost`) for a third; a compact button is any of them as wide as its words; the standard
  one is `spacing.button-width` wide, `rounded.lg`, with a `spacing.stroke.default` edge reserved on every variant so
  none changes size; its padding is `spacing.md` either side, inside the edge, in any button of a set width, and
  `spacing.lg` in a compact one. The words are centred and are never clipped: a half button's ten characters (180 px) are
  wider than the 170 px its padding leaves, so they sit over the padding, 11 px from the edge, and a third's six (108 px)
  over 95 px, 9 or 10 px from it. A destructive action is the action button: its words and its dialog carry the danger, never a
  colour. A button may have a glyph before its words: `spacing.icon` of art, its ink then `spacing.md` of white from the
  first letter, the glyph and the words centred as one group in the words' colour, and a half button holds 7
  characters beside one (a glyph of 28 px, 16, and 7 × 18 = 170 px) and a full-width button 19; a footer third holds
  none, so it takes the word alone or an icon button. The glyphs beside words are Record (the microphone), Play, Back
  (the arrow), Save (the check) and Retry (the refresh); one glyph means one thing everywhere. The icon button is a
  square of `spacing.target` with a glyph and no words, in the button's four variants, and the toggle icon button a
  tonal tile that goes black when on, its glyph the current state — both in the closed set of DESIGN → Exceptions; a
  toggle icon row says the state in words beside the tile, as a toggle row does. No button has a pressed state
  (Motion → Press).
- **Toggle** — a pill track `spacing.toggle-width` by `spacing.toggle-height` with a pill thumb
  `spacing.toggle-thumb-width` wide inset `spacing.xs`: off is the light dither with a black thumb left, on a black
  track with a white thumb right. Its state also in words beside it. A toggle row is its name, the state in `body` words and
  the toggle at the content's right edge, and the whole row toggles. A disabled row shows its reason in words under the name,
  `spacing.xs` below it, and stays `spacing.target` high.
- **Checkbox row, radio button row** — the toggle's look as a square or a disc, `spacing.check-size` across (the toggle's
  height, so a column of toggles and boxes shares one right edge and one height): a checkbox `rounded.md`, a radio
  `rounded.full`. Off is the light dither inside a `spacing.stroke.default` outline — the one place it parts from the
  toggle, whose black thumb is the part that identifies it: a dither alone is 1.3 : 1 on the white glass and could not be
  found. On is black with a white mark in `thumb-on` — the check of the glyph set, or a dot of `spacing.check-dot`, small
  enough to read as chosen (a larger white dot in a black disc reads as an empty ring). The ring is round the control
  alone, not the row. A row is its name in `label`, left, growing and wrapping to 20 characters a line, never cut, and
  the control at the content's right edge `spacing.lg` after it, `spacing.target` high at least with the control
  centred in it, and **no state words**: a check and a dot are shapes, so the state is never a grey alone. The whole row
  acts: a tap anywhere on it, or OK on the control, flips a checkbox, or chooses a radio and clears the other rows of
  its group — a group is rows sharing one parent, `spacing.target` apart, its name in `caption` over it; a radio never
  unchooses itself. A disabled row shows its reason as a toggle row does. A choice that is one setting among settings,
  or a short list beside toggle rows, is a checkbox row or a radio button row; a question that is the screen's one
  task is a radio row; an answer that needs a detail line is a choice card.
- **Value row, detail row, section heading** — a setting's name left and its value right in `text-secondary`. Its words stand
  `spacing.md` in from the row's outer edge, and a value row reserves its `spacing.stroke.default` edge inside that. A value
  that does not fit ends in LVGL's dots (three full stops, the one place they stand; Voice & Content) when its full words are
  one press away. A value row's value is in `body`. A fact's name in `caption` over its value is never focused. Both wrap to 33
  and 22 characters a line and are never cut, since nothing is one press away.
  A section heading is a group's name in `caption` above its group. `spacing.section` of room stands over it, which sets it off
  from the content before. The first heading on a page keeps `spacing.lg` under the page's top padding. A group's name under a
  heading stands `spacing.group` under it and over the group before it. Its radio button rows stand `spacing.item` apart.
- **A row's content is centred on its height** — one line or more: a toggle, checkbox, radio, value, home, network or list-view
  row, a choice card, a dropdown field and a select sheet's option all keep their words and controls on the row's
  vertical axis, and a row that grows with its words grows about its centre. A host test holds it (STICKY_DESIGN_TOKENS §9).
- **List row** — a title and a supporting line in `spacing.list-row-height`, 80 px, `rounded.lg`: `spacing.sm` above
  and below, `spacing.md` either side, `spacing.md` from the next row and no rule between; a one-line row is
  `spacing.target`, 64 px. The ring and the gap separate rows.
- **Timeline** — date chips, then the chosen date's recordings as list rows.
- **Chip, segment** — a chip is a rounded box `rounded.lg` as wide as its words, on the light dither, chosen with a
  black edge of `spacing.stroke.strong` and the fill unchanged — the deck's chip, never a pill; a segment is up to four
  choices on one light-dither track `spacing.target` high, `spacing.sm` round the items and between them, the chosen
  item a black pill behind its white word; every item is a focus stop.
- **Sticky** — one follow-up as a card: its tag and its words in `display` or `body`.
- **Carousel** — onboarding, one card at a time. The card is white in a `spacing.stroke.default` outline, `rounded.lg`, filling
  the content's height, `spacing.card-inset` in. Its title is `display` and its words `body`, `spacing.pair-lg` under it. Its page
  in words ("2 of 4", `caption` `text-secondary`) stands with the page dots beside it, an indicator and never a control. Back, Skip
  and Next (Done on the last card) are stops in three fixed slots, a third of the content wide each. Back (outline) is absent on
  the first card, and Skip (outline) stands in the middle slot on every card. A swipe left or right turns one card on the lift,
  with `sound.at-end` past either end. A turn is one partial refresh. Holding OK leaves, as Skip does, at any depth: the screen's
  own escape, which the router runs when nothing is open.
- **Dialog** — a decision the person must make now, with one action button and Cancel. A destructive action is
  one verb under a title that names what it destroys ("Delete", under "Delete this note?"), and its dialog opens with
  focus on Cancel. It sits
  in the middle of the glass, `spacing.edge` from its sides, behind a catcher the size of the glass. It stands on a `spacing.stroke.strong`
  outline, `rounded.md`, `spacing.lg` inside, its title in `label` and its message in `body` `spacing.pair-xl` under it, and `spacing.group` over the two buttons, half width
  `spacing.item` apart. Its offset, `spacing.overlay-offset` down and to the right in `shadow-3`, stands inside the page's margin
  (Elevation & Depth). A touch outside it is eaten (Focus and input).
- **Network row** — a Wi-Fi network in `spacing.list-row-height`, 80 px, `rounded.lg`, `spacing.md` either side and `spacing.sm`
  above and below. Its name and its facts line are both `caption`, the name up to 28 characters and then LVGL's dots. An open
  network says "Open" and a secured one carries a padlock before "Locked". The connected one carries a check before "Connected"
  and then its padlock or "Open", and the one being joined says "Connecting…". At the right end stands its signal, a glyph alone
  with no word, centred on the row's height. The lit bars are black and the unlit bars the light dither, drawn under them on the
  same canvas (DESIGN → Exceptions). The connected network and the one being joined are the chosen inversion, with their words,
  glyphs and lit bars white and the unlit bars the dark dither. They differ by the check and the words. A failed join is no row
  state: the row goes back to rest and a dialog says why. Rows are `spacing.md` apart in a list view, no box.
- **Keyboard** — a full-glass overlay with Cancel and one action named for what it does ("Join", "Done"): six keys a row, each
  `spacing.target` square, `spacing.key-gap` apart — letters in the order of the alphabet, then digits with common punctuation
  (123), then the rest of the punctuation (#+=), four pages of six rows, each key `rounded.md`: a character key the light dither
  with no edge, black words; Shift, Delete, 123, ABC, #+= and Space white inside a `spacing.stroke.default` edge, and Shift,
  while capitals are on, the chosen inversion. Shift is the word alone. The bottom row is three keys of equal width, 140 px
  across the 444 px grid, each label centred with 25 px (Shift, Space) or 43 px (123, ABC, #+=) either side. The walk is Focus and
  input's. A key types once however long it is held, and the caret is a still bar at the end, so a mistake is Delete. A touch on a key
  types it on release, as a page key acts, and a finger that slides off the key cancels it; the tick sounds on contact. A page switch keeps the place: the same row, the same slot. A count line under the
  field says how much is typed and why the action waits; the keyboard shows no hint line — a keyboard is known — and
  a band of white stands between the keys and the footer. While
  the keyboard is open only the presses that type and the page switches count toward the full refresh, with a ceiling on the
  moves that do not (Motion). The keyboard types into a text input or a password input
  in the slot at its top, which it leaves for the field.
- **Text input, password input** — BYTE-90's input and password input: a white frame `spacing.content` × `spacing.target`,
  a `spacing.stroke.default` outline, `rounded.xl` — a field is no longer the base 12, because the button inside it is
  concentric with its corner (Shapes → Corners) — with its words in `label` `spacing.md` in from the edge and a
  placeholder in `caption` in `text-primary`, after a still bar caret 3 px wide that never blinks. The placeholder is a
  hint, never the field's name: the name is a `caption` in `text-secondary` over the field, `spacing.sm` above it. A
  value longer than the field's 19 characters shows its tail with the caret in view. The frame takes the ring round all of
  it; on a page the caret shows only while it is focused and OK or a tap opens the keyboard, in the keyboard the caret
  always shows (its stops are Focus and input's). At its right end an icon button, ghost, `spacing.xl` (32 px) square
  with square corners, `spacing.md` from the frame's right, top and bottom edges, so 5 px of white stand between its ring
  and the outline; it is hit across the `spacing.target` square at the frame's right end (Exceptions → The icon button).
  A text input's is the cross, shown only while the field holds text and is enabled, which empties it; a password
  input's is the eye, which shows the action a press takes — the open eye while the text is hidden, the slashed eye while
  it shows. A password input masks each character with a bullet and keeps the last typed one in the clear for
  `motion.password-reveal` (Exceptions → A password's last character). A disabled input is the light-dither face with no
  outline, its words and glyph `text-disabled`, no cross, and its reason in `caption` `text-secondary` under it
  (States → Disabled). BYTE-90's leading icon is not ported.
- **Dropdown, select list** — a dropdown is a value row (a setting's name left, the chosen option right in `text-secondary`)
  with a select chevron `spacing.md` after the value; OK or a tap opens the select list, a bottom sheet. The sheet is full
  width, anchored to the glass's bottom and hanging off its sides and bottom, so only its top edge shows: a
  `spacing.stroke.strong` outline round the top edge and its `rounded.2xl` corners, and the offset rising from it in
  `shadow-2`, elevation 2 (Elevation & Depth). From its top: the outline, `spacing.md`, the field's name in `label`, `spacing.md`, a list view of
  rows `spacing.md` apart, `spacing.md`, and its foot, `spacing.footer` high, in the footer's place. It holds Cancel and, until the Help screen ships, a key legend (Open items). A row is an option in `body`, 20 characters to a line,
  wrapping to a second line and growing — never cut — with 64 px as its floor. The chosen row is the inversion in `label` with the
  check at its right end (DESIGN → Exceptions). A page holds four whole rows, or three when four would cover the field that
  opened it. The test is the field's bottom, the ring, white and the offset above the sheet's top: a field ending at y 290
  or higher takes four, one lower takes three, never fewer. The sheet keeps its height, and the scroll indicator shows from the second page.
  It opens with focus on the chosen row, on its page. The page behind is out of the walk, and a touch above the sheet is
  eaten. OK on a row chooses it and closes, the field reporting one change. Cancel, holding OK, or the chosen row closes
  with nothing changed. Focus goes back to the field. It appears and closes on a partial refresh.
- **Horizontal pager** — a strip of chips, or any focus stop, that turns by page, never by pixel: a page is the whole items
  that fit `spacing.content`, `spacing.md` apart, the next page starting with the next item, so no item is cut at the edge;
  the page in words alone ("1 of 2", `caption` in `text-secondary`, left-aligned at the strip's left edge) sits `spacing.md`
  under the strip — no page dots. UP and DOWN walk the items and the strip
  turns when the focus leaves a page; a swipe left or right turns one page, decided when the finger lifts as a vertical swipe
  is; a vertical drag that starts on it is the page's. OK on an item does what the item does and never turns a page.
- **Icons** — a meaningful icon always has its word beside it ("Home", "Wi-Fi"), except the icon button's closed set
  of glyphs (DESIGN → Exceptions); no icon stroke under `spacing.stroke.hairline`. The glyphs are BYTE-90's art at one
  bit on a `spacing.icon` box, eighteen in the widgets component alone — an arrow, a cross, play, pause, stop, the speaker on
  and off, a check, the microphone and a refresh for the buttons, and a signal in three strengths, a padlock, a check for a row's
  facts line, a select chevron and the open and slashed eye, each cut to a box of its own — each in its words' colour or a button's, never a grey; any other
  glyph needs the person's yes and a place here.
- **Status line** — news that asks nothing, as plain words under the title. It is `spacing.list-row-height` high and
  `spacing.content` wide, in `caption`, left, wrapped to two lines and never cut. Copy is 60 characters at most: past them LVGL
  ends the second line in dots and the log says so. It has no glyph, no box, no fill and no edge, so nothing about it can be taken
  for a control. A `spacing.stroke.hairline` rule closes it while news shows. Info, success and error look alike: the words, and the
  sound, tell them apart. Only a screen that can get news reserves it. It is no stop, takes no press and no focus, and never
  covers a control. The newest replaces the one showing in one partial refresh. The same words again redraw nothing but still sound
  their kind, so a press always hears its outcome. It leaves at the next change of screen or when its caller retracts it, never
  on a timer. News that arrives while a dialog, sheet or the keyboard is open is held, then drawn and sounded when it closes.
- **Wait indicator** — still words and a segmented progress bar, stepping no faster than every 5 s.
- **Progress bar, step bar, page dots** — round-capped pieces `spacing.progress` high (the one that runs out first
  `spacing.progress-lead`): a black fill from the left, `spacing.sm` of white, the rest of the track in the light dither;
  its end moves the same distance every step, so 100 % ends short of the track's end, as the deck's does; up to five
  steps `spacing.step-gap` apart, done black, running part-filled, failed a hollow outline and said so in words; page
  dots a hollow `spacing.page-dot` circle, the current page a black pill `spacing.page-pill` wide, `spacing.sm` apart —
  an indicator beside its words, never a control: the deck's dots are no tap target either. Each steps, never glides.
- **Radio row, choice card** — a card on the light dither, `rounded.lg`, its words `spacing.card-inset` in, with a
  `spacing.mark` ring (one choice) or box (many) at the right. A radio row is at least `spacing.list-row-height` high, its `label`
  name wrapping to 18 characters a line and never cut, `spacing.xs` above and below so one line and two both stand 80, three
  116 and four 152, with no detail line. A choice card at least the mark and two `spacing.md` high with a `label` name over its detail in `caption`.
  Chosen is the mark alone: the ring's `spacing.mark-dot` dot, the box's check — the card does not change.
  A group is the rows and one-choice cards that share a parent, as a radio button row's. Choosing one — OK on it, or a tap
  on the lift — clears the rest of its group and sends `LV_EVENT_VALUE_CHANGED` on it, after they are cleared. A chosen
  row chosen again sends nothing (a radio never unchooses itself), and the programmatic set sends nothing. A many-choice
  card is a group of one and sends it on each flip. Moving the focus never chooses. A choice that is the screen's one task
  applies at once: the screen stays, its status line says what is set, and there is no Save. Choosing another is the undo.
- **List view, text view, scroll indicator** — rows or `caption-loose` text taller than their band, no box (the ring
  alone shows a text view, and its words stand `spacing.sm` inside the view on every side, so the first glyph is 12 px
  from the ring's inner edge and a page never shows a part of the line beside it), turn a
  page, never scroll by pixel: a list's band holds whole rows `spacing.md` apart and turns when the focus leaves it, a text
  view's when OK has opened it for reading and UP or DOWN is pressed, the last line of one page the first of the next;
  a swipe up or down turns one page in either, and the page behind does not move. A page with no row that takes focus — a
  page of detail rows or plain text — is **one stop**: the ring, as every control's, round the page's rows from the first
  row's top to the last row's bottom, in the screen's focus order beside the real rows. UP or DOWN leaving it turns the
  page as leaving a row does; OK on it ticks and does nothing; it is no mode and takes no press and no touch. A page that
  holds a row that takes focus has no stop of its own, and its facts show with it. The page in words ("1 of 2", `caption`,
  `text-secondary`, at the rows' left edge, `spacing.md` under the view's whole height so it stands in one place on every
  page) shows when a list view has more than one page and its screen asks for it, as the horizontal pager's does. The
  scroll indicator is the deck's lane:
  `spacing.scroll-thumb` wide and `spacing.scroll-length` long at most, `spacing.scroll-x` from the glass's left, of
  separate round-capped pieces — a short track piece in the light dither, `spacing.sm` of white, a black thumb
  (`surface-on`) for the view's share (never under a dot; the first page at the top end and the last at the bottom end,
  every page between a gap and a dot of track in from them — in proportion for a short list, spread evenly once the
  steps would be under that, so every page turn moves it), `spacing.sm` of white, the rest of the track — hidden when
  everything fits. It is centred on the glass's height, the
  same on every page, for a view that fills the content (Home's list), and on the view itself for one inside a page that
  holds more things (the kit page's), where the glass's middle is not the view's.
- **Range slider** — a light-dither track `spacing.slider-height` high with a black fill inset `spacing.xs`, a white
  knob bar at the fill's end and the value in `caption` beside the knob, right-aligned in the fill, white on it and
  black on the track when the fill is too short ("60%", "Off" at the bottom); its name in `caption` above it. OK
  opens it for editing, UP and DOWN step it, one partial refresh a step, and OK closes it; a finger's drag lands its
  value on lift.
- **Roller** — a value in a long ordered list or range, in three rows of `spacing.target`: the value between its neighbours,
  which stand as still words (`caption`, `text-secondary`, never a grey or a fade); at an end of a list that does not wrap the
  missing neighbour is a blank row. Its name stands in `caption` `spacing.md` above it, always, and it is as wide as its widest
  value and `spacing.lg` either side, `spacing.target` at least, inside a `spacing.stroke.hairline` outline, `rounded.lg`. OK or a
  tap opens it. UP steps to the value above and DOWN to the one below, one partial refresh a step. OK keeps, the field
  reporting one change. Holding OK, or the Cancel the footer shows while it is open, puts the value back. A press that
  takes the focus elsewhere — a tap on another control — closes it keeping the value, as the range slider's edit does.
  Open, the ring is `spacing.stroke.edit` and the middle row is the chosen inversion — the value OK will keep. The
  footer holds Cancel and, until the Help screen ships, a key legend (Open items). A roller its caller marks circular (hours, minutes, months)
  wraps with the tick alone; any other stops at its ends with `sound.at-end`. A tap on a neighbour steps one, one vertical swipe
  steps one on the lift, and nothing follows a finger. Use it for one value in a list of up to about 60; under about eight named
  options use the select list, for a level the range slider.
- **Status block** — a heading in `label` over a line in `body`, `spacing.pair-md` apart between their own boxes. The deck's mark
  stands beside them: a check, a clock or a link-off, `spacing.status-mark` of art at one bit, compiled into the widgets component
  alone, drawn 85 tall and compiled at most 73 wide. The mark stands flush left in a slot `spacing.status-mark-slot` wide, and the words follow a
  `spacing.lg` after the slot, so every block's words stand at one x (`edge` + slot + `lg`, 121 px on the glass), whatever its mark.
  It marks an empty or a done state. With no mark the slot stays empty, and the words stand where they would after a mark.
- **Recording indicator** — "Recording" and its time, with a distinct tone at start and stop.

### States

- **Focused** — the `spacing.stroke.focus` ring on every control, always (open for editing, below, is the one
  exception): `spacing.focus-offset` outside its outer edge, following its corners, black, with white between — a button,
  a chip, a row, a card, a toggle, the control of a checkbox or radio row, an icon button, a range slider, a text
  view, a text input — its ring round the whole frame, and a page of facts — its ring round the page's rows, the same ring at the same offset; the icon button inside it has its own ring, inside the frame. A segment's ring is round the focused item, inside its
  track: the track's `spacing.sm` is the ring's reach, so its outer edge is the track's. A focused, chosen control shows
  both: its chosen mark and the ring round it. Focus stops sit `spacing.md` apart or more, and a container that holds one
  lets its ring out; the footer's 16 px over and under its buttons keep it off the glass's edge.
- **Open for editing** — a control that takes UP and DOWN as its own, a range slider, a roller or a text view open for reading,
  draws its ring at `spacing.stroke.edit`, 6 px, still `spacing.focus-offset` outside it: the weight says the buttons
  now change the value or turn the page. The look says it, not words: the heavier ring, and a roller's
  inverted middle row; its footer shows Cancel (and, until the Help screen ships, a roller's key legend; Open items).
- **Chosen** — inverted: `surface-chosen` with `text-on-fill` on a segment's item and a home row; a chip is chosen by
  a black edge, a card or radio row by the dot or check in its mark, a checkbox or radio by its black fill and its
  white check or dot, never a grey alone.
- **Disabled** — one step lighter than the ground, with no outline: the control keeps its size and its place, its face
  the light dither (a button of any variant, a chip, a card, a row, a toggle's, segment's, slider's or checkbox's
  track), and **no edge of any kind**; every black part — a toggle's thumb, a chosen segment item, a slider's fill, a
  check, a dot — is a white part knocked out of that face; a text input is that face with its outline gone. Its words, and a check, dot or glyph on the face, are
  `text-disabled`, the sparse dither of one black pixel in sixteen; a row's name and state word on the white page, a
  slider's knob and its value in the white fill, and a radio or card mark's dot on its white disc are
  `text-disabled-page`, one in eight. A disabled control looks inert, not faded: it is exempt from contrast, is never
  reached by the buttons and never has a ring, and a disabled tonal button differs from an enabled one by its words
  alone — so the reason says why, in words, solid `caption` in `text-secondary` under the control, `spacing.xs` below
  a row's name. Where the words cannot stand under it, a line the screen already shows says why and is linked to the
  control (the keyboard's count line, for its action). Prefer explaining over disabling.
- **Busy** — the label says what it is doing.
- **Done** — in words ("Done"), never a grey.

## Do's and Don'ts

- Do design every screen in its partial-refresh form first, and check it reads in black and white alone.
- Do show focus with the ring and what is chosen with an inversion — never one for the other, never a grey alone.
- Do give every touch target 64 px, and every action a path by the buttons.
- Do let the buzzer answer a press before the glass does.
- Don't animate anything — no slides, fades, spinners or blinking cursors.
- Don't put words on the dark dither, or white words on any dither — a disabled control's words, in a sparse dither,
  alone sit lighter than their ground; don't rely on a 1 px line.
- Don't use a weight under 600, except `body` at 400, or a size under `caption`. A lighter weight breaks up at one bit a pixel;
  `body`'s 3 px strokes keep its gaps open (Typography → Face).
- Don't time anything to leave — but for a password's last typed character (Exceptions).
- Don't trigger two full refreshes back to back — one full refresh's own time apart at the least.

## Focus and input

### Buttons and touch

- **UP and DOWN move focus, OK acts** — on every screen, including the overlays; anything touchable is reachable this
  way. The one exception is a control OK has opened for editing — a range slider, a roller, a text view being read: UP and DOWN
  are its own until OK closes it, keeping the change; the open look says so. Holding OK closes it instead and
  puts back what it had when OK opened it: the slider's value, the roller's value, the text view's page. The Cancel a roller's
  footer shows while it is open is a touch stop only.
- **A page of facts is one stop** — UP and DOWN leaving it turn the page, OK on it ticks (Components → The kit → List view).
- **The AI button records** — a hold records and its release saves; a press-to-start, press-to-stop way, the hold's
  length, and how a hold is told from OK, a wake and power-off are open (DESIGN → Open items).
- **The first button press shows the focus ring and moves nothing**; the next moves it, and focus wraps at the ends of
  a list. With no ring on the glass — a screen just opened, a sheet or a dialog opened by touch, or after a touch — the
  first UP, DOWN or OK rings the focused control and moves and presses nothing; only the next acts, so the
  first OK never presses what the person cannot see is chosen. The press still ticks. When the focused control is off the
  glass, the first press rings the first control in view instead. A control open for editing takes its keys at once.
- **A touch shows no ring and drops the one showing** — the ring is the buttons'. The place stays: the next button press
  rings the control the finger last pressed, and walks on from there.
- **A press acts on the lift, on the control** — a finger that slides off what it pressed, past its hit area and
  `spacing.md`, presses nothing (WCAG 2.5.2). Lifting off a range slider that way leaves its value as it was when the
  finger went down; a lift on the track, or within `spacing.md` of it, lands the value there.
- **A held UP or DOWN walks on** — once held `motion.key-repeat-delay`, 600 ms, it steps again every
  `motion.key-repeat-period`, 500 ms, each step only once the last one is on the glass and no refresh is running, so the
  walk never runs ahead of what is drawn. Letting go stops it at once; no step is kept for later. Each step ticks, as a
  press does. A control open for editing steps its value the same way. OK never repeats: held, it is the escape.
- **A swipe up or down on a list view or a text view turns one page, as DOWN and UP do** — decided when the finger
  lifts: 64 px of travel (`kTouchSwipeDistance`, one target), or a flick at the end of a drag of 24 px or more; one swipe,
  one page, however far or fast; a drag pulled back, or a finger that rested before lifting, places and turns nothing.
  Nothing follows the finger, the page behind never moves, and at either end nothing wraps (the buttons wrap) — the
  press's tick is the answer. A vertical drag that starts on a slider is the page's, never the slider's: the slider has
  no drag of its own, so the page takes it and the slider ignores the release after it; a horizontal drag lands the
  slider's value on lift, and the buttons do everything a swipe does (the thresholds and the rules as built are
  STICKY_DESIGN_TOKENS §10).
- **A text input is a stop and its button the next, on a page and in the keyboard alike** — the field's ring alone, then the
  next move puts the ring on the button alone. On a page OK on the field opens the keyboard; in the keyboard OK on it
  does nothing but tick. The button is one press, no second level: OK acts, holding OK cancels the keyboard. The eye keeps
  the focus after it toggles; the cross empties the field and goes, and focus passes to the next stop.
- **The keyboard walks key by key** — UP and DOWN step through the stops in the order of the glass: the field, its button,
  every key from the left of row 1 to the end of the bottom row, Cancel, the action, and wrap. OK types the focused key, each
  key has the ordinary `spacing.stroke.focus` ring and none is open for editing, and holding OK cancels the keyboard in one hold.
  The keyboard opens on the first key, so typing starts at once; UP reaches the field.
- **An overlay takes focus when it opens and gives it back to what opened it when it closes** — with the ring if it wore
  one when the overlay opened, without if it did not; every overlay closes with Cancel or Back.
- **Holding OK is the escape, one step a hold**: first it closes a control open for editing, putting back what it had; else
  it cancels the top overlay — the keyboard, a sheet, a dialog, so a dialog over a sheet goes first and the sheet next;
  else it goes back one screen. At home with nothing open it does nothing.
- **A touch outside a dialog or a sheet is eaten** — the tick, and nothing else: no tap reaches the page behind and no drag
  moves it. A sheet has no drag, throw or handle: a swipe on its list turns a page, and its way out is Cancel and holding OK.

### Hit areas

- **64 px, 6.9 mm, the floor for anything touchable** — above WCAG's 6.35 mm, the AA floor, though under the 9 mm a
  fingertip is comfortable with; a footer button, a list row, a key on the keyboard.
- **A text input's button is hit across a 64 px square** — the button, drawn 32 px, and `spacing.md` round it, so the
  square's right, top and bottom edges are the field's; the frame's left of it is the field's own hit area.
- **Indicators are no hit area** — page dots, a scroll indicator, a progress bar and a status block take no press; the
  page they say is turned by the buttons or a swipe.

## Accessibility

- **The interface implies its interactions; no screen carries running instructions** — a button looks pressable, a
  roller shows its neighbours, an open control has its heavier ring and, on a roller, an inverted middle row. A screen
  names things and says outcomes; it does not say which key does what. A closed device has no screen reader, and words
  on the glass do not stand in for one (EN 301 549 5.1.3.1 asks for speech or touch, not text).
- **The key conventions are taught once** — UP and DOWN move or step, OK presses, hold OK goes back, held keys repeat,
  the first press shows the ring and moves nothing. The rule is to teach them once, at first run in the Welcome
  carousel, and keep them on one Help screen, always in the same place (WCAG 3.2.6). No other screen restates them.
  Welcome and Help do not exist yet (Open items → The key legends).
- **A hint stands only where an interaction is unusual for this product and not implied** — one short line, beside its
  control, never restating a convention above. Today's candidates: the Asleep, Off and timed turn-on words on the
  leaving screens, and the empty state's "Hold AI or press Record.", which name hardware keys the glass cannot show.
- **WCAG 2.2 AA where it applies, and EN 301 549's closed functionality where the hardware allows** — the device has
  a buzzer and no speaker, so it cannot speak, and a person who cannot see the glass is not served today. What is not
  met is declared in the conformance table below, not worked round.
- **Every outcome in words, and the ones Motion → Sound lists in sound too** — a success, a failure, a wait and a
  recording's start and stop each have words on the glass, and so has a status line's news, in the band a screen that
  can get it reserves. Sounding today: every press's tick; the end of a list, text view, pager and slider; a recording's
  start and stop and its failure; and a status line's success and failure — reading the card's, on the bench. A wait's sound
  is an open item, and the product's own outcomes — a capture saved, a sync — have none until they exist.
- **The status line is a fixed band under the title, not near the focused control** — it keeps a ring from being covered
  (WCAG 2.4.11), and what a control did is also in place.
- **A page of facts has a stop, so the keys reach every page** — WCAG 2.1.1; the ring is the ordinary one, black on
  white at 21 : 1, round the page, never covered by the status band above or the footer below (2.4.7, 2.4.11).
- **Contrast** — DESIGN → Colors → Contrast floors, on both refreshes.
- **Text size** — nothing under `caption`; a larger text setting that steps every role up one is open.
- **No time limits** — nothing leaves on a timer, but for a password's last typed character, which is feedback and
  loses nothing when it hides (Exceptions → A password's last character).
- **Flashing** — a full refresh is rare and never repeated back to back; whether one counts as more than three flashes
  a second is open.
- **Plain language** — one task per screen, and a destructive action asks first in a dialog.
- **Sleep never loses work** — the device does not sleep while an overlay, a typed entry or a recording is open, except
  on a low battery, when it says so in words, keeps what is open, and then sleeps.

### Conformance

Ratings, in the style of an accessibility conformance report (VPAT 2.x): Supports, Partially supports, Does not
support, Not applicable, with a reason on every row. A closed device is rated by WCAG2ICT and EN 301 549 clause 11.

| Criterion | Rating | Reason |
| --- | --- | --- |
| WCAG 1.1.1 Non-text content | Partially supports | Icons and marks have visible words or are conventional; no programmatic or spoken alternative exists on a closed device |
| 1.3.1 Info and relationships | Partially supports | Structure is shown by titles, groups, the ring and the chosen inversion; it is not exposed programmatically and there is no audio |
| 2.4.2, 2.4.3, 2.4.6, 2.4.7, 2.4.11 | Supports | A title on every screen, one focus order, descriptive labels, a ring on every stop never covered by a pinned bar (ACCESSIBILITY §5, FOCUS §6) |
| 1.3.3 Sensory characteristics | Supports | Keys are named ("UP", "OK"), never by their position or shape |
| 2.5.1, 2.5.2, 2.5.7, 2.5.8 Pointer inputs and target size | Supports | Every gesture has a key alternative; a touch target is at least `spacing.target` |
| 3.3.1 Error identification | Partially supports | An error is said in words on the glass; there is no spoken form (EN 301 549 5.1.3.15) |
| 3.2.6 Consistent help | Partially supports | Optional; one Help screen is planned and not built (Open items → The key legends) |
| 2.5.3 Label in name | Not applicable | There is no programmatic name; the visible label is the name |
| 3.3.2 Labels or instructions | Partially supports | Every input has a label, and instructions stand only where input is unusual (Accessibility, above). Welcome and Help are not built, so the conventions are not taught yet, and some strings restate them (Open items → The key legends) |
| 4.1.2 Name, role, value | Not applicable | Closed software with no assistive technology (EN 301 549 11.4.1.2.2); state is drawn |
| 4.1.3 Status messages | Not applicable | EN 301 549 11.4.1.3.2; the status line and its sounds are good design, not a claim |
| EN 301 549 5.1.3.1 Non-visual access | Does not support | No speech or tactile output; the buzzer sounds event tones only, with no labels. 5.1.3.2 to 5.1.3.16 follow: not applicable |
| EN 301 549 5.1.4 Closed functionality: text size | Partially supports | The smallest role is `caption`, 20 px; the visual angle at a stated viewing distance is not computed (Open items → The viewing distance) |
| EN 301 549 5.1.5 Visual identification of audio | Partially supports | Every outcome sound has words on the glass. A press's tick and the end-of-list double tick have nothing on it (Open items → A press answered without sound) |
| EN 301 549 5.5.2 Identification of controls | Partially supports | The three keys are found by touch; the touch screen and the AI button's role are not |
| EN 301 549 5.7 Key repeat | Does not support | A held UP or DOWN repeats after `motion.key-repeat-delay`, 600 ms, then every `motion.key-repeat-period`, 500 ms, and neither can be adjusted or turned off (Open items → A setting for the keys' repeat) |
| Section 508 402.2 Speech output | Does not support | A display screen with no speech output; not a status-indicator-only screen |

## Exceptions

A value off these tokens is argued here — what, where and why — before a screen uses it.

### The hardware-test demo

`components/hw_test` — a bench tool for V1.0 and V2.0 boards, not a FolloUp screen.

- **What:** its fixtures' sizes (the grey bands, the touch marks and dot, the microphone bar and its
  scale, the benchmark's track and its step, the footer's Back), a test's absolute grey levels (the words on each
  grey band), its timers and lengths (stats shown after a refresh, the benchmark's frames, a
  second of recording, leaving a screen, the page walk's 3 s on each page, the RTC's 60 s before it turns the board
  on), the buzzer's sounds beyond the six (the Buttons screen's one-second tone, at Seeed's 4 kHz, and the console's
  `sound band`, held pitches across the band), the refresh benchmark's bar, which animates as fast as the glass allows, the page walk, which changes screen
  by itself from the console, the type scale page, which draws every primitive rather than a role, and the UI kit
  page, which shows every widget of the kit live on one long page — both pages scroll by pixel (no animation, momentum
  or overscroll), the type scale page making each sample a focus stop so UP and DOWN reach it, each focused
  widget on the kit page coming into view at once, as on every screen that scrolls (the theme's
  `ui::ShowWhenFocused`). The list view and the text view on the kit page still turn pages, as they do everywhere; only
  the page that holds them scrolls.
- **Why:** each measures the glass, the touch, the buzzer or a sensor — a fixture sized to the test, not to reading — the
  benchmark exists to drive the glass as fast as it goes, the page walk to change screen many times over for a
  ghosting run, the type scale page to show the whole scale for choosing the roles, and the kit page to show every
  widget and its states beside one another for the person to judge on the glass, each more than one page holds, so
  Nothing animates, Nothing leaves on a timer, A screen takes a role, Content turns by page and Six words cannot hold
  there.
- **Where it stops:** a size that matches a token uses the token; text, colours by role, focus and hit areas follow
  this file; nothing outside `components/hw_test` uses the exception. The type scale page's sample frames and the Touch
  screen's tap layer are the only controls built outside the kit.
- **Not this exception:** a `touch-exception` marker in `components/widgets` is the touch check being told that a line
  keeps a rule of this file by a way the check cannot see — no momentum and a claimed band, which is how a swipe
  turns one page and nothing scrolls by pixel (Layout → Paging), and no pressed state of its own (Motion → Press). The
  line follows the rule; it spends no value off the tokens, so none of it needs arguing here.

### The icon button

The deck's lone glyph on a button, and the toggle with a glyph for each state.

- **What:** a glyph with no word beside it, on a square button `spacing.target` a side — drawn and hit alike, never a
  smaller glyph in a padded hit area — against Components → Icons' "a meaningful icon always has its word beside it"
  and Voice & Content Strategy → Labels and states' "a button names what happens".
- **Why:** the deck's kit has it — a Back in a bar, a player's controls — where a word would crowd a row that holds
  little else, and these glyphs are widely understood conventions, so the shape is the name. The device has no screen
  reader (DESIGN → Accessibility), so what the glass shows is a button's only name. The exception holds only where the
  glyph alone is that name. The focused icon button's action is told in words where a screen has room, such as its
  title or status line. The keyboard has none, and its eye and cross are known by their glyphs alone.
- **Where it stops:** a closed set, which grows only here —
  - Back (an arrow pointing left), and Close (a cross), which only dismisses what holds no unsaved work, such as a
    notice; Close never deletes, and an overlay still closes with Cancel or Back (Focus and input → Buttons and touch) —
    except a text field's clear, the cross inside a field's right end, which empties that field: the one deleting use
    of Close (the person's decision), never the only way, since the keyboard's Delete is the other;
  - Show and hide, the eye, an icon button inside a password field's right end, two glyphs showing the action, not the
    state — the open eye while the text is hidden (a press shows it), the slashed eye while it shows — unlike the sound
    toggle, which shows its state; the field itself shows the state (bullets or letters);
  - Play, Pause and Stop;
  - sound on and off as a toggle's two glyphs: the glyph shows the current state, never the action a press takes, and
    the state is also in words beside it (Components → Toggle).

  An icon button inside a field is the one drawn smaller than its hit area, against "drawn and hit alike": a 64 px button
  and its 8 px ring cannot sit inside a 64 px field with white between them and the outline. It is drawn `spacing.xl`
  (32 px), square (`rounded.none` on a field of `rounded.xl`), and hit across the `spacing.target` square at the field's
  right end, itself and `spacing.md` on every side; only the cross and the eye take it.

  Disabled, it is the light-dither tile with its glyph in `text-disabled`, in all four variants alike, and the reason in
  words beside or under it.

  Any other glyph has its word beside it. Each glyph means one action everywhere; it is never the only way to a
  destructive action, nor a dialog's action. In every other way it is a button — the focus ring, a variant from
  Components → Button, no stroke under `spacing.stroke.hairline` — and a toggle's state is never a grey alone.

### A password's last character

A masked field shows the last character typed in the clear for a moment, and then a bullet.

- **What:** the last typed character of a password field shows in the clear for `motion.password-reveal`, 1500 ms, and
  then becomes a bullet — one redraw by a timer — against Motion → Waits and things that update on their own ("nothing
  updates on its own faster than every 5 s"; "nothing leaves on a timer"), Accessibility → No time limits and Do's and
  Don'ts' "don't time anything to leave".
- **Why:** the key-by-key walk gives no other proof of what was typed, and a wrong character costs five to nine refreshes to
  find and fix; BYTE-90 does it, with LVGL's own password show time of 1.5 s (the person's decision, superseding the
  keyboard's earlier "no last-character reveal"). The eye shows the whole password at any time, so the timer hides nothing the
  person cannot ask for.
- **Where it stops:** a masked field only, its last character only, once per character and never repeating; the time ends
  at once on the next typing press, Delete, the eye or the keyboard closing; no other word or message leaves on a timer; it is
  feedback and not a limit on a task — nothing the person needs expires (WCAG 2.2.1). The hide is no press: it counts none
  toward the partial limit (Motion).

### The unlit bars of the signal glyph

A Wi-Fi network's signal shows every bar of the glyph, the lit ones black and the unlit ones in the light dither.

- **What:** the unlit bars of the signal glyph are dithered strokes — the light dither on the white page, the dark dither on
  the inverted row — against Colors → The four levels' "Text and lines are never dithered".
- **Why:** the count of lit bars carries the signal, so the unlit bars are decorative and meet no contrast floor; they say
  how much is missing by being faint, as the deck's `wifi_indicator` shows a quiet tint under the lit part, and at one bit the
  only quiet is a dither. A 3 to 4 px arc in a dither is a dotted band — faint on the glass.
- **Where it stops:** the signal glyph on a network row, and nowhere else; if the dots are too faint on the glass, the way out is a
  2 px outline, never another grey.

### The signal glyph without a word

A Wi-Fi network's signal is shown by its glyph alone, with no "Strong", "Fair", "Weak" or "None" beside it.

- **What:** a glyph with no word beside it, against Components → Icons' "a meaningful icon always has its word beside it".
- **Why:** the bars' count and the unlit bars in the light dither are the reading — three lit bars are strong, none lit is no
  signal — as the signal glyph is on every phone and in the deck's own list; a word under it costs the name 32 px of room (the name
  takes 28 characters instead of 26), and the row's facts line already says the state in words ("Connected", "Locked", "Open").
- **Where it stops:** the signal glyph on a network row, and nowhere else; a signal that must be read in words, say on a screen
  of its own, says them.

### The select sheet's check alone

The chosen option of a select sheet is marked by the check, without its word.

- **What:** a glyph with no word beside it, against Components → Icons' "a meaningful icon always has its word beside it".
- **Why:** an option may run to two lines and 20 characters a line, and the row has no room for a word as well. The chosen row
  is also the inversion, so the check is never the only sign of the state.
- **Where it stops:** a select sheet's chosen row, and nowhere else.

## Open items

- **A setting for the keys' repeat** — holding UP or DOWN repeats after `motion.key-repeat-delay` every
  `motion.key-repeat-period`, with no setting to slow or stop it; EN 301 549 5.7 asks for one where a key repeats, so the
  conformance table rates it Does not support. Add the setting once a Settings screen exists.
- **The key legends** — the code still draws a legend in the roller's and the dropdown sheet's footers
  (`roller.h`, `select_sheet.h`), and the kit page's strings restate conventions: "Sliders: OK opens, UP raises, DOWN
  lowers, OK closes" and "Keyboard: OK opens it" (`hw_test.cpp`). All go against the Accessibility rule that no screen
  restates the key conventions; they go with the Help screen and the Welcome teaching, which are not built yet, and
  until then 3.3.2 is rated Partially supports.
- **The frame left on the glass while the device sleeps** — the stickies, the lock screen or a blank glass — decided
  with the designs.
- **The viewing distance** the sizes are set for — `hero`'s 60 px figures read at about 35 cm and `display` at about
  29 cm.
- **The clock's own size** — the lock screen's clock takes a type size of its own, likely one of the 100, 120 and
  160 px figure sizes; `lock-clock` takes `hero` until it is chosen.
- **The AI button** — its name on the glass (AI or OK), a hold's length, and how a hold, a wake and power-off differ.
- **The elevation scale** — the dither of an overlay's offset by elevation level; the dialog's and the sheet's are both the dark
  dither until it is decided.
- **The clock's cost** — a minute tick makes a full refresh every twenty minutes on a still screen — and whether the
  lock screen's clock is 24-hour or shows AM and PM, wherever the device's own state is said.
- **A spoken mode** — none is possible on this hardware; who that leaves out, and what would serve them.
- **A press answered without sound** — how a muted device, or a person who cannot hear the tick, knows a press landed
  before the next frame does; and the news sound with nobody pressing, which has no mute.
- **Where the device's own state is said** — the time, battery, Wi-Fi, storage, recording state, charging and a low
  battery in words have no place on the glass; the status bar holds the title alone, and the low battery said before sleep (Accessibility →
  Sleep never loses work) has no home yet.
- **A status line left up** — it stays until the next change of screen or its caller retracts it, so on a device that sits on
  one screen for days it can read as current when it is not; whether that is acceptable, or a caller's `Clear` is enough.
- **A hairline cleared from a band** — whether the status line's lower rule leaves a ghost when news is retracted, and so
  whether it is always drawn, judged on a V1.0 and a V2.0.
- **The glass's real contrast** — its white and black measured against the nominal 21 : 1.
- **Whether a 2 px hairline and `caption` at 20 px read from a metre away** — on a V1.0 and a V2.0 board.
- **How many times a full refresh flashes**, against WCAG's three a second.
- **Press-to-start and press-to-stop recording** beside the hold.
- **A larger text setting**, and the glyph ranges every string needs.
- **A wait's sound, and an ignored press's** — whether either gets one of the six words, or stays the tick alone.
