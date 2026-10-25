#pragma once

#include QMK_KEYBOARD_H
#include "../../../../../Mejiro_Engine/native/mejiro.h"

// Override when an alternate stenography engine is implemented. Return true
// after selecting it; if unavailable, leave the system unchanged and return false.
// IME OFF is sent by the caller regardless of this result.
bool mejiro_steno_select_alternate_system(void);

void mejiro_engine_init(void);
void mejiro_engine_reset(void);
void mejiro_engine_process(const char *stroke, uint8_t length);
void mejiro_engine_record(mejiro_hid_event event);
bool mejiro_engine_flush_keyboard(void);
bool mejiro_engine_recording_failed(void);
// Transport hooks: use already-resolved HID usages, never feed replay back
// through the input engine. End releases replay keys and restores live state.
void mejiro_keyboard_replay_begin(void);
void mejiro_keyboard_replay_event(const mejiro_hid_event *event);
void mejiro_keyboard_replay_end(void);
