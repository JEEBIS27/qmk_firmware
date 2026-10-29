#pragma once
#include <stddef.h>
void eeprom_read_block(void *destination, const void *address, size_t size);
void eeprom_update_block(const void *source, void *address, size_t size);
