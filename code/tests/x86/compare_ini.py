#!/usr/bin/env python3
"""Compare native INI objects with real original x86 parser/query/serializer code.

Only input/output streams and CRT allocation/string primitives are substituted.
The INI parser, nodes, lookup, CRC, qsort and serialization execute in the EXE.
This is not a Windows process ABI handoff/integration test.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import random

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_ECX

ROOT = Path(__file__).resolve().parents[3]
SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"
HEAP, STACK, SUPPORT, STOP = 0x2000000, 0x3000000, 0xD00000, 0xD0FFF0
INI, STRAW, PIPE, PVT = SUPPORT + 0x100, SUPPORT + 0x300, SUPPORT + 0x400, SUPPORT + 0x500
OUTPUT, END = SUPPORT + 0x1000, SUPPORT + 0x1010


class Machine:
    def __init__(self, original):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        base = original.OPTIONAL_HEADER.ImageBase
        self.uc.mem_map(base, (original.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
        self.uc.mem_write(base, original.get_memory_mapped_image())
        for addr, size in [(0, 0x10000), (HEAP, 0x1000000), (STACK, 0x100000), (SUPPORT, 0x10000)]:
            self.uc.mem_map(addr, size)
        self.uc.mem_write(STOP, b"\xf4")
        self.cursor = HEAP
        self.input, self.position, self.output = b'', 0, bytearray()
        self.counts = {}
        self.hooks = {0x7C8E17: self.allocate, 0x7C9430: self.allocate,
                      0x7C8B3D: lambda: self.ret(), 0x7C93E8: lambda: self.ret(),
                      0x7D5408: self.duplicate, 0x7CAF30: self.strchr,
                      0x7C91D0: self.strncpy, 0x7D15A0: lambda: self.ret(len(self.string(self.arg(0)))),
                      0x7D75E0: self.memset, 0x7CA090: self.memcpy,
                      0x4A2780: self.get, OUTPUT: self.put, END: lambda: self.ret()}
        for descriptor in getattr(original, 'DIRECTORY_ENTRY_IMPORT', []):
            for item in descriptor.imports:
                name = (item.name or b'ordinal').decode()
                target = SUPPORT + 0x2000 + len(self.hooks) * 16
                self.write32(item.address, target)
                self.hooks[target] = lambda name=name: self.unexpected(name)
        self.write32(PIPE, PVT)
        self.write32(PVT + 8, END); self.write32(PVT + 16, OUTPUT)
        self.uc.hook_add(UC_HOOK_CODE, self.hook)
        self.call(0x535AA0, INI)

    def unexpected(self, name):
        raise RuntimeError(f'Unexpected original import {name} at {self.uc.reg_read(UC_X86_REG_EIP):x}')

    def read32(self, addr): return struct.unpack('<I', self.uc.mem_read(addr, 4))[0]
    def signed32(self, addr): return struct.unpack('<i', self.uc.mem_read(addr, 4))[0]
    def write32(self, addr, value): self.uc.mem_write(addr, struct.pack('<I', value & 0xffffffff))
    def arg(self, index): return self.read32(self.uc.reg_read(UC_X86_REG_ESP) + 4 * (index + 1))
    def string(self, addr):
        if not addr: return b''
        out = bytearray()
        while True:
            c = self.uc.mem_read(addr, 1)[0]
            if not c: return bytes(out)
            out.append(c); addr += 1
            if len(out) > 1000000: raise RuntimeError('Unterminated original string')
    def malloc(self, size):
        addr = self.cursor; self.cursor += (size + 15) & ~15
        if self.cursor >= HEAP + 0x1000000: raise RuntimeError('oracle heap exhausted')
        return addr
    def cstring(self, text):
        addr = self.malloc(len(text) + 1); self.uc.mem_write(addr, text + b'\0'); return addr
    def ret(self, result=0, cleanup=0):
        sp = self.uc.reg_read(UC_X86_REG_ESP)
        target = self.read32(sp)
        self.uc.reg_write(UC_X86_REG_EAX, result & 0xffffffff)
        self.uc.reg_write(UC_X86_REG_ESP, sp + 4 + cleanup)
        self.uc.reg_write(UC_X86_REG_EIP, target)
    def allocate(self): self.ret(self.malloc(self.arg(0)))
    def duplicate(self): self.ret(self.cstring(self.string(self.arg(0))))
    def memset(self):
        self.uc.mem_write(self.arg(0), bytes([self.arg(1) & 255]) * self.arg(2)); self.ret(self.arg(0))
    def memcpy(self):
        self.uc.mem_write(self.arg(0), bytes(self.uc.mem_read(self.arg(1), self.arg(2)))); self.ret(self.arg(0))
    def strchr(self):
        text = self.string(self.arg(0)) + b'\0'; offset = text.find(bytes([self.arg(1) & 255]))
        self.ret(0 if offset < 0 else self.arg(0) + offset)
    def strncpy(self):
        text = self.string(self.arg(1)); count = self.arg(2)
        self.uc.mem_write(self.arg(0), text[:count].ljust(count, b'\0')); self.ret(self.arg(0))
    def get(self):
        count = min(self.arg(1), len(self.input) - self.position)
        if count: self.uc.mem_write(self.arg(0), self.input[self.position:self.position + count])
        self.position += count; self.ret(count, 8)
    def put(self):
        count = self.arg(1)
        self.output.extend(self.uc.mem_read(self.arg(0), count)); self.ret(count, 8)
    def hook(self, uc, address, size, data):
        if address == STOP: uc.emu_stop(); return
        if address in self.hooks: self.hooks[address]()
    def call(self, address, this, *args):
        self.counts[hex(address)] = self.counts.get(hex(address), 0) + 1
        sp = STACK + 0xF0000
        for i, value in enumerate((STOP,) + args): self.write32(sp + 4*i, value)
        self.uc.reg_write(UC_X86_REG_ESP, sp); self.uc.reg_write(UC_X86_REG_ECX, this)
        try: self.uc.emu_start(address, STOP + 1, count=5000000)
        except Exception as exc: raise RuntimeError(f'Original stopped at {self.uc.reg_read(UC_X86_REG_EIP):x}: {exc}') from exc
        if self.uc.reg_read(UC_X86_REG_EIP) != STOP: raise RuntimeError('original instruction budget exhausted')
        return self.uc.reg_read(UC_X86_REG_EAX)
    def comments(self, addr):
        out = []
        while addr:
            value = self.read32(addr)
            out.append(self.string(value).hex() if value else None); addr = self.read32(addr + 4)
        return out
    def run(self, inputs, comments):
        for text in inputs:
            self.input, self.position = text, 0
            status = self.call(0x525A60, INI, STRAW, int(comments))
        sections = []
        section = self.read32(INI + 20)
        while section != INI + 28:
            entries = []; entry = self.read32(section + 24)
            while entry != section + 32:
                inline = self.read32(entry + 24)
                entries.append([self.string(self.read32(entry + 12)).hex(), self.string(self.read32(entry + 16)).hex(),
                                self.comments(self.read32(entry + 20)), self.string(inline).hex() if inline else None,
                                *[self.signed32(entry + i) for i in [28, 32, 36]]])
                entry = self.read32(entry + 4)
            sections.append([self.string(self.read32(section + 12)).hex(), self.comments(self.read32(section + 64)), entries])
            section = self.read32(section + 4)
        tail = self.comments(self.read32(INI + 60))
        self.call(0x526470, INI, PIPE)
        queries = []
        for section in sections:
            # Keep the section-name pointer alive across reads, as in the native object.
            name = self.cstring(bytes.fromhex(section[0]))
            for entry in section[2]:
                key = self.cstring(bytes.fromhex(entry[0])); dest = self.malloc(512)
                self.call(0x528A10, INI, name, key, self.cstring(b'missing'), dest, 512)
                queries.append(self.string(dest).hex())
        result = {'status': status, 'sections': sections, 'comments': tail,
                  'serialized': self.output.hex(), 'queries': queries}
        self.call(0x5256F0, INI)
        return result


def cases():
    samples = [b'', b'; comment only\n', b'no section', b'[S]\n', b'[S]\nA=1\n',
        b'[S]\nA=1', b'[S]\nA=1\nB=2', b'[S]\nA=\nB=2\n', b'[ S ]\n K = V \n',
        b'; pre\n[S]\n; entry\nK\t= v ;inline\n\n[E]\n; tail\n',
        b'[S]\nA=first\nA=second\nB=3\n', b'[S]\nA=1\n[S]\nB=2\n',
        b'\xef\xbb\xbf[S]\nA=1\n', b'[S]\r\nA=1\r\nB=2\r\n', b' [S] \n A = one;two\n',
        b'[S]\nK='+b'x'*900+b'\nZ=2\n', b'[S]\nK="a;b"\n', b'[S]\nK=a=b\n',
        b'[S]\n\tA\t=\tB\t; hi\n', b'[S]\n=bad\nA=1\n']
    for i, text in enumerate(samples):
        for keep in [False, True]: yield f'parse-{i}-comments-{keep}', [text], keep
    for i, text in enumerate(samples): yield f'merge-{i}', [b'[S]\nA=base\nZ=last\n', text], True
    randomizer = random.Random(12345)
    for i in range(24):
        lines = ['[S]']
        for _ in range(randomizer.randrange(1, 35)):
            lines.append(randomizer.choice(['A','B','C','a','LongKey']) + '=' + str(randomizer.randrange(100)))
        text = ('\n'.join(lines)+'\n').encode()
        yield f'duplicates-{i}', [text], False
        yield f'duplicate-merge-{i}', [text, b'[S]\nA=updated\nC=changed\na=lower\n'], True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    original = pefile.PE(str(args.exe))
    rows = []
    with tempfile.TemporaryDirectory(prefix='ra2-ini-diff-') as directory:
        for name, inputs, keep in cases():
            paths = []
            for i, text in enumerate(inputs):
                path = Path(directory)/f'{i}.ini'; path.write_bytes(text); paths.append(str(path))
            machine = Machine(original)
            expected = machine.run(inputs, keep)
            actual = json.loads(subprocess.check_output([str(args.native.resolve()), '--snapshot',
                'comments' if keep else 'plain', *paths], text=True))
            passed = expected == actual
            rows.append({'name': name, 'passed': passed,
                         **({} if passed else {'expected':expected,'actual':actual})})
            if not passed: print('FAIL:', name, ','.join(k for k in expected if expected[k] != actual[k]))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'original_sha256':SHA, 'native_sha256':hashlib.sha256(args.native.read_bytes()).hexdigest(), 'kind':'original x86 vs native host normalized INI state',
        'scope':'parser, nodes, comments, serializer and string lookup; streams and CRT primitives substituted; no Windows integration',
        'passed':sum(r['passed'] for r in rows),'total':len(rows),'cases':rows}, indent=2)+'\n')
    print(f"{sum(r['passed'] for r in rows)}/{len(rows)} passed")
    return 0 if all(r['passed'] for r in rows) else 1

if __name__ == '__main__': raise SystemExit(main())
