#!/usr/bin/env python3
"""Execute fixed YR Warhead CRC instructions against field fixtures (not a live process)."""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_EAX

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"

def generate(exe, output, report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(exe)); cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    obj, crc, stack, stop = 0x1000000, 0x1002000, 0x1018000, 0x101F000
    cpu.mem_map(obj, 0x20000)
    def put(a, v): cpu.mem_write(a, struct.pack('<I', v & 0xFFFFFFFF))
    def get(a): return struct.unpack('<I', cpu.mem_read(a, 4))[0]
    def real(a, v): cpu.mem_write(a, struct.pack('<d', v))
    def call(a, this, *args):
        cpu.mem_write(stack, struct.pack('<' + 'I' * (len(args) + 1), stop, *args))
        cpu.reg_write(UC_X86_REG_ESP, stack); cpu.reg_write(UC_X86_REG_ECX, this)
        cpu.emu_start(a, stop, count=1000000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop
        assert cpu.reg_read(UC_X86_REG_ESP) == stack + 4 * (len(args) + 1)
        return cpu.reg_read(UC_X86_REG_EAX)
    rng = random.Random(0x75DEC0); lines = []
    for case in range(64):
        cpu.mem_write(obj, bytes(0x1D0)); cpu.mem_write(crc, bytes(16))
        cpu.mem_write(obj + 0x24, b'P2_WH_CRC\0'); cpu.mem_write(obj + 0x64, b'P2_WH_CRC\0')
        ident, mask = rng.getrandbits(32), rng.getrandbits(16)
        put(obj + 0x10, ident); cpu.mem_write(obj + 0x20, bytes([mask >> 15]))
        for i, off in enumerate((0x144, 0x147, 0x148, 0x149, 0x14A, 0x14B, 0x14C, 0x14D,
                                0x14E, 0x14F, 0x150, 0x151, 0x152, 0x153, 0x17B)):
            cpu.mem_write(obj + off, bytes([(mask >> i) & 1]))
        deform, threshold, prone = rng.randrange(-99, 99) / 8, rng.randrange(-99, 99), rng.randrange(-99, 99) / 8
        shake = [rng.randrange(-100, 100) for _ in range(4)]
        real(obj + 0x98, deform); put(obj + 0x100, threshold); real(obj + 0xF8, prone)
        for off, val in zip((0x17C, 0x180, 0x184, 0x188), shake): put(obj + off, val)
        guid = bytes(rng.randrange(256) for _ in range(16)); cpu.mem_write(obj + 0x15C, guid)
        verses = [rng.randrange(-99, 99) / 8 for _ in range(11)]
        for i, verse in enumerate(verses): real(obj + 0xA0 + i * 8, verse)
        count, death = rng.randrange(10), rng.randrange(-10, 10)
        put(obj + 0x114, count); put(obj + 0x120, death)
        call(0x75DEC0, obj, crc)
        state = [get(crc + n) for n in (0, 4, 8)]
        value = call(0x4A1DE0, crc, 0, 0)
        lines.append(' '.join(map(str, [ident, mask, deform, threshold, prone, *shake, *guid, *verses,
                                        count, death, *state, value])))
    output.write_text('\n'.join(lines) + '\n')
    report.write_text(json.dumps({'exe_sha256': SHA, 'cases': len(lines), 'scope': __doc__,
                                 'fixture_sha256': hashlib.sha256(output.read_bytes()).hexdigest()}, indent=2) + '\n')
    print(f'{len(lines)} original Warhead CRC cases')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ('exe', 'output', 'report'): parser.add_argument('--' + arg, type=Path, required=True)
    args = parser.parse_args(); generate(args.exe, args.output, args.report)
