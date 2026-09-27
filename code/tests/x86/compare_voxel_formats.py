#!/usr/bin/env python3
"""Compare original YR instructions with compiled VoxLib/MotLib instructions.

File vtable operations and the CRT allocator are controlled services. Parser,
matrix, palette and cleanup bodies execute unchanged on both sides. This is
not a Windows injection/gameplay test. Undefined malformed-header behavior is
tested only by the native safety tests, not treated as an original contract.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from compare_image_pal import initialize_exception_chain, PRESERVED

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"
HEAP, STACK, SUPPORT, STOP = 0x2000000, 0x3000000, 0xD00000, 0xD0FFF0
FILE, VTABLE, OBJECT = SUPPORT + 0x100, SUPPORT + 0x200, SUPPORT + 0x300
ENTRIES = {"VoxConstruct": 0x755CD0, "VoxDestroy": 0x755D10, "VoxRead": 0x755DB0,
           "VoxHeader": 0x7564A0, "VoxTailer": 0x7564B0, "MotConstruct": 0x5BD570,
           "MotDestroy": 0x5BD5A0, "MotRead": 0x5BD5C0, "MotScale": 0x5BD730}


def hva(frames=2, layers=3):
    header = struct.pack("<16sII", b"NONE", frames, layers)
    names = b"".join(struct.pack("16s", f"limb{i}".encode()) for i in range(layers))
    matrices = b"".join(struct.pack("<12f", *(100*f + 20*l + n + .25 for n in range(12)))
                        for f in range(frames) for l in range(layers))
    return header + names + matrices


def vxl(palettes=1, contained=False, headers=2, tailers=3, body_size=16):
    header = struct.pack("<16s4I", b"Voxel Animation", palettes, headers, tailers, body_size)
    palette_size = 2 + 768 * palettes if contained else 770 * palettes
    palette = bytes([16, 31]) + bytes((i * 73 + 11) % 256 for i in range(768))
    palette = (palette + bytes(palette_size))[:palette_size]
    sections = b"".join(struct.pack("<16s3I", f"limb{i}".encode(), 1 if i == 0 else 0,
                                   0x12345678, 0xF8F9FAFE) for i in range(headers))
    body = bytes(range(body_size))
    tails = b"".join(struct.pack("<3If12f6f4B", 0, min(4, body_size), min(8, body_size),
        t + .5, *(20*t + n for n in range(12)), 1, 2, 3, 4, 5, 6, 201, 2, 3, 4)
        for t in range(tailers))
    return header + palette + sections + body + tails


class Machine:
    def __init__(self, original, dll, scenario, replacement):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.replacement, self.scenario = replacement, scenario
        self.data = scenario["data"]
        self.trace, self.blocks, self.freed = [], {}, set()
        self.cursor, self.alloc_count, self.position = HEAP, 0, 0
        for pe in (original, dll):
            base = pe.OPTIONAL_HEADER.ImageBase
            self.uc.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
            self.uc.mem_write(base, pe.get_memory_mapped_image())
        self.uc.mem_map(SUPPORT, 0x10000)
        self.uc.mem_map(HEAP, 0x100000)
        self.uc.mem_map(STACK, 0x100000)
        initialize_exception_chain(self.uc)
        self.uc.mem_write(HEAP, bytes([0xA5]) * 0x100000)
        self.uc.mem_write(OBJECT, bytes([0xA5]) * 64)
        self.uc.mem_write(0xB2FB78, bytes([0x7B]) * 768)
        self.uc.mem_write(0xB41178, bytes([0xA5]) * 32768)
        self.uc.mem_write(0xB45A90, bytes([0xA5]) * 128)
        self.write32(FILE, VTABLE)
        self.exports = {e.name.decode().split('@')[1] if e.name.startswith(b'@') else e.name.decode():
                        dll.OPTIONAL_HEADER.ImageBase + e.address
                        for e in dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
        self.hooks = {0x7C8E17: self.allocate, 0x7C8B3D: self.deallocate}
        for i, (slot, handler) in enumerate([(0x1C, self.open), (0x24, self.read),
                                             (0x28, self.seek), (0x34, self.close)]):
            address = SUPPORT + 0x1000 + 16 * i
            self.write32(VTABLE + slot, address)
            self.hooks[address] = handler
        for desc in dll.DIRECTORY_ENTRY_IMPORT:
            for item in desc.imports:
                name = (item.name or b"ordinal").decode()
                address = SUPPORT + 0x2000 + 16 * len(self.hooks)
                self.write32(item.address, address)
                self.hooks[address] = {"malloc": self.allocate, "free": self.deallocate,
                    "memset": self.memset, "memcpy": self.memcpy}.get(name,
                    lambda name=name: self.unexpected(name))
        for address, handler in self.hooks.items():
            self.uc.hook_add(UC_HOOK_CODE, lambda uc, a, size, data, h=handler: h(), begin=address, end=address)
        self.uc.hook_add(UC_HOOK_CODE, lambda uc, *args: uc.emu_stop(), begin=STOP, end=STOP)
        if replacement: self.call("BindPalette", args=(), raw=True)

    def read32(self, address): return struct.unpack("<I", self.uc.mem_read(address, 4))[0]
    def write32(self, address, value): self.uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))
    def arg(self, i): return self.read32(self.uc.reg_read(UC_X86_REG_ESP) + 4 + 4*i)
    def ret(self, value=0, cleanup=0):
        sp = self.uc.reg_read(UC_X86_REG_ESP)
        self.uc.reg_write(UC_X86_REG_EIP, self.read32(sp))
        self.uc.reg_write(UC_X86_REG_ESP, sp + 4 + cleanup)
        self.uc.reg_write(UC_X86_REG_EAX, value)
        self.uc.reg_write(UC_X86_REG_ECX, 0xCCCCCCCC)
        self.uc.reg_write(UC_X86_REG_EDX, 0xDDDDDDDD)

    def open(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == FILE and self.arg(0) == 1
        self.position = 0
        ok = self.scenario.get("open", True)
        self.trace.append(["open", ok]); self.ret(int(ok), 4)

    def read(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == FILE
        destination, requested = self.arg(0), self.arg(1)
        count = min(requested, len(self.data) - self.position)
        assert 0 <= count <= 0x10000
        self.uc.mem_write(destination, self.data[self.position:self.position + count])
        self.position += count
        self.trace.append(["read", requested, count]); self.ret(count, 8)

    def seek(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == FILE and self.arg(1) == 1
        offset = self.arg(0)
        self.position = min(len(self.data), self.position + offset)
        self.trace.append(["seek", offset]); self.ret(self.position, 8)

    def close(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == FILE
        self.trace.append(["close"]); self.ret()

    def allocate(self):
        size = max(1, self.arg(0)); self.alloc_count += 1
        ok = self.alloc_count != self.scenario.get("fail_allocation", 0)
        self.trace.append(["allocate", size, ok])
        if not ok: self.ret(); return
        assert size <= 0x10000
        address = self.cursor
        self.cursor += (size + 15) & ~15
        self.blocks[address] = size
        self.ret(address)

    def deallocate(self):
        address = self.arg(0)
        assert address in self.blocks and address not in self.freed, hex(address)
        self.freed.add(address)
        self.trace.append(["free", self.blocks[address]]); self.ret()

    def memset(self):
        target, value, size = (self.arg(i) for i in range(3))
        self.uc.mem_write(target, bytes([value & 255]) * size); self.ret(target)

    def memcpy(self):
        target, source, size = (self.arg(i) for i in range(3))
        self.uc.mem_write(target, bytes(self.uc.mem_read(source, size))); self.ret(target)

    def unexpected(self, name): raise AssertionError(f"Unexpected imported service: {name}")

    def call(self, name, args=(), raw=False):
        entry = self.exports[name] if self.replacement or raw else ENTRIES[name]
        sp = STACK + 0x80000
        self.write32(sp, STOP)
        for i, value in enumerate(args): self.write32(sp + 4 + 4*i, value)
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.uc.reg_write(UC_X86_REG_ECX, OBJECT)
        self.uc.reg_write(UC_X86_REG_EDX, 0xBAADF00D)
        for reg, value in PRESERVED.items(): self.uc.reg_write(reg, value)
        self.uc.emu_start(entry, STOP + 1, count=500000000)
        assert self.uc.reg_read(UC_X86_REG_EIP) == STOP, f"instruction limit: {name}"
        assert self.uc.reg_read(UC_X86_REG_ESP) == sp + 4 + 4*len(args), f"stack mismatch: {name}"
        for reg, value in PRESERVED.items():
            assert self.uc.reg_read(reg) == value, f"callee-saved register: {name}"
        return self.uc.reg_read(UC_X86_REG_EAX)

    def snapshot(self):
        size = 28 if self.scenario["kind"] == "Vox" else 16
        return {"object": bytes(self.uc.mem_read(OBJECT, size)).hex(),
                "blocks": {hex(a): bytes(self.uc.mem_read(a, n)).hex()
                           for a, n in self.blocks.items() if a not in self.freed},
                "palette": hashlib.sha256(self.uc.mem_read(0xB2FB78, 768)).hexdigest(),
                "lighting": hashlib.sha256(self.uc.mem_read(0xB41178, 32768)).hexdigest(),
                "levels": bytes(self.uc.mem_read(0xB45A90, 128)).hex()}

    def run(self):
        kind = self.scenario["kind"]
        args = (FILE, int(self.scenario.get("palette", False))) if kind == "Vox" else (FILE,)
        assert self.call(kind + "Construct", args) == OBJECT
        snapshots = [self.snapshot()]
        if self.scenario.get("scale") is not None:
            self.call("MotScale", (struct.unpack("<I", struct.pack("<f", self.scenario["scale"]))[0],))
            snapshots.append(self.snapshot())
        if self.scenario.get("accessors"):
            header = self.call("VoxHeader", (0,))
            tailer = self.call("VoxTailer", (0, 1))
            snapshots.append({"header": header, "tailer": tailer})
        for data, opened in self.scenario.get("reloads", []):
            self.data = data; self.scenario["open"] = opened
            result = self.call(kind + "Read", args)
            snapshots.append({"result": result, "state": self.snapshot()})
        self.call(kind + "Destroy")
        snapshots.append(self.snapshot())
        assert not (self.blocks.keys() - self.freed), "allocation leak"
        return {"states": snapshots, "trace": self.trace}


def cases():
    yield {"name": "hva-multiframe-scale", "kind": "Mot", "data": hva(), "scale": -2.0}
    yield {"name": "hva-zero-frames", "kind": "Mot", "data": hva(0, 3)}
    yield {"name": "hva-zero-layers", "kind": "Mot", "data": hva(2, 0)}
    yield {"name": "hva-missing", "kind": "Mot", "data": hva(), "open": False}
    yield {"name": "hva-allocation-failure", "kind": "Mot", "data": hva(), "fail_allocation": 1}
    for length in (24, 71, 72, 73, 119, len(hva()) - 1):
        yield {"name": f"hva-short-{length}", "kind": "Mot", "data": hva()[:length]}
    yield {"name": "hva-reload", "kind": "Mot", "data": hva(),
           "reloads": [(hva(), False), (hva()[:-1], True), (hva(3, 1), True)]}
    yield {"name": "hva-failed-constructor-reload", "kind": "Mot", "data": hva(), "open": False,
           "reloads": [(hva(), True)]}
    yield {"name": "vxl-sections-relocations", "kind": "Vox", "data": vxl(), "accessors": True}
    yield {"name": "vxl-missing", "kind": "Vox", "data": vxl(), "open": False}
    yield {"name": "vxl-empty", "kind": "Vox", "data": vxl(0, headers=0, tailers=0, body_size=0)}
    yield {"name": "vxl-contained", "kind": "Vox", "data": vxl(), "palette": True}
    yield {"name": "vxl-contained-multiple", "kind": "Vox", "data": vxl(2, True), "palette": True}
    yield {"name": "vxl-skip-multiple", "kind": "Vox", "data": vxl(2)}
    for failure in (1, 2, 3):
        yield {"name": f"vxl-allocation-failure-{failure}", "kind": "Vox", "data": vxl(), "fail_allocation": failure}
    for length in (32, 33, 700, 802, 829, 857, 858, 873, 874, 965, len(vxl()) - 1):
        yield {"name": f"vxl-short-{length}", "kind": "Vox", "data": vxl()[:length]}
    for length in (32, 33, 801, len(vxl()) - 1):
        yield {"name": f"vxl-palette-short-{length}", "kind": "Vox", "data": vxl()[:length], "palette": True}
    yield {"name": "vxl-reload", "kind": "Vox", "data": vxl(),
           "reloads": [(vxl(), False), (vxl()[:-1], True), (vxl(0), True)]}
    yield {"name": "vxl-failed-constructor-reload", "kind": "Vox", "data": vxl(), "open": False,
           "reloads": [(vxl(), True)]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--dll", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.original.read_bytes()).hexdigest() == SHA
    original, dll = pefile.PE(str(args.original)), pefile.PE(str(args.dll))
    rows = []
    for scenario in cases():
        reference = Machine(original, dll, dict(scenario), False).run()
        compiled = Machine(original, dll, dict(scenario), True).run()
        if reference != compiled:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.with_suffix('.failure.json').write_text(json.dumps(
                {"case": scenario["name"], "original": reference, "compiled": compiled}, indent=2))
            raise AssertionError(f"Differential mismatch: {scenario['name']}")
        rows.append({"name": scenario["name"], "passed": True,
                     "service_calls": len(compiled["trace"]),
                     "state_sha256": hashlib.sha256(json.dumps(compiled, sort_keys=True).encode()).hexdigest()})
        print(f"PASS {scenario['name']}", flush=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({"original_sha256": SHA,
        "compiled_sha256": hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        "scope": "original vs compiled x86 instructions with controlled file and allocator services",
        "cases": rows}, indent=2) + '\n')


if __name__ == "__main__": main()
