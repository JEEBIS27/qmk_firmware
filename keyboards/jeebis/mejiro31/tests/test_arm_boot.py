#!/usr/bin/env python3
"""Run the linked Rust initializer on Cortex-M, with the ELF's real stack bounds.

Requires pyelftools and unicorn. This checks CPU code and stack use, not USB or
RP2040 peripherals. Run separately from the portable host tests:
    python tests/test_arm_boot.py path/to/firmware.elf
"""
import sys
import struct

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UcError, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_SP, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_PC


def check(path):
    cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    cpu.mem_map(0x10000000, 2 * 1024 * 1024)
    cpu.mem_map(0x20000000, 264 * 1024)
    # Pico SDK's real __aeabi division routines use RP2040's SIO divider.
    # Model just these registers; USB and other peripherals remain unmodelled.
    sio = 0xD0000000
    cpu.mem_map(sio, 4096)
    def divider(uc, access, address, size, value, data):
        if address not in (sio + 0x64, sio + 0x6C):
            return
        dividend = struct.unpack("<I", uc.mem_read(address - 4, 4))[0]
        divisor = value & 0xFFFFFFFF
        if address == sio + 0x6C:
            dividend = dividend if dividend < 0x80000000 else dividend - 0x100000000
            divisor = divisor if divisor < 0x80000000 else divisor - 0x100000000
        quotient = (abs(dividend) // abs(divisor)) * (-1 if (dividend < 0) != (divisor < 0) else 1) if divisor else -1
        remainder = dividend - quotient * divisor
        uc.mem_write(sio + 0x70, struct.pack("<III", quotient & 0xFFFFFFFF, remainder & 0xFFFFFFFF, 3))
    cpu.hook_add(UC_HOOK_MEM_WRITE, divider, begin=sio + 0x60, end=sio + 0x6F)
    with open(path, "rb") as source:
        elf = ELFFile(source)
        symbols = {s.name: s["st_value"] for s in elf.get_section_by_name(".symtab").iter_symbols()}
        for segment in elf.iter_segments():
            if segment["p_type"] == "PT_LOAD" and segment["p_filesz"]:
                cpu.mem_write(segment["p_vaddr"], segment.data())

    bottom = symbols["__process_stack_base__"]
    top = symbols["__process_stack_end__"]
    # Leave space for main -> keyboard_init -> keyboard_post_init_user and its
    # callers. The previous initializer exceeds even the entire empty stack.
    initial = top - 512
    lowest = initial
    returned = False
    stop = 0x101FFF00

    def instruction(uc, address, size, data):
        nonlocal lowest, returned
        lowest = min(lowest, uc.reg_read(UC_ARM_REG_SP))
        if lowest < bottom:
            raise AssertionError(f"stack overflow at 0x{address:08x}: {top - lowest} > {top - bottom} bytes")
        if address == stop:
            returned = True
            uc.emu_stop()

    cpu.hook_add(UC_HOOK_CODE, instruction)
    def invoke(name, *args):
        nonlocal returned
        returned = False
        cpu.reg_write(UC_ARM_REG_SP, initial)
        cpu.reg_write(UC_ARM_REG_LR, stop | 1)
        for register, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2), args):
            cpu.reg_write(register, value)
        try:
            cpu.emu_start(symbols[name] | 1, stop + 2, count=2_000_000)
        except UcError as error:
            raise AssertionError(f"{name} faulted at 0x{cpu.reg_read(UC_ARM_REG_PC):08x}, SP=0x{cpu.reg_read(UC_ARM_REG_SP):08x}: {error}") from error
        assert returned, f"{name} did not return (panic or instruction limit)"
        return cpu.reg_read(UC_ARM_REG_R0)

    invoke("mejiro_qmk_init")
    # Test buffers in SRAM5, outside core 1's 1 KiB exception stack and boot
    # mailbox. Translation uses the SIO divider model; USB is not executed.
    stroke, event = 0x20041800, 0x20041400
    cpu.mem_write(stroke, b"#n-*")
    assert invoke("mejiro_qmk_process", stroke, 4, event) == 0
    assert invoke("mejiro_qmk_is_recording", 0) == 1
    invoke("mejiro_qmk_reset")
    assert invoke("mejiro_qmk_is_recording", 0) == 1
    cpu.mem_write(stroke, struct.pack("<HBBBBHBBBB", 6, 7, 1, 1, 0, 6, 7, 1, 2, 0))
    assert invoke("mejiro_qmk_record_external", stroke, 2) == 0
    cpu.mem_write(stroke, b"KA-")
    assert invoke("mejiro_qmk_process", stroke, 3, event) == 0
    cpu.mem_write(stroke, b"#-*")
    assert invoke("mejiro_qmk_process", stroke, 3, event) == 0
    assert invoke("mejiro_qmk_is_recording", 0) == 0
    cpu.mem_write(stroke, b"#n")
    assert invoke("mejiro_qmk_process", stroke, 2, event) == 0
    assert invoke("mejiro_qmk_replayed_external", stroke, 2) == 2
    print(f"{path}: ARM init/mixed recording replay passed, {top - lowest}/{top - bottom} stack bytes including caller reserve")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit("usage: test_arm_boot.py firmware.elf [firmware.elf ...]")
    for firmware in sys.argv[1:]:
        check(firmware)
