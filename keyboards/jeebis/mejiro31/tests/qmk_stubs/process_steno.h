#pragma once
typedef enum { STENO_MODE_GEMINI, STENO_MODE_BOLT } steno_mode_t;
void steno_set_mode(steno_mode_t);
bool process_steno(uint16_t, keyrecord_t *);
