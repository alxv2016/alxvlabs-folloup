// DESIGN.md's kit as LVGL objects, built from ui's fonts and styles and design_tokens' values, as the agent deck's
// widgets component is built from its theme (STICKY_DESIGN_TOKENS §9 says what the deck is). Three shapes, so a caller knows what it gets from the signature:
//
//   lv_obj_t *MakeX(parent, …)          a styled stock widget; drive it with the plain LVGL API.
//   X MakeX(…) + UpdateX / SetX(const X &, …)   a composite; keep the struct.
//
// Every control joins lvgl_display::Group(): UP and DOWN walk the focus, OK clicks, and a focused control's ring is
// outside it. An editable control — the range slider, a text view being read — opens on OK, takes UP and DOWN as its own while open, and closes on OK. Nothing
// moves: a change lands in one refresh (DESIGN → Motion). Call ui::Start() first, and everything under
// lvgl_display::Lock(). This header declares nothing; include the widget's own header where one is needed.

#pragma once

#include "widgets/button.h"            // MakeButton, ButtonVariant
#include "widgets/checkbox.h"          // MakeCheckbox
#include "widgets/checkbox_row.h"      // CheckboxRow, MakeCheckboxRow, SetCheckboxRowEnabled
#include "widgets/chip.h"              // MakeChip
#include "widgets/choice_card.h"       // MakeChoiceCard, SetChoiceChosen, ChoiceChosen
#include "widgets/detail_row.h"        // DetailRow, MakeDetailRow, SetDetailRow
#include "widgets/dialog.h"            // Dialog, MakeDialog
#include "widgets/dropdown.h"          // Dropdown, MakeDropdown, DropdownSelected, SetDropdownSelected, ChooseDropdown, SetDropdownEnabled, SetDropdownOpener
#include "widgets/glyph.h"             // Glyph, IconAction
#include "widgets/horizontal_pager.h"  // HorizontalPager, MakeHorizontalPager, UpdateHorizontalPager, HorizontalPagerPage, HorizontalPagerPages
#include "widgets/icon_button.h"        // MakeIconButton, IconButtonWords
#include "widgets/keyboard.h"          // KeyboardKind, KeyboardConfig, MakeKeyboardScreen, KeyboardEscape
#include "widgets/list_view.h"         // ListView, MakeListView, UpdateListView, ListViewPage, ListViewPages
#include "widgets/network_row.h"       // NetworkState, MakeNetworkRow, SetNetworkRow
#include "widgets/page_dots.h"         // MakePageDots, SetPageDot
#include "widgets/progress_bar.h"      // ProgressBar, MakeProgressBar, SetProgress
#include "widgets/radio.h"             // MakeRadio
#include "widgets/radio_button_row.h"  // RadioButtonRow, MakeRadioButtonRow, ChooseRadioButtonRow, SetRadioButtonRowEnabled
#include "widgets/radio_row.h"         // MakeRadioRow, SetRadioChosen
#include "widgets/range_slider.h"      // RangeSlider, MakeRangeSlider, RangeSliderValue, SetRangeSliderValue, SetRangeSliderEnabled
#include "widgets/scroll_indicator.h"  // ScrollIndicator, MakeScrollIndicator, UpdateScrollIndicator
#include "widgets/section_heading.h"   // MakeSectionHeading
#include "widgets/select_sheet.h"      // OpenSelectSheet, CloseSelectSheet, SelectSheetOpen, SelectSheetRowsPerPage
#include "widgets/segment.h"           // MakeSegment, SegmentChosen, SetSegmentChosen, SetSegmentEnabled
#include "widgets/status_block.h"      // MakeStatusBlock, StatusMark
#include "widgets/step_bar.h"          // StepBar, StepState, MakeStepBar, SetStep
#include "widgets/text_input.h"        // TextInput, MakeTextInput, MakePasswordInput, InputText, SetInputText, InputAppend, InputErase, SetInputShown, SetInputEnabled, SetInputActivateHandler, InputKeyboardTarget
#include "widgets/text_view.h"         // TextView, MakeTextView, SetTextView, TextViewPage, TextViewPages
#include "widgets/toggle.h"            // MakeToggle
#include "widgets/toggle_icon_button.h" // MakeToggleIconButton
#include "widgets/toggle_icon_row.h"   // ToggleIconRow, MakeToggleIconRow, SetToggleIconRowEnabled
#include "widgets/toggle_row.h"        // ToggleRow, MakeToggleRow, SetToggleRowEnabled
#include "widgets/value_row.h"         // ValueRow, MakeValueRow, SetValueRow
