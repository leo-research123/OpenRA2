#!/usr/bin/env python3
"""Compare fresh diamond Cells with original CreateEmptyMap's no-resize path.

The reference executes its real root constructor, One_Time and CreateEmptyMap.
The final pathfinding flag is false: this compares cell storage and identity,
not the unported world-resize, navigation or simulation initialization graph.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn.x86_const import UC_X86_REG_EIP
from compare_scenario import Machine, ENTRIES, SUPPORT, SHA

ROOT, HEAP = 0x87F7E8, 0x4000000


class MapMachine(Machine):
    def __init__(self, exe, dll, replacement):
        super().__init__(exe, dll, replacement)
        self.uc.mem_map(HEAP, 0x4000000)
        self.heap = HEAP
    def allocate(self):
        size, address = self.arg(0), self.heap
        self.heap += (size+15)&~15
        assert self.heap < HEAP+0x4000000
        self.ret(address)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe','dll','report'): parser.add_argument('--'+name,type=Path,required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    exe,dll = pefile.PE(str(args.exe)),pefile.PE(str(args.dll))
    ENTRIES.update(ScenarioMapRootConstruct=0x5BDA40, ScenarioMapCreateCells=0x565C10,
                   MapOneTime=0x565800, ScenarioCellConstruct=0x47BBF0,
                   ScenarioMapConstructCells=0x565BC0,ScenarioMapDestructCells=0x565B00)
    cases=[]
    for width,height,level in ((8,12,0),(12,8,3),(32,24,13)):
        machines=[MapMachine(exe,dll,candidate) for candidate in (False,True)]
        records=[]
        for m in machines:
            m.uc.mem_write(ROOT,bytes(0x556C))
            m.write32(0xA8B230, SUPPORT+0x9000)
            # The EXE's process startup normally constructs this static Cell.
            # Both paths enter with the same live original sentinel; mapping
            # the PE alone only provides its uninitialized BSS storage.
            candidate=m.replacement
            m.replacement=False
            m.call('ScenarioCellConstruct',ecx=0xABDC50)
            m.replacement=candidate
            m.write32(SUPPORT+0x9000+0x214,1000000)
            assert m.call('ScenarioMapRootConstruct',ecx=ROOT)==ROOT
            if not m.replacement: m.call('MapOneTime',ecx=ROOT,instruction_limit=10000000)
            bounds=SUPPORT+0x8000
            m.uc.mem_write(bounds,struct.pack('<4i',0,0,width,height))
            callargs=(bounds,level) if m.replacement else (bounds,1,level,0)
            try:
                result=m.call('ScenarioMapCreateCells',callargs,ecx=ROOT,instruction_limit=10000000)
            except Exception:
                print('map call failed', 'candidate' if m.replacement else 'original', hex(m.uc.reg_read(UC_X86_REG_EIP)))
                raise
            if m.replacement: assert result&255
            assert m.read32(ROOT+0x140)==0x40000
            slots=struct.unpack('<262144I',m.uc.mem_read(m.read32(ROOT+0x13C),0x100000))
            cells=[]
            for index,cell in enumerate(slots):
                if not cell: continue
                x,y=struct.unpack('<2h',m.uc.mem_read(cell+0x24,4))
                cells.append((index,x,y,m.read32(cell+0x10),m.read32(cell+0x38),m.uc.mem_read(cell+0x11B,1)[0]))
            records.append(cells)
            assert m.read32(SUPPORT+0x9000+0x214)==1000000+len(cells)+1
            # These cells have no owned effects or FoggedObjects. Exercise the
            # original reconstruction precondition, followed by real deletion.
            m.call('ScenarioMapConstructCells',ecx=ROOT,instruction_limit=10000000)
            assert m.read32(SUPPORT+0x9000+0x214)==1000000+2*len(cells)+1
            for ordinal,(_,cell) in enumerate((i,c) for i,c in enumerate(slots) if c):
                assert m.read32(cell+0x10)==1000000+len(cells)+2+ordinal
                assert bytes(m.uc.mem_read(cell+0x24,4))==bytes(4)
            table=m.read32(ROOT+0x13C)
            geometry=bytes(m.uc.mem_read(ROOT+0xEC,16))
            m.call('ScenarioMapDestructCells',ecx=ROOT,instruction_limit=10000000)
            assert m.read32(ROOT+0x13C)==table
            assert bytes(m.uc.mem_read(ROOT+0xEC,16))==geometry
            assert bytes(m.uc.mem_read(table,0x100000))==bytes(0x100000)
        assert records[0]==records[1], (width,height,records[0][:3],records[1][:3])
        cases.append({'width':width,'height':height,'level':level,'cells':len(records[0]),'status':'passed'})
        machines[1].call('ScenarioMapRootDestroy',ecx=ROOT,instruction_limit=10000000)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps({'exe_sha256':SHA,'dll_sha256':hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'scope':__doc__,'cases':cases,'checks':['all 262144 slots','cell coordinates','row-major IDs','level and tile sentinel',
        'invalid-cell identity increment','ConstructCells identity order','DestructCells retained storage and geometry',
        'compiled root cleanup','stack and preserved registers']},indent=2)+'\n')
    print('3 original/compiled fresh-map cell comparisons passed')


if __name__=='__main__': main()
