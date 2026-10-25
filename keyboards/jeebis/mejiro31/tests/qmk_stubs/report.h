// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdint.h>
#define REPORT_ID_PLOVER_HID 0x50
// Same wire layout as tmk_core/protocol/report.h; no keyboard hardware needed.
typedef struct { uint8_t report_id, data[8]; } report_plover_hid_t;
