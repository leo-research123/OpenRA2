#!/usr/bin/env python3
"""Compare complete Scenario WriteINI output with the fixed x86 original.

Both sides mutate real INI nodes and run real Cell coordinate methods. Map
iteration, bounds and projection are not mocked. CRT formatting and existing
session/House boundaries follow compare_scenario_ini's controlled environment.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import random
import struct

import pefile
from unicorn.x86_const import UC_X86_REG_ECX
from compare_scenario_ini import IniMachine, INI, OBJECT, SUPPORT, ENTRIES, SHA

ENTRIES.update(ScenarioWriteINI=0x68AD70, ScenarioCellCoords=0x486840,
               ScenarioMapProjection=0x6D62E0, ScenarioMapBounds=0x578460,
               ScenarioWriteSpecialFlags=0x6B8B30, INIWriteIntegers=0x475F30,
               INIWriteTechnoTypes=0x4766F0, INIWriteMovie=0x475820, INIWriteTheme=0x475950)
MAP_CELLS, OTHER = 0x4000000, OBJECT + 0xB000


class WriterMachine(IniMachine):
    def __init__(self, original, dll, replacement):
        super().__init__(original, dll, replacement)
        self.coord_calls = []
        for desc in original.DIRECTORY_ENTRY_IMPORT:
            for item in desc.imports:
                if item.name == b'wsprintfA':
                    self.write32(item.address, SUPPORT + 0xDF00)
                    self.hooks[SUPPORT + 0xDF00] = self.sprintf
        self.uc.mem_map(MAP_CELLS, 0x100000)

    def on_code(self, uc, address, size, data):
        if hasattr(self, 'coord_calls') and address == (self.exports['ScenarioCellCoords'] if self.replacement else 0x486840):
            self.coord_calls.append(uc.reg_read(UC_X86_REG_ECX))
        super().on_code(uc, address, size, data)

    def format(self, destination, fmt, args, capacity):
        assert fmt in (b'%d', b'Waypoint%d', b'%d,%d', b'%s,%d', b'%f', b'%x,', b'%02d:%02d:%02d'), fmt
        values = []
        for kind in re.findall(rb'%\d*([sdfx])', fmt):
            if kind == b'f':
                values.append(struct.unpack('<d', self.uc.mem_read(args, 8))[0]); args += 8
            else:
                value = self.read32(args); args += 4
                if kind == b's': value = self.string(value)
                elif kind == b'd' and value >= 0x80000000: value -= 0x100000000
                values.append(value)
        text = fmt % tuple(values)
        assert len(text) < capacity, (fmt, capacity, len(text))
        self.uc.mem_write(destination, text + b'\0'); self.ret(len(text))

    def map_setup(self, empty=False):
        self.write32(0x87F7E8 + 0xF4, 4)
        for i, value in enumerate((0,0,4,4)): self.write32(0x87F7E8 + 0xFC + 4 * i, value)
        self.write32(0x87F924, MAP_CELLS)
        self.write32(0x87F7E8 + 0x140, 0x40000)
        vtable = SUPPORT + 0xB00
        self.write32(vtable + 0x48, self.exports['ScenarioCellCoords'] if self.replacement else 0x486840)
        for i, (slot, x, y) in enumerate(((2049,1,4),(1538,2,3),(1027,-1,8))):
            cell = OBJECT + 0xA000 + i * 0x200
            self.write32(cell, vtable)
            self.uc.mem_write(cell + 0x24, struct.pack('<2h', x, y))
            self.uc.mem_write(cell + 0x11B, b'\x05\0')
            if not empty: self.write32(MAP_CELLS + 4 * slot, cell)
        self.write32(0xABDC50, vtable)
        self.uc.mem_write(0xABDC50 + 0x11B, b'\0\0')

    def snapshot(self):
        sections, section = [], self.read32(INI + 20)
        while section != INI + 28:
            entries, entry = [], self.read32(section + 24)
            while entry != section + 32:
                entries.append([self.string(self.read32(entry + 12)).decode(),
                                self.string(self.read32(entry + 16)).decode()])
                entry = self.read32(entry + 4)
            sections.append([self.string(self.read32(section + 12)).decode(), entries])
            section = self.read32(section + 4)
        return sections


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path); parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    original, dll = pefile.PE(str(args.exe)), pefile.PE(str(args.dll))
    cases = []
    a, b = WriterMachine(original,dll,False), WriterMachine(original,dll,True)
    randomizer = random.Random(683610)
    coordinates = [(randomizer.randrange(-2147483648,2147483648), randomizer.randrange(-2147483648,2147483648)) for _ in range(256)]
    coordinates += [(x,y) for x in (-2147483648,-512,-1,0,1,512,2147483647) for y in (-2147483648,-1,0,1,2147483647)]
    for x,y in coordinates:
        for m in (a,b): m.call('ScenarioMapProjection',(x,y,SUPPORT + 0x900,SUPPORT + 0x904),ecx=0)
        assert a.uc.mem_read(SUPPORT + 0x900,8) == b.uc.mem_read(SUPPORT + 0x900,8), ('projection',x,y)
    cases.append({'case':'Map preview projection: 291 signed coordinate and overflow cases','passed':True})
    for width, rect in ((4,(0,0,4,4)), (100,(5,6,85,80)), (2,(-1,-1,4,4)), (0,(0,0,0,0))):
        for m in (a,b):
            m.write32(0x87F7E8 + 0xF4,width)
            for i,value in enumerate(rect): m.write32(0x87F7E8 + 0xFC + i*4,value)
        for x,y in [(x,y) for x in range(-4,12) for y in range(-4,12)] + [(99,101),(200,200),(32767,-32768)]:
            for m in (a,b): m.uc.mem_write(SUPPORT + 0x900,struct.pack('<2h',x,y))
            left=a.call('ScenarioMapBounds',(SUPPORT + 0x900,0),ecx=0x87F7E8) & 255
            right=b.call('ScenarioMapBounds',(SUPPORT + 0x900,0),ecx=0x87F7E8) & 255
            assert left == right, ('map bounds',width,rect,x,y)
    cases.append({'case':'Map usable area: 1036 checkLevel=false boundary cases','passed':True})
    for multiplayer in (False, True):
        for empty in (False, True):
            for separate in (False, True):
                for holes in (False, True):
                    a, b = WriterMachine(original,dll,False), WriterMachine(original,dll,True)
                    for m in (a,b):
                        m.setup(0,False)
                        m.load('[Basic]\nPlayer=Player Two\nIntro=INTRO\nWin=VICTORY\nTheme=HELLMARCH\n'
                               'CarryOverCap=1234\nCarryOverMoney=0.75\nAllowableUnits=TANK,SOLDIER,TANK\n'
                               'AllowableUnitMaximums=3,-1,7\nTimerInherit=yes\nSkipScore=yes\n'
                               '[VariableNames]\n3=Ready,1\n99=Last,0\n[Ranking]\nParTimeHard=00:12:34\n'
                               '[Lighting]\nAmbient=0.8799\nGround=0.017\nDominatorAmbientChangeRate=0.009\n')
                        assert m.call('ScenarioReadINI',(INI,)) & 255 == 1
                        m.map_setup(empty)
                        source = OBJECT
                        if separate:
                            m.call('ScenarioConstruct',ecx=OTHER); m.write32(0xA8B230,OTHER); source = OTHER
                        m.uc.mem_write(source + 7248, b'GLOBAL:Brief\0')
                        m.uc.mem_write(source + 5050, b'GLOBAL:Name\0')
                        m.uc.mem_write(source + 1586, struct.pack('<2h',1,4))
                        m.uc.mem_write(source + 1586 + (8 if holes else 4), struct.pack('<2h',2,3))
                        m.uc.mem_write(OBJECT + 4960, 'Mission\u4e2d\0'.encode('utf-16le'))
                        m.load('[Basic]\nOldKey=delete\n[Header]\nCustom=keep\n[SpecialFlags]\nCustom=keep\n'
                               '[Ranking]\nCustom=keep\n[Lighting]\nNukeAmbient=delete\n[VariableNames]\n1=Old,1\n')
                        m.events.clear(); m.coord_calls.clear()
                        assert m.call('ScenarioWriteINI',(INI,int(multiplayer))) & 255 == 1
                    label = f'WriteINI multiplayer={multiplayer} empty-map={empty} separate-instance={separate} waypoint-holes={holes}'
                    x, y = a.snapshot(), b.snapshot()
                    if x != y:
                        xd, yd = {s:dict(entries) for s,entries in x}, {s:dict(entries) for s,entries in y}
                        delta = [(s,k,xd.get(s,{}).get(k),yd.get(s,{}).get(k)) for s in xd.keys() | yd.keys()
                                 for k in xd.get(s,{}).keys() | yd.get(s,{}).keys()
                                 if xd.get(s,{}).get(k) != yd.get(s,{}).get(k)]
                        raise AssertionError((label,delta or ['section/key ordering',x,y]))
                    assert a.coord_calls == b.coord_calls, (label,'virtual coordinate calls',a.coord_calls,b.coord_calls)
                    assert a.uc.mem_read(0xABDC74,4) == b.uc.mem_read(0xABDC74,4), 'invalid cell mutation'
                    assert a.events == b.events == [], 'no loading callbacks while writing'
                    print('PASS',label)
                    cases.append({'case':label,'passed':True,'sections':len(x),'entries':sum(len(e) for _,e in x),
                                  'get_coords_calls':len(a.coord_calls)})
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps({'sha256':SHA,'probe_sha256':hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'scope':'Complete ordered Scenario WriteINI output using real INI nodes, cell coordinates, map bounds and projection',
        'cases':cases},indent=2)+'\n')


if __name__ == '__main__': main()
