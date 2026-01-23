/*
 * Mejiro command translation table implementation
 */
#include "mejiro_commands.h"

const mejiro_command_t mejiro_commands[] = {
    // 特殊コマンド
    {"#-",    CMD_REPEAT, {.keycode = 0}},
    {"-U",    CMD_UNDO,   {.keycode = 0}},
    
    // キーコード送信
    {"-YA",   CMD_KEYCODE, {.keycode = KC_DQUO}},
    {"-NI",   CMD_KEYCODE, {.keycode = KC_QUOT}},
    {"-TK",   CMD_KEYCODE, {.keycode = KC_PIPE}},
    {"-IA",   CMD_KEYCODE, {.keycode = KC_COLN}},
    {"-NY",   CMD_KEYCODE, {.keycode = KC_SLSH}},
    {"-TN",   CMD_KEYCODE, {.keycode = KC_ASTR}},
    {"-TI",   CMD_KEYCODE, {.keycode = KC_TILD}},
    {"-AU",   CMD_KEYCODE, {.keycode = KC_BSPC}},
    {"-IU",   CMD_KEYCODE, {.keycode = KC_DEL}},
    {"-S",    CMD_KEYCODE, {.keycode = KC_ESC}},

    {"-A",    CMD_KEYCODE, {.keycode = KC_LEFT}},
    {"-N",    CMD_KEYCODE, {.keycode = KC_DOWN}},
    {"-Y",    CMD_KEYCODE, {.keycode = KC_UP}},
    {"-K",    CMD_KEYCODE, {.keycode = KC_RIGHT}},
    {"-I",    CMD_KEYCODE, {.keycode = KC_HOME}},
    {"-T",    CMD_KEYCODE, {.keycode = KC_END}},
    
    {"-n",    CMD_KEYCODE, {.keycode = KC_ENTER}},
    {"n-",    CMD_KEYCODE, {.keycode = KC_SPACE}},
    {"n-n",   CMD_KEYCODE, {.keycode = KC_TAB}},
    {"-ntk",  CMD_KEYCODE, {.keycode = KC_F7}},
    {"n-ntk", CMD_KEYCODE, {.keycode = KC_F8}},
    
    // 文字列送信
    {"-nt",   CMD_STRING,  {.string = "."}},
    {"-nk",   CMD_STRING,  {.string = ","}},
    {"n-nt",  CMD_STRING,  {.string = "?"}},
    {"n-nk",  CMD_STRING,  {.string = "!"}},
};

const uint8_t mejiro_command_count = sizeof(mejiro_commands) / sizeof(mejiro_commands[0]);
