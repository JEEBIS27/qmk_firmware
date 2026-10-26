# SPDX-License-Identifier: GPL-3.0-or-later
# Shared by all named/default/preset builds. Vial options are not upstream
# keyboard.json metadata, so keep them in this common build include.
VIAL_ENABLE = yes
VIAL_INSECURE = no
QMK_SETTINGS = no
TAP_DANCE_ENABLE = no
KEY_OVERRIDE_ENABLE = no
OPT_DEFS += -DMJI_WEB_VIAL
EXTRAINCDIRS += $(KEYBOARD_PATH_1)/keymaps/vial
VIAL_DEFINITION = $(KEYBOARD_PATH_1)/keymaps/vial/vial.json
