#pragma once
#include <stdint.h>
uint32_t eeconfig_read_user(void);
void eeconfig_update_user(uint32_t value);

uint32_t eeconfig_read_kb(void);
void eeconfig_update_kb(uint32_t value);
