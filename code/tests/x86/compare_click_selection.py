#!/usr/bin/env python3
"""Selection helpers against fixed YR: clear, type select/deselect and rectangle.

Actual candidate vector collection executes with a deterministic shared heap.
Object RTTI/type/owner, CanBeSelected and Select/Deselect are traced boundaries.
BuildingType::IsVehicle uses the same original foundation predicate on both
sides. Tactical::SelectThese executes its real body and the real group helper,
including type-mode precedence over the callback and live selectable count.
Successful vector growth and gathering-before-dispatch are covered; injected allocation recovery
and the complete input dispatcher are not claimed.
"""
import argparse,hashlib,itertools,json,random,re,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,MAP,STACK,STOP,SHA
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/GameSelection.cpp','code/core/src/yrpp/MapClassSelection.cpp','code/core/src/yrpp/TacticalClassSelection.cpp','code/tests/x86/click_selection_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'click_selection_probe.dll',
        object_suffix='-events.obj',
        symbols=True)
class Machine(BaseMachine):
    def __init__(self,*args):
        super().__init__(*args)
        self.array=0xB0CEC8
        if self.candidate:
            self.call('SelectionArray',0,0,[])
            self.array=self.c.reg_read(UC_X86_REG_EAX)
    def hook(self,c,pc,size,data):
        sp=c.reg_read(UC_X86_REG_ESP);self_=c.reg_read(UC_X86_REG_ECX)
        if pc==0x7C8E17:
            size=self.read(sp+4)[0];ptr=self.heap;self.heap+=(size+15)&~15
            self.c.mem_write(ptr,bytes(size));self.ret(ptr)
        elif pc==0x7C8B3D:self.ret()
        elif pc==0x50B6F0:
            self.trace.append(['owner',self_]);self.ret(self.read(self_+0x20)[0])
        elif pc==0x7C8D20:
            left,right=self.read(sp+4,2)
            def string(at):return bytes(self.c.mem_read(at,32)).split(b'\0')[0].lower()
            self.ret(0 if string(left)==string(right) else 1)
        elif pc==BASE+0x50000:
            index=(self_-BASE-0x1000)//0x1000;self.trace.append(['type',index]);self.ret(BASE+0x20000+index*0x1000)
        elif pc in (BASE+0x50010,BASE+0x50020):
            index=(self_-BASE-0x1000)//0x1000
            selected=pc==BASE+0x50010;self.trace.append(['select' if selected else 'deselect',index,self.c.mem_read(0x822CF2,1)[0]])
            if self.mode=='clear':
                count=self.read(0xA8ECC8)[0];items=self.read(0xA8ECBC)[0]
                self.c.mem_write(items,bytes(self.c.mem_read(items+4,4*(count-1))));self.put(0xA8ECC8,count-1)
            elif self.mutate:
                self.put(0xA8EC88,0);self.put(BASE+0x70DB0,0)
            self.ret(self.select_result)
        elif pc in (BASE+0x50030,BASE+0x50040,BASE+0x50050,BASE+0x50060):
            index=(self_-BASE-0x1000)//0x1000
            if pc==BASE+0x50030:
                self.trace.append(['rtti',index]);self.ret(self.read(self_+0x24)[0])
            elif pc==BASE+0x50040:
                self.trace.append(['get_owner',index]);self.ret(self.read(self_+0x21C)[0])
            elif pc==BASE+0x50050:
                self.trace.append(['can_select',index]);self.ret(self.read(self_+0x28)[0])
            else:
                self.trace.append(['callback',index,self.c.mem_read(0x822CF2,1)[0]])
                if self.mutate:self.put(BASE+0x70DB0,0)
                self.ret()
    def setup(self):
        self.reset('selection');self.heap=BASE+0x100000;self.mutate=False;self.select_result=1
        self.byte(0xB0FE65,0)
        self.put(0x887324,BASE+0x70000)
        self.put(0xA8EC7C,BASE+0x600);self.put(0xA8EC88,6)
        self.put(0xA8ECBC,BASE+0x800);self.put(0xA8ECC8,0)
        for i in range(6):
            obj=BASE+0x1000+i*0x1000;house=BASE+0x10000+i*0x1000
            self.put(BASE+0x600+i*4,obj);self.put(self.array+i*12,obj,10,20)
            self.put(obj,BASE+0x40000);self.put(obj+0x14,1);self.byte(obj+0x90,1);self.put(obj+0x21C,house)
            self.put(house+0x20,i%2);self.byte(house+0x1ED,(i+1)%2)
            self.c.mem_write(BASE+0x20024+i*0x1000,('UNIT' if i%3 else 'OTHER').encode()+b'\0')
            self.put(obj+0x24,1);self.put(obj+0x28,1)
            self.put(obj+0x520,BASE+0x20000+i*0x1000)
        for slot,pc in ((0x88,0x50000),(0x14C,0x50010),(0x150,0x50020)):self.put(BASE+0x40000+slot,BASE+pc)
        for slot,pc in ((0x2C,0x50030),(0x3C,0x50040),(0x138,0x50050)):self.put(BASE+0x40000+slot,BASE+pc)
        self.put(BASE+0x70DB0,6);self.c.mem_write(BASE+0x900,b'unit\0')
    def group(self,select,whole,mode,voice,mutate,variant):
        self.setup();self.mutate=mutate;self.put(0xA8B238,mode);self.byte(0xB0FE64,whole);self.byte(0x822CF2,voice)
        if variant==1:self.byte(BASE+0x2090,0)
        elif variant==2:self.put(self.array+12,0)
        elif variant==3:self.put(BASE+0x2014,0)
        elif variant==4:
            self.put(0xA8EC88,0);self.put(BASE+0x70DB0,0)
        self.call('SelectType' if select else 'DeselectType',0x7327D0 if select else 0x732600,BASE+0x900,[])
        return {'trace':self.trace,'voice':self.c.mem_read(0x822CF2,1)[0]}
    def rectangle(self,type_mode,callback,whole,mutate,variant,geometry):
        self.setup();self.mode='rectangle';self.mutate=mutate
        self.byte(0xB0FE65,type_mode);self.byte(0xB0FE64,whole);self.put(0xA8B238,1)
        self.put(BASE+0x700B0,5,-10)
        rect=((0,0,30,40),(5,30,1,1),(6,31,1,1),(0,0,0,40),(0,0,30,-1))[geometry]
        self.put(BASE+0xA00,*rect)
        if variant==1:self.byte(BASE+0x2090,0)
        elif variant==2:self.put(self.array+12,0)
        elif variant==3:self.put(BASE+0x2028,0)
        elif variant==4:self.select_result=0
        elif variant in (5,6,7):
            for i in range(6):
                self.put(BASE+0x1024+i*0x1000,6)
                self.put(BASE+0x20408+i*0x1000,1 if variant!=5 else 0)
                self.put(BASE+0x20EF0+i*0x1000,3 if variant==7 else 0)
        elif variant==8:
            for i in range(6):self.put(BASE+0x121C+i*0x1000,0)
            # Group helpers require a valid owner. This profile only targets
            # the ordinary rectangle branch's explicit null-owner gate.
            assert not type_mode
        self.call('SelectRectangle',0x6DA5C0,BASE+0x70000,[BASE+0xA00,BASE+0x50060 if callback else 0])
        return {'trace':self.trace,'voice':self.c.mem_read(0x822CF2,1)[0],'count':self.read(BASE+0x70DB0)[0]}
    def clear(self,count,beacons,mode,attack):
        self.setup();self.mode='clear'
        self.put(0xA8ECC8,count)
        for i in range(count):self.put(BASE+0x800+i*4,BASE+0x1000+i*0x1000)
        self.put(0xB0FE54,mode);self.byte(0xB0FE58,attack);self.put(0x89C410,beacons)
        for i in range(24):
            self.put(0x89C3B0+i*4,BASE+0x80000+i*0x200 if i%3 else 0)
            self.byte(BASE+0x8000C+i*0x200,(i*19+7)%256)
        self.call('ClearSelection',0x48DC90,0,[])
        return {'trace':self.trace,'count':self.read(0xA8ECC8)[0],'mode':self.read(0xB0FE54)[0],
                'attack':self.c.mem_read(0xB0FE58,1)[0],'beacons':[self.c.mem_read(BASE+0x8000C+i*0x200,1)[0] for i in range(24)]}
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,symbols=build(a.compile_db,a.output)
    exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
    cases={'group':list(itertools.product((0,1),(0,1),(0,1,5),(0,1),(0,1),range(5))),
           'clear':list(itertools.product((0,1,3,6),(0,1,24),(-1,0,1,5),(0,1))),
           'rectangle':[case for case in itertools.product((0,1),(0,1),(0,1),(0,1),range(9),range(5)) if not(case[0] and case[4]==8)]}
    differences=[]
    for method,inputs in cases.items():
        for case in inputs:
            expected=getattr(old,method)(*case);actual=getattr(new,method)(*case)
            if expected!=actual:differences.append({'method':method,'input':case,'original':expected,'candidate':actual})
    report={'exe_sha256':SHA,'cases':{k:len(v) for k,v in cases.items()},'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'click-selection-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
    if differences:print(json.dumps(differences[:2],indent=2))
    return bool(differences)
if __name__=='__main__':raise SystemExit(main())
