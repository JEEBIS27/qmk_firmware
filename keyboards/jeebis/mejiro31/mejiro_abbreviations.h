#pragma once

#include <stdint.h>
#include <stdbool.h>

// 略語の結果を格納する構造体
typedef struct {
    char output[256];
    bool success;
} abbreviation_result_t;

// ユーザー略語の検索（完全なストロークで検索）
abbreviation_result_t mejiro_user_abbreviation(const char *stroke);

// 一般略語の検索（完全なストロークで検索）
abbreviation_result_t mejiro_abstract_abbreviation(const char *stroke);
