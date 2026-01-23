/*
 * Mejiro verb conjugation system implementation
 */
#include "mejiro_verb.h"
#include <stdio.h>
#include <string.h>

// 五十音の行をインデックスに変換
static int gyou_to_index(char gyou) {
    switch (gyou) {
        case 'k': return 0;
        case 'g': return 1;
        case 's':
        case 'z': return 2;
        case 't': return 3;
        case 'n': return 4;
        case 'b': return 5;
        case 'm': return 6;
        case 'r': return 7;
        case 'w': return 8;
        default: return 9;  // dummy 行
    }
}

// 下一段活用向けの行インデックス変換
static int gyou_to_index_simo(char gyou) {
    switch (gyou) {
        case 'k': return 0;
        case 'g': return 1;
        case 's': return 2;
        case 'z': return 3;
        case 't': return 4;
        case 'd': return 5;
        case 'n': return 6;
        case 'h': return 7;
        case 'b': return 8;
        case 'm': return 9;
        case 'r': return 10;
        case 'w': return 11;
        default: return 11;  // fallback to w行
    }
}

// 仮名の先頭文字から行を判定
static char kana_to_gyou(const char *kana) {
    if (kana == NULL || kana[0] == '\0') {
        return '\0';
    }

    if (strncmp(kana, "か", 3) == 0 || strncmp(kana, "き", 3) == 0 || strncmp(kana, "く", 3) == 0 ||
        strncmp(kana, "け", 3) == 0 || strncmp(kana, "こ", 3) == 0) {
        return 'k';
    }
    if (strncmp(kana, "が", 3) == 0 || strncmp(kana, "ぎ", 3) == 0 || strncmp(kana, "ぐ", 3) == 0 ||
        strncmp(kana, "げ", 3) == 0 || strncmp(kana, "ご", 3) == 0) {
        return 'g';
    }
    if (strncmp(kana, "さ", 3) == 0 || strncmp(kana, "し", 3) == 0 || strncmp(kana, "す", 3) == 0 ||
        strncmp(kana, "せ", 3) == 0 || strncmp(kana, "そ", 3) == 0) {
        return 's';
    }
    if (strncmp(kana, "ざ", 3) == 0 || strncmp(kana, "じ", 3) == 0 || strncmp(kana, "ず", 3) == 0 ||
        strncmp(kana, "ぜ", 3) == 0 || strncmp(kana, "ぞ", 3) == 0) {
        return 'z';
    }
    if (strncmp(kana, "た", 3) == 0 || strncmp(kana, "ち", 3) == 0 || strncmp(kana, "つ", 3) == 0 ||
        strncmp(kana, "て", 3) == 0 || strncmp(kana, "と", 3) == 0) {
        return 't';
    }
    if (strncmp(kana, "だ", 3) == 0 || strncmp(kana, "ぢ", 3) == 0 || strncmp(kana, "づ", 3) == 0 ||
        strncmp(kana, "で", 3) == 0 || strncmp(kana, "ど", 3) == 0) {
        return 'd';
    }
    if (strncmp(kana, "な", 3) == 0 || strncmp(kana, "に", 3) == 0 || strncmp(kana, "ぬ", 3) == 0 ||
        strncmp(kana, "ね", 3) == 0 || strncmp(kana, "の", 3) == 0) {
        return 'n';
    }
    if (strncmp(kana, "は", 3) == 0 || strncmp(kana, "ひ", 3) == 0 || strncmp(kana, "ふ", 3) == 0 ||
        strncmp(kana, "へ", 3) == 0 || strncmp(kana, "ほ", 3) == 0) {
        return 'h';
    }
    if (strncmp(kana, "ば", 3) == 0 || strncmp(kana, "び", 3) == 0 || strncmp(kana, "ぶ", 3) == 0 ||
        strncmp(kana, "べ", 3) == 0 || strncmp(kana, "ぼ", 3) == 0) {
        return 'b';
    }
    if (strncmp(kana, "ま", 3) == 0 || strncmp(kana, "み", 3) == 0 || strncmp(kana, "む", 3) == 0 ||
        strncmp(kana, "め", 3) == 0 || strncmp(kana, "も", 3) == 0) {
        return 'm';
    }
    if (strncmp(kana, "ら", 3) == 0 || strncmp(kana, "り", 3) == 0 || strncmp(kana, "る", 3) == 0 ||
        strncmp(kana, "れ", 3) == 0 || strncmp(kana, "ろ", 3) == 0) {
        return 'r';
    }
    if (strncmp(kana, "わ", 3) == 0 || strncmp(kana, "あ", 3) == 0 || strncmp(kana, "い", 3) == 0 ||
        strncmp(kana, "う", 3) == 0 || strncmp(kana, "え", 3) == 0 || strncmp(kana, "お", 3) == 0) {
        return 'w';
    }

    return '\0';
}

// 五段活用テーブル
// [行][活用形] = 語尾
static const char *godan_conjugate[10][10] = {
    // k行
    {"か", "か", "か", "き", "く", "い", "こう", "け", "け", "け"},
    // g行  
    {"が", "が", "が", "ぎ", "ぐ", "い", "ごう", "げ", "げ", "げ"},
    // s行
    {"さ", "さ", "さ", "し", "す", "し", "そう", "せ", "せ", "せ"},
    // t行
    {"た", "た", "た", "ち", "つ", "っ", "とう", "て", "て", "て"},
    // n行
    {"な", "な", "な", "に", "ぬ", "ん", "のう", "ね", "ね", "ね"},
    // b行
    {"ば", "ば", "ば", "び", "ぶ", "ん", "ぼう", "べ", "べ", "べ"},
    // m行
    {"ま", "ま", "ま", "み", "む", "ん", "もう", "め", "め", "め"},
    // r行
    {"ら", "ら", "ら", "り", "る", "っ", "ろう", "れ", "れ", "れ"},
    // w行
    {"わ", "わ", "わ", "い", "う", "っ", "おう", "え", "え", "え"},
    // dummy
    {"", "", "", "", "", "", "", "", "", ""}
};

// 上一段活用テーブル
static const char *kami_conjugate[10][10] = {
    // k行
    {"き", "きさ", "きら", "き", "きる", "き", "きよう", "きれ", "きれ", "きろ"},
    // g行
    {"ぎ", "ぎさ", "ぎら", "ぎ", "ぎる", "ぎ", "ぎよう", "ぎれ", "ぎれ", "ぎろ"},
    // z行
    {"じ", "じさ", "じら", "じ", "じる", "じ", "じよう", "じれ", "じれ", "じろ"},
    // t行
    {"ち", "ちさ", "ちら", "ち", "ちる", "ち", "ちよう", "ちれ", "ちれ", "ちろ"},
    // n行
    {"に", "にさ", "にら", "に", "にる", "に", "によう", "にれ", "にれ", "にろ"},
    // b行
    {"び", "びさ", "びら", "び", "びる", "び", "びよう", "びれ", "びれ", "びろ"},
    // m行
    {"み", "みさ", "みら", "み", "みる", "み", "みよう", "みれ", "みれ", "みろ"},
    // r行
    {"り", "りさ", "りら", "り", "りる", "り", "りよう", "りれ", "りれ", "りろ"},
    // w行
    {"い", "いさ", "いら", "い", "いる", "い", "いよう", "いれ", "いれ", "いろ"},
    // dummy
    {"", "", "", "", "", "", "", "", "", ""}
};

// 下一段活用テーブル
static const char *simo_conjugate[12][10] = {
    // k行
    {"け", "けさ", "けら", "け", "ける", "け", "けよう", "けれ", "けれ", "けろ"},
    // g行
    {"げ", "げさ", "げら", "げ", "げる", "げ", "げよう", "げれ", "げれ", "げろ"},
    // s行
    {"せ", "せさ", "せら", "せ", "せる", "せ", "せよう", "せれ", "せれ", "せろ"},
    // z行
    {"ぜ", "ぜさ", "ぜら", "ぜ", "ぜる", "ぜ", "ぜよう", "ぜれ", "ぜれ", "ぜろ"},
    // t行
    {"て", "てさ", "てら", "て", "てる", "て", "てよう", "てれ", "てれ", "てろ"},
    // d行
    {"で", "でさ", "でら", "で", "でる", "で", "でよう", "でれ", "でれ", "でろ"},
    // n行
    {"ね", "ねさ", "ねら", "ね", "ねる", "ね", "ねよう", "ねれ", "ねれ", "ねろ"},
    // h行
    {"へ", "へさ", "へら", "へ", "へる", "へ", "へよう", "へれ", "へれ", "へろ"},
    // b行
    {"べ", "べさ", "べら", "べ", "べる", "べ", "べよう", "べれ", "べれ", "べろ"},
    // m行
    {"め", "めさ", "めら", "め", "める", "め", "めよう", "めれ", "めれ", "めろ"},
    // r行
    {"れ", "れさ", "れら", "れ", "れる", "れ", "れよう", "れれ", "れれ", "れろ"},
    // w行
    {"え", "えさ", "えら", "え", "える", "え", "えよう", "えれ", "えれ", "えろ"}
};

// サ変活用
static const char *sahen_conjugate[10] __attribute__((unused)) = {
    "し", "さ", "さ", "し", "する", "し", "しよう", "すれ", "でき", "しろ"
};
// カ変活用
static const char *kahen_conjugate[10] = {
    "こ", "こさ", "こら", "き", "くる", "き", "こよう", "くれ", "これ", "こい"
};
// 行く（五段特殊）
static const char *iku_conjugate[10] = {
    "いか", "いか", "いか", "いき", "いく", "いっ", "いこう", "いけ", "いけ", "いけ"
};
// ある（五段特殊）
static const char *aru_conjugate[10] = {
    "", "あら", "あら", "あり", "ある", "あっ", "あろう", "あれ", "ありえ", "あれ"
};

// 主要な動詞辞書 (Plover_Mejiroより統合)
static const verb_entry_t verb_dict[] = {
    // 五段活用 (k行)
    {"A-STU", "ある", 'k', VERB_TYPE_GODAN},       // 歩く
    {"I-TNA", "いただ", 'k', VERB_TYPE_GODAN},     // 頂く
    {"U-KNAU", "うご", 'k', VERB_TYPE_GODAN},      // 動く
    {"KA-YA", "かがや", 'k', VERB_TYPE_GODAN},     // 輝く
    {"KI-TU", "きがつ", 'k', VERB_TYPE_GODAN},     // 気が付く
    {"KI-TNU", "きづ", 'k', VERB_TYPE_GODAN},      // 気付く
    {"SI-KNAU", "しご", 'k', VERB_TYPE_GODAN},     // 扱く
    {"TA-TA", "たた", 'k', VERB_TYPE_GODAN},       // 叩く
    {"TN-KU", "つづ", 'k', VERB_TYPE_GODAN},       // 続く
    {"NAU-SNAU", "のぞ", 'k', VERB_TYPE_GODAN},    // 除く
    {"TKA-STA", "はたら", 'k', VERB_TYPE_GODAN},   // 働く
    {"SKNI-KNA", "みが", 'k', VERB_TYPE_GODAN},    // 磨く
    // 五段活用 (g行)
    {"I-SAU", "いそ", 'g', VERB_TYPE_GODAN},       // 急ぐ
    {"KA-SIA", "かせ", 'g', VERB_TYPE_GODAN},      // 稼ぐ
    {"KA-TU", "かつ", 'g', VERB_TYPE_GODAN},       // 担ぐ
    {"SA-SA", "ささ", 'g', VERB_TYPE_GODAN},       // 捧ぐ
    {"SA-SKA", "さわ", 'g', VERB_TYPE_GODAN},      // 騒ぐ
    {"TU-NA", "つな", 'g', VERB_TYPE_GODAN},       // 繋ぐ
    {"TKU-SA", "ふさ", 'g', VERB_TYPE_GODAN},      // 塞ぐ
    {"TKU-SIA", "ふせ", 'g', VERB_TYPE_GODAN},     // 防ぐ
    // 五段活用 (s行)
    {"A-KA", "あか", 's', VERB_TYPE_GODAN},        // 明かす
    {"I-KA", "いか", 's', VERB_TYPE_GODAN},        // 活かす
    {"I-TA", "いた", 's', VERB_TYPE_GODAN},        // 致す
    {"AU-TAU", "おと", 's', VERB_TYPE_GODAN},      // 落とす
    {"KA-KA", "かか", 's', VERB_TYPE_GODAN},       // 欠かす
    {"KA-KU", "かく", 's', VERB_TYPE_GODAN},       // 隠す
    {"KU-STA", "くら", 's', VERB_TYPE_GODAN},      // 暮らす
    {"KAU-STAU", "ころ", 's', VERB_TYPE_GODAN},    // 殺す
    {"KAU-SKA", "こわ", 's', VERB_TYPE_GODAN},     // 壊す
    {"TA-AU", "たお", 's', VERB_TYPE_GODAN},       // 倒す
    {"TU-TKNU", "つぶ", 's', VERB_TYPE_GODAN},     // 潰す
    {"TIA-STA", "てら", 's', VERB_TYPE_GODAN},     // 照らす
    {"NA-AU", "なお", 's', VERB_TYPE_GODAN},       // 直す
    {"NI-KNA", "にが", 's', VERB_TYPE_GODAN},      // 逃がす
    {"NAU-KAU", "のこ", 's', VERB_TYPE_GODAN},     // 残す
    {"NAU-TKNA", "のば", 's', VERB_TYPE_GODAN},    // 伸ばす
    {"TKA-NA", "はな", 's', VERB_TYPE_GODAN},      // 話す
    {"SKNA-KA", "まか", 's', VERB_TYPE_GODAN},     // 任す
    {"SKNI-NA", "みな", 's', VERB_TYPE_GODAN},     // 見なす
    {"YU-STA", "ゆら", 's', VERB_TYPE_GODAN},      // 揺らす
    {"SKA-TA", "わた", 's', VERB_TYPE_GODAN},      // 渡す
    // 五段活用 (t行)
    {"U-KNA", "うが", 't', VERB_TYPE_GODAN},       // 穿つ
    {"SAU-TNA", "そだ", 't', VERB_TYPE_GODAN},     // 育つ
    {"TA-SKNAU", "たも", 't', VERB_TYPE_GODAN},    // 保つ
    {"SKNIA-TNA", "めだ", 't', VERB_TYPE_GODAN},   // 目立つ
    // 五段活用 (b行)
    {"A-SAU", "あそ", 'b', VERB_TYPE_GODAN},       // 遊ぶ
    {"IA-STA", "えら", 'b', VERB_TYPE_GODAN},      // 選ぶ
    {"AU-YAU", "およ", 'b', VERB_TYPE_GODAN},      // 及ぶ
    {"NA-STA", "なら", 'b', VERB_TYPE_GODAN},      // 並ぶ
    {"TKA-KAU", "はこ", 'b', VERB_TYPE_GODAN},     // 運ぶ
    {"TKAU-STAU", "ほろ", 'b', VERB_TYPE_GODAN},   // 滅ぶ
    {"SKNA-NA", "まな", 'b', VERB_TYPE_GODAN},     // 学ぶ
    {"SKNU-SU", "むす", 'b', VERB_TYPE_GODAN},     // 結ぶ
    {"YAU-STAU", "よろこ", 'b', VERB_TYPE_GODAN},  // 喜ぶ
    // 五段活用 (m行)
    {"I-NA", "いな", 'm', VERB_TYPE_GODAN},        // 否む
    {"U-STA", "うらや", 'm', VERB_TYPE_GODAN},     // 羨む
    {"KA-KNA", "かが", 'm', VERB_TYPE_GODAN},      // 屈む
    {"KA-SU", "かす", 'm', VERB_TYPE_GODAN},       // 霞む
    {"SU-SU", "すす", 'm', VERB_TYPE_GODAN},       // 進む
    {"TA-NAU", "たの", 'm', VERB_TYPE_GODAN},      // 頼む
    {"TIU-TKNA", "ついば", 'm', VERB_TYPE_GODAN},  // 啄む
    {"TU-TU", "つつ", 'm', VERB_TYPE_GODAN},       // 包む
    {"TU-SKNA", "つま", 'm', VERB_TYPE_GODAN},     // 摘む
    {"NA-YA", "なや", 'm', VERB_TYPE_GODAN},       // 悩む
    {"NU-SU", "ぬす", 'm', VERB_TYPE_GODAN},       // 盗む
    {"NAU-SNAU", "のぞ", 'm', VERB_TYPE_GODAN},    // 望む
    {"TKA-SA", "はさ", 'm', VERB_TYPE_GODAN},      // 挟む
    {"TKA-STA", "はら", 'm', VERB_TYPE_GODAN},     // 孕む
    {"TKI-KAU", "ひっこ", 'm', VERB_TYPE_GODAN},   // 引っ込む
    {"TKU-KU", "ふく", 'm', VERB_TYPE_GODAN},      // 含む
    {"TKAU-TKAU", "ほほえ", 'm', VERB_TYPE_GODAN}, // 微笑む
    {"YA-SU", "やす", 'm', VERB_TYPE_GODAN},       // 休む
    // 五段活用 (r行)
    {"I-", "い", 'r', VERB_TYPE_GODAN},            // 要る
    {"KA-KNI", "かぎ", 'r', VERB_TYPE_GODAN},      // 限る
    {"KNA-TKNA", "がんば", 'r', VERB_TYPE_GODAN},  // 頑張る
    {"KNA-", "がんば", 'r', VERB_TYPE_GODAN},      // 頑張る
    {"KIA-", "け", 'r', VERB_TYPE_GODAN},          // 蹴る
    {"SYA-TKNIA", "しゃべ", 'r', VERB_TYPE_GODAN}, // しゃべる
    {"SU-TKNIA", "すべ", 'r', VERB_TYPE_GODAN},    // 滑る
    {"T-KA", "たすか", 'r', VERB_TYPE_GODAN},      // 助かる
    {"NA-", "な", 'r', VERB_TYPE_GODAN},           // なる
    {"TKY-", "はい", 'r', VERB_TYPE_GODAN},        // 入る
    {"TKA-TKNA", "はばか", 'r', VERB_TYPE_GODAN},  // 憚る
    {"YA-", "や", 'r', VERB_TYPE_GODAN},           // やる
    {"-YA", "や", 'r', VERB_TYPE_GODAN},           // やる
    {"SKA-", "わか", 'r', VERB_TYPE_GODAN},        // 分かる
    // 五段活用 (w行)
    {"-A", "あ", 'w', VERB_TYPE_GODAN},            // 会う
    {"A-STA", "あら", 'w', VERB_TYPE_GODAN},       // 洗う
    {"A-SAU", "あらそ", 'w', VERB_TYPE_GODAN},     // 争う
    {"IU-", "い", 'w', VERB_TYPE_GODAN},           // 言う
    {"I-SNA", "いざな", 'w', VERB_TYPE_GODAN},     // 誘う
    {"I-SKA", "いわ", 'w', VERB_TYPE_GODAN},       // 祝う
    {"U-SI", "うしな", 'w', VERB_TYPE_GODAN},      // 失う
    {"U-TA", "うた", 'w', VERB_TYPE_GODAN},        // 歌う
    {"U-KNA", "うたが", 'w', VERB_TYPE_GODAN},     // 疑う
    {"U-YA", "うやま", 'w', VERB_TYPE_GODAN},      // 敬う
    {"AU-SKNAU", "おも", 'w', VERB_TYPE_GODAN},    // 思う
    {"AU-", "おも", 'w', VERB_TYPE_GODAN},         // 思う
    {"KA-NA", "かな", 'w', VERB_TYPE_GODAN},       // 叶う
    {"KA-SKNA", "かま", 'w', VERB_TYPE_GODAN},     // 構う
    {"KU-STU", "くる", 'w', VERB_TYPE_GODAN},      // 狂う
    {"SI-A", "しあ", 'w', VERB_TYPE_GODAN},        // 仕合う
    {"SI-TA", "したが", 'w', VERB_TYPE_GODAN},     // 従う
    {"SI-SKNA", "しま", 'w', VERB_TYPE_GODAN},     // 仕舞う
    {"SAU-STAU", "そろ", 'w', VERB_TYPE_GODAN},    // 揃う
    {"TA-TA", "たたか", 'w', VERB_TYPE_GODAN},     // 戦う
    {"TI-KA", "ちか", 'w', VERB_TYPE_GODAN},       // 誓う
    {"TI-KNA", "ちが", 'w', VERB_TYPE_GODAN},      // 違う
    {"TU-KA", "つか", 'w', VERB_TYPE_GODAN},       // 使う
    {"TU-TI", "つちか", 'w', VERB_TYPE_GODAN},     // 培う
    {"TU-TNAU", "つど", 'w', VERB_TYPE_GODAN},     // 集う
    {"TIA-STA", "てら", 'w', VERB_TYPE_GODAN},     // 衒う
    {"TNIA-A", "であ", 'w', VERB_TYPE_GODAN},      // 出会う
    {"TAU-NA", "ともな", 'w', VERB_TYPE_GODAN},    // 伴う
    {"NA-STA", "なら", 'w', VERB_TYPE_GODAN},      // 習う
    {"NI-A", "にあ", 'w', VERB_TYPE_GODAN},        // 似合う
    {"NI-SKA", "にぎわ", 'w', VERB_TYPE_GODAN},    // 賑わう
    {"NIA-KNA", "ねが", 'w', VERB_TYPE_GODAN},     // 願う
    {"NAU-STAU", "のろ", 'w', VERB_TYPE_GODAN},    // 呪う
    {"TKA-STA", "はら", 'w', VERB_TYPE_GODAN},     // 払う
    {"TKI-STAU", "ひろ", 'w', VERB_TYPE_GODAN},    // 拾う
    {"SKNA-TAU", "まと", 'w', VERB_TYPE_GODAN},    // 纏う
    {"SKNI-A", "みあ", 'w', VERB_TYPE_GODAN},      // 見合う
    {"SKNU-KA", "むか", 'w', VERB_TYPE_GODAN},     // 向かう
    {"SKNAU-STA", "もら", 'w', VERB_TYPE_GODAN},   // 貰う
    {"SKA-STA", "わら", 'w', VERB_TYPE_GODAN},     // 笑う
    // 上一段活用 (k行)
    {"TN-KI", "で", 'k', VERB_TYPE_KAMI},          // 出来る
    // 上一段活用 (z行)
    {"SI-SNI", "しん", 'z', VERB_TYPE_KAMI},       // 信じる
    {"KA-SNI", "かん", 'z', VERB_TYPE_KAMI},       // 感じる
    {"KI-SNI", "きん", 'z', VERB_TYPE_KAMI},       // 禁じる
    {"TNA-SNI", "だん", 'z', VERB_TYPE_KAMI},      // 断じる
    // 上一段活用 (m行)
    {"KA-SKNI", "かんが", 'm', VERB_TYPE_KAMI},    // 鑑みる
    // 下一段活用 (k行)
    {"T-KIA", "たす", 'k', VERB_TYPE_SIMO},        // 助ける
    {"TU-TN", "つづ", 'k', VERB_TYPE_SIMO},        // 続ける
    {"TN-KIA", "つづ", 'k', VERB_TYPE_SIMO},       // 続ける
    // 下一段活用 (g行)
    {"K-KNIA", "かか", 'g', VERB_TYPE_SIMO},       // 掲げる
    // 下一段活用 (s行)
    {"SKNA-KA", "まか", 's', VERB_TYPE_SIMO},      // 任せる
    // 下一段活用 (d行)
    {"TN-", "", 'd', VERB_TYPE_SIMO},              // 出る
    // 下一段活用 (b行)
    {"KU-STA", "くら", 'b', VERB_TYPE_SIMO},       // 比べる
    // 下一段活用 (m行)
    {"SA-TNA", "さだ", 'm', VERB_TYPE_SIMO},       // 定める
    {"TKA-SNI", "はじ", 'm', VERB_TYPE_SIMO},      // 始める
    {"SKNAU-TAU", "もと", 'm', VERB_TYPE_SIMO},    // 求める
    // 下一段活用 (r行)
    {"KAU-STIA", "こわ", 'r', VERB_TYPE_SIMO},     // 壊れる
    {"SKA-SU", "わす", 'r', VERB_TYPE_SIMO},       // 忘れる
    // 下一段活用 (w行)
    {"AU-SA", "おさ", 'w', VERB_TYPE_SIMO},        // 抑える
    {"AU-TKNAU", "おぼ", 'w', VERB_TYPE_SIMO},     // 覚える
    {"KA-KNA", "かんが", 'w', VERB_TYPE_SIMO},     // 考える
    {"KA-", "かんが", 'w', VERB_TYPE_SIMO},        // 考える
    {"KU-SKA", "くわ", 'w', VERB_TYPE_SIMO},       // 加える
    {"KAU-TA", "こた", 'w', VERB_TYPE_SIMO},       // 答える
    {"SA-SA", "ささ", 'w', VERB_TYPE_SIMO},        // 支える
    {"SKNA-KNA", "まちが", 'w', VERB_TYPE_SIMO},   // 間違える
    // 特殊活用
    {"I-K", "", '\0', VERB_TYPE_SPECIAL},          // 行く（特殊）
    {"A-", "", '\0', VERB_TYPE_SPECIAL},           // ある（特殊）
    {"K-", "", '\0', VERB_TYPE_KAHEN},             // 来る（カ変）
};

#define VERB_DICT_SIZE (sizeof(verb_dict) / sizeof(verb_dict[0]))

// 「です」の活用マップ
typedef struct {
    const char *particle;  // right_particle
    const char *output;    // 出力
} desu_conjugate_t;

static const desu_conjugate_t desu_conjugate[] = {
    {"", "です"},
    {"n", "でして"},
    {"t", "でした"},
    {"k", "でしょう"},
    {"nt", "です."},
    {"nk", "ですが"},
    {"tk", "ですか?"},
    {"ntk", "ですね"},
    {NULL, NULL}
};

// 補助動詞・助動詞マップ
typedef struct {
    const char *pattern;  // left_particle-right_particle
    int conj_form;        // 活用形
    const char *suffix;   // 接尾辞
} auxiliary_map_t;

static const auxiliary_map_t auxiliary_exception[] = {
    {"tk-", CONJ_KANOU, "る"},              // 可能
    {"tk-n", CONJ_KANOU, "ない"},           // 可能+否定
    {"tk-t", CONJ_KANOU, "た"},             // 可能+過去
    {"tk-k", CONJ_KANOU, "ます"},           // 可能+丁寧
    {"tk-nt", CONJ_KANOU, "なかった"},      // 可能+否定+過去
    {"tk-nk", CONJ_KANOU, "ません"},        // 可能+否定+丁寧
    {"tk-tk", CONJ_KANOU, "ました"},        // 可能+過去+丁寧
    {"tk-ntk", CONJ_KANOU, "て"},           // 可能+て
    {"ntk-", CONJ_MASU, ""},                // 連用
    {"ntk-n", CONJ_NAI, "ず"},              // 否定
    {"ntk-t", CONJ_KATEI, "ば"},            // 仮定
    {"ntk-k", CONJ_IKOU, ""},               // 意向
    {"ntk-nt", CONJ_NAI, "なければ"},       // 否定+仮定
    {"ntk-nk", CONJ_NAI, "なく"},           // 否定+連用
    {"ntk-tk", CONJ_TE_TA, "てください"},   // 丁寧命令
    {"ntk-ntk", CONJ_MEIREI, ""},           // 命令
    {"", -1, ""}  // 終端
};

// 左側補助動詞マップ: [活用形, 補助動詞の語幹, 活用段, 活用行]
typedef struct {
    const char *particle;
    int conj_form;
    const char *stem;
    int verb_type;  // 1=五段, 2=上一段, 3=下一段
    char gyou;
} left_auxiliary_info_t;

static const left_auxiliary_info_t left_auxiliary[] = {
    {"n", CONJ_TE_TA, "て", 2, 'w'},      // ～ている
    {"t", CONJ_SHIEKI, "", 3, 's'},        // ～させる（使役）
    {"k", CONJ_UKEMI, "", 3, 'r'},        // ～られる（受身）
    {"nt", CONJ_TE_TA, "てもら", 1, 'w'}, // ～てもらう
    {"nk", CONJ_TE_TA, "てしま", 1, 'w'}, // ～てしまう
    {NULL, -1, "", 1, '\0'}
};

// 右側助動詞マップ
typedef struct {
    const char *particle;
    int conj_form;
    const char *suffix;
} right_auxiliary_t;

static const right_auxiliary_t right_auxiliary[] = {
    {"", CONJ_JISHO, ""},
    {"n", CONJ_NAI, "ない"},
    {"t", CONJ_TE_TA, "た"},
    {"k", CONJ_MASU, "ます"},
    {"nt", CONJ_NAI, "なかった"},
    {"nk", CONJ_MASU, "ません"},
    {"tk", CONJ_MASU, "ました"},
    {"ntk", CONJ_TE_TA, "て"},
    {NULL, -1, ""}
};

// 左側補助動詞情報を取得
static const left_auxiliary_info_t *get_left_auxiliary(const char *particle) {
    if (particle == NULL) return NULL;
    for (int i = 0; left_auxiliary[i].particle != NULL; i++) {
        if (strcmp(particle, left_auxiliary[i].particle) == 0) {
            return &left_auxiliary[i];
        }
    }
    return NULL;
}

// 右側補助動詞情報を取得
static const right_auxiliary_t *get_right_auxiliary(const char *particle) {
    if (particle == NULL) return NULL;
    for (int i = 0; right_auxiliary[i].particle != NULL; i++) {
        if (strcmp(particle, right_auxiliary[i].particle) == 0) {
            return &right_auxiliary[i];
        }
    }
    return NULL;
}

// 「です」の活用を取得
static const char *get_desu_conjugate(const char *particle) {
    if (particle == NULL) return NULL;
    for (int i = 0; desu_conjugate[i].particle != NULL; i++) {
        if (strcmp(particle, desu_conjugate[i].particle) == 0) {
            return desu_conjugate[i].output;
        }
    }
    return NULL;
}

// 活用形と助動詞を取得
static void get_conjugation_info(const char *left_particle, const char *right_particle,
                                 int *conj_form, char *suffix) {
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "%s-%s", left_particle, right_particle);

    // 例外パターンをチェック
    for (int i = 0; auxiliary_exception[i].conj_form != -1; i++) {
        if (strcmp(pattern, auxiliary_exception[i].pattern) == 0) {
            *conj_form = auxiliary_exception[i].conj_form;
            strcpy(suffix, auxiliary_exception[i].suffix);
            return;
        }
    }

    // 右側助動詞マップ
    for (int i = 0; right_auxiliary[i].particle != NULL; i++) {
        if (strcmp(right_particle, right_auxiliary[i].particle) == 0) {
            *conj_form = right_auxiliary[i].conj_form;
            strcpy(suffix, right_auxiliary[i].suffix);
            return;
        }
    }

    // デフォルト
    *conj_form = CONJ_JISHO;
    suffix[0] = '\0';
}

// メイン動詞活用関数
verb_result_t mejiro_verb_conjugate(const char *left_conso, const char *left_vowel,
                                    const char *left_particle,
                                    const char *right_conso, const char *right_vowel,
                                    const char *right_particle,
                                    const char *left_kana, const char *right_kana) {
    verb_result_t result = {{0}, false};

    char stroke[64];
    snprintf(stroke, sizeof(stroke), "%s%s-%s%s", left_conso, left_vowel, right_conso, right_vowel);

    const left_auxiliary_info_t *left_aux = get_left_auxiliary(left_particle);
    const right_auxiliary_t *right_aux = get_right_auxiliary(right_particle);

    int conj_form = CONJ_JISHO;
    char suffix[64] = {0};
    if (left_aux != NULL) {
        conj_form = left_aux->conj_form;
        suffix[0] = '\0';
    } else {
        get_conjugation_info(left_particle, right_particle, &conj_form, suffix);
    }

    // 「です」処理: right_conso == 'TN' && strlen(right_vowel) == 0
    if (strcmp(right_conso, "TN") == 0 && strlen(right_vowel) == 0) {
        strcpy(result.output, left_kana);
        const char *desu_form = get_desu_conjugate(right_particle);
        if (desu_form != NULL) {
            strcat(result.output, desu_form);
            result.success = true;
            return result;
        }
    }

    const verb_entry_t *verb = NULL;
    for (size_t i = 0; i < VERB_DICT_SIZE; i++) {
        if (strcmp(stroke, verb_dict[i].stroke) == 0) {
            verb = &verb_dict[i];
            break;
        }
    }

    // 辞書にある場合
    if (verb != NULL) {
        if (verb->type == VERB_TYPE_SPECIAL) {
            if (strcmp(stroke, "I-K") == 0) {
                strcpy(result.output, iku_conjugate[conj_form]);
            } else if (strcmp(stroke, "A-") == 0) {
                strcpy(result.output, aru_conjugate[conj_form]);
            } else {
                return result;
            }
        } else {
            strcpy(result.output, verb->stem);
            if (verb->type == VERB_TYPE_GODAN) {
                int idx = gyou_to_index(verb->gyou);
                strcat(result.output, godan_conjugate[idx][conj_form]);
                if (conj_form == CONJ_TE_TA && (verb->gyou == 'g' || verb->gyou == 'n' || verb->gyou == 'b' || verb->gyou == 'm')) {
                    char *te_pos = strstr(result.output, "て");
                    if (te_pos) {
                        strcpy(te_pos, "で");
                        strcat(result.output, te_pos + 3);
                    }
                    char *ta_pos = strstr(result.output, "た");
                    if (ta_pos) {
                        strcpy(ta_pos, "だ");
                        strcat(result.output, ta_pos + 3);
                    }
                }
            } else if (verb->type == VERB_TYPE_KAMI) {
                int idx = gyou_to_index(verb->gyou);
                strcat(result.output, kami_conjugate[idx][conj_form]);
            } else if (verb->type == VERB_TYPE_SIMO) {
                int idx = gyou_to_index_simo(verb->gyou);
                strcat(result.output, simo_conjugate[idx][conj_form]);
            } else if (verb->type == VERB_TYPE_KAHEN) {
                strcat(result.output, kahen_conjugate[conj_form]);
            }
        }

        if (left_aux != NULL) {
            strcat(result.output, left_aux->stem);
            int aux_conj = (right_aux != NULL) ? right_aux->conj_form : CONJ_JISHO;
            if (left_aux->verb_type == 1) {
                int idx_aux = gyou_to_index(left_aux->gyou);
                strcat(result.output, godan_conjugate[idx_aux][aux_conj]);
            } else if (left_aux->verb_type == 2) {
                int idx_aux = gyou_to_index(left_aux->gyou);
                strcat(result.output, kami_conjugate[idx_aux][aux_conj]);
            } else if (left_aux->verb_type == 3) {
                int idx_aux = gyou_to_index_simo(left_aux->gyou);
                strcat(result.output, simo_conjugate[idx_aux][aux_conj]);
            }
            if (right_aux != NULL) {
                strcat(result.output, right_aux->suffix);
            }
        } else {
            strcat(result.output, suffix);
        }
        result.success = true;
        return result;
    }

    // 辞書に無い場合、入力パターンから動詞を推論
    if (strlen(right_kana) == 0) {
        if (strlen(left_kana) > 0) {
            strcpy(result.output, left_kana);
        } else {
            result.output[0] = '\0';
        }
        strcat(result.output, sahen_conjugate[conj_form]);
        char *shizu_pos = strstr(result.output, "しず");
        if (shizu_pos) {
            memcpy(shizu_pos, "せず", 6);
        }
        if (left_aux != NULL) {
            strcat(result.output, left_aux->stem);
            int aux_conj = (right_aux != NULL) ? right_aux->conj_form : CONJ_JISHO;
            if (left_aux->verb_type == 1) {
                int idx_aux = gyou_to_index(left_aux->gyou);
                strcat(result.output, godan_conjugate[idx_aux][aux_conj]);
            } else if (left_aux->verb_type == 2) {
                int idx_aux = gyou_to_index(left_aux->gyou);
                strcat(result.output, kami_conjugate[idx_aux][aux_conj]);
            } else if (left_aux->verb_type == 3) {
                int idx_aux = gyou_to_index_simo(left_aux->gyou);
                strcat(result.output, simo_conjugate[idx_aux][aux_conj]);
            }
            if (right_aux != NULL) {
                strcat(result.output, right_aux->suffix);
            }
        } else {
            strcat(result.output, suffix);
        }
        result.success = true;
        return result;
    }

    if (strlen(right_conso) > 0 && strlen(right_vowel) == 0 && strlen(right_kana) > 0) {
        char gyou = kana_to_gyou(right_kana);
        if (gyou != '\0' && (gyou == 'k' || gyou == 'g' || gyou == 's' || gyou == 't' || 
                              gyou == 'n' || gyou == 'b' || gyou == 'm' || gyou == 'r' || gyou == 'w')) {
            if (strlen(left_kana) > 0) {
                strcpy(result.output, left_kana);
            } else {
                result.output[0] = '\0';
            }
            int idx = gyou_to_index(gyou);
            strcat(result.output, godan_conjugate[idx][conj_form]);
            if (conj_form == CONJ_TE_TA && (gyou == 'g' || gyou == 'n' || gyou == 'b' || gyou == 'm')) {
                char *te_pos = strstr(result.output, "て");
                if (te_pos) memcpy(te_pos, "で", 3);
                char *ta_pos = strstr(result.output, "た");
                if (ta_pos) memcpy(ta_pos, "だ", 3);
            }
            if (left_aux != NULL) {
                strcat(result.output, left_aux->stem);
                int aux_conj = (right_aux != NULL) ? right_aux->conj_form : CONJ_JISHO;
                if (left_aux->verb_type == 1) {
                    int idx_aux = gyou_to_index(left_aux->gyou);
                    strcat(result.output, godan_conjugate[idx_aux][aux_conj]);
                } else if (left_aux->verb_type == 2) {
                    int idx_aux = gyou_to_index(left_aux->gyou);
                    strcat(result.output, kami_conjugate[idx_aux][aux_conj]);
                } else if (left_aux->verb_type == 3) {
                    int idx_aux = gyou_to_index_simo(left_aux->gyou);
                    strcat(result.output, simo_conjugate[idx_aux][aux_conj]);
                }
                if (right_aux != NULL) {
                    strcat(result.output, right_aux->suffix);
                }
            } else {
                strcat(result.output, suffix);
            }
            result.success = true;
            return result;
        }
    }

    if (strcmp(right_vowel, "I") == 0) {
        char gyou = 'w';
        if (strlen(right_kana) > 0) {
            gyou = kana_to_gyou(right_kana);
            if (gyou == '\0') gyou = 'w';
        }
        if (gyou == 'k' || gyou == 'g' || gyou == 'z' || gyou == 't' || 
            gyou == 'n' || gyou == 'b' || gyou == 'm' || gyou == 'r' || gyou == 'w') {
            if (strlen(left_kana) > 0) {
                strcpy(result.output, left_kana);
            } else {
                result.output[0] = '\0';
            }
            int idx = gyou_to_index(gyou);
            strcat(result.output, kami_conjugate[idx][conj_form]);
            if (left_aux != NULL) {
                strcat(result.output, left_aux->stem);
                int aux_conj = (right_aux != NULL) ? right_aux->conj_form : CONJ_JISHO;
                if (left_aux->verb_type == 1) {
                    int idx_aux = gyou_to_index(left_aux->gyou);
                    strcat(result.output, godan_conjugate[idx_aux][aux_conj]);
                } else if (left_aux->verb_type == 2) {
                    int idx_aux = gyou_to_index(left_aux->gyou);
                    strcat(result.output, kami_conjugate[idx_aux][aux_conj]);
                } else if (left_aux->verb_type == 3) {
                    int idx_aux = gyou_to_index_simo(left_aux->gyou);
                    strcat(result.output, simo_conjugate[idx_aux][aux_conj]);
                }
                if (right_aux != NULL) strcat(result.output, right_aux->suffix);
            } else {
                strcat(result.output, suffix);
            }
            result.success = true;
            return result;
        }
    }

    if (strcmp(right_vowel, "IA") == 0) {
        char gyou = 'w';
        if (strlen(right_kana) > 0) {
            gyou = kana_to_gyou(right_kana);
            if (gyou == '\0') gyou = 'w';
        }
        if (gyou == 'k' || gyou == 'g' || gyou == 's' || gyou == 'z' || 
            gyou == 't' || gyou == 'd' || gyou == 'n' || gyou == 'h' || 
            gyou == 'b' || gyou == 'm' || gyou == 'r' || gyou == 'w') {
            if (strlen(left_kana) > 0) {
                strcpy(result.output, left_kana);
            } else {
                result.output[0] = '\0';
            }
            int idx = gyou_to_index_simo(gyou);
            strcat(result.output, simo_conjugate[idx][conj_form]);
            if (left_aux != NULL) {
                strcat(result.output, left_aux->stem);
                int aux_conj = (right_aux != NULL) ? right_aux->conj_form : CONJ_JISHO;
                if (left_aux->verb_type == 1) {
                    int idx_aux = gyou_to_index(left_aux->gyou);
                    strcat(result.output, godan_conjugate[idx_aux][aux_conj]);
                } else if (left_aux->verb_type == 2) {
                    int idx_aux = gyou_to_index(left_aux->gyou);
                    strcat(result.output, kami_conjugate[idx_aux][aux_conj]);
                } else if (left_aux->verb_type == 3) {
                    int idx_aux = gyou_to_index_simo(left_aux->gyou);
                    strcat(result.output, simo_conjugate[idx_aux][aux_conj]);
                }
                if (right_aux != NULL) strcat(result.output, right_aux->suffix);
            } else {
                strcat(result.output, suffix);
            }
            result.success = true;
            return result;
        }
    }

    if (strlen(right_conso) > 0 || strlen(right_vowel) > 0) {
        if (strlen(left_kana) > 0) {
            strcpy(result.output, left_kana);
            strcat(result.output, right_kana);
        } else {
            strcpy(result.output, right_kana);
        }
        int idx = gyou_to_index('r');
        strcat(result.output, godan_conjugate[idx][conj_form]);
        char *gozari_pos = strstr(result.output, "ござり");
        if (gozari_pos) {
            memcpy(gozari_pos + 6, "い", 3);
        }
        if (left_aux != NULL) {
            strcat(result.output, left_aux->stem);
            int aux_conj = (right_aux != NULL) ? right_aux->conj_form : CONJ_JISHO;
            if (left_aux->verb_type == 1) {
                int idx_aux = gyou_to_index(left_aux->gyou);
                strcat(result.output, godan_conjugate[idx_aux][aux_conj]);
            } else if (left_aux->verb_type == 2) {
                int idx_aux = gyou_to_index(left_aux->gyou);
                strcat(result.output, kami_conjugate[idx_aux][aux_conj]);
            } else if (left_aux->verb_type == 3) {
                int idx_aux = gyou_to_index_simo(left_aux->gyou);
                strcat(result.output, simo_conjugate[idx_aux][aux_conj]);
            }
            if (right_aux != NULL) strcat(result.output, right_aux->suffix);
        } else {
            strcat(result.output, suffix);
        }
        result.success = true;
        return result;
    }

    return result;
}

