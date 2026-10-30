# Mejiro31 統一入力エンジン

`jp_default` と `en_default` はともに統一入力エンジン専用のキーマップ。共通の規範は [input_spec.md](input_spec.md)。実装検証用だった `unified` キーマップは両デフォルトへ移行して廃止した。

## 構成

| 場所 | 役割 |
| --- | --- |
| `mejiro_input.c/.h` | 物理入力、候補、論理押下、Tap/Hold、Mod、レイヤー、Sticky、出力状態 |
| `mejiro_layout.c/.h` | Alt layoutとホストJIS/USの解決。QMK非依存 |
| `layouts/catalog.h` | HID usageと修飾ビットで表現した18種類の配列。594エントリ |
| `qmk/keymap_impl.h` | 両キーマップ共通のQMK接続、キーコードの読み替え、USB送信、速記との接続 |
| `qmk/defaults.h` | 各論理位置のTap/Hold初期値。日英の差も通常の割り当てデータ |
| `keymaps/jp_default/keymap.c` | 日本語向けデフォルトの配列選択と初期設定 |
| `keymaps/en_default/keymap.c` | 英語向けデフォルトの配列選択と初期設定 |
| `steno/chord.c/.h` | 速記キーの押下・解放からストロークを確定。キーボード用のコンボキューとは別 |
| `steno/engine.c/.h` | Rustエンジンの出力をQMKのHID送信・IME操作へ接続 |
| `steno/rust/` | 共通メジロエンジンをRP2040向けにリンクする `no_std` ラッパー |
| `ld/RP2040_FLASH_TIMECRIT.ld` | Rust変換処理用スタックを通常RAMへ配置 |

旧 `alt_layout.*`、`alt_layout_config.h`、`alt_layouts_definitions.h`、`jis_transform.h` およびキーボード用FIFOのQMK拡張は廃止。配列データをQMK表現から移植可能なデータへ変換し、594エントリの内容一致を検証した。QMK標準コンボ機能そのものは維持するが、Mejiro31では無効とする。

## デフォルトの差

| 設定 | jp_default | en_default |
| --- | --- | --- |
| 日英の代替配列の初期値 | 英語Graphite / 日本語QWERTY | 両方QWERTY |
| ホストJIS/USの初期値 | US | US |
| 速記モードでの初期処理 | メジロ式 | Plover HID送信 |
| 通常レイヤー右親指 | 英数 / かな、組で@ | ! / ?、組で@ |
| Shiftレイヤー右親指 | ! / ?、組で@ | & / \|、組で# |
| 数字レイヤー右親指 | & / \| | ! / ? |
| 数字Shiftレイヤー右親指 | Ctrl / AltのHold | & / \|の通常キー |

旧en_defaultの記号変換分岐は、通常/Shift・数字/数字Shiftの割り当てデータへ置き換えた。記号専用の実行時変換・切り替え分岐は設けない。Space/Enterも通常のTap/Hold定義であり、独立したQMK Tap-Holdには通さない。

ホストJIS/USはキーマップ名とは独立した設定。以前の起動値を維持しており、JISホストを初期値にする場合は各 `keymap.c` の `MJI_DEFAULT_JIS` を `true` にする。Function上の `MJI_JIS` でも切り替えられる。

JIS/US・Alt有効・メジロ/Plover HID選択は `eeconfig` の32ビットユーザー領域へ保存する。形式マーカー `0x4d310100` と下位3ビットのフラグを使い、VIAのキー割り当て領域は変更しない。JP/ENは同じ保存値を使用する。未初期化・旧形式・不適合なバージョンのときだけビルドの初期値を保存し、有効な値は再起動や再書き込み後も使う。初期値マクロの変更は既存の保存値を上書きしない。同じ値を繰り返し書かない。

## 代替配列の選択

各 `keymap.c` の定義を変更する。

```c
#define MJI_ENGLISH_LAYOUT mji_layout_graphite
#define MJI_JAPANESE_LAYOUT mji_layout_o24
```

英語向けでも日英別のプロファイルを使える。既存のプリセット一覧とビルドスクリプトも更新済みで、en_defaultの `layout` フィールドは両プロファイルへ適用する。27レシピすべてについて選択結果を検証している。

```sh
python3 keymaps/jp_default/build_firmware_presets.py --list
python3 keymaps/en_default/build_firmware_presets.py --list
```

エンジンは「論理キーの役割 → Alt layout → ホストJIS/US」の順に解決して結果を押下ごとに保持する。Ctrl/Alt/GUI等のShift以外の修飾がある場合、Alt layoutを適用しない。出力に必要なShiftはHoldの提供元と別に管理し、後続の役割判定には混入させない。

## Tap/Holdの保存形式

4行12列のマトリクス内で、実スイッチは31位置、未配線の16位置をコンボの論理位置に使用する。コンボの関係は `qmk/keymap_impl.h` の `physical_pairs` に物理位置として定義し、出力文字から独立させる。

| Tapマップ | Holdマップ | 内容 |
| --- | --- | --- |
| 0 | 6 | 通常 |
| 1 | 7 | Shift |
| 2 | 8 | 数字 |
| 3 | 9 | 数字Shift |
| 4 | 10 | Function |
| 5 | 11 | 速記モード内の非速記機能 |

同じセルのTapとHoldが一組。Holdの `KC_NO` はfalse、両方 `KC_TRNS` の場合は下位の組へ透過する。Holdに修飾キーを置けばMod、`MO(n)`を置けばレイヤー、それ以外の対応済み機能を置けばその機能を保持する。

SpaceとEnterのHoldは全通常レイヤーで同じ左Shiftに統一する。左右Shift同時押しでCaps Lockを有効化するホスト設定でも、Space+EnterからCaps Lockが発動しないようにする。Vial保存済みキーマップのEnter Holdも`KC_LSFT`へ変更する必要がある。

編集元は `qmk/defaults.h` のレイヤー別定義で、各位置を `KEY(Tap, Hold)` として並べる。例えば `KEY(KC_SPC, KC_LSFT)` はTapでSpace、Holdで左Shift、`KEY(KC_F, KC_NO)` はFのみ、`KEY(KC_NO, KC_LALT)` は左Altのみ。`KEY(KC_AT, LCA(KC_NO))`はTapで`@`、HoldでCtrl+Altとなり、通常キーを送らず両方の修飾だけを保持する。右親指同士のコンボはKeyboard通常面が`@`、Keyboard Shift・Number・Number Shiftが`#`で、いずれもHoldはCtrl+Alt。四つのマトリクス行は各12位置を6位置ずつ折り返して記載する。コンパイル時にこのペア定義から上記のTap/Hold保存マップを生成するため、離れた二つの面を別々に編集する必要はない。

QMKの `MT` / `LT` への互換分岐と標準Tap-Hold処理は廃止した。QMK形式の修飾付き出力の読み替え、通常の修飾/レイヤーアクション、Sticky用Shiftの `OSM` の読み替えは接続側の形式処理として残る。

コンボの保存位置は次のとおり。数字は0始まりの `row,col`。

| 論理位置 | 通常時の初期値 |
| --- | --- |
| 2,0 / 2,1 / 2,2 / 2,3 | A / S / D / F |
| 2,7 / 2,8 / 2,9 / 2,10 / 2,11 | G / H / J / K / L |
| 3,0 / 3,1 | ; / ' |
| 3,2 / 3,3 | Esc / Tab |
| 3,8 / 3,9 | Delete / Backspace |
| 3,10 | 右親指ペアの@ |

Shift時のマップ選択は論理HoldのMod状態から決める。レイヤーの優先順位は番号ではなくHoldの有効化順。NKROを起動時に有効にする。

従来の6レイヤーVIA保存データとは番号・仮想位置・Hold領域が異なる。既存の保存配列を退避し、新形式の動的キーマップを初期化して使用する。起動のたびに保存配列を消去する処理はない。専用Vial UIは未実装。

## 速記との接続

速記キーはキーボード用の二段圧縮を通さない。`steno/chord.c` はストロークの確定のみを担当し、変換・文脈・履歴・マクロは隣接する `Mejiro_Engine` リポジトリのRustコアで処理する。旧QMK版の変換・コマンド・動詞・略語のC実装は廃止した。

QMK接続層はローマ字入力の日本語IMEを対象とし、Rustの出力をキー列へ変換する。明示的な記号キーはホストJIS/US設定を共有する。undoの削除単位はかな出力のプレビューから算出する。履歴が空でも `-U` はBackspaceを2回送る。使用するIMEでの確定・変換やカーソル移動後には履歴をリセットし、実機とIMEの組み合わせで削除結果を確認する。`#-n` によるKana切り替えはQMK接続層では無効にしている。

`MJI_MODE` による速記への切り替え直前に、通常入力の日英プロファイルを保存する。JP版でメジロ式が有効なら、速記では日本語を選択して `KC_LNG1`（IME ON）を1回送信する。日英両版とも通常入力へ戻るときは保存したプロファイルへ戻し、日本語なら `KC_LNG1`、英語なら `KC_LNG2`（IME OFF）を1回送る。速記中の日英変更は通常入力用の保存値へ影響しない。Alt有効・ホストJIS/USは共通設定のまま維持する。IMEの復元はキーボード側の日英選択に基づき、OSだけで行ったIME変更は取得しない。接続テストで往復、速記中の日英変更、代替配列の実出力、修飾解除後のIME送信を確認した。

速記コマンド `#-t` はIME ON (`KC_LNG1`) を1回送信する。`#-k` は利用可能な別システムへ切り替えてからIME OFF (`KC_LNG2`) を1回送信する。切り替えは `mejiro_steno_select_alternate_system()` フックが担当し、標準実装は何もせず `false` を返す。英語速記等を実装した際はこのフックを上書きし、選択成功時に `true`、未対応時は状態を変更せず `false` を返す。IME OFFは結果によらず実行する。F13/F14や切り替え用の文字列は送信しない。Plover HIDモードではストロークを受信するPlover側の辞書が動作を決める。

## OS出力と速記状態

JIS Alt＋全角半角は移植可能な `mejiro_layout.c` で記号変換より先に解決する。MacのHome/Endは接続側の `encode` コールバックで、配列解決後に通常の出力ストロークへ変換する。変換後の修飾は出力用の情報で、レイヤー判定用Holdへ混入させない。Macの速記キー送信でも同じ変換器を使う。

速記中はCtrl/Alt/GUIの論理Holdを検出し、通常入力の基本レイヤーを参照する。数字等の保持レイヤーは優先する。修飾がなくなれば速記へ戻り、通常入力として登録済みのキーの解放先は変えない。移行時、内蔵メジロ式の未確定ストロークは破棄する。外部Ploverには残っている押下の全解放を送り、送信済みのストロークの確定はPloverに任せる。全体リセット時も同じ処理を使う。

入力リセットは録音状態を保持する。共通Rustコアのコマンド表は変更していない。通常・サニタイザー付きホストテストで再起動時の設定復元、Macの範囲選択と保持、JISショートカット、録音継続・明示的停止後の再生、速記中ショートカットの入力経路を確認する。

### 混在録音の接続

共通エンジンの `Session::record_external()` は、既に送信したHIDイベントを録音する専用API。`Output` の文字・意味操作は従来形式のままとし、外部イベントをUTF-8バイト位置と意味操作数に結び付けて保存する。通常のPlover録音は外部イベントなしで従来どおり使える。共通API・C構造体の詳細は隣接エンジンの `docs/recording.md` と `native/mejiro.h` を参照。

QMKは `register_output` / `unregister_output` / `tap_output` と修飾出力から、実際のホスト用usage・Modを取得する。Consumer/Systemも標準HID usageへ変換する。`steno/engine.c` の一時バッファへ保存し、速記復帰・ショートカット終了・次の速記処理前に一括登録する。モード境界の実解放とIME切り替えも記録し、後から離されたキーの解放も取りこぼさない。

再生は意味出力と `replayed_external()` を順序通りに混ぜ、外部イベントは入力エンジンを経由せず直接送る。生のキーボード出力は再解釈しないので、元のホスト配列／OS用のストロークを維持する。再生中の送信は取り込みを抑制し、終了時は再生キーを解放して実際の保持状態へ戻す。

容量超過時は録音中の枠を中断・破棄し、`mejiro_engine_recording_failed()` に失敗を保持する。新しい録音を開始すると失敗状態を解除する。失敗した枠への最初の録音切り替えは停止として扱い、停止のつもりで送ったコマンドが録音を再開しないようにする。登録エラーによる不完全なマクロは再生せず、通常のキー入力は継続する。速記文字列の録音容量超過も録音を中断してから当該入力を処理し直す。512文字相当ではなく512 UTF-8バイト・8意味操作・128外部イベントという各上限を持つ。押下時間やOSリピートは保存しない。Plover HID送信先のPloverが生成した出力は、ファームウェア側からは録音しない。

テストは共通コアの原子的一括登録・複数録音・自己再録音抑制、QMK/Rustの実接続によるモード往復と二重送信防止、修飾を先に離す操作、文字とコマンドとの混在順序、再生後のキー解放、容量超過中の通常入力継続を含む。ARM検査も初期化から録音継続・外部登録・混在再生まで実行する。

## 時間設定

現在の `config.h` はコンボ150 ms、片側解放後の継続待ち50 ms、明確な長押し300 ms。継続待ちより明確な長押しを長くする。接続テストのスタブは境界試験用の40/15/200 msを使用している。

## 検証とビルド

キーボードディレクトリから:

```sh
tests/run.sh
CC=clang CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' tests/run.sh
```

両デフォルトの接続テスト、5万回のイベント列、解放・同時保持・タイマー境界・容量上限、配列解決、カタログ、全プリセットの選択を確認する。ホストテストのUSB・EEPROM・速記送信はスタブであり、実機検証とは区別する。

QMK環境で:

```sh
rustup target add thumbv6m-none-eabi
qmk compile -kb jeebis/mejiro31 -km jp_default
qmk compile -kb jeebis/mejiro31 -km en_default
```

QMKリポジトリと `Mejiro_Engine` を同じ親ディレクトリに置く。ビルド時にCargoが共通エンジンとQMK用ラッパーを `thumbv6m-none-eabi` の静的ライブラリとして生成し、QMKがUF2へリンクする。手作業で `.a` をコピーする必要はない。

2026-10-24、ARM GCC 14.3.1とQMK CLI 1.2.0でRustエンジンを含む両ターゲットのコンパイル・リンク・UF2生成に成功。ホストテストでは物理速記、文脈依存助詞、undo、繰り返し、マクロ、IME記号操作を確認した。実機への書き込み・USB実レポート・IME上の出力確認は未実施。通常入力で対応済みの機能は基本HID、Mod、レイヤー、Sticky、00、配列/モード切り替え。Tap Danceや動的マクロ等の任意のQMK機能には別途接続処理が必要。

メジロ式変換では32 KiBの処理用スタックを使う。RP2040のメインコア・第2コアの処理用スタックを通常RAMへ置き、ELFの領域配置を確認した。生成UF2はRP2040のブロック形式を検証済み。

### 起動時スタック超過の修正（2026-10-24）

旧ビルドはRust初期化の `*session = Session::new()` が約16 KiBの戻り値と、コンストラクター内部の約17 KiBの一時領域を同時に使い、確保した32 KiBを超えていた。リンク成功とホストテストだけでは検出できず、実機で全キーが無反応になる原因となる。初期値をフラッシュ内の静的テンプレートから直接コピーする方式へ修正した。

`tests/test_arm_boot.py` は生成ELFのARM命令をUnicornで実行し、ELFから取得した実際のスタック境界を検査する。呼び出し元用に512バイトを予約した条件で、旧ELFは33,824バイトで境界超過。今回の両ELFでは初期化・録音継続・外部入力登録・明示的停止・混在再生まで実行し、最大18,976バイトで完了した。Pico SDKの除算に必要なSIOレジスターのみをモデル化する。この検査はUSBやRP2040周辺回路の実機検証を代替しない。ホスト回帰テストと両ターゲットのQMKビルドも成功。今回の変更後の実機確認は未実施。

STN単体で母音を押さない場合、かなは作らず追加音のみを送る。`STN-` は空、`STNtk-` は `ltsu`、`STNntk-` は `-`、`KAntk-STNtk` は `ka-ltsu` をローマ字IMEへ送る。

```sh
# pyelftools と unicorn が必要。QMKリポジトリ直下から実行。
python keyboards/jeebis/mejiro31/tests/test_arm_boot.py \
  .build/jeebis_mejiro31_jp_default.elf \
  .build/jeebis_mejiro31_en_default.elf
```

2026-10-25、共通Rustエンジンのマイナー母音・例外かな更新を含めて両ターゲットを再ビルドした。QMK接続テストにウァ・ヴァ・テュー・ヴュー・ドゥー・ヅォーのローマ字出力確認を追加し、ホスト回帰テストは成功。両ELFのARM初期化・混在録音再生検査は引き続き18,976/32,768バイトで成功した。以下は更新後の成果物で、実機への書き込みは未実施。

- [jp_default UF2](../../../jeebis_mejiro31_jp_default.uf2): 300032 bytes、SHA256 `6c33d850db1a540b574ce936aaba6c9a1da06a0a1af428112e7cbf502e098e11`。
- [en_default UF2](../../../jeebis_mejiro31_en_default.uf2): 299520 bytes、SHA256 `2084a77c86ab6c925b4a8756c3d20f16502bcbc1b7947ce5cf72a18cb8c8b699`。


## Plover HIDへの移行（2026-10-25）

外部Plover向けのGemini PR／USB CDC経路を廃止し、`STENO_ENABLE = no`、`PLOVER_HID_ENABLE = yes` へ切り替えた。内蔵メジロ式と通常入力のエンジンは共通のまま。VIA用Raw HIDは従来のinterface 1を維持し、Plover専用HID interfaceを追加する。今回はChibiOS USBに必要な本家QMKのtransportのみをバックポートし、他のプラットフォームではこの設定をビルドエラーにする。

`steno/plover.c` は既存の `STN_*` 割り当てを現行Plover HIDの64bit配置へ写す。S1/S2、4つの星、N1..N4をそれぞれ独立したプロトコル位置に保持する。旧toyboot4e版の `PLV_X1` 等の旧ビット配置は流用しない。同じキーコードを複数の物理キーへ割り当てた場合だけ参照数を数え、一方の解放で他方の押下を解除しない。ストロークの蓄積・再構成・確定判定は実装しない。

押下・解放はそれぞれ即時に送る。モード変更、ショートカットへの移行、配列変更等の境界では残っているHIDキーをまとめて解放する。Ploverへ送信済みのストロークは取り消せず、全解放時の扱いはPloverに任せる。境界前から押されている物理キーは解放まで無視する。内蔵メジロ式を選択している間はHIDへ送信しない。

Plover 5.1.0以降のMachineを `Plover HID` に設定する。シリアルポートの指定と旧 `plover-machine-hid` プラグインは不要。隣接するPlover_Mejiroの `Mejiro/system.py` には既に現行HIDのキーマップ（`S2-`、`*2`..`*4`、`#3/#4`を含む）が定義されており、変更しない。ホスト側のHIDアクセス権や実機接続の確認は別途必要。

参照: [本家QMK](https://github.com/qmk/qmk_firmware/blob/master/quantum/plover_hid.c)、[現行キー配置](https://github.com/qmk/qmk_firmware/blob/master/quantum/plover_hid.h)、[toyboot4e版](https://github.com/toyboot4e/qmk_firmware/tree/9e58d61f114a2177dd41917529f94917794ff773)。

検証結果: JP/EN両版のビルド、`tests/run.sh`、ASan/UBSan検査が成功。`tests/test_plover_usb.py` は生成ELF内の実際のUSB定義からReport ID 0x50・64bit・9byteの専用IN endpoint・VIA interface 1の維持・CDCと旧速記送信関数の除去を検査する。両ELFで成功した。`tests/test_arm_boot.py` も両版で18,976/32,768バイトのスタック使用で成功。今回のHID通信の実機確認は未実施。

```sh
python tests/test_plover_usb.py ../../../../.build/jeebis_mejiro31_jp_default.elf ../../../../.build/jeebis_mejiro31_en_default.elf
```

## 片手モード

内蔵メジロ式とPlover側のメジロ式は、共通Engineの片手モードを使用する。
記号も含めた左側の全押し（`#STKNYIAUntk-`）で左手、右側の全押し（`-STKNYIAUntk*`）で右手へ移行する。
反対側のキーを含む入力はその打鍵から両手入力として処理し、両手モードへ戻る。両側全押しでも両手へ戻る。
左手モードの略語キーは左の `#`、右手モードは右の `*`。たとえば左 `#KAU-`／右 `-KAU*` は「こと」、左 `#n-`／右 `-n*` は「しない」。
かな・母音・追加音は共通表を使う。片手モードのい段・え段は記号キーなしなら通常のかな（`KI`＝き、`KIA`＝け）で、記号付きの場合だけ一般略語の判定後、登録動詞より先に上一段・下一段を生成する（`TIA*`＝てる）。子音単体の五段動詞生成は従来どおり。一般略語を優先して空き枠に片手動詞を使う。母音・促音・Undoも共通Engineで管理する。

| 親指 | 単独 | 一般略語＋記号 | 動詞活用 |
| --- | --- | --- | --- |
| `n` | Space | 、 | ない |
| `t` | 、 | に | た |
| `k` | 。 | の | ます |
| `tk` | ・ | で | ました |
| `nt` | ？ | と | なかった |
| `nk` | を | を | ません |
| `ntk` | Enter | は | て |

片手モードの `U*` はUndo、`AU*` はBackspace。左は `#U-`／`#AU-`、右は `-U*`／`-AU*` で入力モードを維持する。

Plover HID使用時はPlover側にも対応版の導入が必要。内蔵版は更新したEngineをリンクしたUF2を書き込むと利用できる。
ビルドは `just build`（隣接するローカルEngineを使用）で行う。全押し・左右切り替え・記号キー・両手復帰は実際のストローク確定処理を通すホスト回帰テストに含める。

### NixによるQMK開発環境

QMKリポジトリの `nix/flake.nix` と `nix/flake.lock` に開発環境を定義する。QMK CLI、ARM/AVRコンパイラー、書き込みツール、Make、Git、just、rustup、ARM検査用のpyelftools・Unicornを含む。Nixpkgsの版はlockファイルで固定する。

```sh
just dev                # 開発環境へ入る
just dev --command arm-none-eabi-gcc --version
just build              # 環境の外からでも、同じNix環境でJP/ENをビルド
just test               # 同じ環境でホスト回帰テスト
```

初回だけRustツールチェーンとRP2040用ターゲットを用意する（導入済みなら不要）。

```sh
just dev --command rustup toolchain install stable --profile minimal
just dev --command rustup target add thumbv6m-none-eabi
```

Rustツールチェーンはrustupで管理し、既存のユーザー設定を使用する。`just build`・`just test` に手動のPATH追加は不要。初回はNixのパッケージ取得に時間がかかる場合がある。直接 `build_vial.py` を実行してコンパイラーが見つからない場合も、このプロジェクトのNix開発環境で再実行する。`just sync`（`--prepare-only`）にはコンパイラーを要求しない。
