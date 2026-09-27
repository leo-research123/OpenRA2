#!/usr/bin/env python3
"""Check the restored FoggedObject declaration against the fixed YR executable.

Combines the existing clang-cl layout/dispatch report with original RTTI/vtables
and executes small original query methods, including stack cleanup and registers.
Does not claim native FoggedObject lifecycle, serialization or rendering parity.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EIP, UC_X86_REG_ESI,
    UC_X86_REG_ESP,
)

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"
PRIMARY = [
    0x410260, 0x410300, 0x410310, 0x4D27D0, 0x410450,
    0x4D2510, 0x4D24A0, 0x4103E0, 0x4D2910, 0x410470,
    0x410480, 0x4D27B0, 0x4D27C0, 0x4D2810, 0x410490,
    0x4104A0, 0x4104B0, 0x410440, 0x4104C0, 0x4104F0,
    0x410520, 0x410530, 0x410540, 0x410570, 0x4D28D0,
]
OBJECT, OUTPUT, RECORD, STACK, STOP = (0x02001000, 0x02002000, 0x02003000,
                                       0x02008000, 0x02009000)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--msvc-report", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    if hashlib.sha256(args.exe.read_bytes()).hexdigest() != SHA:
        raise ValueError("Unexpected gamemd.exe SHA-256")
    compiled = json.loads(args.msvc_report.read_text())
    assert compiled["status"] == "passed"
    slots = {"FoggedClassID": 0x03, "FoggedLoad": 0x05, "FoggedSave": 0x06,
             "FoggedDelete": 0x08, "FoggedWhat": 0x0B, "FoggedSize": 0x0C,
             "FoggedCRC": 0x0D, "FoggedCell": 0x18, "FoggedCoords": 0x12,
             "FoggedOwner": 0x0F, "FoggedUpdate": 0x17}
    for name, slot in slots.items():
        assert compiled["slots"][name] == slot, name

    pe = pefile.PE(str(args.exe))
    image = pe.get_memory_mapped_image()
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(image) + 0xFFF) & ~0xFFF)
    uc.mem_write(pe.OPTIONAL_HEADER.ImageBase, image)
    uc.mem_map(0x02000000, 0x10000)
    read32 = lambda at: struct.unpack("<I", uc.mem_read(at, 0x04))[0]

    names = {0x822440: b".?AVFoggedObjectClass@@",
             0x8224A0: b".?AV?$DynamicVectorClass@UDrawRecord@FoggedObjectClass@@@@"}
    for at, name in names.items():
        assert bytes(uc.mem_read(at, len(name) + 1)) == name + b"\0"
    tables = [(0x7E8B38, 0x00, PRIMARY),
              (0x7E8B1C, 0x04, [0x4105E0, 0x4105F0, 0x410600, 0x410210, 0x410220, 0x410230]),
              (0x7E8B14, 0x08, [0x410580]), (0x7E8B0C, 0x0C, [0x410590])]
    for table, offset, expected in tables:
        assert [read32(table + 0x04 * i) for i in range(len(expected))] == expected
        locator = read32(table - 0x04)
        assert read32(locator + 0x04) == offset
        assert read32(locator + 0x0C) == 0x822438

    # Constructor returns observed at the ends of the five original functions.
    for at, opcodes in {0x4D0978: b"\xC3", 0x4D0C31: b"\xC2\x0C\x00",
                        0x4D0EEA: b"\xC2\x0C\x00", 0x4D136B: b"\xC2\x08\x00",
                        0x4D1648: b"\xC2\x04\x00"}.items():
        assert bytes(uc.mem_read(at, len(opcodes))) == opcodes, hex(at)

    calls = []

    def run(entry, arguments=(), cleanup=0x00, this=OBJECT):
        uc.mem_write(STACK, struct.pack("<" + "I" * (len(arguments) + 1), STOP, *arguments))
        uc.reg_write(UC_X86_REG_ESP, STACK)
        uc.reg_write(UC_X86_REG_ECX, this)
        uc.reg_write(UC_X86_REG_EDX, 0xDEADBEEF)
        saved = {r: 0x12345000 + i for i, r in enumerate(
            [UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP])}
        for register, value in saved.items():
            uc.reg_write(register, value)
        uc.emu_start(entry, STOP, count=0x1000)
        assert uc.reg_read(UC_X86_REG_EIP) == STOP, hex(entry)
        assert uc.reg_read(UC_X86_REG_ESP) == STACK + 0x04 + cleanup, hex(entry)
        assert all(uc.reg_read(r) == v for r, v in saved.items()), hex(entry)
        result = uc.reg_read(UC_X86_REG_EAX)
        calls.append({"entry": hex(entry), "arguments": list(arguments),
                      "callee_cleanup": hex(cleanup), "eax": hex(result)})
        return result

    uc.mem_write(OBJECT, b"\xA5" * 0x78)
    assert run(0x4D27B0) == 0x3D
    assert run(0x4D27C0) == 0x78
    coords = [(0, 0), (0x100, 0x201), (-1, -0xFF), (-0x100, -0x101),
              (0x7FFFFFFF, -0x80000000)]
    for x, y in coords:
        uc.mem_write(OBJECT + 0x34, struct.pack("<3i", x, y, 0x1234))
        uc.mem_write(OUTPUT, b"\xA5" * 0x10)
        assert run(0x4D28D0, (OUTPUT,), 0x04) == OUTPUT
        # Original signed integer division truncates toward zero, then to WORD.
        expected = struct.pack("<2H", int(x / 0x100) & 0xFFFF, int(y / 0x100) & 0xFFFF)
        assert bytes(uc.mem_read(OUTPUT, 0x04)) == expected
        assert bytes(uc.mem_read(OUTPUT + 0x04, 0x0C)) == b"\xA5" * 0x0C

    assert run(0x4D27D0, (OBJECT, 0), 0x08, this=0) == 0x80004003
    assert run(0x4D27D0, (OBJECT, OUTPUT), 0x08, this=0) == 0
    guid = bytes(uc.mem_read(OUTPUT, 0x10))
    assert guid.hex() == "0e0b471cd769d211b8f2006008c809ed"

    # Original inherited queries intentionally ignore stored House/Position.
    assert run(PRIMARY[0x0F]) == 0
    assert run(PRIMARY[0x12], (OUTPUT,), 0x04) == OUTPUT
    assert bytes(uc.mem_read(OUTPUT, 0x0C)) == bytes(0x0C)
    for count in (0, -1, 1):
        uc.mem_write(OBJECT + 0x60, struct.pack("<I", RECORD))
        uc.mem_write(OBJECT + 0x6C, struct.pack("<i", count))
        uc.mem_write(RECORD, struct.pack("<I", 0x12345678))
        assert run(0x4D28C0) == (0x12345678 if count > 0 else 0)
    uc.mem_write(OBJECT + 0x30, struct.pack("<I", 0x24))
    assert run(0x4D2790) == 0

    report = {"status": "passed", "exe_sha256": SHA, "scope": __doc__,
              "msvc_report": str(args.msvc_report), "compiled_slots": slots,
              "vtables": [{"va": hex(a), "object_offset": hex(o),
                           "entries": list(map(hex, v))} for a, o, v in tables],
              "class_id_bytes": guid.hex(), "original_calls": calls}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Fogged ABI: 4 vtables, 5 constructor returns, {len(calls)} original query calls passed")


if __name__ == "__main__":
    main()
