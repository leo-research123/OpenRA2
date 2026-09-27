#!/usr/bin/env python3
"""Execute the pinned gamemd LCW/LZO encoders; store input/output byte pairs.

No codec instructions are mocked. LZO's supplied dictionary is cleared to make
allocation history irrelevant. LCW's one-byte overread bug is excluded from
reference generation and tested separately as a documented host correction.
"""
import argparse
import hashlib
import random
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"


def generate(executable, output):
    assert hashlib.sha256(executable.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(executable))
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    source, dest, work, stack, stop, size_ptr = 0x1000000, 0x1020000, 0x1040000, 0x1080000, 0x1090000, 0x1091000
    cpu.mem_map(source, 0x100000)
    rng = random.Random(552490)
    cases = [bytes(rng.randrange(256) for _ in range(n)) for n in [2, 3, 13, 14, 62, 63, 64, 65, 66, 127, 128, 257, 1024, 8192]]
    cases += [b'A' * n for n in [2, 64, 65, 66, 67, 128, 8192]]
    cases += [(b'ABCD0123456789' * 900)[:n] for n in [64, 8192]]
    cases += [bytes(rng.randrange(8) for _ in range(1024)) for _ in range(3)]
    rows = []
    for codec in ['lcw', 'lzo']:
        for data in cases:
            cpu.mem_write(source, data + bytes(128))
            cpu.mem_write(dest, bytes(0x20000))
            cpu.mem_write(work, bytes(0x10000))
            cpu.reg_write(UC_X86_REG_ECX, source)
            if codec == 'lcw':
                cpu.reg_write(UC_X86_REG_EDX, dest)
                args, entry = [len(data)], 0x551e50
            else:
                cpu.reg_write(UC_X86_REG_EDX, len(data))
                args, entry = [dest, size_ptr, work], 0x55bb90
            cpu.mem_write(stack, struct.pack('<' + 'I' * (len(args) + 1), stop, *args))
            cpu.reg_write(UC_X86_REG_ESP, stack)
            cpu.emu_start(entry, stop, count=200_000_000)
            assert cpu.reg_read(UC_X86_REG_EIP) == stop
            assert cpu.reg_read(UC_X86_REG_ESP) == stack + 4 * (len(args) + 1)
            if codec == 'lcw':
                size = cpu.reg_read(UC_X86_REG_EAX)
            else:
                assert cpu.reg_read(UC_X86_REG_EAX) == 0
                size = struct.unpack('<I', cpu.mem_read(size_ptr, 4))[0]
            assert 0 < size < 0xffff
            rows.append(f'{codec} {data.hex()} {bytes(cpu.mem_read(dest, size)).hex()}\n')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(''.join(rows))
    print(f'Generated {len(rows)} original codec samples from {SHA}')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    generate(a.exe, a.output)
