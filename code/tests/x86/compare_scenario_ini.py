#!/usr/bin/env python3
"""Scenario ReadINI differential with real original INI nodes on both sides.

Scenario, SpecialFlags, INI lookup/conversion and list code execute as compiled
x86 instructions. Only CRT primitives and downstream session/renderer/House
construction boundaries are controlled. No complete Scenario entry is mocked.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

import pefile
from unicorn.x86_const import *
from compare_scenario import Machine, OBJECT, SUPPORT, STACK, STOP, ENTRIES, SHA, PRESERVED
from compare_ini_values import ValuesMachine
from compare_rules import RulesMachine

INI, STRAW = SUPPORT + 0xC00, SUPPORT + 0xD00
ENTRIES['ScenarioReadINI'] = 0x689E90
ENTRIES.update(ScenarioSpecialFlags=0x6B8CA0, ScenarioReadLocalVariables=0x689B20,
               INIIntegerList=0x475D70, INITechnoList=0x4764F0)


class IniMachine(Machine):
    def __init__(self, original, dll, replacement):
        super().__init__(original, dll, replacement)
        self.input, self.position, self.token = b'', 0, 0
        self.events = []
        self.hooks.update({0x7C9430: self.allocate, 0x7C93E8: self.deallocate,
            0x7D5408: self.duplicate, 0x7CAF30: self.strchr,
            0x7C91D0: self.strncpy, 0x7D15A0: lambda: self.ret(len(self.string(self.arg(0)))),
            0x7D75E0: self.memset, 0x7CA090: self.memcpy,
            0x7C8EF4: self.sprintf, 0x7C9CC2: self.strtok, 0x7CA530: self.sscanf,
            0x4A2780: self.get, 0x69AE90: self.progress, 0x48D080: self.pump,
            0x545000: self.reset_lighting})
        actions = {'strlen': lambda: self.ret(len(self.string(self.arg(0)))),
            'fegetround': self.fegetround, 'fesetround': self.fesetround,
            'strcmp': self.strcmp, 'strchr': self.strchr,
            '__stdio_common_vsscanf': self.vsscanf, '__stdio_common_vsprintf': self.vsprintf,
            'strspn': lambda: self.span(False), 'strcspn': lambda: self.span(True),
            'atoi': self.atoi, 'memmove': self.memcpy, '__std_exception_copy': self.exception}
        for desc in dll.DIRECTORY_ENTRY_IMPORT:
            for item in desc.imports:
                name = (item.name or b'').decode()
                if name in actions: self.hooks[self.read32(item.address)] = actions[name]
        self.execute(0x535AA0, INI)

    string = Machine.string_bytes
    strtok = ValuesMachine.strtok
    scan_float = RulesMachine.scan_float

    def fegetround(self):
        # UCRT fenv.h uses UP=0x0100, DOWN=0x0200 (opposite x87 order).
        mode = (self.uc.reg_read(UC_X86_REG_FPCW) >> 10) & 3
        self.ret((0, 0x0200, 0x0100, 0x0300)[mode])

    def fesetround(self):
        modes = {0: 0, 0x0100: 2, 0x0200: 1, 0x0300: 3}
        mode = modes.get(self.arg(0))
        if mode is None:
            self.ret(1); return
        self.uc.reg_write(UC_X86_REG_FPCW, (self.uc.reg_read(UC_X86_REG_FPCW) & ~0x0C00) | (mode << 10))
        self.uc.reg_write(UC_X86_REG_MXCSR, (self.uc.reg_read(UC_X86_REG_MXCSR) & ~0x6000) | (mode << 13))
        self.ret(0)

    def malloc(self, size):
        result = self.heap; self.heap += (size + 15) & ~15
        assert self.heap < OBJECT + 0x100000
        return result

    def cstring(self, value):
        address = self.malloc(len(value) + 1); self.uc.mem_write(address, value + b'\0'); return address

    def duplicate(self): self.ret(self.cstring(self.string(self.arg(0))))

    def strchr(self):
        offset = (self.string(self.arg(0)) + b'\0').find(bytes([self.arg(1) & 255]))
        self.ret(self.arg(0) + offset if offset >= 0 else 0)

    def strcmp(self):
        a, b = self.string(self.arg(0)), self.string(self.arg(1))
        self.ret((a > b) - (a < b))

    def span(self, complement):
        text, chars = self.string(self.arg(0)), self.string(self.arg(1))
        count = 0
        for byte in text:
            if (byte in chars) == complement: break
            count += 1
        self.ret(count)

    def atoi(self):
        match = re.match(rb'\s*([+-]?\d+)', self.string(self.arg(0)))
        self.ret(int(match[1]) if match else 0)

    def exception(self):
        raise AssertionError('replacement exception: ' + self.string(self.read32(self.arg(0))).decode())

    def format(self, destination, fmt, args, capacity):
        assert fmt in (b'%d', b'Waypoint%d', b'%d,%d', b'%s,%d', b'%sSav'), fmt
        values = []
        for i, kind in enumerate(re.findall(rb'%([sd])', fmt)):
            value = self.read32(args + i * 4)
            values.append(self.string(value) if kind == b's' else (value if value < 0x80000000 else value - 0x100000000))
        text = fmt % tuple(values)
        assert len(text) < capacity
        self.uc.mem_write(destination, text + b'\0'); self.ret(len(text))

    def sprintf(self):
        self.format(self.arg(0), self.string(self.arg(1)), self.uc.reg_read(UC_X86_REG_ESP) + 12, 512)

    def vsprintf(self): self.format(self.arg(2), self.string(self.arg(4)), self.arg(6), self.arg(3))

    def scan(self, text, fmt, outputs):
        if fmt == b'%f': return self.scan_float(text, outputs[0])
        assert fmt in (b'%d,%d', b'%d,%d,%d', b'%d,%d,%d,%d', b'%02d:%02d:%02d', b'%x'), fmt
        pos = count = 0
        for piece in re.findall(rb'%\d*[dx]|[^%]+', fmt):
            if not piece.startswith(b'%'):
                if not text[pos:].startswith(piece): break
                pos += len(piece); continue
            while pos < len(text) and chr(text[pos]).isspace(): pos += 1
            width = int(piece[1:-1] or b'0')
            rest = text[pos:pos + width] if width else text[pos:]
            match = re.match(rb'[+-]?(?:0[xX])?[0-9a-fA-F]+' if piece[-1:] == b'x' else rb'[+-]?\d+', rest)
            if not match: break
            self.write32(outputs[count], int(match[0], 16 if piece[-1:] == b'x' else 10))
            pos += len(match[0]); count += 1
        return count

    def sscanf(self):
        fmt = self.string(self.arg(1))
        self.ret(self.scan(self.string(self.arg(0)), fmt, [self.arg(2 + i) for i in range(fmt.count(b'%'))]))

    def vsscanf(self):
        fmt, args = self.string(self.arg(4)), self.arg(6)
        self.ret(self.scan(self.string(self.arg(2)), fmt, [self.read32(args + 4 * i) for i in range(fmt.count(b'%'))]))

    def get(self):
        count = min(self.arg(1), len(self.input) - self.position)
        if count: self.uc.mem_write(self.arg(0), self.input[self.position:self.position + count])
        self.position += count; self.ret(count, cleanup=8)

    def progress(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == 0xA8B238
        self.events.append(self.arg(0)); self.ret(cleanup=4)

    def pump(self): self.events.append(-1); self.ret()
    def reset_lighting(self): self.events.append(-2); self.ret()

    def execute(self, address, this, *args):
        sp = STACK + 0xF0000
        for i, value in enumerate((STOP,) + args): self.write32(sp + 4 * i, value)
        self.uc.reg_write(UC_X86_REG_ESP, sp); self.uc.reg_write(UC_X86_REG_ECX, this)
        self.uc.emu_start(address, STOP, count=4000000)
        assert self.uc.reg_read(UC_X86_REG_EIP) == STOP
        assert self.uc.reg_read(UC_X86_REG_ESP) == sp + 4 * (len(args) + 1)
        return self.uc.reg_read(UC_X86_REG_EAX)

    def load(self, text):
        self.execute(0x5257C0, INI, 0, 0)
        self.input, self.position = text.encode(), 0
        assert self.execute(0x525A60, INI, STRAW, 0) == 1

    def setup(self, mode, armageddon):
        self.call('ScenarioConstruct')
        self.write32(0xA8B238, mode); self.uc.mem_write(0xA8ED6B, bytes([armageddon]))
        self.uc.mem_write(OBJECT, b'\xff' * 4)
        for i, value in enumerate((11, 12, 13, 14, 2, 15, 16, 17, 18)):
            self.write32(OBJECT + 4396 + 4 * i, value)
        self.write32(0xA83C9C, OBJECT + 0x3900); self.write32(0xA83CA8, 4)
        for i, name in enumerate(('Americans', 'Russians', 'Neutral', 'Special')):
            country = OBJECT + 0x5000 + i * 0x400
            self.write32(OBJECT + 0x3900 + 4 * i, country)
            # Neutral/Special lookup also accepts display Name aliases.
            identifier = f'Country{i}' if mode and armageddon and i >= 2 else name
            self.uc.mem_write(country + 36, identifier.encode() + b'\0')
            self.uc.mem_write(country + 100, name.encode() + b'\0')
            self.write32(country + 0xb8, i + 20 if mode == 0 else i)
        self.write32(0xA8EB04, OBJECT + 0x3B00); self.write32(0xA8EB10, 2)
        for i, name in enumerate(('TANK', 'SOLDIER')):
            address = OBJECT + 0x6000 + 0x100 * i
            self.write32(OBJECT + 0x3B00 + 4 * i, address)
            self.uc.mem_write(address + 36, name.encode() + b'\0')
        for registry, entries, names in ((0xABF390, OBJECT + 0x3D00, ('INTRO', 'VICTORY')),
                                        (0xA83D24, OBJECT + 0x3E00, ('SCORE', 'HELLMARCH'))):
            self.write32(registry + 4, entries); self.write32(registry + 16, len(names))
            for i, name in enumerate(names): self.write32(entries + 4 * i, self.cstring(name.encode()))
        if mode == 0:
            self.write32(0xA8022C, OBJECT + 0x3C00); self.write32(0xA80238, 2)
            for i, name in enumerate(('Americans', 'Player Two')):
                house = self.malloc(0x160B8); self.houses.append(house)
                self.write32(OBJECT + 0x3C00 + i * 4, house)
                self.write32(house + 0x34, OBJECT + 0x5000 + i * 0x400)
                self.uc.mem_write(house + 90100, name.encode() + b'\0')
                self.write32(house + 22260, 987)
        else:
            self.write32(0xA8DA78, OBJECT + 0x3800); self.write32(0xA8DA84, 1)
            self.write32(OBJECT + 0x3800, OBJECT + 0x4000)
            self.uc.mem_write(OBJECT + 0x4000, 'Player One\0'.encode('utf-16le'))
            for offset, value in ((75, 0), (83, 2), (91, -1), (95, -2), (99, -1), (107, 0)):
                self.write32(OBJECT + 0x4000 + offset, value)
            self.write32(0x8871E0, OBJECT + 0x8000)
            self.write32(OBJECT + 0x8000 + 0x1434, 11)
            self.write32(0xA8B25C, 15000); self.write32(0xA8B274, 0); self.write32(0x822CF4, 9)
            self.uc.mem_write(0x83ED14, bytes(range(10, 19)))
            self.write32(0xB054E0, 1); self.write32(0xB054D4, OBJECT + 0x3A00)
            self.write32(OBJECT + 0x3A00, OBJECT + 0x7000)
            self.write32(OBJECT + 0x7000 + 772, OBJECT + 0x7400)
            self.write32(OBJECT + 0x7000 + 784, 53); self.uc.mem_write(OBJECT + 0x7400, b'LightGrey\0')


def compare(a, b, label):
    x, y = bytearray(a.state()), bytearray(b.state())
    for offset in (0x34D4, 0x34F0, 0x350C):
        # Allocation identities and the target's uninitialized unknown_18 are
        # not state. Verify capacity/count/growth and every list item separately.
        for state in (x, y):
            state[offset + 4:offset + 8] = bytes(4)
            state[offset + 24:offset + 28] = bytes(4)
        count = a.read32(OBJECT + offset + 16)
        assert count == b.read32(OBJECT + offset + 16)
        assert a.uc.mem_read(a.read32(OBJECT + offset + 4), count * 4) == b.uc.mem_read(b.read32(OBJECT + offset + 4), count * 4)
    differences = [(hex(i), x[i], y[i]) for i in range(len(x)) if x[i] != y[i]]
    assert not differences, (label, differences[:30])
    assert a.events == b.events, (label, 'event order', a.events, b.events)
    assert a.read32(0xA8ED7C) == b.read32(0xA8ED7C), 'NewINIFormat'
    assert a.house_events == b.house_events and len(a.houses) == len(b.houses), 'House creation sequence'
    for left, right in zip(a.houses, b.houses):
        xx, yy = a.uc.mem_read(left, 0x160B8), b.uc.mem_read(right, 0x160B8)
        assert xx == yy, (label, 'house fields', [hex(i) for i in range(len(xx)) if xx[i] != yy[i]][:20])
    assert a.houses.index(a.read32(0xA83D4C)) == b.houses.index(b.read32(0xA83D4C)), 'selected player'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path); parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    original, dll = pefile.PE(str(args.exe)), pefile.PE(str(args.dll))
    samples = [
        '[Basic]\nPlayer=Player Two\n',
        '[Basic]\nPlayer=player two\nCarryOverMoney=175%\nCarryOverCap=1250\nNewINIFormat=4\n'
        'AllowableUnits=TANK,missing,tank,SOLDIER,TANK,SOLDIER\nAllowableUnitMaximums=3\n'
        '[Header]\nWaypoint1=31,32\n[VariableNames]\n3=Ready,1\n[Ranking]\nParTimeEasy=01:02:03\n'
        '[Lighting]\nAmbient=0.8799\nRed=-1\nGround=0.037\nNukeAmbientChangeRate=2.9\nDominatorAmbientChangeRate=0.025\n',
        '[Basic]\nPlayer=Player Two\nAllowableUnits=,TANK,,,SOLDIER\nAllowableUnitMaximums=oops,-1,9\n'
        'NextScenario=ALL02UMD.MAP\nAltNextScenario=next.map\nTimerInherit=yes\nEndOfGame=yes\n'
        'SkipScore=yes\nOneTimeOnly=yes\nSkipMapSelect=yes\nTruckCrate=yes\nTrainCrate=yes\nFillSilos=yes\n'
        'IgnoreGlobalAITriggers=yes\nPercent=41\nStartingDropships=3\nFreeRadar=yes\nHomeCell=4\nAltHomeCell=5\n',
    ]
    flags = ('TiberiumExplosive', 'MCVDeploy', 'InitialVeteran', 'IonStorms', 'Meteorites', 'Visceroids',
             'TiberiumGrows', 'TiberiumSpreads', 'DestroyableBridges', 'FixedAlliance', 'FogOfWar', 'Inert', 'HarvesterImmune')
    samples += ['[Basic]\nPlayer=Player Two\n[SpecialFlags]\n' + '\n'.join(f'{key}=no' for key in flags) + '\n']
    samples += [
        '[Basic]\nPlayer=Player Two\nIntro=intro\nBrief=missing\nWin=VICTORY\nLose=intro\n'
        'Action=VICTORY\nPostScore=INTRO\nPreMapSelect=VICTORY\nTheme=HELLMARCH\n'
        'MultiplayerOnly=yes\nTiberiumGrowthEnabled=no\nVeinGrowthEnabled=no\nIceGrowthEnabled=no\n'
        'TiberiumDeathToVisceroid=no\nCarryOverMoney=-0.25\n[Ranking]\nParTimeMedium=00:04:05\n'
        'ParTimeHard=00:05:06\nUnderParMessage=TXT:Good\nOverParTitle=TXT:Slow\nOverParMessage=TXT:TryAgain\n',
        '[Header]\nStartX=21\nStartY=22\nWidth=23\nHeight=24\nNumberStartingPoints=1\nNumCoopHumanStartSpots=3\n'
        '[Basic]\nPlayer=Player Two\nAllowableUnits=TANK,SOLDIER,TANK,SOLDIER,TANK,SOLDIER,TANK,SOLDIER\n'
        'AllowableUnitMaximums=,\n'
    ]
    light_keys = ('Ambient', 'Red', 'Green', 'Blue', 'Ground', 'Level', 'IonAmbient', 'IonRed', 'IonGreen',
                  'IonBlue', 'IonGround', 'IonLevel', 'NukeAmbientChangeRate', 'DominatorAmbient',
                  'DominatorRed', 'DominatorGreen', 'DominatorBlue', 'DominatorGround', 'DominatorLevel',
                  'DominatorAmbientChangeRate')
    for value in ('1.125', '-0.3333', '1e9', '1e-9'):
        samples.append('[Basic]\nPlayer=Player Two\n[Lighting]\n' + '\n'.join(f'{key}={value}' for key in light_keys) + '\n')
    cases = []
    for mode in (0, 3, 4, 5):
        for armageddon in (False, True):
            for i, sample in enumerate(samples):
                a, b = IniMachine(original, dll, False), IniMachine(original, dll, True)
                for m in (a, b):
                    m.setup(mode, armageddon); m.load(sample)
                    assert m.call('ScenarioReadINI', (INI,)) & 255 == 1
                label = f'ReadINI mode={mode} armageddon={armageddon} input={i}'
                compare(a, b, label)
                if mode == 0:
                    for m in (a, b):
                        m.events.clear(); m.load('[Basic]\nPlayer=Player Two\n')
                        assert m.call('ScenarioReadINI', (INI,)) & 255 == 1
                    compare(a, b, label + ' repeated')
                print('PASS', label)
                cases.append({'case': label, 'passed': True})
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'sha256': SHA, 'probe_sha256': hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'scope': 'Scenario ReadINI, actual INI queries, list state, flags, player setup and ordered downstream callbacks',
        'cases': cases}, indent=2) + '\n')


if __name__ == '__main__': main()
