#!/usr/bin/env python3
"""Observe YR cliff/slope shadow tables by executing their original initializers."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    if hashlib.sha256(args.exe.read_bytes()).hexdigest() != SHA:
        raise ValueError("Unexpected gamemd.exe SHA-256")
    pe = pefile.PE(str(args.exe))
    image = pe.get_memory_mapped_image()
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(image) + 0xFFF) & ~0xFFF)
    uc.mem_write(pe.OPTIONAL_HEADER.ImageBase, image)
    uc.mem_map(0x02000000, 0x10000)
    report = {"exe_sha256": SHA, "scope": __doc__}
    for name, entry, table, count in (("cliffs", 0x543F10, 0xABC2D0, 40),
                                      ("slopes", 0x544690, 0xABC210, 10)):
        uc.reg_write(UC_X86_REG_ESP, 0x02008000)
        uc.mem_write(0x02008000, struct.pack("<I", 0x02009000))
        uc.emu_start(entry, 0x02009000, count=10000)
        if uc.reg_read(UC_X86_REG_EIP) != 0x02009000:
            raise RuntimeError("Initializer did not return")
        report[name] = {"initializer": hex(entry), "table": hex(table),
                        "records": [struct.unpack("<4i", uc.mem_read(table + 16 * i, 16))
                                    for i in range(count)]}
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print("Captured 40 cliff and 10 slope records from original x86 execution")


if __name__ == "__main__":
    main()
