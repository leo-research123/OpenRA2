#!/usr/bin/env python3
"""Compare fixed YR Rules methods with the actual Microsoft x86 core build.

Both sides consume real original INI nodes, built by the original parser. The
replacement executes the core's INI query methods. Only CRT allocation/string/
scanf primitives are controlled. No Rules or container method is mocked. This
is an instruction differential, not a Windows game-process installation test.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

import pefile
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from compare_ini_values import ValuesMachine
from compare_ini import INI, STACK, SUPPORT, STOP, SHA
from compare_image_pal import initialize_exception_chain, PRESERVED

OBJECT, DATA, SIZE = 0x2E00000, 0x2E10000, 0x18C0
ENTRIES = {
    'Construct': 0x665650, 'Destroy': 0x667A30, 'Invalid': 0x678850,
    'ElevationModel': 0x66D150, 'WallModel': 0x66D1F0,
    'MultiplayerDialogSettings': 0x671EA0, 'Maximums': 0x672230,
    'IQ': 0x674240, 'JumpjetControls': 0x6743D0, 'Difficulties': 0x674500,
    'Difficulty': 0x66D270,
    'ColorAdd': 0x66D480, 'Powerups': 0x673E80,
    'LandCharacteristics': 0x674000, 'Movies': 0x674550,
    'FindAnim': 0x422B20, 'FindMovie': 0x48DF30,
}


class RulesMachine(ValuesMachine):
    def __init__(self, original, dll, replacement, fill=0xCD):
        super().__init__(original)
        self.replacement = replacement
        self.mask, self.freed = bytearray(SIZE), []
        self.import_counts = {}
        self.queries = []
        # Windows CRT's initial x87/MXCSR state; _ftol may change x87 later.
        self.uc.reg_write(UC_X86_REG_FPCW, 0x027F)
        self.uc.reg_write(UC_X86_REG_MXCSR, 0x1F80)
        initialize_exception_chain(self.uc)
        base = dll.OPTIONAL_HEADER.ImageBase
        pe = dll if replacement else original
        self.module_range = (pe.OPTIONAL_HEADER.ImageBase,
                             pe.OPTIONAL_HEADER.ImageBase + pe.OPTIONAL_HEADER.SizeOfImage)
        self.uc.mem_map(base, (dll.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
        self.uc.mem_write(base, dll.get_memory_mapped_image())
        self.exports = {}
        for item in dll.DIRECTORY_ENTRY_EXPORT.symbols:
            if item.name:
                name = item.name.decode()
                name = name.split('@')[1] if name.startswith('@') else name
                self.exports[name] = base + item.address
        self.hooks[0x7C8B3D] = self.free
        self.hooks[0x7C93E8] = self.free
        self.hooks[0x7C9D66] = self.atof
        # Return a CRT double using the real x87 instruction/stack convention.
        self.uc.mem_write(SUPPORT + 0xE000, b'\xdd\x05' + struct.pack('<I', SUPPORT + 0xE100) + b'\xc3')
        for descriptor in dll.DIRECTORY_ENTRY_IMPORT:
            for item in descriptor.imports:
                name = (item.name or b'ordinal').decode()
                address = SUPPORT + 0x4000 + len(self.hooks) * 16
                self.write32(item.address, address)
                action = {
                    'malloc': self.allocate, 'free': self.free, 'memset': self.memset,
                    'strchr': self.strchr, 'strlen': lambda: self.ret(len(self.string(self.arg(0)))),
                    '__stdio_common_vsscanf': self.vsscanf,
                    '__stdio_common_vsprintf': self.vsprintf,
                    'memcpy': self.memcpy, 'memmove': self.memcpy,
                    '_stricmp': self.compare, 'atof': self.atof,
                    'strcmp': self.compare_case_sensitive,
                }.get(name, lambda name=name: self.unexpected(name))
                self.hooks[address] = lambda action=action, name=name: self.import_call(name, action)
        self.uc.mem_write(OBJECT, bytes([fill]) * SIZE)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write)
        self.lists = self.offsets('RulesLists')
        self.pointers = self.offsets('RulesPointers')
        self.pointer_lists = self.offsets('RulesPointerLists')

    def import_call(self, name, action):
        self.import_counts[name] = self.import_counts.get(name, 0) + 1
        action()

    def compare_case_sensitive(self):
        left, right = self.string(self.arg(0)), self.string(self.arg(1))
        self.ret((left > right) - (left < right))

    def hook(self, uc, address, size, data):
        if getattr(self, 'replacement', False) and address in ENTRIES.values():
            raise AssertionError(f'replacement called original Rules method {address:X}')
        if hasattr(self, 'queries') and address in (0x5276D0, 0x5295F0, 0x5283D0):
            self.queries.append((address, self.string(self.arg(0)).decode(),
                                 self.string(self.arg(1)).decode()))
        super().hook(uc, address, size, data)

    def on_write(self, uc, access, address, size, value, data):
        if OBJECT <= address < OBJECT + SIZE:
            self.mask[address - OBJECT:address - OBJECT + size] = bytes([1]) * size

    def offsets(self, name):
        address, output = self.exports[name], []
        while self.read32(address) != 0xFFFFFFFF:
            output.append(self.read32(address)); address += 4
        return output

    def free(self):
        self.freed.append(self.arg(0)); self.ret()

    def scan_float(self, text, output):
        match = re.match(rb'\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)', text)
        if not match:
            return 0
        self.uc.mem_write(output, struct.pack('<f', float(match[1])))
        return 1

    def sscanf(self):
        if self.string(self.arg(1)) == b'%f':
            self.ret(self.scan_float(self.string(self.arg(0)), self.arg(2)))
        else:
            super().sscanf()

    def vsscanf(self):
        # uint64 options, buffer, size_t count, format, locale, va_list.
        fmt, args = self.string(self.arg(4)), self.arg(6)
        text = self.string(self.arg(2))
        if fmt == b'%f':
            self.ret(self.scan_float(text, self.read32(args)))
            return
        assert fmt == b'%d,%d,%d', fmt
        pos = count = 0
        for i in range(3):
            if i:
                if text[pos:pos + 1] != b',': break
                pos += 1
            match = re.match(rb'\s*([+-]?\d+)', text[pos:])
            if not match: break
            self.write32(self.read32(args + i * 4), int(match[1]))
            pos += len(match[0]); count += 1
        self.ret(count)

    def vsprintf(self):
        fmt, args = self.string(self.arg(4)), self.arg(6)
        assert fmt == b'%d,%d,%d', fmt
        text = fmt % tuple(self.signed32(args + i * 4) for i in range(3))
        assert len(text) < self.arg(3), 'format buffer too small'
        self.uc.mem_write(self.arg(2), text + b'\0'); self.ret(len(text))

    def atof(self):
        match = re.match(rb'\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)', self.string(self.arg(0)))
        self.uc.mem_write(SUPPORT + 0xE100, struct.pack('<d', float(match[1]) if match else 0.0))
        self.uc.reg_write(UC_X86_REG_EIP, SUPPORT + 0xE000)

    def invoke(self, name, *args, this=OBJECT, edx=0):
        sp = STACK + 0xF0000
        for i, value in enumerate((STOP,) + args): self.write32(sp + 4 * i, value)
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.uc.reg_write(UC_X86_REG_ECX, this)
        self.uc.reg_write(UC_X86_REG_EDX, edx)
        for reg, value in PRESERVED.items(): self.uc.reg_write(reg, value)
        entry = self.exports['Rules' + name] if self.replacement else ENTRIES[name]
        self.uc.emu_start(entry, STOP + 1, count=2000000)
        assert self.uc.reg_read(UC_X86_REG_EIP) == STOP, (name, 'instruction limit', hex(self.uc.reg_read(UC_X86_REG_EIP)))
        assert self.uc.reg_read(UC_X86_REG_ESP) == sp + 4 * (1 + len(args)), (name, 'stack cleanup')
        for reg, value in PRESERVED.items(): assert self.uc.reg_read(reg) == value, (name, 'preserved register')
        return self.uc.reg_read(UC_X86_REG_EAX)

    def setup_lists(self, pointer, other, borrowed=False):
        for index, offset in enumerate(self.lists):
            address = DATA + index * 32
            data = (pointer, other, pointer, other, pointer, other)
            self.uc.mem_write(address, struct.pack('<6I', *data))
            self.write32(OBJECT + offset + 4, address)
            self.write32(OBJECT + offset + 8, 6)
            self.uc.mem_write(OBJECT + offset + 12, bytes([1, not borrowed or index % 3 != 1]))
            self.write32(OBJECT + offset + 16, 6)


def compare(a, b, label, lists=False):
    left, right = bytearray(a.uc.mem_read(OBJECT, SIZE)), bytearray(b.uc.mem_read(OBJECT, SIZE))
    mask = bytearray(a.mask)
    for offset in a.lists:
        # Vtables must belong to their respective PE; virtual dispatch is
        # exercised below by invalidation. Pointer identity is not serialized.
        for machine in (a, b):
            vtable = machine.read32(OBJECT + offset)
            begin, end = machine.module_range
            assert begin <= vtable < end, (label, 'vtable module', hex(offset), hex(vtable))
            assert begin <= machine.read32(vtable + 16) < end, (label, 'FindItemIndex dispatch')
        mask[offset:offset + 4] = bytes(4)
    differences = [hex(i) for i in range(SIZE) if mask[i] and left[i] != right[i]]
    assert not differences, (label, differences[:32])
    if lists:
        assert a.uc.mem_read(DATA, len(a.lists) * 32) == b.uc.mem_read(DATA, len(b.lists) * 32), (label, 'list contents')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.original.read_bytes()).hexdigest() == SHA
    dll_bytes = args.dll.read_bytes()
    dll_sha = hashlib.sha256(dll_bytes).hexdigest()
    original, dll = pefile.PE(str(args.original)), pefile.PE(data=dll_bytes)
    rows = []
    def record(name):
        rows.append({'case': name, 'passed': True}); print('PASS', name, flush=True)
    def pair(fill=0xCD):
        a, b = RulesMachine(original, dll, False, fill), RulesMachine(original, dll, True, fill)
        a.invoke('Construct'); b.invoke('Construct')
        compare(a, b, 'construct')
        return a, b

    for fill in (0, 0xCD, 0xA5):
        a, b = pair(fill)
        record(f'construction from {fill:02X} storage: all target-written bytes, {len(a.lists)} containers')
    a, b = pair()
    pointer, other = DATA + 0x10000, DATA + 0x10100
    for m in (a, b):
        for offset in m.pointers: m.write32(OBJECT + offset, pointer)
        m.setup_lists(pointer, other)
    for removed in (0, 1, 0, 1):
        a.invoke('Invalid', pointer, removed); b.invoke('Invalid', pointer, removed)
        compare(a, b, 'invalidation', lists=True)
    record('all declared scalar pointers and pointer/integer lists; duplicate removal and both flags')
    for m in (a, b): m.setup_lists(pointer, other, borrowed=True)
    a.invoke('Destroy'); b.invoke('Destroy')
    compare(a, b, 'destruction', lists=True)
    assert a.freed == b.freed
    record(f'destruction: same ordered releases of {len(a.freed)} owned buffers; borrowed buffers retained')

    a, b = pair()
    for m in (a, b):
        m.write32(OBJECT + 4012, pointer)  # CrushWarhead
        m.write32(OBJECT + 4008, other)    # C4Warhead
        m.write32(OBJECT + 1024, pointer)  # PrerequisiteProcAlternate
        m.write32(OBJECT + 2216, other)    # ThirdPowerPlant
    a.invoke('Invalid', pointer, 1); b.invoke('Invalid', pointer, 1)
    compare(a, b, 'cross-member clears with distinct references')
    assert a.read32(OBJECT + 4012) == pointer and a.read32(OBJECT + 4008) == 0
    assert a.read32(OBJECT + 1024) == pointer and a.read32(OBJECT + 2216) == 0
    record('two original cross-member invalidation quirks with distinct object identities')

    a, b = pair()
    readers = ['ElevationModel', 'WallModel', 'MultiplayerDialogSettings', 'Maximums',
               'IQ', 'JumpjetControls', 'Difficulties']
    inputs = [b'[Unrelated]\nKey=1\n', b'''[IQ]
MaxIQLevels=8
SuperWeapons=7
Production=9
[ElevationModel]
ElevationIncrement=5
ElevationIncrementBonus=1.25
ElevationBonusCap=80%
[WallModel]
AlliedWallTransparency=yes
WallPenetratorThreshold=.75
[JumpjetControls]
TurnRate=6
Speed=19
Climb=2.5
CruiseHeight=900
Acceleration=1.4
WobblesPerSecond=.8
WobbleDeviation=15
[MultiplayerDialogSettings]
Money=12000
AIDifficulty=2
AllyChangeAllowed=no
SuperWeaponsAllowed=no
FogOfWar=yes
[Maximums]
Players=12
[Easy]
Firepower=9
FirePower=1.5
Armor=1.4
ContentScan=yes
[Normal]
BuildTime=.8
[Difficult]
DestroyWalls=no
''', b'''[IQ]
SuperWeapons=3
Production=invalid
[WallModel]
WallPenetratorThreshold=50%
[Easy]
Armor=.9
[MultiplayerDialogSettings]
Money=9000
''']
    for index, text in enumerate(inputs):
        for m in (a, b): m.load(text)
        for name in readers:
            expected, actual = a.invoke(name, INI) & 255, b.invoke(name, INI) & 255
            assert expected == actual, (name, 'return', expected, actual)
            compare(a, b, f'{name} input {index}')
        record(f'seven readers, input {index}: absent sections / base values / partial overlays')
    for section in ('Easy', 'Normal', 'Difficult', 'Missing'):
        results = []
        for m in (a, b):
            name = m.cstring(section.encode())
            results.append(m.invoke('Difficulty', name, this=INI, edx=OBJECT + 5432) & 255)
        assert results[0] == results[1]
        compare(a, b, 'fastcall difficulty ' + section)
    record('difficulty helper fastcall registers, output state and ContentScan return')

    # Build an exhaustive input from keys observed while real original readers
    # execute, independently of the replacement's strings or member mappings.
    marker = b''.join(b'[' + section.encode() + b']\n_Present=1\n' for section in
        ('ElevationModel', 'WallModel', 'MultiplayerDialogSettings', 'Maximums',
         'IQ', 'JumpjetControls', 'Easy', 'Normal', 'Difficult'))
    a.load(marker); b.load(marker); a.queries.clear()
    for name in readers:
        a.invoke(name, INI); b.invoke(name, INI)
        compare(a, b, name + ' marker sections')
    queries = list(dict.fromkeys(a.queries))
    for variant in (0, 1):
        sections = {}
        for index, (address, section, key) in enumerate(queries):
            value = ('yes' if variant else 'no') if address == 0x5295F0 else (
                str(37 + index + variant) if address == 0x5276D0 else f'{index + variant + 2}.125')
            sections.setdefault(section, []).append(key + '=' + value)
        text = '\n'.join('[' + section + ']\n' + '\n'.join(values)
                         for section, values in sections.items()).encode() + b'\n'
        for m in (a, b): m.load(text)
        for name in readers:
            assert a.invoke(name, INI) & 255 == b.invoke(name, INI) & 255, (name, 'full corpus return')
            compare(a, b, name + ' full original key corpus')
    record(f'all {len(queries)} section/key reads observed in original code, both bool values')

    # The original-game composition must resolve the singleton to the EXE slot.
    result = b.call(b.exports['RulesInstanceSlot'], 0)
    assert result == 0x8871E0, hex(result)
    record('original Rules singleton resolves to 0x8871E0')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'sha256': SHA, 'replacement_sha256': dll_sha,
        'method': 'x86 instruction differential',
        'cases': rows, 'replacement_crt_calls': b.import_counts}, indent=2) + '\n')


if __name__ == '__main__': main()
