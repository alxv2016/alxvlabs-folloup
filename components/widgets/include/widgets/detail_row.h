// A detail row: a fact's name in caption over its value in label; never focused (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

struct DetailRow {
    lv_obj_t *root;
    lv_obj_t *label;
    lv_obj_t *value;
};

DetailRow MakeDetailRow(lv_obj_t *parent, const char *name, const char *value);
void SetDetailRow(const DetailRow &row, const char *value);

}  // namespace widgets
