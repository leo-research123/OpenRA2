#!/usr/bin/env python3
"""Execute fixed YR startup tables and Tactical projection, without an EXE hook.

The fixtures contain outputs, not original code. The client fixture provides
only camera fields read by 6D2140; it does not certify Tactical construction.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"


def generate(executable, output, report):
    if hashlib.sha256(executable.read_bytes()).hexdigest() != SHA:
        raise ValueError("Expected the fixed gamemd executable")
    exe = pefile.PE(str(executable))
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = exe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (exe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    cpu.mem_write(base, exe.get_memory_mapped_image())
    obj, coord, point, stack, stop = 0x1000000, 0x1002000, 0x1003000, 0x1018000, 0x101F000
    cpu.mem_map(obj, 0x20000)

    def call(entry, args=(), ecx=obj):
        cpu.mem_write(stack, struct.pack('<' + 'I' * (1 + len(args)), stop, *args))
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.reg_write(UC_X86_REG_ECX, ecx & 0xFFFFFFFF)
        cpu.emu_start(entry, stop, count=500000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop
        assert cpu.reg_read(UC_X86_REG_ESP) == stack + 4 * (1 + len(args))
        return cpu.reg_read(UC_X86_REG_EAX)

    startup = []
    for control in (0x27F, 0x37F, 0xE7F):
        cpu.reg_write(UC_X86_REG_FPCW, control)
        for entry in (0x6D1830, 0x6D18C0, 0x6D1BB0):
            call(entry)
        bits = struct.unpack('<Q', cpu.mem_read(0xB0CD48, 8))[0]
        assert bits == 0x3FC25E5374344960
        startup.append({'input_fpcw': hex(control), 'height_scale_bits': hex(bits),
                        'output_fpcw': hex(cpu.reg_read(UC_X86_REG_FPCW))})

    def signed(value):
        return value if value < 0x80000000 else value - 0x100000000

    rng = random.Random(20260914)
    heights = sorted(set(list(range(-32768, 32769, 31)) +
        [-2147483648, 2147483647, -104, -1, 0, 1, 104, 727, 728, 729] +
        [rng.randint(-2147483648, 2147483647) for _ in range(512)]))
    rows = []
    for height in heights:
        rows.append(('H', height, signed(call(0x6D20E0, ecx=height))))
    values = [-2147483648, -65536, -257, -256, -1, 0, 1, 128, 255, 256, 511, 131071, 2147483647]
    coordinates = [(x, y, z) for x in values for y in values for z in [-104, 0, 727, 728, 13208]]
    coordinates += [tuple(rng.randint(-2147483648, 2147483647) for _ in range(3)) for _ in range(512)]
    for xyz in coordinates:
        cpu.mem_write(coord, struct.pack('<3i', *xyz))
        assert call(0x6D1F10, (point, coord)) == point
        xy = struct.unpack('<2i', cpu.mem_read(point, 8))
        rows.append(('P', *xyz, *xy))
    clients = []
    for _ in range(512):
        xyz = (rng.randrange(-2048, 131072), rng.randrange(-2048, 131072), rng.randrange(-104, 13521))
        camera = (rng.randrange(-16000, 16000), rng.randrange(-4000, 16000))
        bounds = rng.choice([(640, 400), (1280, 720), (1920, 1080), (1, 1), (-360, -180), (2147483647, 2147483647)])
        clients.append((xyz, camera, bounds))
    # Exact expanded culling edges in pixels (coordinates are projected first).
    for width, height in [(640, 400), (1280, 720)]:
        for x in [-361, -360, width + 360, width + 361]:
            for y in [-181, -180, height + 180, height + 181]:
                clients.append(((0, 0, 0), (-x, -y), (width, height)))
    for xyz, camera, bounds in clients:
        cpu.mem_write(coord, struct.pack('<3i', *xyz))
        cpu.mem_write(obj + 0xB0, struct.pack('<2i', *camera))
        cpu.mem_write(0xB0CE28, struct.pack('<4i', 37, 53, *bounds))
        visible = call(0x6D2140, (coord, point)) & 0xFF
        xy = struct.unpack('<2i', cpu.mem_read(point, 8))
        rows.append(('C', *xyz, *camera, *bounds, *xy, visible))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(''.join(' '.join(map(str, row)) + '\n' for row in rows))
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps({'exe_sha256': SHA, 'startup': startup,
        'entries': ['6D1830', '6D18C0', '6D1BB0', '6D20E0', '6D1F10', '6D2140'],
        'samples': {kind: sum(row[0] == kind for row in rows) for kind in ('H', 'P', 'C')},
        'scope': __doc__, 'fixture_sha256': hashlib.sha256(output.read_bytes()).hexdigest()}, indent=2) + '\n')
    print(f'Generated {len(rows)} original projection results')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    generate(args.exe, args.output, args.report)
