#!/usr/bin/env python3
"""LightConvert EXE/DLL instruction differential: state, palettes and ownership.

All installed algorithm bodies are forbidden in replacement runs. File/CRT and
allocation services use the same doubles as the resource tests.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from compare_image_convert import ConvertMachine, ART, SCREEN, SURFACE
from compare_image_resources import OBJECT, SUPPORT, SHA

INDEXES, SCENARIO, LIGHT_ITEMS = SUPPORT + 0x9800, 0xA00000, SUPPORT + 0x9c00

class LightMachine(ConvertMachine):
    def call(self, *args, **kwargs):
        self.light_update_hits = 0
        return super().call(*args, **kwargs)

    def on_instruction(self, uc, address, size, context):
        if self.replacement and address == self.exports['RA2Images_LightUpdate']:
            self.light_update_hits += 1
            assert self.light_update_hits == 1, \
                'LightUpdate re-entered its patched entry through the canonical game vtable'
        super().on_instruction(uc, address, size, context)

    def __init__(self, exe, dll, replacement, mode=2, mmx=0, event=0):
        super().__init__(exe, dll, replacement)
        for a in (0xA9FAB4, 0xA9FABC, 0xA9FAB0, 0xA9FAC0): self.write32(a, 0)
        self.write32(0xA8B230, SCENARIO)
        for start in (3411, 3417, 3424):
            for i, value in enumerate((65, 110, 150)): self.write32(SCENARIO + 4*(start+i), value)
        if event: self.write32((0xA9FAB4, 0xA9FABC, 0xA9FAB0, 0xA9FAC0)[event-1], 1)
        self.write32(0x829D20, mode)
        self.write32(0x8205D0, mode)
        self.write32(0x84E860, mmx << 16)
        self.write32(0xA8EB78, 2)
        self.write32(0x887308, SURFACE)
        self.uc.mem_write(INDEXES, bytes(int(i % 3 != 0) for i in range(256)))
        self.uc.mem_write(0x87F698, struct.pack('<IIIBB2xII', 0x7E186C, LIGHT_ITEMS, 16, 1, 0, 0, 10))
        self.uc.mem_write(0xABBED0, bytes(self.uc.mem_read(ART, 768)))
        self.uc.mem_write(0x885780, bytes(self.uc.mem_read(SCREEN, 768)))

    def snapshot(self):
        result = super().snapshot()
        result['light'] = bytes(self.uc.mem_read(OBJECT, 0x1b4)).hex()
        result['light_vector'] = bytes(self.uc.mem_read(0x87F698, 24)).hex()
        result['light_items'] = bytes(self.uc.mem_read(LIGHT_ITEMS, 64)).hex()
        result['mode'] = self.read32(0x829D20)
        return result

def lifetime(m, shades=53, skip=1, custom=True, initial=(1000, 870, 630)):
    m.call(0x555DA0, OBJECT, args=(ART, SCREEN, SURFACE, *initial, skip, INDEXES if custom else 0, shades), kind='value')
    result = [m.snapshot()]
    for values in ((999, 567, 1234, 0), (1700, 999, 2400, 1), (-1, 0, 0, 0), (-1, 0, 0, 1), (-4, 4000, -9, 0)):
        m.call(0x556090, OBJECT, args=values)
        result.append(m.snapshot())
    m.call(0x556510, OBJECT)
    result.append(m.snapshot())
    return result

def find(m):
    result = []
    for rgb in ((1000, 1000, 1000), (570, 340, 110), (571, 341, 111), (1200, -50, 750)):
        result.append(m.call(0x544E70, rgb[0], rgb[1], args=(rgb[2],), kind='value'))
    result.append(m.snapshot())
    for pointer in set(result[:-1]): m.call(0x556520, pointer, args=(1,))
    return result

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True); p.add_argument('--dll', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True); p.add_argument('--case')
    a = p.parse_args()
    assert hashlib.sha256(a.exe.read_bytes()).hexdigest() == SHA
    exe, dll = pefile.PE(str(a.exe)), pefile.PE(str(a.dll))
    cases = []
    for mode in (-1, 0, 1, 2, 3, 4):
        for mmx in (0, 1):
            for shades in (1, 27, 53):
                cases.append((f'palette_{mode}_{mmx}_{shades}', mode, mmx, 0, lambda m, s=shades: lifetime(m, s)))
    for event in range(5):
        for custom in (False, True):
            cases.append((f'constructor_{event}_{custom}', 2, 0, event,
                          lambda m, c=custom: lifetime(m, skip=0, custom=c, initial=(-1, 2, 3))))
    cases.append(('find_and_reuse', 2, 0, 0, find))
    if a.case: cases = [row for row in cases if row[0] == a.case]
    assert cases
    passed = []
    for name, mode, mmx, event, operation in cases:
        values = []
        for replacement in (False, True):
            m = LightMachine(exe, dll, replacement, mode, mmx, event)
            values.append([operation(m), m.snapshot()])
        if values[0] != values[1]:
            a.report.with_suffix('.failure.json').write_text(json.dumps({'case':name, 'original':values[0], 'replacement':values[1]}, indent=2))
            raise AssertionError(name)
        passed.append(name); print('Passed', name, flush=True)
    a.report.write_text(json.dumps({'scope':__doc__, 'target_sha256':SHA,
        'dll_sha256':hashlib.sha256(a.dll.read_bytes()).hexdigest(), 'passed':len(passed), 'cases':passed}, indent=2)+'\n')

if __name__ == '__main__': main()
