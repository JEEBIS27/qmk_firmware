/*
 * Mejiro command translation table
 */
#pragma once

#include QMK_KEYBOARD_H

// コマンドタイプ
typedef enum {
    CMD_REPEAT,       // 前回出力を繰り返し
    CMD_UNDO,         // 前回出力をundo（Backspace）
    CMD_KEYCODE,      // 単一キーコード送信
    CMD_STRING,       // 文字列送信
} mejiro_cmd_type_t;

// コマンド定義
typedef struct {
    const char *pattern;              // メジロID文字列
    mejiro_cmd_type_t type;           // コマンドタイプ
    union {
        uint16_t keycode;             // CMD_KEYCODEの場合
        const char *string;           // CMD_STRINGの場合
    } action;
} mejiro_command_t;

// コマンドテーブルの宣言
extern const mejiro_command_t mejiro_commands[];
extern const uint8_t mejiro_command_count;
