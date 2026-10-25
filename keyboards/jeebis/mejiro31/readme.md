# Mejiro31

二段圧縮、Tap/Hold、レイヤー、Modを統一入力エンジンで扱う31キーキーボード。速記モードのメジロ式変換にはRust製の共通 `Mejiro_Engine` を使用する。

- [操作仕様・RMK移植の基準](input_spec.md)
- [実装構成・配列設定・検証手順](engine.md)

## ビルド

母音・例外かな・活用・助詞の定義は [Mejiro_Engine](https://github.com/JEEBIS27/Mejiro_Engine) で管理する。Plover版も同じRustコアを使用するため、定義変更時にC/Pythonへ移植する必要はない。ウァ・ウィ・ウェ・ウォ・イェの促音／長音と、テュ・ヴュ・トゥ・ドゥも共通定義に従う。

QMKリポジトリと `Mejiro_Engine` を同じ親ディレクトリへ配置し、Rustの `thumbv6m-none-eabi` ターゲットを導入する。QMK環境で、日本語向けまたは英語向けのデフォルトを選択する。

```sh
rustup target add thumbv6m-none-eabi
qmk compile -kb jeebis/mejiro31 -km jp_default
qmk compile -kb jeebis/mejiro31 -km en_default
```

両方とも同じ統一入力エンジンを使用する。日英の代替配列と、ホストのJIS/USは独立した設定。初期値と設定方法は [engine.md](engine.md) を参照。

生成物はQMKルートの `jeebis_mejiro31_jp_default.uf2` / `jeebis_mejiro31_en_default.uf2`。

## 書き込み

PCBのBOOTボタンを押しながらUSBへ接続し、表示された `RPI-RP2` ドライブへ対応するUF2をコピーする。BOOTボタンはUSB端子から離れた側。

旧ファームウェアのVIA保存形式とは互換性がないため、保存配列を退避し、新形式の動的キーマップを初期化して使用する。
