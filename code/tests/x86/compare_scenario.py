#!/usr/bin/env python3
"""Execute fixed YR and compiled Scenario/Randomizer methods in x86 Unicorn.

Only the shared OS clock, allocator, text resource and notification boundaries
are controlled. Scenario/RNG methods are never mocked. This does not install a
DLL or certify full scene/gameplay behavior. The destructor reference is the
normal-exit inline block, not the erroneous YRpp Rules destructor address.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import random

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from compare_image_pal import initialize_exception_chain, PRESERVED

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"
OBJECT, SUPPORT, STACK, STOP = 0x2000000, 0xD00000, 0x3000000, 0xD0FFF0
ENTRIES = {"RandomConstruct": 0x65C6D0, "RandomNext": 0x65C780, "RandomRange": 0x65C7E0,
           "ScenarioConstruct": 0x6832C0, "ScenarioReset": 0x683610,
           "ScenarioGlobal": 0x689670, "ScenarioLocal": 0x689910,
           "ScenarioID": 0x68BCB0, "ScenarioWaypoint": 0x68BD80,
           "CellLighting": 0x484680, "ScenarioLighting": 0x555AC0,
           "ScenarioUpdateLighting": 0x53C280, "ScenarioRecalcLighting": 0x53AD00,
           "ScenarioCellLighting": 0x4AE4C0, "ScenarioHashLighting": 0x53AC80}
ENTRIES.update(ScenarioSave=0x689310, ScenarioLoad=0x689470)
ENTRIES.update(ScenarioPause=0x683EB0, ScenarioResume=0x683FB0)
ENTRIES.update(ScenarioAssignHouses=0x687F10, HouseInitialize=0x4FCE00, NodeCountry=0x696F90, NodeStart=0x696F50)


class Machine:
    def __init__(self, original, dll, replacement):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.replacement = replacement
        self.freed, self.notifications, self.clock_calls = [], [], 0
        self.clock = 16000
        self.render_calls, self.palettes, self.palette_index = [], [], 0
        self.stream, self.stream_cursor, self.transfers, self.swizzles = bytearray(), 0, [], []
        self.pause_events = []
        self.houses, self.house_events = [], []
        self.step = 16
        self.heap = OBJECT + 0x10000
        for pe in (original, dll):
            base = pe.OPTIONAL_HEADER.ImageBase
            self.uc.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
            self.uc.mem_write(base, pe.get_memory_mapped_image())
        self.uc.mem_map(OBJECT, 0x100000)
        self.uc.mem_map(STACK, 0x100000)
        self.uc.mem_map(SUPPORT, 0x10000)
        initialize_exception_chain(self.uc)
        # Same TEB created by initialize_exception_chain. Supply static TLS as
        # the Windows loader would, without running CRT or game initialization.
        self.uc.mem_map(SUPPORT + 0x20000, 0x20000)
        self.write32(SUPPORT + 0x12000 + 0x2C, SUPPORT + 0x20000)
        if hasattr(dll, "DIRECTORY_ENTRY_TLS"):
            tls = dll.DIRECTORY_ENTRY_TLS.struct
            self.write32(tls.AddressOfIndex, 0)
            self.write32(SUPPORT + 0x20000, SUPPORT + 0x21000)
            self.uc.mem_write(SUPPORT + 0x21000, bytes(self.uc.mem_read(
                tls.StartAddressOfRawData, tls.EndAddressOfRawData - tls.StartAddressOfRawData)))
        self.exports = {e.name.decode().split('@')[1]: dll.OPTIONAL_HEADER.ImageBase + e.address
                        for e in dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name and e.name.startswith(b'@')}
        self.hooks = {0x7C8E17: self.allocate, 0x7C8B3D: self.deallocate,
                      0x7CA489: self.wcscpy, 0x734E60: self.text,
                      0x6E57F0: lambda: self.notify(True), 0x6E5820: lambda: self.notify(False),
                      0x626690: self.palette, 0x626C60: lambda: self.ret(2),
                      0x4F42F0: self.redraw, SUPPORT + 0x1800: self.colors,
                      SUPPORT + 0x1820: self.read_stream, SUPPORT + 0x1840: self.write_stream,
                      0x6CF240: self.swizzle}
        self.hooks.update({0x4F54A0: self.create_house,
            0x50B840: lambda: self.house_update('color'), 0x50BA00: lambda: self.house_update('laser'),
            0x4F6EC0: lambda: self.house_update('handicap'), 0x7C8D20: self.stricmp,
            0x7B66D0: self.wstring, 0x7B7F90: self.narrow_string,
            0x7B6760: self.ret, 0x7B46F0: self.ret,
            0x7B5400: lambda: self.ret(self.read32(self.uc.reg_read(UC_X86_REG_ECX)))})
        for address, name, kind in (
                (0x406F00, 'suspend audio', ''), (0x406F40, 'resume audio', ''),
                (0x5BF500, 'input', 'bool'), (0x7241A0, 'tooltip', 'tooltip'),
                (0x6938C0, 'release capture', 'map'), (0x5BDC80, 'cursor', 'cursor'),
                (0x5BDAA0, 'restore cursor', 'map'), (0x4F4480, 'render', 'map'),
                (SUPPORT + 0x1860, 'hide cursor', 'mouse'), (SUPPORT + 0x1880, 'show cursor', 'mouse')):
            self.hooks[address] = lambda name=name, kind=kind: self.pause_dependency(name, kind)
        for pe in (original, dll):
            for desc in pe.DIRECTORY_ENTRY_IMPORT:
                for imported in desc.imports:
                    name = (imported.name or b"ordinal").decode()
                    if pe is original and name != 'timeGetTime':
                        continue
                    address = SUPPORT + 0x2000 + 16 * len(self.hooks)
                    self.write32(imported.address, address)
                    self.hooks[address] = {
                        'timeGetTime': self.time, 'malloc': self.allocate, 'free': self.deallocate,
                        'memset': self.memset, 'memcpy': self.memcpy,
                        'memchr': self.memchr,
                        'strncpy': self.strncpy, 'wcsncpy': lambda: self.strncpy(wide=True),
                        'strcpy': self.strcpy, 'wcscpy': self.wcscpy, '_stricmp': self.stricmp,
                    }.get(name, lambda name=name: self.unexpected(name))
        self.uc.hook_add(UC_HOOK_CODE, self.on_code)
        self.write32(0xA8B230, OBJECT)
        self.write32(0xA8B238, 0)
        self.write32(0xA8ED84, 123)
        self.write32(0xB05458, 0)
        self.uc.mem_write(SUPPORT + 0x100, "Skirmish fixture\0".encode('utf-16le'))
        self.write32(SUPPORT + 0x400, SUPPORT + 0x500)
        self.write32(SUPPORT + 0x500 + 12, SUPPORT + 0x1820)
        self.write32(SUPPORT + 0x500 + 16, SUPPORT + 0x1840)
        self.write32(SUPPORT + 0x700, SUPPORT + 0x600)
        self.write32(SUPPORT + 0x600 + 12, SUPPORT + 0x1860)
        self.write32(SUPPORT + 0x600 + 16, SUPPORT + 0x1880)
        self.write32(0x887640, SUPPORT + 0x700)
        self.write32(0x87E758, SUPPORT + 0x800)

    def write32(self, address, value): self.uc.mem_write(address, struct.pack('<I', value & 0xffffffff))
    def read32(self, address): return struct.unpack('<I', self.uc.mem_read(address, 4))[0]
    def arg(self, i): return self.read32(self.uc.reg_read(UC_X86_REG_ESP) + 4 * (i + 1))
    def ret(self, value=0, cleanup=0):
        sp = self.uc.reg_read(UC_X86_REG_ESP)
        self.uc.reg_write(UC_X86_REG_EIP, self.read32(sp))
        self.uc.reg_write(UC_X86_REG_ESP, sp + 4 + cleanup)
        self.uc.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
    def on_code(self, uc, address, size, data):
        if address == STOP:
            uc.emu_stop()
        elif address in self.hooks:
            self.hooks[address]()
        elif self.replacement and address in ENTRIES.values():
            raise AssertionError(f'replacement called original Scenario/RNG method {address:X}')
    def unexpected(self, name): raise AssertionError('unexpected import: ' + name)
    def time(self):
        now = self.clock; self.clock = (self.clock + self.step) & 0xffffffff
        self.clock_calls += 1; self.ret(now)
    def allocate(self):
        size = self.arg(0); address = self.heap; self.heap += (size + 15) & ~15
        assert self.heap < OBJECT + 0x100000
        self.ret(address)
    def deallocate(self): self.freed.append(self.arg(0)); self.ret()
    def memset(self):
        address, value, count = self.arg(0), self.arg(1), self.arg(2)
        if count: self.uc.mem_write(address, bytes([value & 255]) * count)
        self.ret(address)
    def memcpy(self):
        address, source, count = self.arg(0), self.arg(1), self.arg(2)
        if count: self.uc.mem_write(address, bytes(self.uc.mem_read(source, count)))
        self.ret(address)
    def memchr(self):
        address, value, count = self.arg(0), self.arg(1) & 255, self.arg(2)
        index = bytes(self.uc.mem_read(address, count)).find(bytes([value]))
        self.ret(address + index if index >= 0 else 0)
    def read_stream(self):
        assert self.arg(0) == SUPPORT + 0x400
        address, count, transferred = self.arg(1), self.arg(2), self.arg(3)
        data = self.stream[self.stream_cursor:self.stream_cursor + count]
        self.uc.mem_write(address, bytes(data)); self.stream_cursor += len(data)
        self.transfers.append(['read', count])
        if transferred: self.write32(transferred, len(data))
        self.ret(0 if len(data) == count else 1, cleanup=16)
    def write_stream(self):
        assert self.arg(0) == SUPPORT + 0x400
        address, count, transferred = self.arg(1), self.arg(2), self.arg(3)
        self.stream.extend(self.uc.mem_read(address, count))
        self.transfers.append(['write', count])
        if transferred: self.write32(transferred, count)
        self.ret(cleanup=16)
    def swizzle(self):
        assert self.arg(0) == 0xB0C110
        slot = self.arg(1)
        self.swizzles.append([self.read32(slot), slot])
        self.write32(slot, 0); self.ret(cleanup=8)
    def pause_dependency(self, name, kind):
        event, cleanup = [name], 0
        if kind == 'bool': event.append(self.uc.reg_read(UC_X86_REG_ECX) & 255)
        if kind in ('map', 'cursor'): assert self.uc.reg_read(UC_X86_REG_ECX) == 0x87F7E8
        if kind == 'mouse': assert self.uc.reg_read(UC_X86_REG_ECX) == SUPPORT + 0x700
        if kind == 'tooltip':
            assert self.uc.reg_read(UC_X86_REG_ECX) == SUPPORT + 0x900
            event.append(self.arg(0) & 255); cleanup = 4
        if kind == 'cursor': event.extend([self.arg(0), self.arg(1) & 255]); cleanup = 8
        self.pause_events.append(event); self.ret(cleanup=cleanup)
    def wcscpy(self):
        dest, source, index = self.arg(0), self.arg(1), 0
        while True:
            unit = bytes(self.uc.mem_read(source + index, 2)); self.uc.mem_write(dest + index, unit)
            if unit == b'\0\0': break
            index += 2
        self.ret(dest)
    def string_bytes(self, address):
        result = bytearray()
        while self.uc.mem_read(address + len(result), 1)[0]: result.extend(self.uc.mem_read(address + len(result), 1))
        return bytes(result)
    def strcpy(self):
        self.uc.mem_write(self.arg(0), self.string_bytes(self.arg(1)) + b'\0'); self.ret(self.arg(0))
    def strncpy(self, wide=False):
        dest, source, count = self.arg(0), self.arg(1), self.arg(2)
        width = 2 if wide else 1
        data, ended = bytearray(), False
        for i in range(count):
            unit = bytes(width) if ended else bytes(self.uc.mem_read(source + width * i, width))
            ended = ended or unit == bytes(width); data.extend(unit)
        if data: self.uc.mem_write(dest, bytes(data))
        self.ret(dest)
    def stricmp(self):
        left, right = self.string_bytes(self.arg(0)).lower(), self.string_bytes(self.arg(1)).lower()
        self.ret((left > right) - (left < right))
    def wstring(self):
        self.write32(self.uc.reg_read(UC_X86_REG_ECX), self.arg(0)); self.ret(cleanup=4)
    def narrow_string(self):
        source = self.read32(self.uc.reg_read(UC_X86_REG_EDX))
        units = bytes(self.uc.mem_read(source, 40)); output = bytearray()
        for i in range(20):
            if units[2*i:2*i+2] == b'\0\0': break
            output.append(units[2*i])
        self.uc.mem_write(SUPPORT + 0xA00, bytes(output) + b'\0')
        target = self.uc.reg_read(UC_X86_REG_ECX)
        self.write32(target, SUPPORT + 0xA00); self.ret(target)
    def create_house(self):
        house, country = self.uc.reg_read(UC_X86_REG_ECX), self.arg(0)
        index = len(self.houses); self.houses.append(house)
        self.write32(house + 0x30, index); self.write32(house + 0x34, country)
        self.house_events.append(['create', self.read32(country + 0xb8)])
        self.ret(house, cleanup=4)
    def house_update(self, name):
        house = self.uc.reg_read(UC_X86_REG_ECX)
        event = [name, self.read32(house + 0x30)]
        if name == 'handicap': event.append(self.arg(0))
        self.house_events.append(event); self.ret(cleanup=4 if name == 'handicap' else 0)
    def text(self): self.ret(SUPPORT + 0x100, 8)
    def notify(self, global_):
        self.notifications.append([global_, self.uc.reg_read(UC_X86_REG_ECX)])
        self.ret()
    def palette(self):
        iterator = self.uc.reg_read(UC_X86_REG_ECX)
        if self.palette_index == 0:
            assert bytes(self.uc.mem_read(iterator, 9)) == bytes(9)
        result = self.palettes[self.palette_index] if self.palette_index < len(self.palettes) else 0
        self.palette_index += 1
        self.render_calls.append(['palette', result])
        self.ret(result)
    def colors(self):
        self.render_calls.append(['colors', self.uc.reg_read(UC_X86_REG_ECX), *[self.arg(i) for i in range(4)]])
        self.ret(cleanup=16)
    def redraw(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == 0x87F7E8
        self.render_calls.append(['redraw', self.arg(0)])
        self.pause_events.append(['redraw', self.arg(0)])
        self.ret(cleanup=4)
    def call(self, name, args=(), ecx=OBJECT, edx=0, instruction_limit=1000000):
        sp = STACK + 0xF0000
        self.uc.mem_write(sp, struct.pack('<' + 'I' * (1 + len(args)), STOP, *[x & 0xffffffff for x in args]))
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.uc.reg_write(UC_X86_REG_ECX, ecx & 0xffffffff)
        self.uc.reg_write(UC_X86_REG_EDX, edx & 0xffffffff)
        for register, value in PRESERVED.items(): self.uc.reg_write(register, value)
        entry = self.exports[name] if self.replacement else ENTRIES[name]
        self.uc.emu_start(entry, STOP, count=instruction_limit)
        assert self.uc.reg_read(UC_X86_REG_EIP) == STOP, (name, 'instruction limit')
        assert self.uc.reg_read(UC_X86_REG_ESP) == sp + 4 * (1 + len(args)), (name, 'stack')
        for register, value in PRESERVED.items(): assert self.uc.reg_read(register) == value, (name, 'register')
        return self.uc.reg_read(UC_X86_REG_EAX)
    def state(self):
        return normalize_record(self.uc.mem_read(OBJECT, 0x3740))


def normalize_record(data):
        state = bytearray(data)
        # Empty clock functors occupy a byte plus padding, copied from an
        # unspecified original stack temporary. They carry no clock state.
        for offset in (0x614, 0x620, 0x11e8, 0x1218, 0x1224, 0x1230, 0x123c, 0x1248, 0x34c0):
            state[offset + 4:offset + 8] = bytes(4)
        # Pointer identities differ across executable modules. Check their
        # content/dispatch separately, not as serialized addresses here.
        for offset in (0x34d4, 0x34f0, 0x350c, 0x11f4): state[offset:offset+4] = bytes(4)
        return bytes(state)


def compare(a, b, label):
    x, y = a.state(), b.state()
    differences = [hex(i) for i in range(len(x)) if x[i] != y[i]]
    assert x == y, (label, differences[:30])
    assert a.clock_calls == b.clock_calls, (label, 'clock calls', a.clock_calls, b.clock_calls)
    assert a.notifications == b.notifications, (label, 'notifications')


def lighting_cases(original, dll):
    a, b = Machine(original, dll, False), Machine(original, dll, True)
    cells, slots, registry = OBJECT + 0x8000, OBJECT + 0x20000, OBJECT + 0x60000
    for m in (a, b):
        m.call('ScenarioConstruct')
        m.write32(0x87F7E8 + 0xf4, 2)
        m.write32(0x87F7E8 + 0x13c, slots)
        m.write32(slots + 1025 * 4, cells)
        m.write32(slots + 514 * 4, cells + 0x200)
        # A third, null cell terminates the real MapClass iterator.
        for i in range(2):
            m.write32(cells + i * 0x200 + 0x104, 0x10000)
            m.uc.mem_write(cells + i * 0x200 + 0x11b, bytes([i + 1]))
        # Real vector/scheme representations, with only the downstream
        # renderer's virtual UpdateColors boundary controlled by this test.
        m.write32(registry, SUPPORT + 0x1800)
        for i in range(8):
            light, scheme = registry + 0x100 + i * 16, registry + 0x1000 + i * 0x400
            m.write32(light, registry - 4)
            m.write32(scheme + 780, light)
            m.write32(scheme + 784, 1 if i == 3 else 2)
        for vector, indices, light in ((0x87F698, [0, 1], True), (0xB054D0, [2, 3], False),
                                       (registry + 0x8000, [4, 5], False), (registry + 0x8100, [6, 7], False)):
            data = registry + 0x9000 + (vector & 0xfff)
            m.write32(vector + 4, data)
            m.write32(vector + 16, len(indices))
            for i, value in enumerate(indices):
                m.write32(data + i * 4, registry + (0x100 + value * 16 if light else 0x1000 + value * 0x400))
        m.palettes = [registry + 0x8000, registry + 0x8100]
    rng = random.Random(0x484680)
    for index in range(250):
        ambient = rng.choice([0, 100, -1, 0x7fffffff, 0x80000000, rng.getrandbits(32)])
        parameters = [[rng.getrandbits(32) for _ in range(5)] for _ in range(4)]
        cell_data = struct.pack('<IH', rng.getrandbits(32), rng.getrandbits(16))
        height = rng.getrandbits(8)
        for m in (a, b):
            m.write32(OBJECT + 0x352c, ambient)
            for offset, lighting in zip((0x3534, 0x354c, 0x3564, 0x3580), parameters):
                m.uc.mem_write(OBJECT + offset, struct.pack('<5I', *lighting))
            m.uc.mem_write(cells + 0x104, cell_data)
            m.uc.mem_write(cells + 0x11b, bytes([height]))
            m.write32(0xA9FAB4, index & 1)
            m.write32(0xA9FAC0, (index >> 1) % 6)
            m.write32(0xA9FABC, (index >> 3) % 3)
            m.call('CellLighting', ecx=cells)
        assert a.uc.mem_read(cells, 0x200) == b.uc.mem_read(cells, 0x200), ('cell', index)
    for quality in (-1, 0, 1, 2, 3, 7):
        for values in ((-1, 500, 1001), (0, 31, 1000), (0x80000000, 0x7fffffff, 999)):
            for alias in (False, True):
                for m in (a, b):
                    m.write32(0xA8EB78, quality)
                    m.uc.mem_write(cells, struct.pack('<3I', *[v & 0xffffffff for v in values]))
                    m.call('ScenarioLighting', (cells + (0 if alias else 8),), ecx=cells, edx=cells + 4)
                assert a.uc.mem_read(cells, 12) == b.uc.mem_read(cells, 12), ('quality', quality, values, alias)
    for storm in (0, 1):
        for dominator in range(6):
            for nuke in range(3):
                for chrono in (0, 1):
                    for m in (a, b):
                        m.write32(0xA9FAB4, storm); m.write32(0xA9FAC0, dominator)
                        m.write32(0xA9FABC, nuke); m.write32(0xA9FAB0, chrono)
                        m.render_calls.clear(); m.palette_index = 0
                        m.call('ScenarioUpdateLighting')
                    assert a.render_calls == b.render_calls, ('render order', storm, dominator, nuke, chrono)
                    assert a.uc.mem_read(cells, 0x400) == b.uc.mem_read(cells, 0x400), 'map cell values'
                    compare(a, b, 'lighting')
    return {'case':'Lighting: 250 cell states, 36 quantization/alias cases, 72 effect combinations and renderer order', 'passed':True}


def stream_cases(original, dll):
    for running in (False, True):
        a, b = Machine(original, dll, False), Machine(original, dll, True)
        for m in (a, b):
            m.call('ScenarioConstruct')
            for i, offset in enumerate((0x34d4, 0x34f0, 0x350c)):
                m.write32(OBJECT + offset + 4, OBJECT + 0x8000 + i * 0x200)
                m.write32(OBJECT + offset + 8, 30)
                m.write32(OBJECT + offset + 16, 25 if i == 0 else 3)
                m.write32(OBJECT + offset + 24, 71 + i)
                for j in range(30): m.write32(OBJECT + 0x8000 + i * 0x200 + 4 * j, 0x9999 + i * 100 + j)
            m.write32(OBJECT + 0x614, 950 if running else -1)
            m.write32(OBJECT + 0x61c, 77)
            m.uc.mem_write(OBJECT + 0x11f8, b'GUI:Timer\0')
            m.call('ScenarioSave', (SUPPORT + 0x400,))
        assert normalize_record(a.stream[:0x3740]) == normalize_record(b.stream[:0x3740]), 'saved record'
        assert a.stream[0x3740:] == b.stream[0x3740:], 'saved list payload'
        assert a.transfers == b.transfers, 'write calls'
        compare(a, b, 'save timers')
        for m in (a, b): m.call('ScenarioLoad', (SUPPORT + 0x400,))
        compare(a, b, 'load record')
        assert a.transfers == b.transfers and a.stream_cursor == b.stream_cursor == len(a.stream), 'read calls'
        assert a.swizzles == b.swizzles and len(a.swizzles) == 25, 'stable swizzle slots'
        for offset in (0x34d4, 0x34f0, 0x350c):
            count = a.read32(OBJECT + offset + 16)
            assert a.uc.mem_read(a.read32(OBJECT + offset + 4), count * 4) == b.uc.mem_read(b.read32(OBJECT + offset + 4), count * 4)
    return {'case':'Scenario stream: full x86 record, three lists, swizzle and running/paused timer paths', 'passed':True}


def pause_cases(original, dll):
    for mode in (0, 3, 5):
        for ticking in (False, True):
            for tips in (False, True):
                a, b = Machine(original, dll, False), Machine(original, dll, True)
                for m in (a, b):
                    m.call('ScenarioConstruct')
                    m.write32(0xA8B238, mode)
                    m.write32(OBJECT + 0x614, 500 if ticking else -1)
                    m.write32(SUPPORT + 0x800, 0x42)
                    m.write32(SUPPORT + 0x808, 0x87654321)
                    m.write32(0x887368, SUPPORT + 0x900 if tips else 0)
                    m.write32(0xA8EB80, 1)
                for name in ('ScenarioPause', 'ScenarioPause', 'ScenarioResume', 'ScenarioResume'):
                    a.call(name); b.call(name)
                    compare(a, b, name)
                    assert a.pause_events == b.pause_events, (name, 'pause dependency order', a.pause_events, b.pause_events)
                    assert a.read32(0x83D834) == b.read32(0x83D834), 'saved volume'
                    assert a.uc.mem_read(SUPPORT + 0x800, 32) == b.uc.mem_read(SUPPORT + 0x800, 32), 'volume object'
    return {'case':'Pause/resume: 12 mode/timer/tooltip combinations, repeated calls and dependency order', 'passed':True}


def house_cases(original, dll):
    for mode in (3, 4, 5):
        a, b = Machine(original, dll, False), Machine(original, dll, True)
        for m in (a, b):
            m.call('ScenarioConstruct')
            m.write32(0xA8B238, mode)
            m.write32(0xA8DA78, OBJECT + 0x3800); m.write32(0xA8DA84, 3)
            for i in range(3):
                player = OBJECT + 0x4000 + 0x100 * i
                m.write32(OBJECT + 0x3800 + i * 4, player)
                m.uc.mem_write(player, ['Local player\0', 'Opponent\0', 'Observer\0'][i].encode('utf-16le'))
                for offset, value in ((75, -3 if i == 2 else i), (83, 4 - i), (91, i - 1),
                                      (95, -2), (99, -1), (107, -1 if i == 2 else 0)):
                    m.write32(player + offset, value)
            m.write32(0xA83C9C, OBJECT + 0x3900); m.write32(0xA83CA8, 4)
            for i, name in enumerate(('Americans', 'Russians', 'Neutral', 'Special')):
                country = OBJECT + 0x5000 + i * 0x400
                m.write32(OBJECT + 0x3900 + 4 * i, country)
                m.uc.mem_write(country + 36, name.encode() + b'\0'); m.write32(country + 0xb8, i)
            m.write32(0x8871E0, OBJECT + 0x8000)
            m.write32(OBJECT + 0x8000 + 0x1434, 11)
            m.uc.mem_write(OBJECT + 0x8000 + 0x17e3, b'\1')
            m.write32(0xA8B25C, 15000); m.write32(0xA8B274, 1)
            m.write32(0x822CF4, 9)
            for slot in range(8): m.write32(0xA8B29C + 4 * slot, -1)
            for address, value in ((0xA8B29C, 1), (0xA8B2BC, -2), (0xA8B2DC, 7), (0xA8B2FC, 2), (0xA8B27C, 2)):
                m.write32(address + 3 * 4, value)
            m.uc.mem_write(0x83ED14, bytes(range(10, 19)))
            m.write32(0xB054E0, 1); m.write32(0xB054D4, OBJECT + 0x3A00)
            m.write32(OBJECT + 0x3A00, OBJECT + 0x7000)
            m.write32(OBJECT + 0x7000 + 772, OBJECT + 0x7400)
            m.write32(OBJECT + 0x7000 + 784, 53)
            m.uc.mem_write(OBJECT + 0x7400, b'LightGrey\0')
            m.call('ScenarioAssignHouses')
        compare(a, b, 'house assignment')
        assert a.house_events == b.house_events and len(a.houses) == len(b.houses) == 6, 'house creation/update order'
        for left, right in zip(a.houses, b.houses):
            x, y = bytes(a.uc.mem_read(left, 0x160b8)), bytes(b.uc.mem_read(right, 0x160b8))
            assert x == y, ('house fields', mode, [hex(i) for i in range(len(x)) if x[i] != y[i]][:20])
        assert a.uc.mem_read(OBJECT + 0x4000, 0x2000) == b.uc.mem_read(OBJECT + 0x4000, 0x2000), 'player and country state'
        for singleton in (0xA83D4C, 0xAC1198):
            assert a.houses.index(a.read32(singleton)) == b.houses.index(b.read32(singleton)), 'local/observer singleton'
    return {'case':'House assignment: LAN/Internet/skirmish, sorted players, observer, AI slot, neutral and special houses', 'passed':True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--write-random-fixture', action='store_true',
                        help='regenerate the native RNG fixture from the fixed EXE')
    args = parser.parse_args()
    assert hashlib.sha256(args.original.read_bytes()).hexdigest() == SHA
    original, dll = pefile.PE(str(args.original)), pefile.PE(str(args.dll))
    rows, random_lines = [], []
    for seed in (0, 1, 0x12345678, 0xffffffff):
        a, b = Machine(original, dll, False), Machine(original, dll, True)
        a.call('RandomConstruct', (seed,)); b.call('RandomConstruct', (seed,))
        for i in range(750):
            reference = a.call('RandomNext')
            assert reference == b.call('RandomNext'), (seed, i)
            random_lines.append(f'{seed} {i} {reference}\n')
        for low, high in ((0, 100), (-20, 500), (77, -33), (0, 0x7ffffffe), (0, 0x7fffffff)):
            for i in range(32): assert a.call('RandomRange', (low, high)) == b.call('RandomRange', (low, high))
        compare(a, b, 'random')
        rows.append({'case': f'RNG seed {seed:08X}', 'passed': True})
    fixture = Path(__file__).resolve().parents[1] / 'fixtures/scenario_random_reference.txt'
    fixture_bytes = ''.join(random_lines).encode('ascii')
    if args.write_random_fixture: fixture.write_bytes(fixture_bytes)
    assert fixture.read_bytes() == fixture_bytes, 'native RNG fixture differs from fixed EXE execution'
    a, b = Machine(original, dll, False), Machine(original, dll, True)
    a.call('ScenarioConstruct'); b.call('ScenarioConstruct'); compare(a, b, 'construct')
    for name, index, value in [('ScenarioGlobal', 49, 7), ('ScenarioGlobal', 49, 7),
                               ('ScenarioLocal', 99, 253), ('ScenarioLocal', 100, 3)]:
        assert a.call(name, (index, value)) & 255 == b.call(name, (index, value)) & 255
        compare(a, b, name)
    for m in (a, b): m.write32(0xA8B238, 5)
    a.call('ScenarioReset'); b.call('ScenarioReset'); compare(a, b, 'reset skirmish')
    rows.append({'case':'Scenario construction, variables and skirmish reset', 'passed':True})
    for m in (a, b):
        for i, offset in enumerate((0x34d4, 0x34f0, 0x350c)):
            m.write32(OBJECT + offset + 4, OBJECT + 0x8000 + 64*i)
            m.write32(OBJECT + offset + 8, 4)
            m.uc.mem_write(OBJECT + offset + 13, bytes([i != 1]))
    b.call('ScenarioDestroy')
    a.uc.reg_write(UC_X86_REG_ESI, OBJECT); a.uc.reg_write(UC_X86_REG_EBX, 0)
    a.uc.reg_write(UC_X86_REG_ESP, STACK + 0xF0000)
    a.uc.emu_start(0x6BEAD2, 0x6BEB83, count=10000)
    # Outer delete and singleton clearing are the owner's responsibility.
    assert a.freed[-1] == OBJECT
    assert a.freed[:-1] == b.freed == [OBJECT + 0x8080, OBJECT + 0x8000]
    compare(a, b, 'destruct')
    rows.append({'case':'Scenario inline destructor: owned and borrowed lists', 'passed':True})
    rows.append(lighting_cases(original, dll))
    rows.append(stream_cases(original, dll))
    rows.append(pause_cases(original, dll))
    rows.append(house_cases(original, dll))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({
        'sha256': SHA,
        'probe_sha256': hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'rng_fixture_sha256': hashlib.sha256(fixture_bytes).hexdigest(),
        'method': 'x86 instruction differential',
        'scope': 'Implemented Scenario methods with controlled downstream module boundaries; no full game execution.',
        'cases': rows}, indent=2)+'\n')
    for row in rows: print('PASS', row['case'])


if __name__ == '__main__': main()
