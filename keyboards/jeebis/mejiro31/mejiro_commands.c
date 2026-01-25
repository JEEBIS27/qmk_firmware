/*
 * Mejiro command translation table implementation
 */
#include "mejiro_commands.h"

const mejiro_command_t mejiro_commands[] = {
    // 特殊コマンド
    {"#-",    CMD_REPEAT, {.keycode = 0}},
    {"-U",    CMD_UNDO,   {.keycode = 0}},

    // コマンド
    {"Sk#-",   CMD_KEYCODE, {.keycode = LCTL(KC_Z)}},
    {"Kk#-",   CMD_KEYCODE, {.keycode = LCTL(KC_X)}},
    {"Nk#-",   CMD_KEYCODE, {.keycode = LCTL(KC_C)}},
    {"Ak#-",   CMD_KEYCODE, {.keycode = LCTL(KC_V)}},
    {"Ank#-",  CMD_KEYCODE, {.keycode = LCTL(LSFT(KC_V))}},

    // キーコード送信
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
    {"-YA",   CMD_STRING,  {.string = "\""}},
    {"-NI",   CMD_STRING,  {.string = "'"}},
    {"-TK",   CMD_STRING,  {.string = "|"}},
    {"-IA",   CMD_STRING,  {.string = ":"}},
    {"-NY",   CMD_STRING,  {.string = "/"}}, // ・
    {"-TN",   CMD_STRING,  {.string = "*"}},
    {"-KY",   CMD_STRING,  {.string = "**"}}, // ※
    {"-TI",   CMD_STRING,  {.string = "~"}},
    {"-YI",   CMD_STRING,  {.string = "("}},
    {"-TY",   CMD_STRING,  {.string = ")"}},
    {"-SYI",   CMD_STRING,  {.string = "(("}}, //【
    {"-STY",   CMD_STRING,  {.string = "))"}}, // 】
    {"-NA",   CMD_STRING,  {.string = "["}},
    {"-KN",   CMD_STRING,  {.string = "]"}},
    {"-SNA",  CMD_STRING,  {.string = "{"}}, // 『
    {"-SKN",  CMD_STRING,  {.string = "}"}}, //  』
    {"-NYIA", CMD_STRING,  {.string = "<"}}, // 〈
    {"-TKNY", CMD_STRING,  {.string = ">"}}, //  〉
    {"-SNYIA", CMD_STRING,  {.string = "<<"}}, //《
    {"-STKNY", CMD_STRING,  {.string = ">>"}}, // 》
    {"-SYA",  CMD_STRING,  {.string = "\"\"{#Left}"}},
    {"-SNI",  CMD_STRING,  {.string = "''{#Left}"}},
    {"-TYI",  CMD_STRING,  {.string = "(){#Left}"}},
    {"-STYI",  CMD_STRING,  {.string = "(()){#Left}"}}, // 【】
    {"-KNA",  CMD_STRING,  {.string = "[]{#Left}"}},
    {"-SKNA", CMD_STRING,  {.string = "{}{#Left}"}}, // 『』
    {"-TKNYIA",CMD_STRING,  {.string = "<>{#Left}"}}, // 〈〉
    {"-STKNYIA",CMD_STRING,  {.string = "<<>>{#Left}"}}, // 《》
    {"-TKIA", CMD_STRING,  {.string = "||"}}, // ‖
    {"-KA",   CMD_STRING,  {.string = "///"}}, // …
    {"-TNI",  CMD_STRING,  {.string = "|v"}}, // ↓
    {"-KYA",  CMD_STRING,  {.string = "|^"}}, // ↑
    {"-IAU",  CMD_STRING,  {.string = "<-"}}, // ←
    {"-STK",  CMD_STRING,  {.string = "->"}}, // →
    {"-STKIAU",CMD_STRING,  {.string = "<->"}}, // ↔
    {"-NYA",  CMD_STRING,  {.string = "v/"}}, // ↙
    {"-TNY",  CMD_STRING,  {.string = "/^"}}, // ↗
    {"-NYI",  CMD_STRING,  {.string = "^/"}}, // ↖
    {"-KNY",  CMD_STRING,  {.string = "/v"}}, // ↘
    {"-nt",   CMD_STRING,  {.string = "."}},
    {"-nk",   CMD_STRING,  {.string = ","}},
    {"n-nt",  CMD_STRING,  {.string = "?"}},
    {"n-nk",  CMD_STRING,  {.string = "!"}},
};

const uint8_t mejiro_command_count = sizeof(mejiro_commands) / sizeof(mejiro_commands[0]);
