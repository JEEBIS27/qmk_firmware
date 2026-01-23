/*
 * Mejiro command translation table implementation
 */
#include "mejiro_commands.h"

const mejiro_command_t mejiro_commands[] = {
    // 特殊コマンド
    {"#-",    CMD_REPEAT, {.keycode = 0}},
    {"-U",    CMD_UNDO,   {.keycode = 0}},

    // コマンド
    {"#-S",   CMD_KEYCODE, {.keycode = LCTL(KC_Z)}},
    {"#-K",  CMD_KEYCODE,  {.keycode = LCTL(KC_X)}},
    {"#-N",   CMD_KEYCODE, {.keycode = LCTL(KC_C)}},
    {"#-A",   CMD_KEYCODE, {.keycode = LCTL(KC_V)}},
    {"#n-A",  CMD_KEYCODE, {.keycode = LCTL(LSFT(KC_V))}},

    // キーコード送信
    {"-YA",   CMD_KEYCODE, {.keycode = KC_DQUO}},
    {"-NI",   CMD_KEYCODE, {.keycode = KC_QUOT}},
    {"-TK",   CMD_KEYCODE, {.keycode = KC_PIPE}},
    {"-IA",   CMD_KEYCODE, {.keycode = KC_COLN}},
    {"-NY",   CMD_KEYCODE, {.keycode = KC_SLSH}},
    {"-TN",   CMD_KEYCODE, {.keycode = KC_ASTR}},
    {"-TI",   CMD_KEYCODE, {.keycode = KC_TILD}},
    {"-YI",   CMD_KEYCODE, {.keycode = KC_LPRN}},
    {"-TY",   CMD_KEYCODE, {.keycode = KC_RPRN}},
    {"-NA",   CMD_KEYCODE, {.keycode = KC_LBRC}},
    {"-KN",   CMD_KEYCODE, {.keycode = KC_RBRC}},
    {"-SNA",  CMD_KEYCODE, {.keycode = KC_LCBR}},
    {"-SKN",  CMD_KEYCODE, {.keycode = KC_RCBR}},
    {"-NYIA", CMD_KEYCODE, {.keycode = KC_LABK}},
    {"-TKNY", CMD_KEYCODE, {.keycode = KC_RABK}},
    {"-AU",   CMD_KEYCODE, {.keycode = KC_BSPC}},
    {"-IU",   CMD_KEYCODE, {.keycode = KC_DEL}},
    {"-S",    CMD_KEYCODE, {.keycode = KC_ESC}},

    {"-A",    CMD_KEYCODE, {.keycode = KC_LEFT}},
    {"-N",    CMD_KEYCODE, {.keycode = KC_DOWN}},
    {"-Y",    CMD_KEYCODE, {.keycode = KC_UP}},
    {"-K",    CMD_KEYCODE, {.keycode = KC_RIGHT}},
    {"-I",    CMD_KEYCODE, {.keycode = KC_HOME}},
    {"-T",    CMD_KEYCODE, {.keycode = KC_END}},
    
    {"-An",    CMD_KEYCODE, {.keycode = LSFT(KC_LEFT)}},
    {"-Nn",    CMD_KEYCODE, {.keycode = LSFT(KC_DOWN)}},
    {"-Yn",    CMD_KEYCODE, {.keycode = LSFT(KC_UP)}},
    {"-Kn",    CMD_KEYCODE, {.keycode = LSFT(KC_RIGHT)}},
    {"-In",    CMD_KEYCODE, {.keycode = LSFT(KC_HOME)}},
    {"-Tn",    CMD_KEYCODE, {.keycode = LSFT(KC_END)}},
    
    {"-n",    CMD_KEYCODE, {.keycode = KC_ENTER}},
    {"n-",    CMD_KEYCODE, {.keycode = KC_SPACE}},
    {"n-n",   CMD_KEYCODE, {.keycode = KC_TAB}},
    {"-ntk",  CMD_KEYCODE, {.keycode = KC_F7}},
    {"n-ntk", CMD_KEYCODE, {.keycode = KC_F8}},
    
    // 文字列送信
    {"-KY",   CMD_STRING,  {.string = "\" "}}, // ※
    {"-TKIA", CMD_STRING,  {.string = "||"}},
    {"-KA",   CMD_STRING,  {.string = "/// "}}, // …
    {"-SYA",  CMD_STRING,  {.string = "@@"}}, // ""
    {"-SNI",  CMD_STRING,  {.string = "&&"}}, // ''
    {"-nt",   CMD_STRING,  {.string = "."}},
    {"-nk",   CMD_STRING,  {.string = ","}},
    {"n-nt",  CMD_STRING,  {.string = "?"}},
    {"n-nk",  CMD_STRING,  {.string = "!"}},
};

const uint8_t mejiro_command_count = sizeof(mejiro_commands) / sizeof(mejiro_commands[0]);
