#!/usr/bin/env python3
"""YR 0x00518D80 normal Infantry frame branch; real Facing, explicit type/sequence and visibility fixture."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX

SHA = '7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def generate(exe, output, report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(exe)); cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    obj, typ, seq, vt, stub, stack, stop = [0x1000000 + n for n in (0, 0x2000, 0x4000, 0x6000, 0x8000, 0x18000, 0x1F000)]
    cpu.mem_map(obj, 0x20000)
    def put(a, v): cpu.mem_write(a, struct.pack('<I', v & 0xFFFFFFFF))
    put(obj, vt); put(obj + 0x6C0, typ); put(typ + 0xE3C, seq)
    cpu.mem_write(stub, bytes.fromhex('B001C20400')); put(vt + 0x440, stub)
    put(0xA8ED84, 0)
    rows = []
    for action in range(42):
        start, count, stride = action * 100, action % 7 + 1, (action % 3) * 9
        put(seq + action * 36, start); put(seq + action * 36 + 4, count); put(seq + action * 36 + 8, stride)
        put(obj + 0x6C4, action)
        for direction in range(8):
            raw = direction * 0x2000
            put(obj + 0x388, raw)  # Facing.Desired, zero ROT disables interpolation
            for phase in (0, 1, 19):
                put(obj + 0xF8, phase)
                put(stack, stop); cpu.reg_write(UC_X86_REG_ESP, stack); cpu.reg_write(UC_X86_REG_ECX, obj)
                cpu.emu_start(0x518D80, stop, count=100000)
                assert cpu.reg_read(UC_X86_REG_EIP) == stop and cpu.reg_read(UC_X86_REG_ESP) == stack + 4
                value = cpu.reg_read(UC_X86_REG_EAX)
                rows.append(' '.join(map(str, (action, start, count, stride, raw, phase, value))))
    output.write_text('\n'.join(rows) + '\n')
    report.write_text(json.dumps({'exe_sha256': SHA, 'cases': len(rows), 'scope': __doc__,
        'fixture_sha256': hashlib.sha256(output.read_bytes()).hexdigest()}, indent=2) + '\n')
    print(f'{len(rows)} original Infantry frame cases')
if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for arg in ('exe', 'output', 'report'): p.add_argument('--' + arg, type=Path, required=True)
    a = p.parse_args(); generate(a.exe, a.output, a.report)
