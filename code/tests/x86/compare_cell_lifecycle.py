#!/usr/bin/env python3
"""Compare real Cell constructors/destructors, excluding original unspecified bits.

Both paths use the real Abstract/Scenario identity code. Owned PixelFX deletion
is observed at its external virtual boundary; this does not implement PixelFX.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from compare_scenario import Machine, ENTRIES, OBJECT, SUPPORT, SHA
from unicorn.x86_const import UC_X86_REG_ECX


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'dll', 'report'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    exe, dll = pefile.PE(str(args.exe)), pefile.PE(str(args.dll))
    ENTRIES.update(ScenarioCellConstruct=0x47BBF0, ScenarioCellDestroy=0x47BB60)
    machines = [Machine(exe, dll, candidate) for candidate in (False, True)]
    address, effect, light = OBJECT + 0x20000, SUPPORT + 0x5000, SUPPORT + 0x6000
    size = 0x148
    # Compare every initialized field, rather than copying a fabricated object.
    mask = bytearray([255] * size)
    for start, end in ((0, 16), (0x21, 0x24), (0x123, 0x124), (0x125, 0x128),
                       (0x129, 0x12C), (0x139, 0x13C), (0x144, 0x148)):
        mask[start:end] = bytes(end - start)
    mask[0x14:0x18] = b'\x07\0\0\0'  # AbstractFlags low three bits
    mask[0x12C:0x130] = b'\x1F\0\0\0'  # AltFlags low five bits
    mask[0x140:0x144] = b'\xFF\xFF\x7F\0'  # CellFlags low 23 bits
    cases = 0
    for fill in (0, 0xA5, 0xFF):
        for coords in ((0, 0), (4, 7)):
            for references in (False, True):
                results, events = [], [[], []]
                for m, destroyed in zip(machines, events):
                    m.heap = address
                    m.write32(OBJECT + 0x214, 73)  # active Scenario UniqueID
                    m.write32(0x89E748, 0)  # original 47B2F0 initialized empty cell
                    m.uc.mem_write(address - 16, bytes([fill]) * (size + 32))
                    assert m.call('ScenarioCellConstruct', ecx=address) == address
                    data = bytes(m.uc.mem_read(address, size))
                    assert m.read32(address + 0x10) == 74
                    assert m.read32(OBJECT + 0x214) == 74
                    results.append(data)
                    if m.replacement:
                        for offset in (0, 4, 8, 12):
                            vtable = m.read32(address + offset)
                            assert dll.OPTIONAL_HEADER.ImageBase <= vtable < dll.OPTIONAL_HEADER.ImageBase + dll.OPTIONAL_HEADER.SizeOfImage
                    assert bytes(m.uc.mem_read(address - 16, 16)) == bytes([fill]) * 16
                    assert bytes(m.uc.mem_read(address + size, 16)) == bytes([fill]) * 16
                    m.uc.mem_write(address + 0x24, struct.pack('<2h', *coords))
                    m.write32(address + 0x34, light)
                    m.write32(light + 0x194, 9)
                    m.uc.mem_write(0xA8E9A0, bytes([references]))
                    m.write32(address + 0xFC, effect)
                    m.write32(effect, effect + 0x20)
                    m.write32(effect + 0x20, effect + 0x40)
                    def destroy_effect(m=m, destroyed=destroyed):
                        assert m.uc.reg_read(UC_X86_REG_ECX) == effect and m.arg(0) == 1
                        destroyed.append('pixel_fx'); m.ret(effect, cleanup=4)
                    m.hooks[effect + 0x40] = destroy_effect
                    m.call('ScenarioCellDestroy', ecx=address)
                    assert m.read32(address + 0xFC) == 0
                    assert m.read32(light + 0x194) == 9 - int(references and coords != (0, 0))
                    assert m.read32(address + 0x34) == (light if coords == (0, 0) else 0)
                    if m.replacement:
                        assert m.freed.pop() == address
                assert events == [['pixel_fx'], ['pixel_fx']]
                for offset, (left, right, bits) in enumerate(zip(*results, mask)):
                    assert (left & bits) == (right & bits), (fill, hex(offset), left, right, bits)
                cases += 1
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'exe_sha256': SHA,
        'dll_sha256': hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'cases': cases, 'status': 'passed', 'scope': __doc__,
        'checks': ['all initialized fields and defined flag bits', 'Scenario ID increment',
                   'compiler-owned four interface vtables', 'stack and preserved registers',
                   'output canaries', 'PixelFX virtual deletion', 'live light reference flag',
                   'no original Cell constructor/destructor fallback'],
        'excluded': ['padding and original unspecified high flag/occupation bits',
                     'PixelFX implementation', 'FoggedObject contents', 'full map lifecycle']}, indent=2) + '\n')
    print(f'{cases} original/compiled Cell lifecycle cases passed')


if __name__ == '__main__':
    main()
