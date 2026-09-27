#!/usr/bin/env python3
"""Generate resource geometry fixtures by executing gamemd 1.001 instructions.

Requires tests/x86/requirements.txt and the user's matching executable. No original
code is copied into the fixtures. No allocator, INI, renderer or game services are
mocked: these operations only touch the explicitly initialized data below.
"""
import argparse
import hashlib
import struct
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"


def generate(executable, output):
    assert hashlib.sha256(executable.read_bytes()).hexdigest() == SHA
    exe = pefile.PE(str(executable))
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = exe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (exe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    cpu.mem_write(base, exe.get_memory_mapped_image())
    obj, xy, stack, stop, cells = 0x1000000, 0x1001000, 0x1018000, 0x101f000, 0x1200000
    cpu.mem_map(obj, 0x20000)
    cpu.mem_map(cells, 0x100000)
    cpu.mem_write(cells, struct.pack('<262144I', *range(1, 262145)))
    cpu.reg_write(UC_X86_REG_FPCW, 0x27f)

    def call(entry, args=()):
        cpu.mem_write(stack, struct.pack('<' + 'I' * (len(args) + 1), stop, *args))
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.reg_write(UC_X86_REG_ECX, obj)
        cpu.emu_start(entry, stop, count=20000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop
        assert cpu.reg_read(UC_X86_REG_ESP) == stack + 4 * (len(args) + 1)
        value = cpu.reg_read(UC_X86_REG_EAX)
        return value if value < 0x80000000 else value - 0x100000000

    # Fixed target geometry (Fundamentals.h / original startup): 104 leptons per level.
    cpu.mem_write(0x89e7c0, struct.pack('<I', 104))
    points = [(0, 0), (0, 255), (255, 0), (255, 255), (128, 128), (-1, -257), (511, 511), (17, 219)]
    floor = []
    for level in [-1, 0, 3, 127]:
        for slope in range(21):
            for x, y in points:
                cpu.mem_write(obj + 0x11b, struct.pack('<bB', level, slope))
                cpu.mem_write(xy, struct.pack('<ii', x, y))
                floor.append((level, slope, x, y, call(0x47b3a0, (xy,))))
    output.mkdir(parents=True, exist_ok=True)
    (output / 'map_floor_reference.txt').write_text(''.join(' '.join(map(str, row)) + '\n' for row in floor))

    iterator = []
    cpu.mem_write(obj + 316, struct.pack('<I', cells))
    for width in [2, 3, 8]:
        cpu.mem_write(obj + 244, struct.pack('<I', width))
        call(0x578350)
        for step in range((2 * width - 1) * 3):
            before = struct.unpack('<I', cpu.mem_read(obj + 280, 4))[0]
            result = call(0x578290)
            x, y, remaining, after = struct.unpack('<iiiI', cpu.mem_read(obj + 268, 16))
            iterator.append((width, step, (before - cells) // 4, result - 1, x, y, remaining, (after - cells) // 4))
    (output / 'map_iterator_reference.txt').write_text(''.join(' '.join(map(str, row)) + '\n' for row in iterator))
    print(f'Generated {len(floor)} floor and {len(iterator)} iterator samples from {SHA}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    generate(args.exe, args.output)
