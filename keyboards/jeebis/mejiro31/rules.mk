VIA_ENABLE = yes
include $(KEYBOARD_PATH_1)/qmk/vial_rules.mk
STENO_ENABLE = no
PLOVER_HID_ENABLE = yes
STRING_ENABLE = yes
OS_DETECTION_ENABLE = yes
COMBO_ENABLE = no
# The Rust session's registered-verb path (e.g. KNAnt-ntk*) can exceed
# 32 KiB on Cortex-M0+, before sending any keys. Keep room for callers
# and other inflections in the RP2040's shared 256 KiB SRAM region.
USE_PROCESS_STACKSIZE = 0x10000
LDFLAGS += -L$(TOP_DIR)/platforms/chibios/boards/common/ld

SRC += $(KEYBOARD_PATH_1)/mejiro_input.c
SRC += $(KEYBOARD_PATH_1)/mejiro_layout.c
SRC += $(KEYBOARD_PATH_1)/steno/plover.c
SRC += $(KEYBOARD_PATH_1)/steno/chord.c
SRC += $(KEYBOARD_PATH_1)/steno/engine.c

MEJIRO_RUST_DIR := $(KEYBOARD_PATH_1)/steno/rust
MEJIRO_ENGINE_DIR := $(abspath $(KEYBOARD_PATH_1)/../../../../Mejiro_Engine)
MEJIRO_RUST_LIB := $(MEJIRO_RUST_DIR)/target/thumbv6m-none-eabi/release/libmejiro_qmk.a
$(MEJIRO_RUST_LIB): $(MEJIRO_RUST_DIR)/Cargo.toml $(MEJIRO_RUST_DIR)/Cargo.lock $(MEJIRO_RUST_DIR)/src/lib.rs $(MEJIRO_ENGINE_DIR)/Cargo.toml $(wildcard $(MEJIRO_ENGINE_DIR)/src/*.rs)
	cargo build --locked --release --target thumbv6m-none-eabi --manifest-path $(MEJIRO_RUST_DIR)/Cargo.toml
EXTRALDFLAGS += $(MEJIRO_RUST_LIB)
$(BUILD_DIR)/$(TARGET).elf: $(MEJIRO_RUST_LIB)
