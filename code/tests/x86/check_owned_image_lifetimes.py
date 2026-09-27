#!/usr/bin/env python3
"""Execute core-owned image construction, virtual dispatch and deletion on x86.

Uses the fixed EXE for external state and existing allocator/CRT doubles.
This does not simulate a complete Windows process or a rendered game session.
"""
import argparse
import hashlib
import json
from pathlib import Path
import pefile
from compare_image_light import LightMachine, ART, SCREEN, SURFACE
from compare_image_resources import OBJECT, SUPPORT, SHA


def native_method(machine, pointer, slot=0):
    table = machine.read32(pointer)
    base = machine.dll.OPTIONAL_HEADER.ImageBase
    assert base <= table < base + machine.dll.OPTIONAL_HEADER.SizeOfImage, ('foreign table', hex(table))
    entry = machine.read32(table + slot * 4)
    assert any(lo <= entry < hi for lo, hi in machine.code_ranges), ('foreign method', hex(entry))
    return entry


def blitters(machine, owner, expected):
    pointers = [machine.read32(owner + 8 + i * 4) for i in range(89)]
    assert sum(bool(p) for p in pointers) == expected
    for pointer in filter(None, pointers):
        native_method(machine, pointer)
        native_method(machine, pointer, 1)
    return pointers


def live_allocations(machine):
    return {p for p in machine.allocations if p not in machine.freed}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--dll', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    exe, dll = pefile.PE(str(args.exe)), pefile.PE(str(args.dll))
    passed = []
    for bpp in (1, 2):
        for skip in (0, 1):
            for flags in (0, 1):
                m = LightMachine(exe, dll, True)
                m.dll = dll
                baseline = live_allocations(m)
                owner = m.allocate_block(0x188) if flags else OBJECT
                m.write32(SURFACE + 16, bpp)
                assert m.call(0x48E740, owner, args=(ART, SCREEN, SURFACE, 1 if bpp == 1 else 7, skip), kind='value') == owner
                native_method(m, owner)
                assert m.read32(0x89ECF8 + 16) == 1
                if skip: m.call(0x48EBF0, owner)
                pointers = blitters(m, owner, 25 if bpp == 1 else 89)
                # Call the compiler-generated method table without an EXE detour.
                source, target = SUPPORT + 0xA000, SUPPORT + 0xA100
                m.uc.mem_write(source, bytes((1, 2, 3)))
                m.uc.mem_write(target, bytes(6))
                m.call(native_method(m, pointers[0], 1), pointers[0], args=(target, source, 3, 0, 0, 0, 1000, 0))
                m.call(0x490490, owner)
                blitters(m, owner, 0)
                m.call(0x48EBF0, owner)
                blitters(m, owner, 25 if bpp == 1 else 89)
                # The native scalar deleting destructor implements both flags.
                m.call(native_method(m, owner), owner, args=(flags,), kind='value')
                assert m.read32(0x89ECF8 + 16) == 0
                assert m.read32(0x88A080 + 16) == 0
                assert live_allocations(m) == baseline
                passed.append(f'convert/{bpp}/{skip}/flags={flags}')
    for flags in (0, 1):
        m = LightMachine(exe, dll, True, event=1)
        m.dll = dll
        baseline = live_allocations(m)
        owner = m.allocate_block(0x1B4) if flags else OBJECT
        m.call(0x555DA0, owner, args=(ART, SCREEN, SURFACE, 1000, 800, 600, 0, 0, 27), kind='value')
        blitters(m, owner, 89)
        m.call(native_method(m, owner, 1), owner, args=(500, 700, 900, 1))
        assert m.light_update_hits == 0, 'native virtual update detoured through EXE hook'
        assert m.uc.mem_read(owner + 0x1B0, 1) == b'\1'
        m.call(native_method(m, owner), owner, args=(flags,), kind='value')
        assert m.read32(0x89ECF8 + 16) == 0
        assert m.read32(0x88A080 + 16) == 0
        assert live_allocations(m) == baseline
        passed.append(f'light/flags={flags}')
    report = {'scope': __doc__, 'exe_sha256': SHA, 'dll_sha256': hashlib.sha256(args.dll.read_bytes()).hexdigest(), 'passed': passed}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(passed)} core image lifetime cases passed')


if __name__ == '__main__':
    main()
