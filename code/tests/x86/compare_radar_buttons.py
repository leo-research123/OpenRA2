#!/usr/bin/env python3
"""Original 0x00653850 dialog key branch vs compiled production input.

Other Update duties are controlled boundaries, not certified by this test.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
BASE,STACK,STOP=0x01000000,0x0101E000,0x0101F000
SOURCES=['code/core/src/yrpp/RadarClassButtons.cpp','code/tests/x86/radar_buttons_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'radar_buttons_probe.dll',
        object_suffix='-buttons.obj')
class Machine:
    def __init__(self,exe,dll,candidate):
        self.c=Uc(UC_ARCH_X86,UC_MODE_32);self.candidate=candidate
        for pe in (exe,dll):
            base=pe.OPTIONAL_HEADER.ImageBase
            self.c.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
            self.c.mem_write(base,pe.get_memory_mapped_image())
        self.c.mem_map(BASE,0x20000);self.c.mem_map(0,4096)
        self.entry=next(dll.OPTIONAL_HEADER.ImageBase+s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols if b'RadarButtonKey' in s.name)
        self.c.hook_add(UC_HOOK_CODE,self.hook)
    def put(self,p,*v):self.c.mem_write(p,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
    def read(self,p):return struct.unpack('<I',self.c.mem_read(p,4))[0]
    def ret(self,v=0,pop=0):
        sp=self.c.reg_read(UC_X86_REG_ESP);self.c.reg_write(UC_X86_REG_EIP,self.read(sp))
        self.c.reg_write(UC_X86_REG_ESP,sp+4+pop);self.c.reg_write(UC_X86_REG_EAX,v)
    def hook(self,c,pc,size,data):
        if pc==0x4A9700:self.ret(pop=8) # Parent Update has its own audit.
        elif pc==0x53BAE0:self.ret(1) # Audio not part of this key branch.
        elif pc in (BASE+0x5000,BASE+0x5010):
            slot=8 if pc==BASE+0x5000 else 4
            self.calls.append(slot);self.ret(self.flags[0 if slot==8 else 1])
    def run(self,mode,key,flags):
        self.c.mem_write(BASE,bytes(0x19000));self.put(0,0xffffffff)
        self.calls=[];self.flags=flags or (0,0)
        self.put(0xA8B238,mode,BASE+0x4000 if flags is not None else 0)
        self.put(BASE+0x4000,BASE+0x4100);self.put(BASE+0x4104,BASE+0x5010,BASE+0x5000)
        self.put(0xA8EDA0,0x1234);self.put(0xB45B68,0)
        self.put(BASE+0x2000,key);self.put(BASE+0x2100,0x7fffffff,0x7fffffff)
        self.put(STACK,STOP,*([key] if self.candidate else [BASE+0x2000,BASE+0x2100]))
        self.c.reg_write(UC_X86_REG_ESP,STACK);self.c.reg_write(UC_X86_REG_ECX,BASE)
        self.c.reg_write(UC_X86_REG_EDX,0)
        self.c.emu_start(self.entry if self.candidate else 0x653850,STOP,count=100000)
        assert self.c.reg_read(UC_X86_REG_EIP)==STOP,'instruction limit/trap'
        assert self.c.reg_read(UC_X86_REG_ESP)==STACK+(8 if self.candidate else 12),'stack imbalance'
        return {'dialog':self.read(0xA8EDA0),'mode_virtual_calls':self.calls}
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert digest(a.exe)==SHA
    a.output.mkdir(parents=True,exist_ok=True);dll=build(a.compile_db,a.output)
    exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));original,candidate=Machine(exe,image,False),Machine(exe,image,True)
    differences=[];count=0
    for mode,key,flags in itertools.product(range(6),[0,0x80F2,0x80F3,0x80F4],[None,(0,0),(0,1),(1,0),(1,1)]):
        expected=original.run(mode,key,flags);actual=candidate.run(mode,key,flags);count+=1
        if expected!=actual:differences.append({'input':[mode,key,flags],'original':expected,'candidate':actual})
    report={'exe_sha256':SHA,'cases':count,'mismatches':len(differences),'differences':differences,
            'boundary':'dialog key branch only; parent update, mouse, audio and movies excluded',
            'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'buttons-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    return bool(differences)
if __name__=='__main__':raise SystemExit(main())
