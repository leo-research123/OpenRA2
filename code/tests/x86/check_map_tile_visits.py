#!/usr/bin/env python3
"""Run YR DrawTiles' candidate traversal, excluding resource lookup and drawing.

The synthetic map_loading fixture has map size 8x12, viewport 200x140 and
camera (-100,215). Its inverse-projected origin is (1408,2261). Run the real
0x006D7604..0x006D782D instructions and real CoordinatesLegal (0x00568300).
Pixel projection, surface clipping and TMP rendering are outside this oracle.
"""
import argparse
import hashlib
import json
import struct

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EBX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EAX

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"


def check(exe):
    with open(exe, "rb") as source:
        assert hashlib.sha256(source.read()).hexdigest() == SHA
    pe = pefile.PE(exe)
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    cpu.mem_map(0x1000000, 0x20000)
    stack, rect = 0x1010000, 0x1001000
    cpu.mem_write(rect, struct.pack("<4i", 0, 0, 200, 140))
    cpu.mem_write(stack + 0x7C, struct.pack("<2i", 1408, 2261))
    cpu.mem_write(0x87F7E8 + 0xF4, struct.pack("<2i", 8, 12))
    cpu.reg_write(UC_X86_REG_EBX, rect)
    cpu.reg_write(UC_X86_REG_ESP, stack)
    visits = []

    def after_legal(uc, address, size, _):
        if uc.reg_read(UC_X86_REG_EAX) & 0xFF:
            visits.append(struct.unpack("<2h", uc.mem_read(stack + 0x10, 4)))
        # Skip only resource lookup / rectangle intersection / pixel drawing.
        # Row/column increments and all loop limits execute original bytes.
        uc.reg_write(UC_X86_REG_EIP, 0x6D7804)

    cpu.hook_add(UC_HOOK_CODE, after_legal, begin=0x6D76E5, end=0x6D76E5)
    cpu.emu_start(0x6D7604, 0x6D782D, count=100000)
    assert cpu.reg_read(UC_X86_REG_EIP) == 0x6D782D
    assert cpu.reg_read(UC_X86_REG_ESP) == stack
    assert len(visits) == len(set(visits)) == 154
    print(json.dumps({"exe_sha256": SHA, "visited": len(visits), "cells": visits}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True)
    check(parser.parse_args().exe)
