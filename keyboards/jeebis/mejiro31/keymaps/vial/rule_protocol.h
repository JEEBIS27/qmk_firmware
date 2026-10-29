// SPDX-License-Identifier: GPL-3.0-or-later
// Mejiro runtime rules: versioned, double-bank EEPROM and Raw HID transport.
#pragma once
#include "eeprom.h"
#include "steno/engine.h"

#define MJI_RULE_BANK_SIZE 4096U
#define MJI_RULE_BANK_A 4096U
#define MJI_RULE_BANK_B 8192U
#define MJI_RULE_CHUNKS 9U
#define MJI_RULE_HEADER_SIZE 10U

_Static_assert(sizeof(mejiro_rule) == 144, "Mejiro rule wire layout changed");
_Static_assert(MJI_RULE_HEADER_SIZE + MEJIRO_RULE_CAPACITY * sizeof(mejiro_rule) <= MJI_RULE_BANK_SIZE,
               "Mejiro rules exceed one EEPROM bank");

static uint16_t rule_bank = MJI_RULE_BANK_A;
static uint16_t rule_generation;
static mejiro_rule rule_receive;
static uint16_t rule_received;

static uint16_t rule_crc(uint16_t crc, const uint8_t *bytes, size_t length) {
    while (length--) {
        crc ^= (uint16_t)*bytes++ << 8;
        for (uint8_t i = 0; i < 8; ++i) crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
    }
    return crc;
}

static void rule_eeprom_read(uint16_t address, void *output, size_t length) {
    eeprom_read_block(output, (const void *)(uintptr_t)address, length);
}
static void rule_eeprom_write(uint16_t address, const void *input, size_t length) {
    eeprom_update_block(input, (void *)(uintptr_t)address, length);
}

static bool rule_bank_valid(uint16_t base, uint16_t *generation) {
    uint8_t header[MJI_RULE_HEADER_SIZE];
    rule_eeprom_read(base, header, sizeof(header));
    if (memcmp(header, "M31D", 4) || header[4] != 1 || header[5] > MEJIRO_RULE_CAPACITY) return false;
    uint16_t crc = 0xffff;
    mejiro_rule rule;
    for (uint8_t i = 0; i < header[5]; ++i) {
        rule_eeprom_read(base + MJI_RULE_HEADER_SIZE + i * sizeof(rule), &rule, sizeof(rule));
        crc = rule_crc(crc, (const uint8_t *)&rule, sizeof(rule));
    }
    if (crc != (uint16_t)(header[8] | (uint16_t)header[9] << 8)) return false;
    *generation = (uint16_t)(header[6] | (uint16_t)header[7] << 8);
    return true;
}

static bool rule_bank_load(uint16_t base) {
    uint8_t header[MJI_RULE_HEADER_SIZE];
    rule_eeprom_read(base, header, sizeof(header));
    mejiro_qmk_rule_clear_all();
    for (uint8_t i = 0; i < header[5]; ++i) {
        mejiro_rule rule;
        rule_eeprom_read(base + MJI_RULE_HEADER_SIZE + i * sizeof(rule), &rule, sizeof(rule));
        int32_t result = rule.hidden
            ? mejiro_qmk_rule_hide(rule.kind, rule.key, rule.key_len)
            : mejiro_qmk_rule_set(&rule);
        if (result != 0) {
            mejiro_qmk_rule_clear_all();
            return false;
        }
    }
    return true;
}

static void mejiro_rules_load(void) {
    uint16_t a = 0, b = 0;
    bool valid_a = rule_bank_valid(MJI_RULE_BANK_A, &a);
    bool valid_b = rule_bank_valid(MJI_RULE_BANK_B, &b);
    if (valid_a && valid_b) {
        rule_bank = (int16_t)(b - a) > 0 ? MJI_RULE_BANK_B : MJI_RULE_BANK_A;
    } else if (valid_b) {
        rule_bank = MJI_RULE_BANK_B;
    } else if (valid_a) {
        rule_bank = MJI_RULE_BANK_A;
    } else {
        rule_bank = MJI_RULE_BANK_A;
        rule_generation = 0;
        mejiro_qmk_rule_clear_all();
        return;
    }
    rule_generation = rule_bank == MJI_RULE_BANK_A ? a : b;
    if (!rule_bank_load(rule_bank)) {
        uint16_t fallback = rule_bank == MJI_RULE_BANK_A ? MJI_RULE_BANK_B : MJI_RULE_BANK_A;
        if (((fallback == MJI_RULE_BANK_A) ? valid_a : valid_b) && rule_bank_load(fallback)) {
            rule_bank = fallback;
            rule_generation = fallback == MJI_RULE_BANK_A ? a : b;
        }
    }
}

static bool mejiro_rules_save(void) {
    int32_t count = mejiro_qmk_rule_count();
    if (count < 0 || count > MEJIRO_RULE_CAPACITY) return false;
    uint16_t target = rule_bank == MJI_RULE_BANK_A ? MJI_RULE_BANK_B : MJI_RULE_BANK_A;
    const uint8_t invalid[4] = {0};
    rule_eeprom_write(target, invalid, sizeof(invalid));
    uint16_t crc = 0xffff;
    for (int32_t i = 0; i < count; ++i) {
        mejiro_rule rule;
        if (mejiro_qmk_rule_get((size_t)i, &rule) != 0) return false;
        rule_eeprom_write(target + MJI_RULE_HEADER_SIZE + (uint16_t)i * sizeof(rule), &rule, sizeof(rule));
        crc = rule_crc(crc, (const uint8_t *)&rule, sizeof(rule));
    }
    uint16_t generation = rule_generation + 1;
    uint8_t header[MJI_RULE_HEADER_SIZE] = {
        'M', '3', '1', 'D', 1, (uint8_t)count,
        (uint8_t)generation, (uint8_t)(generation >> 8),
        (uint8_t)crc, (uint8_t)(crc >> 8),
    };
    // Publish the magic last. A power loss leaves the previous bank valid.
    rule_eeprom_write(target + 4, header + 4, sizeof(header) - 4);
    rule_eeprom_write(target, header, 4);
    uint16_t verified;
    if (!rule_bank_valid(target, &verified) || verified != generation) return false;
    rule_bank = target;
    rule_generation = generation;
    return true;
}

static void rule_reply(uint8_t *data, uint8_t status) {
    data[3] = status;
}

static void rule_protocol(uint8_t *data) {
    if (memcmp(data + 4, "M31RUL01", 8)) { data[0] = 0xff; return; }
    if (!ready) { rule_reply(data, 3); return; }
    uint8_t op = data[2];
    if (op == 0) {
        data[12] = (uint8_t)mejiro_qmk_rule_count();
        data[13] = MEJIRO_RULE_CAPACITY;
        data[14] = 1;
        data[15] = MEJIRO_RULE_KEY_CAPACITY;
        data[16] = MEJIRO_RULE_VALUE_CAPACITY;
        data[17] = MEJIRO_RULE_ROW_CAPACITY;
        rule_reply(data, 0);
        return;
    }
    if (op == 1) {
        mejiro_rule rule;
        uint8_t index = data[12], chunk = data[13];
        if (chunk >= MJI_RULE_CHUNKS || mejiro_qmk_rule_get(index, &rule) != 0) { rule_reply(data, 2); return; }
        memcpy(data + 16, (const uint8_t *)&rule + 16U * chunk, 16);
        rule_reply(data, 0);
        return;
    }
    if (!vial_unlocked) { rule_reply(data, 1); return; }
    if (op == 2) {
        memset(&rule_receive, 0, sizeof(rule_receive));
        rule_received = 0;
        rule_reply(data, 0);
        return;
    }
    if (op == 3) {
        uint8_t chunk = data[12];
        if (chunk >= MJI_RULE_CHUNKS) { rule_reply(data, 2); return; }
        memcpy((uint8_t *)&rule_receive + 16U * chunk, data + 16, 16);
        rule_received |= (uint16_t)1U << chunk;
        rule_reply(data, 0);
        return;
    }
    if (op == 4) {
        uint8_t action = data[12];
        if (rule_received != (1U << MJI_RULE_CHUNKS) - 1U || action > 2) { rule_reply(data, 2); return; }
        rule_received = 0;
        int32_t result = action == 0 ? mejiro_qmk_rule_set(&rule_receive)
            : action == 1 ? mejiro_qmk_rule_hide(rule_receive.kind, rule_receive.key, rule_receive.key_len)
            : mejiro_qmk_rule_clear(rule_receive.kind, rule_receive.key, rule_receive.key_len);
        if (result == -2) { rule_reply(data, 4); return; }
        if (result != 0) { rule_reply(data, 2); return; }
        rule_reply(data, mejiro_rules_save() ? 0 : 5);
        return;
    }
    rule_reply(data, 2);
}
