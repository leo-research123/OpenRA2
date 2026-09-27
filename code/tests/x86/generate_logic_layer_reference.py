#!/usr/bin/env python3
"""Fixed YR Layer Add/Sort, real comparator and Object GetYSort instructions; preallocated vectors."""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX, UC_X86_REG_EIP

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"

def generate(exe, output, report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(exe)); cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    layer, items, vt, objects, stack, stop = 0x1000000, 0x1001000, 0x1002000, 0x1004000, 0x1018000, 0x101F000
    cpu.mem_map(layer, 0x20000)
    def put(a, v): cpu.mem_write(a, struct.pack('<I', v & 0xFFFFFFFF))
    def get(a): return struct.unpack('<I', cpu.mem_read(a, 4))[0]
    def call(a, this, *args):
        cpu.mem_write(stack, struct.pack('<' + 'I' * (len(args) + 1), stop, *args))
        cpu.reg_write(UC_X86_REG_ESP, stack); cpu.reg_write(UC_X86_REG_ECX, this)
        cpu.emu_start(a, stop, count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop
        assert cpu.reg_read(UC_X86_REG_ESP) == stack + 4 * (len(args) + 1)
    # Object GetYSort makes two GetRenderCoords calls at slot 0xAC.
    put(vt + 0xB8, 0x5F6BD0)
    # Minimal GetRenderCoords fixture copies Location to the caller's CoordStruct.
    # GetYSort itself and comparator remain original machine code.
    stub = vt + 0x400
    cpu.mem_write(stub, bytes.fromhex('8B4424048B91A000000089108B91A40000008950048B91A8000000895008C20400'))
    put(vt + 0xAC, stub)
    rng = random.Random(0x551A30); lines = []
    for case in range(96):
        size, sorted_insert, passes = case % 12, case % 2, (case // 2) % 4
        keys = [rng.randrange(-5, 6) for _ in range(size)]
        cpu.mem_write(layer, bytes(0x18)); put(layer + 4, items); put(layer + 8, 32)
        for i, key in enumerate(keys):
            obj = objects + i * 0x100; cpu.mem_write(obj, bytes(0x100)); put(obj, vt)
            put(obj + 0xA0, key); put(obj + 0xA4, 0)
            call(0x5519B0, layer, obj, sorted_insert)
        for _ in range(passes): call(0x551A30, layer)
        assert get(layer + 0x10) == size
        order = [(get(items + i * 4) - objects) // 0x100 for i in range(size)]
        lines.append(' '.join(map(str, [size, sorted_insert, passes, *keys, *order])))
    output.write_text('\n'.join(lines) + '\n')
    report.write_text(json.dumps({'exe_sha256': SHA, 'cases': len(lines), 'scope': __doc__,
                                 'fixture_sha256': hashlib.sha256(output.read_bytes()).hexdigest()}, indent=2) + '\n')
    print(f'{len(lines)} original Layer cases')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ('exe', 'output', 'report'): parser.add_argument('--' + arg, type=Path, required=True)
    args = parser.parse_args(); generate(args.exe, args.output, args.report)
