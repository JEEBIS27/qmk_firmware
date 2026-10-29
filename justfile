set shell := ["bash", "-euc"]
set positional-arguments

# コマンド一覧
default:
    @"{{just_executable()}}" --list

kb := 'keyboards/jeebis/mejiro31'


# 隣のローカルEngineでVial UF2を作成（事前syncは不要）
build *args:
    python3 {{kb}}/build_vial.py --engine-source ../Mejiro_Engine "$@"

# ローカルEngineを使うホストテスト
test:
    bash {{kb}}/tests/run.sh

# 通常の開発チェック（USB・実機は別）
check: test

# 指定EngineでVial作業領域を準備（未コミットの変更も反映）
sync source='../Mejiro_Engine':
    python3 {{kb}}/build_vial.py --prepare-only --engine-source "$1"

# buildと同じVial版ビルド
alias build-vial := build

# ビルド済みQWERTY JP/ENをMejiro31サイトへ取り込む
sync-site destination='../Mejiro31':
    "{{just_executable()}}" --justfile "$1/justfile" sync "{{justfile_directory()}}/.build"

# サイト用QWERTY JP/ENをビルドして取り込む
build-site: build sync-site
