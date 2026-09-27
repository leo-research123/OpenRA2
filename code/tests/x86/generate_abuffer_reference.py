#!/usr/bin/env python3
"""Capture ABuffer pixel outputs from pinned gamemd.exe instructions.

Executes the original LUT initializer and contiguous-row shroud/fog/AlphaShape
loops. Covers old/source 0..255; excludes clipping, ring wrapping, SHP decoding,
object lifecycle and full frames. Fixture contains observations, no game code/art.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP,
)

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"


def generate(executable, output, report):
    if hashlib.sha256(executable.read_bytes()).hexdigest() != SHA:
        raise ValueError("Unexpected gamemd.exe SHA-256")
    pe = pefile.PE(str(executable))
    image = pe.get_memory_mapped_image()
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(image) + 0xFFF) & ~0xFFF)
    uc.mem_write(pe.OPTIONAL_HEADER.ImageBase, image)
    uc.mem_map(0x02000000, 0x10000)
    uc.mem_map(0x03000000, 0x40000)
    uc.reg_write(UC_X86_REG_ESP, 0x02008000)
    source, dest = 0x03000000, 0x03010000

    def run(start, end, **registers):
        ids = dict(eax=UC_X86_REG_EAX, ebx=UC_X86_REG_EBX,
                   ecx=UC_X86_REG_ECX, edx=UC_X86_REG_EDX,
                   esi=UC_X86_REG_ESI, edi=UC_X86_REG_EDI)
        for name, reg in ids.items():
            uc.reg_write(reg, registers.get(name, 0))
        uc.emu_start(start, end, count=10000000)
        if uc.reg_read(UC_X86_REG_EIP) != end:
            raise RuntimeError(f"Loop did not reach 0x{end:08X}")

    run(0x00420A81, 0x00420AE7)
    uc.mem_write(source, bytes(range(256)) * 256)
    initial = struct.pack("<65536H", *(old for old in range(256) for _ in range(256)))
    loops = [
        ("shroud", 0x0047F226, 0x0047F23C, dict(esi=source, edi=dest, ecx=65536)),
        ("fog", 0x0047F4A2, 0x0047F4E7, dict(esi=source, eax=dest, edi=65536)),
        ("alpha_shape", 0x0042164C, 0x00421678, dict(edi=source, eax=dest, ecx=65536)),
    ]
    payload = bytearray(b"ABUFREF1")
    records = []
    for name, start, end, registers in loops:
        uc.mem_write(dest, initial)
        run(start, end, **registers)
        # Retain exact WORD results; compression to bytes also checks their range.
        values = bytes(struct.unpack("<65536H", uc.mem_read(dest, len(initial))))
        payload.extend(values)
        records.append(dict(name=name, start=f"0x{start:08X}", stop_before=f"0x{end:08X}",
                            cases=len(values), output_sha256=hashlib.sha256(values).hexdigest()))
    output.write_bytes(payload)
    metadata = dict(scope=__doc__, target_sha256=SHA, loops=records,
                    lut_sha256=hashlib.sha256(uc.mem_read(0x0088A118, 65536)).hexdigest(),
                    fixture_sha256=hashlib.sha256(payload).hexdigest())
    report.write_text(json.dumps(metadata, indent=2) + "\n")
    print(json.dumps(metadata, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    generate(args.exe, args.output, args.report)
