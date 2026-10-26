# Vial needs its own quantum core; the normal QMK checkout does not contain it.
ifeq ($(wildcard quantum/vial.c),)
$(error Use python3 keyboards/jeebis/mejiro31/build_vial.py to prepare the Vial core and build this keymap)
endif
VIA_ENABLE = yes
VIAL_ENABLE = yes
VIAL_INSECURE = no
QMK_SETTINGS = no
TAP_DANCE_ENABLE = no
KEY_OVERRIDE_ENABLE = no
COMBO_ENABLE = no
MEJIRO_PROFILE ?= 0
MEJIRO_LAYOUT ?= 0
OPT_DEFS += -DMJI_EN_PROFILE=$(MEJIRO_PROFILE)
OPT_DEFS += -DMJI_WEB_EN_DEFAULT=$(MEJIRO_LAYOUT) -DMJI_WEB_JP_DEFAULT=$(MEJIRO_LAYOUT)
