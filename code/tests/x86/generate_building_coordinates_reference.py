#!/usr/bin/env python3
"""Execute building coordinate virtual entries and projection in fixed YR code.

Original Building vtable selects GetCoords (+0x48), forwarding GetCenterCoords
(+0x58), GetTargetCoords (+0xA4) and GetRenderCoords (+0xAC). Location, Type,
Foundation and TargetCoordOffset fields are supplied. No drawing, placement,
game simulation or original-window pixel equivalence is claimed by this fixture.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"


def generate(executable, output, report):
    if hashlib.sha256(executable.read_bytes()).hexdigest() != SHA:
        raise ValueError("Expected the fixed gamemd executable")
    exe = pefile.PE(str(executable))
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = exe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (exe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    cpu.mem_write(base, exe.get_memory_mapped_image())
    obj, typ, render, center, point = 0x1000000, 0x1002000, 0x1004000, 0x1005000, 0x1006000
    stack, stop = 0x1018000, 0x101F000
    cpu.mem_map(obj, 0x20000)
    vtable = 0x7E3EBC
    def entry(slot):
        return struct.unpack('<I', cpu.mem_read(vtable + slot, 4))[0]
    assert [entry(slot) for slot in (0x48, 0x58, 0xA4, 0xAC)] == [0x447AC0, 0x410540, 0x4500A0, 0x459EF0]
    cpu.mem_write(obj, struct.pack('<I', vtable))
    cpu.mem_write(typ + 0xEBC, struct.pack('<3i', 15, -31, 64))
    forwarded, target = 0x1007000, 0x1008000

    def call(entry, args=(), ecx=obj):
        cpu.mem_write(stack, struct.pack('<' + 'I' * (1 + len(args)), stop, *args))
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.reg_write(UC_X86_REG_ECX, ecx)
        cpu.emu_start(entry, stop, count=500000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop
        assert cpu.reg_read(UC_X86_REG_ESP) == stack + 4 * (1 + len(args))
        return cpu.reg_read(UC_X86_REG_EAX)

    for initializer in (0x6D1830, 0x6D18C0, 0x6D1BB0):
        call(initializer)
    cpu.mem_write(obj + 0x520, struct.pack('<I', typ))
    rows = []
    for foundation in range(22):
        cpu.mem_write(typ + 0xEF0, struct.pack('<I', foundation))
        for x, y in ((128, 128), (8 * 256 + 128, 3 * 256 + 128),
                     (81 * 256 + 128, 74 * 256 + 128), (-129, 511)):
            for height in (-104, -1, 0, 1, 104, 624, 727, 728, 729, 832, 1248, 1560):
                location = (x, y, height)
                cpu.mem_write(obj + 0x9C, struct.pack('<3i', *location))
                assert call(entry(0xAC), (render,)) == render
                assert call(entry(0x48), (center,)) == center
                assert call(entry(0x58), (forwarded,)) == forwarded
                assert cpu.mem_read(forwarded, 12) == cpu.mem_read(center, 12)
                assert call(entry(0xA4), (target,)) == target
                assert call(0x6D1F10, (point, render)) == point
                rows.append((foundation, *location,
                    *struct.unpack('<3i', cpu.mem_read(render, 12)),
                    *struct.unpack('<3i', cpu.mem_read(center, 12)),
                    *struct.unpack('<2i', cpu.mem_read(point, 8)),
                    *struct.unpack('<3i', cpu.mem_read(target, 12))))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(''.join(' '.join(map(str, row)) + '\n' for row in rows))
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps({'exe_sha256': SHA, 'samples': len(rows),
        'entries': ['0x00459EF0', '0x00447AC0', '0x00410540', '0x004500A0', '0x006D1F10'],
        'vtable': '0x007E3EBC', 'slots': ['0x48', '0x58', '0xA4', '0xAC'],
        'columns': 'foundation location_xyz render_xyz coords_xyz screen_xy target_xyz',
        'scope': __doc__, 'fixture_sha256': hashlib.sha256(output.read_bytes()).hexdigest()}, indent=2) + '\n')
    print(f'Generated {len(rows)} original building coordinate results')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    generate(args.exe, args.output, args.report)
