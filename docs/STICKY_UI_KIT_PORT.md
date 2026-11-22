# FolloUp — UI Kit Port

> **STATUS: PARTLY IMPLEMENTED.** The agent deck's landscape widgets are built and seen on a V1.0; the checkbox and
> radio rows, the button icons, the one disabled look, and BYTE-90's Wi-Fi network row, dialog offset, keyboard's key grid,
> dropdown with its select sheet and horizontal pager are built in the working tree and not yet seen on the glass; the
> text input and password input with the keyboard's field are built the same way; the interaction framework, sound, status bar, overlay stack and the rest are not started.

The plan for porting UI from the agent deck's landscape kit and BYTE-90's widgets to FolloUp's e-ink kit on the
reTerminal Sticky (800 x 480 e-paper, one bit plus dither, three keys UP, DOWN and AI/OK, GT911 touch, a buzzer): each
item's source, target, status, dependency and open question. For anyone picking up the next item.

**Source of truth.** The two reference repos for what a control looks like and does: the agent deck,
`/Users/tieuvong/Development/claude-agent-deck-3` (`components/widgets/`, `components/chiptune/`), and BYTE-90,
`/Users/tieuvong/Development/byte90-series3` (`components/ui_widgets/`, `ui_router`, `ui_sheet`, `ui_dialog`,
`ui_toast`, `ui_keyboard`, `ui_icons`, `ui_haptics`) — both outside this repo: separate repositories, on the author's machine,
that a reader of this repo cannot open (as STICKY_DESIGN_TOKENS marks `[DECK]`). [DESIGN.md](../DESIGN.md) wins for every rule, token and look;
this doc holds the order, the status and the decisions of the port.

**Related.** [DESIGN.md](../DESIGN.md) (the rules and the closed Exceptions), [STICKY_DESIGN_TOKENS](STICKY_DESIGN_TOKENS.md)
(the ported widgets, the ring outside, paging and the swipe: [§9](STICKY_DESIGN_TOKENS.md#9-the-ring-outside-and-the-ported-widgets),
[§10](STICKY_DESIGN_TOKENS.md#10-paging-and-the-swipe)), [APP_ARCHITECTURE](APP_ARCHITECTURE.md) (the layering a new
component follows), [STICKY_EPAPER_WAVEFORMS](STICKY_EPAPER_WAVEFORMS.md) (the refresh a frame gets).

**Stability.** Code comments cite this doc's `§n`; never renumber a section.

**Legend.**

| Tag | Source |
| --- | --- |
| `[P]` | The person's decision in the kit-port task, recorded with `verify.py decision` |
| `[SEEN]` | Seen on the glass by the person, on a V1.0 board running this repo's firmware |
| `[?]` | Unverified, or proposed and not yet agreed — tracked as an item below |

| Status | Meaning |
| --- | --- |
| Done | Built, in the working tree, not committed; the item leaves this doc when its commit lands |
| Drafting | A `ui-designer` draft in progress, awaiting the person's yes |
| Not started | Agreed, with no draft yet |
| Blocked by `<item>` | Cannot start until that item is Done |

| § | Section | Look here for |
| --- | --- | --- |
| 1 | [Order and decisions](#1-order-and-decisions) | The port order, and the decisions the person made that bind every item |
| 2 | [Done](#2-done) | What is built in the working tree, and what is left on it |
| 3 | [Drafting](#3-drafting) | The drafts in progress and each one's open choice |
| 4 | [Not started](#4-not-started) | The seven items, the framework first |
| 5 | [Conflicts with today's design](#5-conflicts-with-todays-design) | Where an item runs against DESIGN.md or the code |
| 6 | [What to measure or test first](#6-what-to-measure-or-test-first) | The runs that settle an open question |

---

## 1. Order and decisions

**The order is: finish the kit port, then the missing components and new additions (the drafts) with this plan doc
tracking them, then the interaction framework as its own task `[P]`.** The kit's own widgets come before anything that
depends on an overlay or a router. The order of items 1 to 7 in [§4](#4-not-started) after the framework is proposed
`[?]`.

**The decisions that bind every item:**

- **A port starts from the source's own pictures and keeps its shapes and proportions, scaled to the glass.** A control
  is drawn by the `ui-designer` beside the source, and the coordinating session checks each draft picture against the
  source before showing it to the person.
- **The keyboard walks key by key** `[P]` — the walk is [DESIGN → Focus and input](../DESIGN.md#focus-and-input). Every key
  types once however long it is held, and only the presses that type and the page switches count toward the partial
  refresh limit, with a ceiling of 60 on the rest `[P]`.
- **Drop shadows are an elevation scale** `[P]`: levels 0 to 3 by the density of the offset's dither, a 2 px white keyline round it.
- **A row's content is always vertically centred** `[P]`.
- **The buzzer takes the 8-bit chiptune style, with FolloUp's words** `[P]`: tick, at-end, error, success (only once the
  action succeeded), recording start and recording stop.
- **Hold OK is an escape chain** `[P]`: leave an open edit and restore its value, else close the top overlay, else go back.
- **The icon-only button is an exception with a closed set** `[P]`, kept in
  [DESIGN → The icon button](../DESIGN.md#the-icon-button).
- **A swipe on a list view or text view turns one page**, as the buttons do; nothing scrolls by pixel `[P]`.
- **The horizontal scroller is ported** `[P]`, and turns by page.
- **The keyboard is built as a kit component on the kit page first** `[P]`, and becomes an overlay once the interaction
  framework lands.

---

## 2. Done

**Built in the kit-port task, in the working tree, not committed.** The person said yes to the built screens and the
V1.0 glass showed them `[SEEN]`; the person's sign-off on the findings in [§6](#6-what-to-measure-or-test-first) is
pending.

| Item | Source | FolloUp's target | Status | Decision it carries |
| --- | --- | --- | --- | --- |
| Landscape widgets: button, chip, choice card, detail row, list view, page dots, progress bar, radio row, range slider, scroll indicator, section heading, segment, status block, step bar, text view, toggle, toggle row, value row | The deck's `components/widgets/src/` | `components/widgets/` (one `src/<name>.cpp` and `include/<name>.h` each) | Done | The look is [STICKY_DESIGN_TOKENS §9](STICKY_DESIGN_TOKENS.md#9-the-ring-outside-and-the-ported-widgets) |
| The scroll indicator's thumb past 15 pages | The deck's scroll indicator | `components/widgets/src/internal/scroll_rules.h`, `tests/host/scroll_test.cpp` | Done | Page 0 and the last page keep the ends; the pages between spread evenly from a gap and a dot in from each end, so every page turn moves the thumb `[P]` |
| Names over details on line boxes | The deck's detail row | `ui::FitLineBox` in `components/ui/line_box.cpp`, `tests/host/fit_line_box_lvgl_test.cpp` | Done | A name takes the room before its detail |
| The text view's inset | The deck's text view | `components/widgets/src/text_view.cpp` | Done | No box; the words inset `spacing.sm`, 8 px, about 12 px from the ring `[P]` |
| The status block with no mark; the toggle row centred | The deck's status block and toggle row | `components/widgets/src/status_block.cpp`, `toggle_row.cpp` | Done | No mark sits one gap in; a disabled toggle row's name and reason take `spacing.xs`, 12 px, between them `[P]` |
| The Display screen's layout and focus | The hardware-test demo | `components/hw_test/hw_test.cpp` | Done | Speed sits above the footer; focus returns to the Display button that opened a sub-screen `[P]` |
| The bench console's `key up\|down\|ok\|hold [n]` | — | `lvgl_display::PressButton` in `components/lvgl_display/input.cpp`, the console in `hw_test.cpp` | Done | Presses go through the real input path so the kit page and Home page 2 are reachable without hands `[P]` |
| The icon button's exception | — | [DESIGN → The icon button](../DESIGN.md#the-icon-button) | Done | — |
| Checkbox and radio: `MakeCheckbox`, `MakeCheckboxRow`, `MakeRadio`, `MakeRadioButtonRow`, each with a row like the toggle row | The person's words: rendered like the toggle switch | `components/widgets/` (`src/checkbox.cpp`, `checkbox_row.cpp`, `radio.cpp`, `radio_button_row.cpp`; a shared setting row in `src/internal/setting_row.cpp`) | Done | 44 px control (`spacing.check-size`), checkbox `rounded.md`, radio `rounded.full`; off the light dither with a 3 px outline, on black with a white check or a 16 px dot; the ring round the control alone, no state words, the whole row acts, a radio clears its group and never unchooses itself `[P]` |
| Button icons: `MakeIconButton`, `MakeToggleIconButton`, `MakeToggleIconRow`, and `MakeButton`'s glyph | BYTE-90's `icon_button`, `toggle_icon_button`, `button`; the deck's `icon_button`, `toggle_icon_button` | `components/widgets/` (`src/icon_button.cpp`, `toggle_icon_button.cpp`, `toggle_icon_row.cpp`, `glyph.cpp`); the ten button glyphs among those in `marks/marks.c`, made by `marks/make_marks.py` from BYTE-90's PNGs at the 40 px art box | Done | The closed set is [DESIGN → The icon button](../DESIGN.md#the-icon-button) and the compiler holds it (`IconAction`); no pressed state; 7 characters beside a glyph in a half button `[P]` |
| One disabled look: the light-dither face, no outline, every black part knocked out to white, words in a sparse dither (one black pixel in sixteen on the face, one in eight on the page) | The person on the glass: a disabled button's black label did not read as disabled | `detail::DitherWhenDisabled` and every widget's disabled style; the two sparse dithers in `components/lvgl_display/include/glass_rules.h` and `tests/host/glass_rules_test.cpp`, `tests/host/disabled_words_lvgl_test.cpp` | Done | **O2** `[P]`: disabled controls are exempt from contrast and should look inert; the palette gains two patterns; DESIGN → States → Disabled is rewritten. A chip, radio row and choice card now have a disabled look too |

| Wi-Fi network row: `MakeNetworkRow`, `SetNetworkRow`, the signal glyphs, the lock and the check | BYTE-90 `ui_widgets/src/network_list_item.cpp`, `wifi_indicator.cpp` | `components/widgets/` (`src/network_row.cpp`); six more glyphs in `marks/marks.c`, `make_marks.py` taking a crop; `caption-strong` in DESIGN | Done | Names in `caption-strong`; lit bars black, unlit in the light dither (the dark dither on the inverted row); the connected network's check is on its facts line before "Connected" `[P]`; the signal glyph stands alone, centred on the row, with no word under it, and the name takes 28 characters `[P]`; the unlit bars are a registered exception in DESIGN |
| The dialog and the elevation scale: `MakeDialog`, `ui::AddOffset`, the levels in `design_tokens.h` | The person: the modal's shadow is a dithered grey, the sheet's lighter than the dialog's | `components/widgets/src/dialog.cpp`, `components/ui/ui.cpp`, `tests/host/elevation_lvgl_test.cpp` | Done | Elevation 0 to 3, the offset in the eighth, the light dither and the dark dither, 6 px for all, a 2 px white keyline; the dialog is 3 and the sheet 2 `[P]` |
| Keyboard: the key grid, the key-by-key walk, the footer, the typing-only partial count | BYTE-90 `ui_keyboard/` | `components/widgets/src/keyboard.cpp`, `internal/keyboard_pages.h`; `lvgl_display::SetPartialCounting`, `NoteTyping`; `tests/host/keyboard_pages_test.cpp`, `glass_rules_test.cpp` | Done | Six 64 px keys a row, 12 px apart, letters in alphabetical order, Shift the word alone, the bottom row three equal keys of 140 px; every key is a stop with the ordinary 4 px ring, UP and DOWN step key by key (the field, its button, the keys, Cancel, the action), OK types, hold OK cancels; only the presses that type and the page switches count toward the full refresh, a full one is forced after 60 that did not count `[P]`; a key types on release `[P]`. The keyboard leaves a slot and a `KeyboardTarget` for the field below; `KeyboardJoinField` puts the field and its button first in the walk |
| Text input and password input, with the cross and the eye inside the field, as the keyboard's field | BYTE-90 `ui_widgets/src/input.cpp`, `password_input.cpp`, the trailing button | `components/widgets/` (`src/text_input.cpp`, `src/icon_button.cpp`'s `detail::MakeIconButtonSized`); `IconAction::kClear`, `kEyeOpen`, `kEyeSlashed`; `glyph_eye_on` and `glyph_eye_off` in `marks/marks.c`; `tests/host/text_input_lvgl_test.cpp` | Done | A 432 × 64 frame, `rounded.xl`, a 3 px outline, a still caret, the placeholder in `caption`, 19 characters and then the tail; the button ghost, 32 px and square, hit across the 64 px square at the right end; the eye shows the action; the password's last character shows 1.5 s (`motion.password-reveal`) and the hide counts none toward the partial limit; in the keyboard the field is the walk's first stop and its button the second `[P]` `[HOST]` |
| Dropdown and its select sheet: `MakeDropdown`, `OpenSelectSheet` | BYTE-90 `ui_widgets/src/dropdown.cpp`, `ui_sheet` | `components/widgets/src/dropdown.cpp`, `select_sheet.cpp`, `internal/sheet_rules.h`; `tests/host/sheet_rules_test.cpp` | Done | The closed field is a value row with the chevron beside the value; the list is a bottom sheet, options in `caption-strong` wrapping to more lines, the chosen row inverted with the check alone, four rows a page or three, opened and closed on a partial refresh `[P]` |
| Horizontal pager: `MakeHorizontalPager` | BYTE-90 `ui_widgets/src/horizontal_scroller.cpp` | `components/widgets/src/horizontal_pager.cpp`; a horizontal axis in `internal/swipe.cpp` | Done | Pages of whole items, "1 of 2" in words alone (no page dots), a horizontal swipe turns one page and a vertical drag is the page's `[P]` |
| A row's content is centred on its height | The person: menu items are always vertically centred | `tests/host/row_centring_lvgl_test.cpp`; `components/widgets/src/select_sheet.cpp` and the setting rows | Done | One rule, one test: every row kind, one line and more, within 1 px `[P]` |

---

## 3. Drafting

**Each item below is in gate 1 with the `ui-designer`: a mock in DESIGN.md's tokens, then the person's yes, before any
code.**

None.

---

## 4. Not started

**The interaction framework (1) follows the drafts as its own task `[P]`; the order of 2 to 7 is proposed `[?]`.** Each is built by the three gates of the `ui-designer` where it
is visible.

| # | Item | Source | FolloUp's target | Status | Depends on | Decision or open question |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | **Interaction framework**: an overlay stack with a focus group per screen and per overlay; the first key press shows the ring and moves nothing, a tap shows no ring; a screen stack and routes with no transitions; hold OK as the escape chain; slide-off cancels a press, extended to the range slider `[?]`; hold-to-repeat on UP and DOWN (holding steps the focus until released) `[P]`; the root press lock | BYTE-90 `ui_router/focus.cpp`, `overlays.cpp`, `touch.cpp`, `routes.cpp` | New; `components/lvgl_display/input.cpp` for the ring, and a router that replaces `hw_test`'s hand-kept back state | Not started | The drafts | The first-press and tap rule is [DESIGN → Focus and input](../DESIGN.md#focus-and-input) and is not built today. Lifting off the range slider sets an end value today; extending slide-off to cancel it is proposed `[?]` |
| 2 | **Sound**: the deck's chiptune note format (NES pulse voices at 50, 25 and 12.5 % duty, slides, arpeggios) on the Sticky's LEDC-driven magnetic buzzer, with press tick, at-end, error, success and recording start and stop; BYTE-90's ta-da and nudge melodies where they fit | The deck's `components/chiptune/`; BYTE-90's `ui_haptics`, `chiptune` | New component on the buzzer | Not started | — | The style and the words are in [§1](#1-order-and-decisions). **Open:** the buzzer's pitch range, and its timing during a refresh `[?]` |
| 3 | **Status bar**: battery indicator, Wi-Fi indicator, a toolbar with Back as an icon button | BYTE-90 `ui_widgets/src/battery_indicator.cpp`, `wifi_indicator.cpp`; the deck's `battery_glyph.cpp` | `ui::NewStatusBar` in `components/ui/ui.cpp` | Not started | Wi-Fi signal glyphs (the icon button is built) | DESIGN asks for time, battery, Wi-Fi, storage and recording state; `ui::NewStatusBar` takes a title only today |
| 4 | **Overlays**: the dialog as a real overlay (one action and Cancel, hold OK cancels; the touch catcher the select sheet already has); the select sheet and the keyboard on the overlay stack; a status line | BYTE-90 `ui_dialog`, `ui_sheet`, `ui_toast`, `ui_keyboard` | The dialog's box, the sheet and the keyboard's screen exist as kit components; the stack, the focus hand-over and the escape chain do not | Blocked by 1 | The select sheet takes and returns the focus group and the back handler itself today ([STICKY_DESIGN_TOKENS §12](STICKY_DESIGN_TOKENS.md#12-the-select-sheet)); the keyboard replaces its page and builds it again; the stack replaces both. They follow [DESIGN → The kit](../DESIGN.md#the-kit) |
| 5 | **Widgets**: wrapping detail and radio rows; a roller as a stepped picker (OK opens, UP and DOWN step) `[?]`; a carousel turned by page `[?]` | BYTE-90 `ui_widgets/src/choice_row.cpp`, `roller.cpp`, `carousel.cpp`; the deck's `wrapping_label.cpp`, `roller.cpp` | `components/widgets/` | Not started | — | Detail and radio rows cut words with LVGL's dots today; BYTE-90's choice row grows with its words and never rolls |
| 6 | **Gaps from the deck comparison** `[?]`: a list page with no focusable rows is unreachable by keys; choice cards and radio rows send no event and keep no exclusivity; no distinct at-end sound; one global focus group | The deck's `list_view`, `choice_card`, `radio_row` | `components/widgets/src/list_view.cpp`, `choice_card.cpp`, `radio_row.cpp`, `internal/base.cpp` | Blocked by 1 (the focus group), 2 (the sound) | — | The focus group is the framework's per-screen group |
| 7 | **Practices from BYTE-90** `[?]`: kit rules in DESIGN (a second caller moves a control into the kit; nothing duplicates a kit control); a table of chosen and disabled looks per control; `static_assert`s on token arithmetic; a host check that DESIGN.md's tokens equal `design_tokens.h`; the specimen rule (every new control on the kit page, walked by keys and touch); a `ui_probe`-style check of ring position and clipping | BYTE-90 `ui_probe`, `ui_theme`, `ui_tuner` | DESIGN.md, `components/design_tokens/`, `tests/host/` | Not started | — | `ui::NewButton` and `ui::NewRow` duplicate `widgets` controls today and move into the kit |

---

## 5. Conflicts with today's design

| Requirement | Conflicts with | Options |
| --- | --- | --- |
| The keyboard in BYTE-90's 48 px keys, about ten a row | The 64 px hit-area floor | Settled `[P]`: six 64 px keys a row; no exception in DESIGN |
| A first key press that shows the ring and moves nothing; a tap that shows no ring | `components/lvgl_display/input.cpp` does neither | Item 1 builds it |
| A status bar with time, battery, Wi-Fi, storage and recording state | `ui::NewStatusBar` takes a title only | Item 3 |
| One kit with no duplicate controls | `ui::NewButton` and `ui::NewRow` beside `widgets` | Item 7 moves them |
| Slide-off cancels a press on every control | Lifting off the range slider sets an end value | Item 1 |

---

## 6. What to measure or test first

| Question | How | Settles |
| --- | --- | --- |
| Why ghosting builds on a V1.0 as the kit page scrolls past the choice cards' light dither and clears going down `[SEEN]` — the likely cause is the scans' full refresh, only after 150 scan frames and a 2 s pause ([STICKY_EPAPER_WAVEFORMS §3.4](STICKY_EPAPER_WAVEFORMS.md#34-which-refresh-a-frame-gets)); not measured `[?]` | The same `key` walk on Seeed's partial preset and on Scan 2 x 12, with the person watching; awaiting the person | DESIGN's open item "the dither over many partials", and whether choice cards keep the light dither |
| Whether the kit shows the same on a V2.0 (SSD2677) | A run of the kit page and its key walk on a V2.0 board; not done | The kit's V2.0 look, and the same ghosting question there |
| The buzzer's pitch range and its timing during a refresh `[?]` | A bench run of the chiptune voices on the LEDC buzzer, with a refresh in flight | Item 2 |
| Whether the keyboard reads and hits on the glass, and whether its ring moves — uncounted — leave a ghost | The kit page's keyboard walked by keys and touch on a V1.0, long enough to type a password; awaiting the person | The keyboard in [§2](#2-done) |
| Whether the sheet's partial close ghosts where its chosen row was, and whether the offsets and the keyline read | The kit page's dropdowns opened and closed ten times on a V1.0 | The select sheet and the elevation scale in [§2](#2-done) |
