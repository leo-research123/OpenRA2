#!/usr/bin/env python3
"""Placement cursor/copy state, cell bits, upgrade eligibility and sidebar reset.

Both sides execute the four foundation method bodies. FoundationBoundsSize,
CoordinatesLegal and GetCellAt use the same original helper implementations;
ordered cell queries and actual cell flag writes are compared. Entire 120/50
cell scratch spans and unchanged state are compared after pointer normalization.
Original no-op atexit registration is suppressed. Placement command validation,
queue execution and rendering the placement shape are separate requirements.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,MAP,STACK,STOP,SHA
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/DisplayClassFoundation.cpp','code/core/src/yrpp/BuildingClassUpgrade.cpp','code/core/src/yrpp/GameSidebar.cpp','code/tests/x86/placement_cursor_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'placement_cursor_probe.dll',
        object_suffix='-placement.obj',
        symbols=True)
class Machine(BaseMachine):
    def __init__(self,*args):
        super().__init__(*args);self.buffers=[0x8A041C,0x8A0298]
        if self.candidate:
            for i in range(2):
                self.call('Buffer',0,i,[]);self.buffers[i]=self.c.reg_read(UC_X86_REG_EAX)
    def hook(self,c,pc,size,data):
        sp=c.reg_read(UC_X86_REG_ESP)
        if pc==0x7C8D20:
            left,right=self.read(sp+4,2)
            def string(at):return bytes(c.mem_read(at,24)).split(b'\0')[0].lower()
            self.ret(0 if string(left)==string(right) else 1)
        elif pc in (0x568300,0x5657A0):
            self.trace.append(['legal' if pc==0x568300 else 'cell',*self.cell(self.read(sp+4)[0])])
        elif pc==0x7C978A:self.ret()
    def setup(self):
        self.reset('foundation');self.byte(0x8A0618,0);self.byte(0x8A0619,0);self.put(0x8A03F8,0)
        self.c.mem_write(BASE+0x8000,b'\xA5'*0x1200)
        self.put(BASE+0x80EC,0,0,40,30)
        # Original map cell lookup traverses its cell-pointer vector. Independent
        # cells distinguish out-of-bounds skips from accidental flag writes.
        self.put(BASE+0x813C,BASE+0x100000)
        self.c.mem_write(BASE+0x100000,struct.pack('<I',BASE+0x20000)*0x40000)
        self.grid={}
        for y in range(15,66):
            for x in range(15,66):
                ptr=BASE+0x21000+len(self.grid)*0x148;self.grid[x,y]=ptr
                self.put(BASE+0x100000+(y*512+x)*4,ptr);self.put(ptr+0x12C,0xABCDEFA5)
        self.c.mem_write(BASE+0x500,b'\x27'*480)
        self.c.mem_write(BASE+0x900,b'\x59'*480)
        for which in range(2):self.c.mem_write(self.buffers[which],b'\x3B'*(200 if which else 480))
    def list(self,address,variant):
        values=[[(0,0)],[(0,0),(1,0),(1,1)],[(0,0),(0,2),(-2,0)],[],[(32766,-32768),(-32768,32766)],[(0,32767),(32767,0)]][variant]
        self.c.mem_write(address,b''.join(struct.pack('<hh',*c) for c in values+[(32767,32767)]))
    def foundation(self,pending,old,new,center,variant,alias):
        self.setup();d=BASE+0x8000;offset=0x1182 if pending else 0x1174;pointer=0x118C if pending else 0x117C
        self.c.mem_write(d+offset,struct.pack('<hhhh',*center,2,-1))
        self.list(BASE+0x500,variant);self.list(BASE+0x900,(variant+1)%6)
        self.put(d+pointer,BASE+0x900 if old else 0)
        source=BASE+0x500 if new else 0
        if alias and new:
            self.c.mem_write(self.buffers[pending],bytes(self.c.mem_read(source,200 if pending else 480)))
            source=self.buffers[pending]
        self.call('SetPending' if pending else 'SetActive',0x4A8D50 if pending else 0x4A8BF0,d,[source])
        state=bytearray(self.c.mem_read(d,0x1200));ptr=self.read(d+pointer)[0]
        state[pointer:pointer+4]=struct.pack('<I',1 if ptr==self.buffers[pending] else ptr)
        return {'trace':self.trace,'state':state.hex(),'active':bytes(self.c.mem_read(self.buffers[0],480)).hex(),'pending':bytes(self.c.mem_read(self.buffers[1],200)).hex(),
                'cells':[(x,y,self.read(ptr+0x12C)[0]) for (x,y),ptr in self.grid.items() if self.read(ptr+0x12C)[0]!=0xABCDEFA5]}
    def mark(self,pending,mark,base,variant):
        self.setup();d=BASE+0x8000
        self.list(BASE+0x500,variant);self.put(d+(0x118C if pending else 0x117C),BASE+0x500)
        self.c.mem_write(BASE+0x700,struct.pack('<hh',*base))
        self.call('MarkPending' if pending else 'MarkActive',0x4A9650 if pending else 0x4A95A0,d,[BASE+0x700,mark])
        return {'trace':self.trace,'state':bytes(self.c.mem_read(d,0x1200)).hex(),
                'cells':[(x,y,self.read(ptr+0x12C)[0]) for (x,y),ptr in self.grid.items() if self.read(ptr+0x12C)[0]!=0xABCDEFA5]}
    def upgrade(self,owner,name,level,capacity,requested):
        self.reset('upgrade');obj=BASE+0x1000;type_=BASE+0x4000;upgrade=BASE+0x8000
        self.put(obj+0x21C,BASE+0x500);self.put(obj+0x520,type_);self.byte(obj+0x702,level)
        self.put(type_+0x14E0,capacity);self.put(upgrade+0x16FC,requested)
        self.c.mem_write(type_+0x24,b'POWER\0');self.c.mem_write(upgrade+0xE88,('power' if name else 'OTHER').encode()+b'\0')
        self.call('CanUpgrade',0x452670,obj,[upgrade,BASE+0x500 if owner else BASE+0x600])
        return bool(self.c.reg_read(UC_X86_REG_EAX)&255)
    def sidebar(self,first,second,object_):
        self.reset('sidebar');self.put(0xB0FE5C,first,second)
        self.call('ClearSidebar',0x734270,object_,[])
        return [*self.read(0xB0FE5C,2),self.c.reg_read(UC_X86_REG_EAX)]
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,symbols=build(a.compile_db,a.output)
    exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
    cases={'foundation':list(itertools.product((0,1),(0,1),(0,1),((0,0),(40,40),(20,20),(60,40),(-32768,32767)),range(6),(0,1))),
           'mark':list(itertools.product((0,1),(0,1),((0,0),(40,40),(20,20),(60,40),(-32768,32767)),range(6))),
           'upgrade':list(itertools.product((0,1),(0,1),(0,1,2,3,127,128,255),(-1,0,1,3,127),(-2,-1,0,1,2,3,4))),
           'sidebar':list(itertools.product((0,BASE+0x1000,BASE+0x2000),repeat=3))}
    differences=[]
    for method,inputs in cases.items():
        for case in inputs:
            expected=getattr(old,method)(*case);actual=getattr(new,method)(*case)
            if expected!=actual:differences.append({'method':method,'input':case,'original':expected,'candidate':actual})
    report={'exe_sha256':SHA,'cases':{k:len(v) for k,v in cases.items()},'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'placement-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
    if differences:print(json.dumps(differences[:1],indent=2))
    return bool(differences)
if __name__=='__main__':raise SystemExit(main())
