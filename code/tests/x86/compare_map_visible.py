#!/usr/bin/env python3
"""Compare original/full compiled Map SetVisibleRect and virtual-call order.

Original 0x578460 performs height-aware membership for both. See executes as a
trace boundary; this does not certify Techno sight propagation.
"""
import argparse,hashlib,itertools,json,random,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
BASE,MAP,STACK,STOP=0x01000000,0x87F7E8,0x0131E000,0x0131F000
SOURCES=['code/core/src/yrpp/MapClassVisibleRect.cpp','code/core/src/yrpp/DrawingRectangle.cpp','code/tests/x86/map_visible_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'map_visible_probe.dll',
        object_suffix='-visible.obj')
class Machine:
    def __init__(self,exe,dll,candidate):
        self.c=Uc(UC_ARCH_X86,UC_MODE_32);self.candidate=candidate
        for pe in (exe,dll):
            base=pe.OPTIONAL_HEADER.ImageBase;self.c.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
            self.c.mem_write(base,pe.get_memory_mapped_image())
        self.c.mem_map(BASE,0x400000);self.c.mem_map(0,4096)
        self.entry=next(dll.OPTIONAL_HEADER.ImageBase+s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols if b'MapVisibleRect' in s.name)
        self.c.hook_add(UC_HOOK_CODE,self.hook)
    def put(self,p,*v):self.c.mem_write(p,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
    def read(self,p,n=1):return struct.unpack('<'+'I'*n,self.c.mem_read(p,4*n))
    def ret(self,v=0,pop=0):
        sp=self.c.reg_read(UC_X86_REG_ESP);self.c.reg_write(UC_X86_REG_EIP,self.read(sp)[0]);self.c.reg_write(UC_X86_REG_ESP,sp+4+pop);self.c.reg_write(UC_X86_REG_EAX,v)
    def hook(self,c,pc,size,data):
        obj=c.reg_read(UC_X86_REG_ECX);sp=c.reg_read(UC_X86_REG_ESP)
        if pc==0x4F42F0:self.calls.append(['redraw',self.read(sp+4)[0]]) # Executes original redraw.
        elif pc==0x50B6F0:
            self.calls.append(['control',(obj-BASE-0x30000)//0x100]);self.ret(self.read(obj)[0])
        elif pc==BASE+0x5000:
            out=self.read(sp+4)[0];self.c.mem_write(out,bytes(self.c.mem_read(obj+0x500,4)));self.ret(out,4)
        elif pc==BASE+0x5010:
            self.calls.append(['kind',(obj-BASE-0x10000)//0x1000]);self.ret(self.read(obj+0x504)[0])
        elif pc==BASE+0x5020:
            self.calls.append(['see',(obj-BASE-0x10000)//0x1000,*self.read(sp+4,2)]);self.ret(pop=8)
    def run(self,world,rect,seed):
        rng=random.Random(seed);self.calls=[];self.c.mem_write(BASE,bytes(0x300000));self.put(0,0xffffffff)
        self.put(MAP,BASE+0x6000);self.put(MAP+0xC,seed%4);self.put(MAP+0xEC,*world);self.put(MAP+0x1158,0xABCDEF00|(seed%256))
        self.put(BASE+0x6000,*([0x4F42F0]*32));self.put(MAP+0x13C,BASE+0x100000)
        self.put(0x887324,BASE+0x40000);self.put(BASE+0x2000,*rect)
        self.put(0xA8EC7C,BASE+0x7000);self.put(0xA8EC88,8)
        self.put(BASE+0x8000+0x1B8,BASE+0x5000);self.put(BASE+0x8000+0x2C,BASE+0x5010);self.put(BASE+0x8000+0x120,BASE+0x5020)
        # Missing cells use the original InvalidCell fallback at 0x00ABDC50.
        self.c.mem_write(0xABDC50,bytes(0x148))
        for i in range(8):
            obj=BASE+0x10000+i*0x1000;owner=BASE+0x30000+i*0x100;cell=BASE+0x50000+i*0x200
            self.put(BASE+0x7000+4*i,obj);self.put(obj,BASE+0x8000);self.put(obj+0x21C,owner)
            self.put(owner,rng.randrange(2));self.put(obj+0x504,6 if i==0 else 73)
            self.c.mem_write(obj+0x3D5,bytes([rng.randrange(2)]));self.c.mem_write(obj+0x81,bytes([rng.randrange(2)]));self.c.mem_write(obj+0x90,bytes([rng.randrange(2)]))
            x,y=rng.randrange(1,150),rng.randrange(1,150);self.put(obj+0x500,(y<<16)|x)
            self.put(BASE+0x100000+4*(x+y*512),cell);self.c.mem_write(cell+0x11B,bytes([rng.randrange(14),rng.randrange(3)]))
        self.put(STACK,STOP,BASE+0x2000);self.c.reg_write(UC_X86_REG_ESP,STACK);self.c.reg_write(UC_X86_REG_ECX,MAP);self.c.reg_write(UC_X86_REG_EDX,0)
        self.c.emu_start(self.entry if self.candidate else 0x567230,STOP,count=1000000)
        assert self.c.reg_read(UC_X86_REG_EIP)==STOP,'instruction limit/trap'
        assert self.c.reg_read(UC_X86_REG_ESP)==STACK+8,'stack imbalance'
        return {'rect':self.read(MAP+0xFC,4),'bitfield':self.read(MAP+0xC)[0],'redraws':self.read(MAP+0x1158)[0],
                'tactical_redraw':self.c.mem_read(BASE+0x40000+0xD7D,1)[0],
                'inside':[self.c.mem_read(BASE+0x10000+i*0x1000+0x3D5,1)[0] for i in range(8)],'calls':self.calls}
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True)
    dll=build(a.compile_db,a.output);exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll))
    original,candidate=Machine(exe,image,False),Machine(exe,image,True);differences=[];count=0;reentries=0
    rectangles=[(-10,-10,160,160),(0,0,100,100),(2,2,96,92),(5,3,40,52),(150,150,20,20),(3,7,0,9),(1,2,-1,3)]
    for world,rect,seed in itertools.product([(0,0,100,100),(0,0,64,60),(7,9,80,100)],rectangles,range(32)):
        expected=original.run(world,rect,seed);actual=candidate.run(world,rect,seed);count+=1;reentries+=sum(c[0]=='see' for c in expected['calls'])
        if expected!=actual:differences.append({'input':[world,rect,seed],'original':expected,'candidate':actual})
    report={'exe_sha256':SHA,'cases':count,'observed_see_calls':reentries,'mismatches':len(differences),'differences':differences,
            'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'visible-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(differences)
if __name__=='__main__':raise SystemExit(main())
