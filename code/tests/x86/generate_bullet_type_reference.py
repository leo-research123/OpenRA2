#!/usr/bin/env python3
"""Fixed YR BulletType CRC instructions, explicit fields and real base CRC; not a live process."""
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
    def call(a, this, *args):
        cpu.mem_write(stack, struct.pack('<' + 'I' * (len(args) + 1), stop, *args))
        cpu.reg_write(UC_X86_REG_ESP, stack); cpu.reg_write(UC_X86_REG_ECX, this)
        cpu.emu_start(a, stop, count=1000000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop
        assert cpu.reg_read(UC_X86_REG_ESP) == stack + 4 * (len(args) + 1)
        return cpu.reg_read(UC_X86_REG_EAX)
    rng = random.Random(0x46C560); lines = []
    for case in range(64):
        cpu.mem_write(obj, bytes(0x2F8)); cpu.mem_write(crc, bytes(16))
        cpu.mem_write(obj + 0x24, b'P2_BCRC\0'); cpu.mem_write(obj + 0x64, b'P2_BCRC\0')
        ident, mask = rng.getrandbits(32), rng.getrandbits(17)
        put(obj + 0x10, ident); cpu.mem_write(obj + 0x20, bytes([mask >> 16]))
        for i, off in enumerate((0x294, 0x29A, 0x29B, 0x29C, 0x29D, 0x29E, 0x29F, 0x2A0,
                                0x2A1, 0x2A2, 0x2A3, 0x2A4, 0x2A5, 0x2A6, 0x2A7, 0x2F7)):
            cpu.mem_write(obj + off, bytes([(mask >> i) & 1]))
        elasticity = rng.randrange(-99, 99) / 8
        ints = [rng.randrange(-1000, 1000) for _ in range(3)]
        cpu.mem_write(obj + 0x2C8, struct.pack('<d', elasticity))
        for off, val in zip((0x2D0, 0x2DC, 0x2F0), ints): put(obj + off, val)
        call(0x46C560, obj, crc)
        state = [get(crc + n) for n in (0, 4, 8)]
        value = call(0x4A1DE0, crc, 0, 0)
        lines.append(' '.join(map(str, [ident, mask, elasticity, *ints, *state, value])))
    output.write_text('\n'.join(lines) + '\n')
    report.write_text(json.dumps({'exe_sha256': SHA, 'cases': len(lines), 'scope': __doc__,
                                 'fixture_sha256': hashlib.sha256(output.read_bytes()).hexdigest()}, indent=2) + '\n')
    print(f'{len(lines)} original Bullet CRC cases')
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ('exe', 'output', 'report'): parser.add_argument('--' + arg, type=Path, required=True)
    args = parser.parse_args(); generate(args.exe, args.output, args.report)
