/*
 * Mejiro transformation logic
 */
#pragma once

#include QMK_KEYBOARD_H

// メジロ式変換結果
typedef struct {
    char output[128];
    size_t kana_length; // 変換前ひらがなの文字数
    bool success;
} mejiro_result_t;

// メジロIDからひらがなへの変換
mejiro_result_t mejiro_transform(const char *mejiro_id);

// ひらがなをヘボン式ローマ字に変換
void kana_to_roma(const char *kana_input, char *roma_output, size_t output_size);
