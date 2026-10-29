# Mejiro31

二段圧縮、Tap/Hold、レイヤー、Modを統一入力エンジンで扱う31キーキーボード。速記モードのメジロ式変換にはRust製の共通 `Mejiro_Engine` を使用する。

- [操作仕様・RMK移植の基準](input_spec.md)
- [実装構成・配列設定・検証手順](engine.md)

## ビルド

母音・例外かな・活用・助詞の定義は [Mejiro_Engine](https://github.com/JEEBIS27/Mejiro_Engine) で管理する。Plover版も同じRustコアを使用するため、定義変更時にC/Pythonへ移植する必要はない。ウァ・ウィ・ウェ・ウォ・イェの促音／長音と、テュ・ヴュ・トゥ・ドゥも共通定義に従う。

Rustの `thumbv6m-none-eabi` ターゲットを導入する。QMK環境で、日本語向けまたは英語向けのデフォルトを選択する。

```sh
rustup target add thumbv6m-none-eabi
qmk compile -kb jeebis/mejiro31 -km jp_default
qmk compile -kb jeebis/mejiro31 -km en_default
```

両方とも同じ統一入力エンジンを使用する。日英の代替配列と、ホストのJIS/USは独立した設定。初期値と設定方法は [engine.md](engine.md) を参照。

生成物はQMKルートの`.build/jeebis_mejiro31_jp_default.uf2` / `.build/jeebis_mejiro31_en_default.uf2`です。同じ場所にチェックサムとビルド情報も保存します。従来互換のコピーはQMKルートの `jeebis_mejiro31_jp_default.uf2` / `jeebis_mejiro31_en_default.uf2`。

## Ploverとの接続

外部Plover向け速記はPlover HIDを使用する。Plover 5.1.0以降でMachineを `Plover HID` に設定する。シリアルポートの指定は不要。内蔵メジロ式との切り替えや通常入力は従来と同じ操作で使用できる。

## 書き込み

PCBのBOOTボタンを押しながらUSBへ接続し、表示された `RPI-RP2` ドライブへ対応するUF2をコピーする。BOOTボタンはUSB端子から離れた側。

旧ファームウェアのVIA保存形式とは互換性がないため、保存配列を退避し、新形式の動的キーマップを初期化して使用する。

## Vial対応（全キーマップ共通）

Mejiro31は通常の日本語版・英語版・既存プリセットを含め、すべてVial対応です。従来のビルドコマンドでサイトにも接続できるUF2を生成します。

```sh
make jeebis/mejiro31:jp_default
qmk compile -kb jeebis/mejiro31 -km en_default
```

内部で固定バージョンのVialコアと、`Mejiro_Engine`の既定ブランチの最新版を取得します。エンジンは毎回リモートを確認するため、ビルド時にはネット接続が必要です。取得に失敗した場合は古いキャッシュで続行せず停止します。使用したエンジンのコミットIDはログと、UF2と同じ場所の`*.uf2.json`に記録します。出力UF2は従来どおりリポジトリ直下にコピーされます。初期配列と既存の二段圧縮・Tap/Hold処理は維持します。サイトに接続しなくてもキーボードとして使用できます。

[実装・依存関係・検証手順](keymaps/vial/readme.md)
