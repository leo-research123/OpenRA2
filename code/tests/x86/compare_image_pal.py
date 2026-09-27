#!/usr/bin/env python3
"""Execute original and compiled PAL entry instructions with matched services.

This is a differential test of 72ADE0/4A3890, not of the remaining original
CCFile/Convert implementations and not a Windows game integration test.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"
HEAP, STACK, SUPPORT, STOP = 0x2000000, 0x3000000, 0xD00000, 0xD0FFF0
VTABLE, FILE, NAME, PAL_OUT, CONVERT_OUT = SUPPORT + 0x100, SUPPORT + 0x300, SUPPORT + 0x400, SUPPORT + 0x500, SUPPORT + 0x504
DESTRUCT, EXISTS, SIZE, READ = SUPPORT + 0x1000, SUPPORT + 0x1010, SUPPORT + 0x1020, SUPPORT + 0x1030
PRESERVED = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x23456789,
             UC_X86_REG_EDI: 0x3456789A, UC_X86_REG_EBP: 0x456789AB}


def initialize_exception_chain(uc):
    # MSVC C++ EH prologues link records through FS:[0]. Supply a separate
    # segment so ordinary null accesses still fault in allocation-failure cases.
    # This supplies thread storage only; no exception handler is substituted.
    gdt, teb = SUPPORT + 0x10000, SUPPORT + 0x12000
    uc.mem_map(gdt, 0x3000)
    def descriptor(base, limit, access, flags):
        return ((limit & 0xFFFF) | ((base & 0xFFFFFF) << 16) | (access << 40)
                | (((limit >> 16) & 0xF) << 48) | (flags << 52)
                | (((base >> 24) & 0xFF) << 56))
    uc.mem_write(gdt + 8, struct.pack("<3Q",
        descriptor(0, 0xFFFFF, 0x9B, 0xC),  # flat 32-bit code
        descriptor(0, 0xFFFFF, 0x93, 0xC),  # flat 32-bit data/stack
        descriptor(teb, 0xFFF, 0x93, 0x4)))
    uc.mem_write(teb, struct.pack("<I", 0xFFFFFFFF))
    uc.reg_write(UC_X86_REG_GDTR, (0, gdt, 0xFFF, 0))
    uc.reg_write(UC_X86_REG_CS, 8)
    for segment in (UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS):
        uc.reg_write(segment, 16)
    uc.reg_write(UC_X86_REG_FS, 24)


class Machine:
    def __init__(self, original, dll, scenario, replacement):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.scenario, self.replacement = scenario, replacement
        self.trace, self.allocations, self.freed = [], {}, []
        self.cursor, self.allocation_count = HEAP, 0
        self.file_object, self.expected_palette = None, None
        self.map_pe(original)
        if dll is not None:
            self.map_pe(dll)
        self.uc.mem_map(SUPPORT, 0x10000)
        self.uc.mem_write(STOP, b"\xf4")
        self.uc.mem_map(HEAP, 0x100000)
        self.uc.mem_map(STACK, 0x100000)
        initialize_exception_chain(self.uc)
        self.uc.mem_write(HEAP, bytes([0xA5]) * 0x100000)
        self.uc.mem_write(NAME, b"test.pal\0")
        self.write32(PAL_OUT, 0xDEAD0100)
        self.write32(CONVERT_OUT, 0xDEAD0200)
        self.write32(0x887310, SUPPORT + 0x800)
        self.write32(FILE, VTABLE)
        for offset, target in [(0, DESTRUCT), (0x14, EXISTS), (0x2C, SIZE), (0x24, READ)]:
            self.write32(VTABLE + offset, target)
        self.hooks = {
            0x4739F0: self.construct_file, EXISTS: self.exists, SIZE: self.size,
            READ: self.read, 0x7C8E17: self.allocate, 0x7C8B3D: self.deallocate,
            0x48E740: self.construct_convert, 0x43AE50: lambda: self.ret(),
            0x431B80: self.destroy_inline, DESTRUCT: self.destroy_virtual,
        }
        self.exports = ({e.name.decode(): dll.OPTIONAL_HEADER.ImageBase + e.address
                        for e in dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name} if dll is not None else {})
        self.import_names = {}
        for descriptor in (dll.DIRECTORY_ENTRY_IMPORT if dll is not None else []):
            for item in descriptor.imports:
                address = SUPPORT + 0x2000 + 16 * len(self.hooks)
                self.write32(item.address, address)
                name = (item.name or str(item.ordinal).encode()).decode()
                self.import_names[address] = name
                # Core Memory.cpp now reaches the CRT directly. Keep the same
                # allocator double for original entries and imported CRT calls.
                handler = {"memset": self.memset, "memcpy": self.memcpy,
                           "malloc": self.allocate, "free": self.deallocate}.get(name)
                self.hooks[address] = handler or (lambda name=name: self.unexpected(name))
        if replacement:
            for original_address, export in [(0x72ADE0, "RA2Images_CreateFromFile"),
                                              (0x4A3890, "RA2Images_ReadWholeFile")]:
                delta = (self.exports[export] - original_address - 5) & 0xFFFFFFFF
                self.uc.mem_write(original_address, b"\xe9" + struct.pack("<I", delta))
        self.uc.hook_add(UC_HOOK_CODE, self.on_instruction)

    def map_pe(self, pe):
        assert pe.FILE_HEADER.Machine == 0x14C
        base = pe.OPTIONAL_HEADER.ImageBase
        self.uc.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
        self.uc.mem_write(base, pe.get_memory_mapped_image())

    def read32(self, address):
        return struct.unpack("<I", self.uc.mem_read(address, 4))[0]

    def write32(self, address, value):
        self.uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))

    def arg(self, index):
        return self.read32(self.uc.reg_read(UC_X86_REG_ESP) + 4 + 4 * index)

    def ret(self, result=0xABCD0000, cleanup=0):
        sp = self.uc.reg_read(UC_X86_REG_ESP)
        self.uc.reg_write(UC_X86_REG_EIP, self.read32(sp))
        self.uc.reg_write(UC_X86_REG_ESP, sp + 4 + cleanup)
        self.uc.reg_write(UC_X86_REG_EAX, result)
        self.uc.reg_write(UC_X86_REG_ECX, 0xCCCCCCCC)
        self.uc.reg_write(UC_X86_REG_EDX, 0xDDDDDDDD)

    def construct_file(self):
        assert self.uc.mem_read(self.arg(0), 9) == b"test.pal\0"
        self.file_object = self.uc.reg_read(UC_X86_REG_ECX)
        self.write32(self.file_object, VTABLE)
        self.trace.append(["construct_file"])
        self.ret(self.file_object, 4)

    def exists(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == self.file_object
        assert self.arg(0) == 0
        self.trace.append(["exists", False])
        self.ret(0xABCD0000 | int(self.scenario.get("exists", True)), 4)

    def size(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == self.file_object
        size = self.scenario.get("size", 768)
        self.trace.append(["size", size])
        self.ret(size)

    def allocate(self):
        # Both the original hook policy and core normalize zero to one byte.
        size = max(self.arg(0), 1)
        self.allocation_count += 1
        fail = self.allocation_count == self.scenario.get("fail_allocation", 0)
        self.trace.append(["allocate", size, not fail])
        if fail:
            self.ret(0)
            return
        assert size < 0x10000
        pointer = self.cursor
        self.cursor += (max(size, 1) + 15) & ~15
        self.allocations[pointer] = size
        self.ret(pointer)

    def read(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == self.file_object
        destination, count = self.arg(0), self.arg(1)
        assert self.allocations[destination] == max(count, 1)
        available = min(count, self.scenario.get("read_count", count))
        offset = self.scenario.get("offset", 0)
        payload = bytes((i + offset) % 256 for i in range(available))
        self.uc.mem_write(destination, payload)
        self.trace.append(["read", count, available])
        self.ret(available, 8)

    def deallocate(self):
        pointer = self.arg(0)
        assert pointer in self.allocations and pointer not in self.freed
        self.trace.append(["delete", self.allocations[pointer]])
        self.freed.append(pointer)
        self.ret()

    def construct_convert(self):
        pointer = self.uc.reg_read(UC_X86_REG_ECX)
        palette, reference, surface, shades, skip = [self.arg(i) for i in range(5)]
        assert self.allocations[pointer] == 0x188
        assert palette == reference == self.read32(PAL_OUT)
        assert self.read32(CONVERT_OUT) == 0xDEAD0200  # output write happens AFTER construction
        assert self.freed == [HEAP]  # raw file released BEFORE allocating/constructing Convert
        assert surface == SUPPORT + 0x800 and shades == 1 and skip == 0
        self.trace.append(["construct_convert", surface, shades, skip])
        # Deterministic external constructor result; we do NOT claim its body
        # was replaced or tested. Preserve a real-looking vtable and fields.
        self.write32(pointer, 0x7E0000)
        self.write32(pointer + 4, 2)
        self.ret(pointer, 20)

    def destroy_inline(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == self.file_object
        self.trace.append(["destroy_file"])
        self.ret()

    def destroy_virtual(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == self.file_object
        assert self.arg(0) == 0  # never delete the stack object
        self.trace.append(["destroy_file"])
        self.ret(self.file_object, 4)

    def memset(self):
        pointer, value, count = [self.arg(i) for i in range(3)]
        self.uc.mem_write(pointer, bytes([value & 255]) * count)
        self.ret(pointer)

    def memcpy(self):
        target, source, count = [self.arg(i) for i in range(3)]
        self.uc.mem_write(target, bytes(self.uc.mem_read(source, count)))
        self.ret(target)

    def unexpected(self, name):
        raise AssertionError(f"Unexpected external dependency: {name}")

    def on_instruction(self, uc, address, size, user):
        if address == STOP:
            uc.emu_stop()
            return
        if self.replacement:
            assert not (0x72ADE5 <= address < 0x72AF0F), "Executed original CreateFromFile body"
            assert not (0x4A3895 <= address < 0x4A38D0), "Executed original ReadWholeFile body"
        if address in self.hooks:
            self.hooks[address]()

    def run(self, whole_file=False):
        sp = STACK + 0x80000
        self.write32(sp, STOP)
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        for reg, value in PRESERVED.items():
            self.uc.reg_write(reg, value)
        if whole_file:
            self.file_object = FILE
            self.uc.reg_write(UC_X86_REG_ECX, FILE)
            self.uc.reg_write(UC_X86_REG_EDX, 0xBAADF00D)
            entry = 0x4A3890
        else:
            self.uc.reg_write(UC_X86_REG_ECX, NAME)
            self.uc.reg_write(UC_X86_REG_EDX, PAL_OUT)
            self.write32(sp + 4, CONVERT_OUT)
            entry = 0x72ADE0
        fault = None
        try:
            self.uc.emu_start(entry, STOP + 1, count=1000000)
        except UcError as error:
            fault = str(error)
        if fault:
            assert not whole_file and self.scenario.get("fail_allocation") == 2, (
                fault, hex(self.uc.reg_read(UC_X86_REG_EIP)), self.trace, self.scenario, self.replacement)
            assert self.read32(PAL_OUT) == 0 and self.read32(CONVERT_OUT) == 0xDEAD0200
        else:
            assert self.uc.reg_read(UC_X86_REG_EIP) == STOP, "Instruction limit reached"
            assert self.uc.reg_read(UC_X86_REG_ESP) == sp + (4 if whole_file else 8)
            for reg, value in PRESERVED.items():
                assert self.uc.reg_read(reg) == value, f"Callee-saved register {reg} changed"
        palette = self.read32(PAL_OUT)
        result = {"trace": self.trace, "palette": palette, "convert": self.read32(CONVERT_OUT),
                  "fault": bool(fault), "live": [(p, n) for p, n in self.allocations.items() if p not in self.freed]}
        if palette in self.allocations:
            colors = bytes(self.uc.mem_read(palette, 768))
            # Independent expected color values, including all high-bit inputs.
            offset = self.scenario.get("offset", 0)
            count = self.scenario.get("read_count", self.scenario.get("size", 768))
            expected = bytes((((i + offset) % 256 if i < count else 0xA5) * 4) & 255 for i in range(768))
            assert colors == expected
            result["palette_sha256"] = hashlib.sha256(colors).hexdigest()
        if whole_file:
            result["return"] = self.uc.reg_read(UC_X86_REG_EAX)
            pointer = result["return"]
            if pointer:
                result["payload"] = bytes(self.uc.mem_read(pointer, self.allocations[pointer])).hex()
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--dll", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    original, dll = pefile.PE(str(args.exe)), pefile.PE(str(args.dll))
    cases = [("palette_missing", {"exists": False}, False),
             ("palette_raw_allocation_failure", {"fail_allocation": 1}, False),
             ("palette_allocation_fault", {"fail_allocation": 2}, False),
             ("convert_allocation_failure", {"fail_allocation": 3}, False),
             ("palette_short_read", {"read_count": 37}, False),
             ("palette_extra_bytes", {"size": 800}, False)]
    cases += [(f"palette_channels_{i}", {"offset": i}, False) for i in range(256)]
    cases += [("whole_missing", {"exists": False}, True),
              ("whole_allocation_failure", {"fail_allocation": 1}, True),
              ("whole_empty", {"size": 0}, True),
              ("whole_short", {"read_count": 17}, True),
              ("whole_complete", {"size": 1024}, True)]
    results = []
    for name, scenario, whole in cases:
        expected = Machine(original, dll, scenario, False).run(whole)
        actual = Machine(original, dll, scenario, True).run(whole)
        assert actual == expected, (name, expected, actual)
        results.append({"name": name, "passed": True,
                        "palette_sha256": actual.get("palette_sha256"), "expected_fault": actual["fault"]})
    report = {"target_sha256": SHA, "dll_sha256": hashlib.sha256(args.dll.read_bytes()).hexdigest(),
              "method": "Unicorn execution of original entry and compiled replacement through patched original entry",
              "remaining_dependencies": ["CCFile constructor/virtual I/O/destructor", "allocator", "Convert constructor 0x48E740"],
              "scope": "72ADE0 and 4A3890 only; substituted shared services; not a Windows integration test",
              "passed": len(results), "cases": results}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Passed {len(results)} PAL/file x86 differential cases")


if __name__ == "__main__":
    main()
