#!/usr/bin/env python3
"""Execute original radar bounds, height-aware usable area and ground picking.

Cells are data fixtures with original x86 layout. Execute 654490's complete
567230 parent range clamp and redraw call with an empty Techno registry. No
bounds, usable area, iterator, inverse matrix or picking algorithm is replaced.
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
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'output', 'report'):
        p.add_argument('--' + name, type=Path, required=True)
    args = p.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(args.exe))
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    tactical, point, result, stack, stop = 0x1000000, 0x1002000, 0x1002100, 0x1018000, 0x101f000
    slots, storage, map_obj = 0x1100000, 0x1200000, 0x87f7e8
    cpu.mem_map(tactical, 0x2000000)

    def put(address, *values):
        cpu.mem_write(address, struct.pack('<' + 'I' * len(values), *(v & 0xffffffff for v in values)))

    def call(entry, obj, args=()):
        put(stack, stop, *args)
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.reg_write(UC_X86_REG_ECX, obj)
        cpu.reg_write(UC_X86_REG_FPCW, 0xe7f)
        cpu.emu_start(entry, stop, count=20000000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop
        assert cpu.reg_read(UC_X86_REG_ESP) == stack + 4 * (len(args) + 1)
        return cpu.reg_read(UC_X86_REG_EAX)

    put(0xA8EC88,0) # This geometry fixture has no Techno objects.
    put(0x887324,0) # No live Tactical drawing singleton in the bound setup.
    cpu.mem_write(0xabdc50, bytes(0x148))
    put(tactical + 0xde4, 0x408888ce, 0x410888ce, 0, 0, 0xc08888ce, 0x410888ce, 0, 0, 0, 0, 0x3f800000, 0)
    rng = random.Random(38109)
    rows = []
    for width, height, visible in [(8, 12, (2, 2, 4, 4)), (64, 60, (2, 3, 60, 52)), (84, 80, (3, 2, 77, 72))]:
        for pattern in (0, 1, 2):
            rows.append(('E', width, height, *visible, pattern))
            cpu.mem_write(slots, bytes(262144 * 4))
            put(map_obj + 316, slots)
            put(map_obj + 236, 0, 0, width, height, *visible)
            index = 0
            coordinates = []
            for y in range(width + height):
                for x in range(width + height):
                    if x + y <= width or abs(x - y) >= width or x + y > width + 2 * height:
                        continue
                    level = 0 if pattern == 0 else 3 if pattern == 1 else (x * 7 + y * 3) % 14
                    slope = 2 if pattern == 2 and (x + y) % 3 == 0 else 0
                    cell = storage + index * 0x148
                    cpu.mem_write(cell, bytes(0x148))
                    cpu.mem_write(cell + 0x24, struct.pack('<hh', x, y))
                    cpu.mem_write(cell + 0x11b, bytes([level, slope]))
                    put(slots + 4 * (x + y * 512), cell)
                    coordinates.append((x, y))
                    index += 1
            put(point,*visible)
            call(0x654490, map_obj, (point,))
            bounds = struct.unpack('<7i', cpu.mem_read(map_obj + 0x1490, 28))
            rows.append(('B', *bounds))
            for x, y in rng.sample(coordinates, min(len(coordinates), 80)) + [(-1, 0), (0, 0), (511, 511)]:
                cpu.mem_write(point, struct.pack('<hh', x, y))
                for check_level in (0, 1):
                    value = call(0x578460, map_obj, (point, check_level)) & 255
                    rows.append(('U', x, y, check_level, value))
            for _ in range(120):
                camera = (rng.randrange(-1000, 1000), rng.randrange(0, (width + height) * 30))
                viewport = rng.choice([(0, 0, 1280, 720), (13, 37, 640, 400)])
                mouse = (rng.randrange(viewport[2]), rng.randrange(viewport[3]))
                put(tactical + 0xb0, *camera)
                put(0x886fa0, *viewport)
                put(point, *mouse)
                call(0x6d6590, tactical, (result, point))
                picked = struct.unpack('<hh', cpu.mem_read(result, 4))
                rows.append(('T', *camera, *viewport, *mouse, *picked))
    args.output.write_text(''.join(' '.join(map(str, row)) + '\n' for row in rows))
    args.report.write_text(json.dumps({'scope': __doc__, 'exe_sha256': SHA,
        'fixture_sha256': hashlib.sha256(args.output.read_bytes()).hexdigest(),
        'cases': {k: sum(row[0] == k for row in rows) for k in 'EBUT'},
        'entries': ['654490', '567230', '421B60', '4F42F0', '578AC0', '578350', '578290', '578460', '6D6590', '5AFB80', '5657A0']}, indent=2) + '\n')
    print('Generated', len(rows), 'original radar world cases')


if __name__ == '__main__':
    main()
