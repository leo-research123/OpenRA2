#!/usr/bin/env python3
"""Execute fixed original radar machine code; Surface stubs expose only buffers.

Geometry and RGB sampling results are produced by gamemd.exe, never replacement
code. Fit/frame entry fragments are stopped before unrelated UI rendering.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *

SHA = '7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'output', 'pixels', 'report'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(args.exe))
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    obj, point, coord, rawrect = 0x1000000, 0x1002000, 0x1002100, 0x1002200
    surface, table, buckets = 0x1002300, 0x1002400, 0x1004000
    raw, pixels, stack, stop = 0x1010000, 0x1110000, 0x1190000, 0x119f000
    cpu.mem_map(obj, 0x200000)

    def put(address, *values):
        cpu.mem_write(address, struct.pack('<' + 'I' * len(values), *(v & 0xffffffff for v in values)))

    def read(address, count=1):
        return struct.unpack('<' + 'i' * count, cpu.mem_read(address, count * 4))

    def run(entry, arguments=(), end=stop):
        put(stack, stop, *arguments)
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.reg_write(UC_X86_REG_ECX, obj)
        cpu.reg_write(UC_X86_REG_FPCW, 0xe7f)
        cpu.emu_start(entry, end, count=20000000)
        assert cpu.reg_read(UC_X86_REG_EIP) == end
        if end == stop:
            assert cpu.reg_read(UC_X86_REG_ESP) == stack + 4 * (len(arguments) + 1)

    def fit(width, height):
        put(rawrect, 0, 0, width, height)
        put(stack + 0x28, width)
        cpu.reg_write(UC_X86_REG_EBP, rawrect)
        cpu.reg_write(UC_X86_REG_EBX, obj)
        run(0x6548b3, end=0x654930)
        return read(stack + 0x84)[0], read(stack + 0x10)[0], read(stack + 0x8c)[0]

    rng = random.Random(20260915)
    rows = []
    for width, height in [(2, 2), (8, 8), (79, 97), (160, 147), (362, 108),
                          (160, 160), (140, 108), (200, 240), (1022, 1024)]:
        rows.append(('S', width, height, *fit(width, height)))
    for _ in range(500):
        width, height = rng.randrange(2, 1025), rng.randrange(2, 1025)
        size_x, size_y, factor = fit(width, height)
        if min(size_x, size_y) < 1:
            continue
        rows.append(('S', width, height, size_x, size_y, factor))
        bounds = ((140 - size_x) // 2, (108 - size_y) // 2, size_x, size_y)
        xoffset, origin = rng.randrange(-80, 150), rng.randrange(2, 512)
        put(obj + 0x1488, factor)
        put(obj + 0x1490, xoffset, 0, origin, *bounds)
        world = (rng.randrange(-1000, 132000), rng.randrange(-1000, 132000), rng.randrange(3000))
        put(coord, *world)
        for restrict in (0, 1):
            run(0x6557f0, (point, coord, restrict))
            rows.append(('P', factor, xoffset, origin, *bounds, *world, restrict, *read(point, 2)))
        put(obj + 0x1258, table)
        put(table, buckets)
        mouse = (rng.randrange(-10, 151), rng.randrange(-10, 119))
        put(point, *mouse)
        run(0x656750, (point, coord, coord + 16))
        assert read(coord + 16)[0] == 0
        result = struct.unpack('<hh', cpu.mem_read(coord, 4))
        rows.append(('I', factor, xoffset, origin, *bounds, *mouse, *result))
        cell = (rng.randrange(512), rng.randrange(512))
        viewport = rng.choice([(640, 400), (1280, 720), (1920, 1080), (8192, 8192), (1, 1)])
        cpu.mem_write(coord + 0x24, struct.pack('<hh', *cell))
        put(0x886fa8, *viewport)
        cpu.reg_write(UC_X86_REG_EAX, coord)
        cpu.reg_write(UC_X86_REG_ESI, obj)
        run(0x656f5e, end=0x657134)
        rows.append(('F', factor, xoffset, origin, *bounds, *cell, *viewport, *read(obj + 0x14dc, 4)))

    # Existing BSurface: width, height, lock, unlock are replaced with buffer
    # access only. The complete original 6547C0 resampling loop runs untouched.
    vtable, funcs = 0x1008000, 0x1009000
    cpu.mem_write(funcs, bytes.fromhex('8b4104c3'))
    cpu.mem_write(funcs + 16, bytes.fromhex('8b4108c3'))
    cpu.mem_write(funcs + 32, bytes.fromhex('8b410cc20800'))
    cpu.mem_write(funcs + 48, bytes.fromhex('c3'))
    for offset, fn in [(0x7c, funcs), (0x80, funcs + 16), (0x5c, funcs + 32), (0x60, funcs + 48)]:
        put(vtable + offset, fn)
    put(obj + 0x1220, surface)
    put(obj + 0x123c, raw)
    put(obj + 0x1274, 0x100a000)
    put(0x8a0dd0, 11, 3, 0, 3, 5, 2)  # Original RGB565 Drawing shift globals.
    cases = [(2, 2, 1), (8, 8, 2), (79, 97, 3), (160, 147, 4), (362, 108, 5),
             (160, 160, 6), (140, 108, 7), (200, 240, 8)]
    binary = bytearray(struct.pack('<4sI', b'RDR1', len(cases)))
    for width, height, seed in cases:
        sx, sy, factor = fit(width, height)
        source = bytes(((i * 17 + seed * 31) & 255) for i in range(width * height * 3))
        if seed == 6:
            source = bytes([127, 128, 129]) * (width * height)
        cpu.mem_write(raw, source)
        cpu.mem_write(pixels, bytes(sx * sy * 2))
        put(surface, vtable, sx, sy, pixels)
        put(obj + 0x1240, width, height)
        put(obj + 0x1488, factor)
        run(0x6547c0, (point, rawrect, rawrect, 0))
        assert read(point, 4) == (0, 0, sx, sy)
        result = bytes(cpu.mem_read(pixels, sx * sy * 2))
        binary.extend(struct.pack('<4I', width, height, sx, sy))
        binary.extend(source)
        binary.extend(result)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(''.join(' '.join(map(str, row)) + '\n' for row in rows))
    args.pixels.write_bytes(binary)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'scope': __doc__, 'exe_sha256': SHA,
        'geometry_sha256': hashlib.sha256(args.output.read_bytes()).hexdigest(),
        'pixels_sha256': hashlib.sha256(binary).hexdigest(),
        'geometry_cases': len(rows), 'image_cases': len(cases),
        'entries': ['6548B3..654930', '6557F0', '656750', '656F5E..657134', '6547C0'],
        'surface_stubs': ['GetWidth', 'GetHeight', 'Lock', 'Unlock'],
        'fpcw': '0xE7F', 'format': 'RGB565'}, indent=2) + '\n')
    print(f'Generated {len(rows)} geometry and {len(cases)} image cases')


if __name__ == '__main__':
    main()
