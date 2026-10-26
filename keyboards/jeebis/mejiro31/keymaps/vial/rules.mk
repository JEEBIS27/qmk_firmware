# SPDX-License-Identifier: GPL-3.0-or-later
# Vial is enabled keyboard-wide; this is the generic site-download preset.
MEJIRO_PROFILE ?= 0
MEJIRO_LAYOUT ?= 0
OPT_DEFS += -DMJI_EN_PROFILE=$(MEJIRO_PROFILE)
OPT_DEFS += -DMJI_WEB_EN_DEFAULT=$(MEJIRO_LAYOUT) -DMJI_WEB_JP_DEFAULT=$(MEJIRO_LAYOUT)
