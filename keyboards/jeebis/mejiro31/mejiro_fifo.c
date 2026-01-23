#include "mejiro_fifo.h"
#include "mejiro_commands.h"
#include "mejiro_transform.h"
#include <stdlib.h>
#include <string.h>

#define MEJIRO_MAX_KEYS 32

static uint16_t chord[MEJIRO_MAX_KEYS];
static uint8_t  chord_len = 0;
static uint8_t  down_count = 0;
static bool     chord_active = false;
static bool     should_send_passthrough = false;  // 変換失敗時のパススルーフラグ

#define HISTORY_SIZE 20
static char     history_outputs[HISTORY_SIZE][64];
static uint8_t  history_lengths[HISTORY_SIZE];
static uint8_t  history_count = 0;

static bool contains(uint16_t kc) {
    for (uint8_t i = 0; i < chord_len; i++) {
        if (chord[i] == kc) return true;
    }
    return false;
}

bool is_stn_key(uint16_t kc) {
    switch (kc) {
        case STN_N1: case STN_S1: case STN_TL: case STN_PL: case STN_HL: case STN_ST1: case STN_ST3:
        case STN_FR: case STN_PR: case STN_LR: case STN_TR: case STN_DR:
        case STN_N2: case STN_S2: case STN_KL: case STN_WL: case STN_RL: case STN_ST2: case STN_ST4:
        case STN_RR: case STN_BR: case STN_GR: case STN_SR: case STN_ZR:
        case STN_N3: case STN_N4:
        case STN_A:  case STN_O:  case STN_E:  case STN_U:
            return true;
        default:
            return false;
    }
}

static void reset_chord(void) {
    chord_len = 0;
    down_count = 0;
    chord_active = false;
    should_send_passthrough = false;
}

static void append_kc(uint16_t kc) {
    if (chord_len >= MEJIRO_MAX_KEYS) return;
    if (contains(kc)) return;
    chord[chord_len++] = kc;
}

static void send_backspace_times(uint8_t cnt) {
    for (uint8_t i = 0; i < cnt; i++) {
        tap_code(KC_BSPC);
    }
}

// メジロIDのビット順: S,T,K,N,Y,I,A,U,n,t,k,# | S,T,K,N,Y,I,A,U,n,t,k,*
// ビットインデックスへ変換（押されていると1に立てる）
static uint8_t stn_to_bit(uint16_t kc) {
    switch (kc) {
        // 左手
        case STN_S1: case STN_S2: return 0;  // S-
        case STN_TL:              return 1;  // T-
        case STN_KL:              return 2;  // K-
        case STN_WL:              return 3;  // N-
        case STN_PL:              return 4;  // Y-
        case STN_HL:              return 5;  // I-
        case STN_RL:              return 6;  // A-
        case STN_ST1: case STN_ST2: return 7;  // U- (*1,*2)
        case STN_N3:              return 8;  // n- (#3)
        case STN_A:               return 9;  // t- (A-)
        case STN_O:               return 10; // k- (O-)
        case STN_N1: case STN_N2: return 11; // #
        // 右手
        case STN_SR: case STN_TR: return 12; // -S (-T,-S)
        case STN_LR:              return 13; // -T (-L)
        case STN_GR:              return 14; // -K (-G)
        case STN_BR:              return 15; // -N (-B)
        case STN_PR:              return 16; // -Y (-P)
        case STN_FR:              return 17; // -I (-F)
        case STN_RR:              return 18; // -A (-R)
        case STN_ST3: case STN_ST4: return 19; // -U (*3,*4)
        case STN_N4:              return 20; // -n (#4)
        case STN_U:               return 21; // -t (-U)
        case STN_E:               return 22; // -k (-E)
        case STN_DR: case STN_ZR: return 23; // * (-D,-Z)
        default:
            return 0xFF;
    }
}

// メジロID列に変換して送信
// 両手: 左→"-"→右
// 左のみ: 左→"-"（末尾ハイフンで左手を示す）
// 右のみ: "-"→右（先頭ハイフンで右手を示す）
// 変換表（例）:
//   "#"  : repeat last output
//   "-U" : undo last output (同じ長さだけBackspace)
//   "-AU": Backspace 1回
//   "-IU": Delete 1回
//   "-S" : Escape
static void convert_and_send(void) {
    if (chord_len == 0) return;

    uint32_t bits = 0;
    for (uint8_t i = 0; i < chord_len; i++) {
        uint8_t idx = stn_to_bit(chord[i]);
        if (idx != 0xFF) {
            bits |= (1UL << idx);
        }
    }

    static const char *labels_left[12] = {"S","T","K","N","Y","I","A","U","n","t","k","#"};
    static const char *labels_right[12] = {"S","T","K","N","Y","I","A","U","n","t","k","*"};

    char out[64];
    uint8_t pos = 0;
    bool left_has = (bits & 0xFFF) != 0;
    bool right_has = (bits & 0xFFF000) != 0;

    // 左側
    for (uint8_t i = 0; i < 12; i++) {
        if (bits & (1UL << i)) {
            const char *p = labels_left[i];
            while (*p && pos < sizeof(out) - 1) out[pos++] = *p++;
        }
    }

    // ハイフン配置
    if (left_has && right_has) {
        if (pos < sizeof(out) - 1) out[pos++] = '-';
    } else if (left_has && !right_has) {
        if (pos < sizeof(out) - 1) out[pos++] = '-';
    } else if (!left_has && right_has) {
        if (pos < sizeof(out) - 1) out[pos++] = '-';
    }

    // 右側
    for (uint8_t i = 0; i < 12; i++) {
        uint8_t bit = 12 + i;
        if (bits & (1UL << bit)) {
            const char *p = labels_right[i];
            while (*p && pos < sizeof(out) - 1) out[pos++] = *p++;
        }
    }

    out[pos] = '\0';
    if (pos == 0) return;

    // 変換テーブル検索
    for (uint8_t i = 0; i < mejiro_command_count; i++) {
        if (strcmp(out, mejiro_commands[i].pattern) == 0) {
            switch (mejiro_commands[i].type) {
                case CMD_REPEAT:
                    if (history_count > 0) {
                        uint8_t idx = history_count - 1;
                        send_string(history_outputs[idx]);
                    }
                    return;
                
                case CMD_UNDO:
                    if (history_count > 0) {
                        uint8_t idx = history_count - 1;
                        send_backspace_times(history_lengths[idx]);
                        history_count--;
                    } else {
                        // 履歴がない場合はデフォルトで2文字削除
                        send_backspace_times(2);
                    }
                    return;
                
                case CMD_KEYCODE:
                    tap_code(mejiro_commands[i].action.keycode);
                    return;
                
                case CMD_STRING: {
                    const char *str = mejiro_commands[i].action.string;
                    send_string(str);
                    // 出力した文字列を履歴に追加
                    if (history_count < HISTORY_SIZE) {
                        strncpy(history_outputs[history_count], str, sizeof(history_outputs[0]) - 1);
                        history_outputs[history_count][sizeof(history_outputs[0]) - 1] = '\0';
                        history_lengths[history_count] = (uint8_t)strlen(history_outputs[history_count]);
                        history_count++;
                    } else {
                        // 履歴が満杯の場合は古いものをシフト
                        for (uint8_t j = 0; j < HISTORY_SIZE - 1; j++) {
                            strcpy(history_outputs[j], history_outputs[j + 1]);
                            history_lengths[j] = history_lengths[j + 1];
                        }
                        strncpy(history_outputs[HISTORY_SIZE - 1], str, sizeof(history_outputs[0]) - 1);
                        history_outputs[HISTORY_SIZE - 1][sizeof(history_outputs[0]) - 1] = '\0';
                        history_lengths[HISTORY_SIZE - 1] = (uint8_t)strlen(history_outputs[HISTORY_SIZE - 1]);
                    }
                    return;
                }
            }
        }
    }

    // デフォルト: メジロ式変換を試行
    mejiro_result_t transformed = mejiro_transform(out);
    
    // 変換成功時は変換結果を出力、失敗時は元のSTN_キーコードをそのまま送信
    if (transformed.success) {
        const char *final_output = transformed.output;
        uint8_t final_length = (uint8_t)transformed.kana_length;
        send_string(final_output);
        
        if (history_count < HISTORY_SIZE) {
            strncpy(history_outputs[history_count], final_output, sizeof(history_outputs[0]) - 1);
            history_outputs[history_count][sizeof(history_outputs[0]) - 1] = '\0';
            history_lengths[history_count] = final_length;
            history_count++;
        } else {
            // 履歴が満杯の場合は古いものをシフトして最新を追加
            for (uint8_t i = 0; i < HISTORY_SIZE - 1; i++) {
                strcpy(history_outputs[i], history_outputs[i + 1]);
                history_lengths[i] = history_lengths[i + 1];
            }
            strncpy(history_outputs[HISTORY_SIZE - 1], final_output, sizeof(history_outputs[0]) - 1);
            history_outputs[HISTORY_SIZE - 1][sizeof(history_outputs[0]) - 1] = '\0';
            history_lengths[HISTORY_SIZE - 1] = final_length;
        }
    } else {
        // メジロ変換失敗時はパススルーフラグをセット
        should_send_passthrough = true;
    }
}

void mejiro_on_press(uint16_t kc) {
    if (!chord_active) chord_active = true;
    append_kc(kc);
    down_count++;
}

void mejiro_on_release(uint16_t kc) {
    if (down_count > 0) down_count--;
    if (chord_active && down_count == 0) {
        convert_and_send();
        reset_chord();
    }
}

bool mejiro_should_send_passthrough(void) {
    return should_send_passthrough;
}

void mejiro_send_passthrough_keys(void) {
    // chord配列にあるすべてのキーコードをtap_code()で送信
    for (uint8_t i = 0; i < chord_len; i++) {
        tap_code(chord[i]);
    }
}
