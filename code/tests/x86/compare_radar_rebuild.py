#!/usr/bin/env python3
"""Original Radar reset/postload ownership, order and movie recovery.

Both execute the original hash allocation and mode routines. Range and image
construction are trace boundaries; their graphics differ by approved policy.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
BASE,OBJ,STACK,STOP=0x01000000,0x87F7E8,0x013E0000,0x013F0000
SOURCES=['code/core/src/yrpp/RadarClassRebuild.cpp','code/tests/x86/radar_rebuild_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'radar_rebuild_probe.dll',
        object_suffix='-rebuild.obj')
class Machine:
    def __init__(self,exe,dll,candidate):
        self.c=Uc(UC_ARCH_X86,UC_MODE_32);self.candidate=candidate
        for pe in (exe,dll):
            base=pe.OPTIONAL_HEADER.ImageBase;self.c.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);self.c.mem_write(base,pe.get_memory_mapped_image())
        self.c.mem_map(BASE,0x400000);self.c.mem_map(0,4096)
        self.exports={s.name.decode().strip('@').split('@')[0]:dll.OPTIONAL_HEADER.ImageBase+s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols}
        self.c.hook_add(UC_HOOK_CODE,self.hook)
    def put(self,p,*v):self.c.mem_write(p,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
    def read(self,p,n=1):return struct.unpack('<'+'I'*n,self.c.mem_read(p,n*4))
    def ret(self,v=0,pop=0):
        sp=self.c.reg_read(UC_X86_REG_ESP);self.c.reg_write(UC_X86_REG_EIP,self.read(sp)[0]);self.c.reg_write(UC_X86_REG_ESP,sp+4+pop);self.c.reg_write(UC_X86_REG_EAX,v)
    def resources(self):return [self.read(OBJ+i)[0] for i in (0x121C,0x1220,0x123C,0x1258,0x1274)]
    def hook(self,c,pc,size,data):
        sp=c.reg_read(UC_X86_REG_ESP)
        if pc==0x7C8E17:
            n=self.read(sp+4)[0];assert n<0x10000
            p=self.heap;self.heap+=(n+15)&~15;self.c.mem_write(p,b'\xCD'*n);self.trace.append(['allocate',n,p]);self.ret(p)
        elif pc in (0x7C8B3D,self.exports['RadarRebuildFree']):self.trace.append(['free',self.read(sp+4)[0]]);self.ret()
        elif pc==0x654490:self.trace.append(['range',c.reg_read(UC_X86_REG_ECX),self.read(sp+4)[0],self.resources()]);self.ret(pop=4)
        elif pc==0x654650:self.trace.append(['image',c.reg_read(UC_X86_REG_ECX),self.resources()]);self.ret()
        elif pc==0x656CB0:self.trace.append(['mode',*self.read(sp+4,2)]) # Execute original state transition.
        elif pc==0x406060:self.trace.append(['audio_stop']);self.ret()
    def call(self,pc):
        self.put(0,0xffffffff);self.put(STACK,STOP);self.c.reg_write(UC_X86_REG_ESP,STACK);self.c.reg_write(UC_X86_REG_ECX,OBJ);self.c.reg_write(UC_X86_REG_EDX,0)
        self.c.emu_start(pc,STOP,count=2000000)
        assert self.c.reg_read(UC_X86_REG_EIP)==STOP,'instruction limit/trap'
        assert self.c.reg_read(UC_X86_REG_ESP)==STACK+4,'stack imbalance'
    def run(self,post,mode,pending,available,old_hash):
        self.c.mem_write(OBJ,bytes(0x150C));self.c.mem_write(BASE,bytes(0x100000));self.trace=[];self.heap=BASE+0x100000
        self.put(OBJ,BASE+0x4000);self.put(BASE+0x4000,*([0x654490]*64));self.put(OBJ+0xFC,2,2,96,92)
        self.put(OBJ+0x14AC,1,mode,pending,7);self.put(OBJ+0x14FC,17);self.put(OBJ+0x1500,123,0,11)
        self.c.mem_write(OBJ+0x14D8,bytes([available]));self.put(0xA8B238,0);self.put(0xA83D4C,0)
        self.put(0x8871E0,BASE+0x5000);self.put(BASE+0x5000,13,20);self.put(0x829FF4,20)
        self.put(0xA8EC7C,BASE+0x6000);self.put(0xA8EC88,4)
        for i in range(4):self.put(BASE+0x6000+4*i,BASE+0x10000+i*0x1000);self.c.mem_write(BASE+0x10423+i*0x1000,bytes([i%2]))
        if old_hash:
            self.call(0x6558D0);self.trace=[]
        for off,value in zip((0x121C,0x1220,0x123C,0x1274),(0x11111111,0x22222222,0x33333333,0x55555555)):self.put(OBJ+off,value)
        if post:self.put(OBJ+0x1258,0x44444444) # Serialized address is not a live allocation.
        self.call(self.exports['RadarPostLoad' if post else 'RadarReset'] if self.candidate else (0x655B20 if post else 0x655990))
        return {'trace':self.trace,'resources':self.resources(),'state':self.read(OBJ+0x14AC,4),'frame':self.read(OBJ+0x14FC)[0],
                'timer':self.read(OBJ+0x1500,3),'detail':self.read(0x829FF4)[0],
                'tracked':[self.c.mem_read(BASE+0x10423+i*0x1000,1)[0] for i in range(4)]}
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert digest(a.exe)==SHA
    a.output.mkdir(parents=True,exist_ok=True);dll=build(a.compile_db,a.output);exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,False),Machine(exe,image,True)
    differences=[];count=0
    for case in itertools.product((False,True),range(5),range(5),(0,1),(False,True)):
        expected=old.run(*case);actual=new.run(*case);count+=1
        if expected!=actual:differences.append({'input':case,'original':expected,'candidate':actual})
    report={'exe_sha256':SHA,'cases':count,'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'rebuild-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(differences)
if __name__=='__main__':raise SystemExit(main())
