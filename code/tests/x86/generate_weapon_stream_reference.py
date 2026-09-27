#!/usr/bin/env python3
"""Fixed YR Weapon Save and Abstract Save instructions; only IStream Write is intercepted."""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"
def generate(exe, output, report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(exe)); cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    obj, vt, stream, svt, lists, write, stack, stop = [0x1000000 + n for n in
        (0, 0x2000, 0x3000, 0x4000, 0x5000, 0x10000, 0x18000, 0x1F000)]
    cpu.mem_map(obj, 0x20000)
    def put(a, v): cpu.mem_write(a, struct.pack('<I', v & 0xFFFFFFFF))
    def get(a): return struct.unpack('<I', cpu.mem_read(a, 4))[0]
    captured = bytearray(); sizes = []; fail_call = 0
    def intercept(uc, address, size, user):
        if address != write: return
        sp = uc.reg_read(UC_X86_REG_ESP)
        dest, source, count, transferred = (get(sp + n) for n in (4, 8, 12, 16))
        assert dest == stream
        sizes.append(count)
        failed = len(sizes) == fail_call
        if not failed: captured.extend(uc.mem_read(source, count))
        if transferred: put(transferred, 0 if failed else count)
        uc.reg_write(UC_X86_REG_EAX, 0x80004005 if failed else 0)
        uc.reg_write(UC_X86_REG_ESP, sp + 20); uc.reg_write(UC_X86_REG_EIP, get(sp))
    cpu.hook_add(UC_HOOK_CODE, intercept)
    put(vt + 0x30, 0x7730D0); put(stream, svt); put(svt + 0x10, write)
    lines = []
    for anim, report_count, down in itertools.product((0, 1, 2, 5), repeat=3):
        cpu.mem_write(obj, bytes(0x160)); put(obj, vt); cpu.mem_write(obj + 0x20, b'\1')
        for base_off, count, array in ((0xF4, anim, lists), (0xBC, report_count, lists + 0x100), (0xD8, down, lists + 0x200)):
            put(obj + base_off + 4, array); put(obj + base_off + 16, count)
        for i in range(anim): put(lists + i * 4, (0x200, 0x201, 0)[i % 3])
        for i in range(report_count): put(lists + 0x100 + i * 4, 17 - 9 * i)
        for i in range(down): put(lists + 0x200 + i * 4, 100 + i)
        captured.clear(); sizes.clear()
        cpu.mem_write(stack, struct.pack('<IIII', stop, obj, stream, 1))
        cpu.reg_write(UC_X86_REG_ESP, stack); cpu.emu_start(0x772EB0, stop, count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop
        assert cpu.reg_read(UC_X86_REG_ESP) == stack + 16
        assert cpu.reg_read(UC_X86_REG_EAX) == 0
        assert sizes == [4, 0x160] + [4] * (3 + anim + report_count + down)
        tail = list(struct.unpack('<' + 'I' * ((len(captured) - 0x164) // 4), captured[0x164:]))
        for i in range(anim): tail[1 + i] = {0: 0, 0x200: 1, 0x201: 2}[tail[1 + i]]
        dirty = cpu.mem_read(obj + 0x20, 1)[0]
        lines.append(' '.join(map(str, [anim, report_count, down, len(captured), dirty, *tail])))
    output.write_text('\n'.join(lines) + '\n')
    report.write_text(json.dumps({'exe_sha256': SHA, 'cases': len(lines), 'scope': __doc__,
        'normalization': 'Only animation pointer tokens in the external tail. Raw base body not compared.',
        'fixture_sha256': hashlib.sha256(output.read_bytes()).hexdigest()}, indent=2) + '\n')
    print(f'{len(lines)} original Weapon Save cases')
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ('exe', 'output', 'report'): parser.add_argument('--' + arg, type=Path, required=True)
    args = parser.parse_args(); generate(args.exe, args.output, args.report)
