STENO_ENABLE = yes
TAPPING_ENABLE = yes
STRING_ENABLE = yes
OS_DETECTION_ENABLE = yes
COMBO_ENABLE = no

SRC += $(KEYBOARD_PATH_1)/combo_fifo.c
SRC += $(KEYBOARD_PATH_1)/alt_layout.c
SRC += $(KEYBOARD_PATH_1)/mejiro_fifo.c
SRC += $(KEYBOARD_PATH_1)/mejiro_commands.c
SRC += $(KEYBOARD_PATH_1)/mejiro_transform.c
SRC += $(KEYBOARD_PATH_1)/mejiro_verb.c
