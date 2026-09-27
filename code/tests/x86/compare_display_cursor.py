#!/usr/bin/env python3
"""Display::ConvertAction and shared attack-move predicate against fixed YR.

Production method bodies execute on both sides. House waypoint queries, map
height/membership and unit virtual calls are shared trace boundaries. Cursor
SetCursor is recorded, not a graphics substitute claimed as an implementation.
The five Mouse cursor methods also execute with an observed WWMouse::Draw
boundary; driver painting and cursor animation Update remain separate.
All 73 actions, fog, waypoint remapping, color bytes, hover and selected-unit
range/MoveToShroud behavior are observed. Palette/driver lifetime is separate.
"""
import argparse,hashlib,itertools,json,random,re,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,MAP,STACK,STOP,SHA
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/DisplayClassCursor.cpp','code/core/src/yrpp/GameAttackMove.cpp','code/core/src/yrpp/MouseClassCursor.cpp','code/tests/x86/display_cursor_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'display_cursor_probe.dll',
        object_suffix='-cursor.obj',
        symbols=True)
class Machine(BaseMachine):
    def hook(self,c,pc,size,data):
        if self.mode=='mouse':
            sp=c.reg_read(UC_X86_REG_ESP)
            if pc==0x6C8C40:
                self.trace.append(['clock']);self.ret(self.ticks)
            elif pc==BASE+0x50050:
                point,shape,index=self.read(sp+4,3)
                self.trace.append(['draw',*self.read(point,2),shape,index,*self.read(MAP+0x5560,3),self.c.mem_read(MAP+0x555C,1)[0]])
                self.ret(pop=12)
            return
        sp=c.reg_read(UC_X86_REG_ESP);self_=c.reg_read(UC_X86_REG_ECX)
        if pc==0x5023B0:
            self.trace.append(['waypoint',*self.cell(self.read(sp+4)[0])]);self.ret(BASE+0x80000 if self.has_path else 0,4)
        elif pc==0x502460:
            point,path,index=self.read(sp+4,3);self.trace.append(['properties',point]);self.put(path,self.path);self.byte(index,7);self.ret(self.has_path,12)
        elif pc==0x5657A0:
            self.trace.append(['cell',*self.cell(self.read(sp+4)[0])]);self.ret(BASE+0x70000,4)
        elif pc==0x578460:
            at,height=self.read(sp+4,2);self.trace.append(['membership',*self.cell(at),height&255]);self.ret(self.usable,8)
        elif pc==0x578080:
            at=self.read(sp+4)[0];self.trace.append(['height',*self.read(at,3)]);self.ret(312,4)
        elif pc==0x54F5C0:
            key=self.read(sp+4)[0];self.trace.append(['key',key]);self.ret(bool(self.keys&(1<<(key-10))),4)
        elif pc==BASE+0x50000:
            cursor,mini=self.read(sp+4,2);self.trace.append(['cursor',cursor,mini&255]);self.ret(self.cursor_result,8)
        elif pc==BASE+0x50010:
            self.trace.append(['rtti',self_]);self.ret(6 if self.kind==3 else (0 if self.kind==1 else 2))
        elif pc==BASE+0x50020:
            self.trace.append(['type',self_]);self.ret(BASE+0x9000)
        elif pc==BASE+0x50030:
            self.trace.append(['range',self.read(sp+4)[0]]);self.ret(self.in_range,4)
        elif pc==BASE+0x50040:
            self.trace.append(['can_attack_move',self_]);self.ret(self.can_move)
            if self.mutate:self.put(0xA8ECC8,0)
    def setup(self):
        self.reset('cursor');self.mutate=False
        self.put(0x87F770,BASE+0x200);self.put(0xA8EC00,10,11,12,13)
        self.put(0xA8ECBC,BASE+0x500);self.put(BASE+0x500,BASE+0x1000,BASE+0x2000,0)
        for at in [BASE+0x1000,BASE+0x2000,BASE+0x3000]:
            self.put(at,BASE+0x40000);self.put(at+0x14,1);self.put(at+0x520,BASE+0x9000)
        for slot,pc in [(0x2C,0x50010),(0x84,0x50020),(0x3AC,0x50030),(0x4C0,0x50040)]:self.put(BASE+0x40000+slot,BASE+pc)
        self.put(MAP,BASE+0x60000);self.put(BASE+0x60048,BASE+0x50000)
        self.put(0x87F6C8,BASE+0xB000);self.put(BASE+0xB174,BASE+0xC000)
        self.put(0x8A0730,416);self.c.emu_start(0x4A85E0,0x4A85EE) # Execute the zero-cell initializer.
        self.c.mem_write(BASE+0xC000,bytes((i*71+19)%256 for i in range(512)))
        self.c.mem_write(0x885180,bytes((i*37+23)%256 for i in range(768)))
        self.put(BASE+0x80000,1,2,3)
    def convert(self,action,fog,mini,profile):
        self.setup()
        # Profiles independently vary object kind, selection, range, palette,
        # planning path/dragging and attack-move keys; random rows supplement.
        self.kind,self.has_path,planning,drag,self.usable,self.path,selected,self.in_range,shroud_move,cache,shifts,self.keys,explicit,self.can_move,debug,invisible,self.cursor_result,cell,bridge=profile
        self.put(0xA8ECC8,selected);self.byte(0xB0FE58,explicit);self.byte(0xA8ED6B,debug)
        self.byte(BASE+0x9C8D,shroud_move);self.byte(BASE+0xA701,invisible)
        self.put(BASE+0x1014,0 if self.kind==1 else 1)
        self.put(BASE+0x3014,0 if self.kind==1 else 1)
        self.byte(MAP+0x11B3,planning);self.put(MAP+0x11BC,BASE+0x80000 if drag else 0)
        self.byte(MAP+0x11D0,1);self.c.mem_write(MAP+0x11CC,bytes(cache))
        self.put(BASE+0x1020C,self.path);self.put(BASE+0x70140,0x100 if bridge else 0)
        self.put(0x8A0DD0,*shifts);self.c.mem_write(BASE+0x600,struct.pack('<hh',*cell))
        self.call('ConvertRadarAction',0x4AAE90,MAP,[BASE+0x600,fog,BASE+0x3000 if self.kind else 0,action,mini])
        return {'result':bool(self.c.reg_read(UC_X86_REG_EAX)&255),'trace':self.trace,
                'display':bytes(self.c.mem_read(MAP+0x11CC,5)).hex(),'hover':self.c.mem_read(BASE+0x3431,1)[0],
                'path':self.read(BASE+0x1020C)[0],'palette':bytes(self.c.mem_read(BASE+0xC000,512)).hex(),'dragged':self.read(BASE+0x80000,3)}
    def attack_move(self,keys,explicit,flags,can_move,mutate):
        self.setup();self.keys=keys;self.can_move=can_move;self.mutate=mutate
        self.put(0xA8ECC8,len(flags));self.byte(0xB0FE58,explicit)
        for i,flag in enumerate(flags):
            if flag==-1:self.put(BASE+0x500+4*i,0)
            else:self.put(BASE+0x1000+i*0x1000+0x14,flag)
        self.call('RadarAttackMove',0x731BF0,0,[])
        return {'result':bool(self.c.reg_read(UC_X86_REG_EAX)&255),'trace':self.trace,'count':self.read(0xA8ECC8)[0]}
    def mouse(self,operation,index,mini,startup,same,art,ticks):
        self.setup();self.mode='mouse';self.ticks=ticks
        self.put(0xABF294,BASE+0x800 if art else 0);self.byte(0xABF2DD,startup)
        self.c.mem_write(BASE+0x800,struct.pack('<4h',0,53,37,600))
        self.put(0x887640,BASE+0x900);self.put(BASE+0x900,BASE+0xA00);self.put(BASE+0xA04,BASE+0x50050)
        self.put(BASE+0x6004C,self.exports['MouseOverride'] if self.candidate else 0x5BDC80)
        self.put(MAP+0x5560,index if same else (index+1)%86,index,7);self.byte(MAP+0x555C,mini if same else 1-mini)
        self.put(0xABF2A0,123,0x12345678,19)
        name,entry,args={'set':('MouseSet',0x5BDA80,[index,mini]),'override':('MouseOverride',0x5BDC80,[index,mini]),
                         'restore':('MouseRestore',0x5BDAA0,[]),'mini':('MouseMini',0x5BDAB0,[mini]),'last':('MouseLast',0x40D280,[])}[operation]
        self.call(name,entry,MAP,args)
        result=self.c.reg_read(UC_X86_REG_EAX)
        return {'result':None if operation=='mini' else (result if operation=='last' else bool(result&255)),
                'trace':self.trace,'state':bytes(self.c.mem_read(MAP+0x555C,16)).hex(),
                'initialized':self.c.mem_read(0xABF2DD,1)[0],
                'timer':[self.read(0xABF2A0)[0],self.read(0xABF2A8)[0]]}

def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,symbols=build(a.compile_db,a.output)
    exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
    rng=random.Random(0x4AAE90);profiles=[]
    for i in range(24):
        profiles.append((i%4,(i//4)%2,(i//8)%2,i%2,(i//2)%2,(-1,0,5,11,23)[i%5],i%3,i%2,(i//3)%2,
            (0,0,0) if i%2 else (231,123,75),(11,3,0,3,5,2) if i%2 else (10,3,0,3,5,3),i%16,(i//3)%2,i%2,i%2,(i//2)%2,i%2,
            (-1,-1) if i%6==0 else (-12,87),i%2))
    # Independent flags avoid correlated profiles hiding an entire branch.
    for _ in range(32):
        profiles.append((rng.randrange(4),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.choice((-1,0,5,11,23)),
            rng.randrange(3),rng.randrange(2),rng.randrange(2),rng.choice(((0,0,0),(231,123,75))),
            rng.choice(((11,3,0,3,5,2),(10,3,0,3,5,3))),rng.randrange(16),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),
            rng.choice(((0,0),(-1,-1),(40,50),(-12,87))),rng.randrange(2)))
    cases={'convert':[(action,fog,mini,profile) for action,fog,mini,profile in itertools.product(range(73),(0,1),(0,1),profiles)],
           'attack_move':list(itertools.product(range(16),(0,1),((),(0,),(1,),(1,1),(0,1),(-1,1)),(0,1),(0,1)))}
    cases['mouse']=[]
    for op,index,mini,startup,same in itertools.product(('set','override','restore','mini','last'),range(86),(0,1),(0,1),(0,1)):
        cases['mouse'].append((op,index,mini,startup,same,1,0xF1234567 if index%2 else 1234))
    for op,index,mini in itertools.product(('set','override','restore'),range(86),(0,1)):
        cases['mouse'].append((op,index,mini,1,0,0,1234))
    differences=[]
    for method,inputs in cases.items():
        for case in inputs:
            expected=getattr(old,method)(*case);actual=getattr(new,method)(*case)
            if expected!=actual:differences.append({'method':method,'input':case,'original':expected,'candidate':actual})
    report={'exe_sha256':SHA,'cases':{k:len(v) for k,v in cases.items()},'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'cursor-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
    if differences:print(json.dumps(differences[:2],indent=2))
    return bool(differences)
if __name__=='__main__':raise SystemExit(main())
