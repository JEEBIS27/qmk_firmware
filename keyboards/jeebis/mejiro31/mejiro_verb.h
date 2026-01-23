/*
 * Mejiro verb conjugation system
 * Based on Plover_Mejiro/Mejiro/dictionaries/default/verb.py
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

// 活用形のインデックス
#define CONJ_NAI    0  // ない形
#define CONJ_SHIEKI 1  // 使役形
#define CONJ_UKEMI  2  // 受身形
#define CONJ_MASU   3  // ます形
#define CONJ_JISHO  4  // 辞書形
#define CONJ_TE_TA  5  // て・た形
#define CONJ_IKOU   6  // 意向形
#define CONJ_KATEI  7  // 仮定形
#define CONJ_KANOU  8  // 可能形
#define CONJ_MEIREI 9  // 命令形

// 動詞の活用タイプ
typedef enum {
    VERB_TYPE_GODAN,   // 五段活用
    VERB_TYPE_KAMI,    // 上一段活用
    VERB_TYPE_SIMO,    // 下一段活用
    VERB_TYPE_SAHEN,   // サ変活用
    VERB_TYPE_KAHEN,   // カ変活用
    VERB_TYPE_SPECIAL  // 特殊活用
} verb_type_t;

// 動詞エントリ
typedef struct {
    const char *stroke;   // メジロIDストローク
    const char *stem;     // 語幹
    char gyou;            // 行 ('k', 'g', 's', 't', 'n', 'b', 'm', 'r', 'w')
    verb_type_t type;     // 活用タイプ
} verb_entry_t;

// 動詞変換結果
typedef struct {
    char output[128];
    bool success;
} verb_result_t;

// 公開API
verb_result_t mejiro_verb_conjugate(const char *left_conso, const char *left_vowel, 
                                    const char *left_particle,
                                    const char *right_conso, const char *right_vowel, 
                                    const char *right_particle,
                                    const char *left_kana, const char *right_kana);
