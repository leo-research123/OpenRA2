#!/usr/bin/env python3
"""Execute original camera clamp and center publication. No replacement code runs here.

6D6000 is run with the original editor clamp bypass; 6D8640 is tested separately.
Inputs provide only the original map/viewport fields and calibrated inverse matrix.
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

SHA = '7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'output', 'report'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(args.exe))
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    obj, point, stack, stop = 0x1000000, 0x1004000, 0x1018000, 0x101f000
    cpu.mem_map(obj, 0x20000)

    def call(entry, arguments):
        cpu.mem_write(stack, struct.pack('<' + 'I' * (len(arguments) + 1), stop, *arguments))
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.reg_write(UC_X86_REG_ECX, obj)
        cpu.reg_write(UC_X86_REG_FPCW, 0xe7f)
        cpu.emu_start(entry, stop, count=10000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop
        assert cpu.reg_read(UC_X86_REG_ESP) == stack + 4 * (len(arguments) + 1)
        return cpu.reg_read(UC_X86_REG_EAX)

    def put(address, *values): cpu.mem_write(address, struct.pack('<' + 'i' * len(values), *values))
    def read(address, count): return struct.unpack('<' + 'i' * count, cpu.mem_read(address, count * 4))
    rng = random.Random(20260915)
    rows = []
    for map_width, visible in [(84, (2, 2, 80, 70)), (50, (4, 8, 40, 20)), (8, (2, 2, 4, 4))]:
        for width, height in [(1, 1), (640, 400), (1280, 720), (1920, 1080), (8192, 8192)]:
            put(0x87f8dc, map_width)
            put(0x87f8e4, *visible)
            put(0x886fa8, width, height)
            # Query both extremes, then probe all returned boundary neighbors.
            bounds = []
            for position in [(-100000, -100000), (100000, 100000)]:
                put(point, *position)
                call(0x6d8640, (point,))
                bounds.append(read(point, 2))
            xs = [bounds[0][0] + d for d in (-1, 0, 1)] + [bounds[1][0] + d for d in (-1, 0, 1)]
            ys = [bounds[0][1] + d for d in (-1, 0, 1)] + [bounds[1][1] + d for d in (-1, 0, 1)]
            for x in xs:
                for y in ys:
                    put(point, x, y)
                    changed = call(0x6d8640, (point,)) & 255
                    rows.append(('C', map_width, *visible, width, height, x, y, *read(point, 2), changed))
    matrix = [0x408888ce, 0x410888ce, 0, 0, 0xc08888ce, 0x410888ce, 0, 0, 0, 0, 0x3f800000, 0]
    cpu.mem_write(obj + 0xde4, struct.pack('<12I', *matrix))
    cpu.mem_write(0xa8ed6b, b'\x01')
    for _ in range(512):
        width, height = rng.choice([(640, 400), (1280, 720), (1920, 1080), (1, 1), (8192, 8192)])
        x, y = rng.randrange(-30000, 30000), rng.randrange(-30000, 30000)
        put(0xb0ce28, 0, 0, width, height)
        put(0x886fa8, width, height)
        put(point, x, y)
        call(0x6d6000, (point,))
        rows.append(('V', width, height, x, y, *read(obj + 0xb0, 2), *read(obj + 0xd80, 4)))
        assert read(obj + 0xd64, 2) == (x, y) and read(obj + 0xd74, 2) == (x, y)
        assert cpu.mem_read(obj + 0xd7d, 1) == b'\x01'
    # Original world-coordinate positioning, including clamp bypass. All
    # geometry callees execute from the pinned EXE; there are no output mocks.
    for bypass in (0,1):
        cpu.mem_write(0xa8ed6b,bytes([bypass]))
        cpu.mem_write(0xb0cd48,struct.pack('<Q',0x3fc25e5374344960))
        for _ in range(128):
            map_width,visible=rng.choice([(84,(2,2,80,70)),(50,(4,8,40,20)),(8,(2,2,4,4))])
            width,height=rng.choice([(640,400),(1280,720),(8192,8192)])
            world=(rng.randrange(-8192,60000),rng.randrange(-8192,60000),rng.randrange(-128,2048))
            put(0x87f8dc,map_width);put(0x87f8e4,*visible)
            put(0xb0ce28,0,0,width,height);put(0x886fa8,width,height)
            put(point,*world);call(0x6d6070,(point,))
            rows.append(('T',bypass,map_width,*visible,width,height,*world,
                         *read(obj+0xd64,2),*read(obj+0xb0,2),*read(obj+0xd80,4)))
    # Client -> world uses a distinct invalid sentinel (-1,-1,-1), permits
    # negative pixels and adds the camera before the inverse matrix.
    put(0xb0ce08,-1,-1,-1)
    for camera in ((0,0),(-3000,4500),(17000,-9000)):
        put(obj+0xb0,*camera)
        for viewport in ((0,0,640,400),(37,53,1280,720)):
            put(0xb0ce28,*viewport)
            for x in (-1,0,1,viewport[2]-1,viewport[2]):
                for y in (-1,0,viewport[3]-1,viewport[1]+viewport[3]-1,viewport[1]+viewport[3]):
                    put(point,x,y)
                    call(0x6d2280,(point+32,point))
                    rows.append(('I',*camera,*viewport,x,y,*read(point+32,3)))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(''.join(' '.join(map(str, row)) + '\n' for row in rows))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'scope': __doc__, 'exe_sha256': SHA,
        'fixture_sha256': hashlib.sha256(args.output.read_bytes()).hexdigest(),
        'cases': {kind: sum(row[0] == kind for row in rows) for kind in ('C', 'V','T','I')},
        'entries': ['0x6D8640', '0x6D6000', '0x6D8B30', '0x5AFB80','0x6D6070','0x6D2280']}, indent=2) + '\n')
    print('Generated', len(rows), 'original camera cases')


if __name__ == '__main__': main()
