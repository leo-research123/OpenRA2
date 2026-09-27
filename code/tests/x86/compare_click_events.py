#!/usr/bin/env python3
"""Full wire-byte comparison of six original Event constructors and SW lookup.

Pre-filled object storage catches writes to IsExecuted and unused payload bytes,
including failure paths. The original debug logger is a no-op in this EXE.
No event execution, queue dispatch, or entire LeftMouseButtonUp claim is made.
"""
import argparse,hashlib,itertools,json,random,re,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,MAP,STACK,STOP,SHA
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/EventClassConstruction.cpp','code/core/src/yrpp/SuperWeaponTypeClassInput.cpp','code/tests/x86/click_event_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'click_event_probe.dll',
        object_suffix='-events.obj',
        symbols=True)
class Machine(BaseMachine):
    def hook(self,c,pc,size,data):pass
    def construct(self,kind,house,type_,id_,rtti,naval,cell,frame,seed):
        self.reset('event');obj=BASE+0x1000;where=BASE+0x500
        self.c.mem_write(obj-4,bytes((i*71+seed)%256 for i in range(0x77)))
        self.put(0xA8ED84,frame);self.c.mem_write(where,struct.pack('<hh',*cell))
        name,entry,args={'target':('TargetEvent',0x4C65E0,[house,type_,id_,rtti]),
             'cell':('CellEvent',0x4C6650,[house,type_,where]),'place':('PlaceEvent',0x4C6AE0,[house,type_,rtti,id_,naval,where]),
             'simple':('SimplePlaceEvent',0x4C69E0,[house,type_,rtti,where]),'ground':('GroundPlaceEvent',0x4C6A60,[house,type_,rtti,id_,where]),'special':('SpecialEvent',0x4C6B60,[house,type_,id_,where])}[kind]
        self.call(name,entry,obj,args)
        return {'bytes':bytes(self.c.mem_read(obj-4,0x77)).hex(),'returned':self.c.reg_read(UC_X86_REG_EAX)}
    def find_action(self,values,action):
        self.reset('lookup');self.put(0xA8E334,BASE+0x500);self.put(0xA8E340,len(values))
        for i,value in enumerate(values):self.put(BASE+0x500+i*4,BASE+0x1000+i*0x200);self.put(BASE+0x10BC+i*0x200,value)
        self.call('FindAction',0x6CEEB0,action,[])
        return self.c.reg_read(UC_X86_REG_EAX)
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,symbols=build(a.compile_db,a.output)
    exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
    rng=random.Random(0x4C65E0);cases={'construct':[],'find_action':[]}
    for kind,house,type_,frame in itertools.product(('target','cell','place','simple','ground','special'),(-0x80000000,-128,-1,0,1,127,128,255,256,0x7FFFFFFF),(0,1,2,11,18,21,22,23,255,511),(0,-1,0x12345678)):
        cases['construct'].append((kind,house,type_,rng.choice((-1,0,17,0x12345678)),rng.choice((-1,0,1,6,255,256,0x12345678)),rng.choice((-1,0,1,0x87654321)),rng.choice(((-32768,32767),(-1,-1),(0,0),(40,50))),frame,rng.randrange(256)))
    for values,action in itertools.product(((),(20,),(20,37,20),(0,1,5,20,37,72,0xFFFFFFFF)),range(74)):
        cases['find_action'].append((values,action))
    differences=[]
    for method,inputs in cases.items():
        for case in inputs:
            expected=getattr(old,method)(*case);actual=getattr(new,method)(*case)
            if expected!=actual:differences.append({'method':method,'input':case,'original':expected,'candidate':actual})
    report={'exe_sha256':SHA,'cases':{k:len(v) for k,v in cases.items()},'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'click-events-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
    if differences:print(json.dumps(differences[:2],indent=2))
    return bool(differences)
if __name__=='__main__':raise SystemExit(main())
