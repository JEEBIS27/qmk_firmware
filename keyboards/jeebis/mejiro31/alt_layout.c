/*
Copyright 2025 JEEBIS <jeebis.iox@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "alt_layout.h"

// グローバル変数の実装
bool is_alt_mode = false;
bool force_qwerty_active = false;
uint16_t default_layer = 0; // (0:_QWERTY, 1: _GEMINI)

// alt_transform()関数の定義（各キーマップで実装される）
// extern uint16_t alt_transform(uint16_t kc);
