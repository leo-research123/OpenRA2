#!/usr/bin/env python3
"""YR 0x005353D0 vs production selection representative used by radar input.

Object virtual predicates, coords and CombatDamage are trace boundaries. sqrt
executes the original math routine. Compare result identity, virtual call order,
priority gates, target distance tie breaking and persistent x87 rounding mode.
This does not certify the predicates themselves or subsequent command dispatch.
"""
import argparse,hashlib,itertools,json,random,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
BASE,STACK,STOP=0x01000000,0x0103E000,0x0103F000
SOURCES=['code/core/src/yrpp/UnsortedSelection.cpp','code/tests/x86/selection_representative_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'selection_representative_probe.dll',
        object_suffix='-selection.obj')
class Machine:
    def __init__(self,exe,dll,candidate):
        self.c=Uc(UC_ARCH_X86,UC_MODE_32);self.candidate=candidate
        for pe in (exe,dll):
            base=pe.OPTIONAL_HEADER.ImageBase;self.c.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
            self.c.mem_write(base,pe.get_memory_mapped_image())
        self.c.mem_map(BASE,0x40000);self.c.mem_map(0,4096)
        self.exports={s.name.decode().strip('@').split('@')[0]:dll.OPTIONAL_HEADER.ImageBase+s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols}
        self.c.hook_add(UC_HOOK_CODE,self.hook)
    def put(self,p,*v):self.c.mem_write(p,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
    def read(self,p):return struct.unpack('<I',self.c.mem_read(p,4))[0]
    def ret(self,v=0,pop=0):
        sp=self.c.reg_read(UC_X86_REG_ESP);self.c.reg_write(UC_X86_REG_EIP,self.read(sp))
        self.c.reg_write(UC_X86_REG_ESP,sp+4+pop);self.c.reg_write(UC_X86_REG_EAX,v&0xffffffff)
    def hook(self,c,pc,size,data):
        sp=c.reg_read(UC_X86_REG_ESP);obj=c.reg_read(UC_X86_REG_ECX)
        if pc==self.exports['SelectionSetRound']:
            c.reg_write(UC_X86_REG_FPCW,0xE7F);c.reg_write(UC_X86_REG_MXCSR,0x7F80);self.ret()
        elif pc in (BASE+0x30000,BASE+0x30010,BASE+0x30020,BASE+0x30030,0x6F3970):
            name={BASE+0x30000:'emp',BASE+0x30010:'armed',BASE+0x30020:'type',BASE+0x30030:'coords',0x6F3970:'damage'}[pc]
            index=(obj-BASE-0x1000)//0x1000;config=self.objects[index]
            self.trace.append([name,index])
            if name=='coords':
                out=self.read(sp+4);self.put(out,*config['coords']);self.ret(out,4)
            elif name=='damage':
                assert self.read(sp+4)==0xffffffff;self.ret(config[name],4)
            else:self.ret(config[name])
    def run(self,objects,mode,cell,target,cw):
        c=self.c;c.mem_write(BASE,bytes(0x30000));self.put(0,0xffffffff);self.trace=[]
        self.objects=[*objects,dict(coords=target)]
        self.put(0xA8ECBC,BASE+0x500);self.put(0xA8ECC8,len(objects))
        for i,config in enumerate(self.objects):
            obj=BASE+0x1000+i*0x1000;self.put(BASE+0x500+4*i,obj);self.put(obj,BASE+0x28000)
            self.put(obj+0x14,config.get('techno',0));c.mem_write(obj+0x298,bytes([config.get('berzerk',0)]))
        for offset,pc in ((0x37C,0x30000),(0x2AC,0x30010),(0x2C,0x30020),(0x48,0x30030)):
            self.put(BASE+0x28000+offset,BASE+pc)
        c.mem_write(BASE+0x600,struct.pack('<hh',*cell))
        self.put(STACK,STOP);c.reg_write(UC_X86_REG_ESP,STACK)
        c.reg_write(UC_X86_REG_ECX,BASE+0x600 if mode&1 else 0)
        c.reg_write(UC_X86_REG_EDX,BASE+0x1000+len(objects)*0x1000 if mode&2 else 0)
        c.reg_write(UC_X86_REG_FPCW,cw);c.reg_write(UC_X86_REG_MXCSR,0x7F80 if cw==0xE7F else 0x1F80)
        c.emu_start(self.exports['SelectionRepresentative'] if self.candidate else 0x5353D0,STOP,count=100000)
        assert c.reg_read(UC_X86_REG_EIP)==STOP,'instruction limit/trap'
        assert c.reg_read(UC_X86_REG_ESP)==STACK+4,'stack imbalance'
        return {'result':c.reg_read(UC_X86_REG_EAX),'trace':self.trace,'rounding':c.reg_read(UC_X86_REG_FPCW)&0xF00}
def actor(techno=1,berzerk=0,emp=0,armed=0,type=1,damage=1,coords=(0,0,0)):
    return dict(techno=techno,berzerk=berzerk,emp=emp,armed=armed,type=type,damage=damage,coords=coords)
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll=build(a.compile_db,a.output)
    exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,False),Machine(exe,image,True)
    cases=[];rng=random.Random(0x5353D0)
    archetypes=[actor(techno=0),actor(berzerk=1,armed=1),actor(emp=1,armed=1),actor(),actor(armed=1,type=6),actor(armed=1,damage=0),actor(armed=1,damage=-1),actor(armed=1)]
    for i,j,mode,cw in itertools.product(range(8),range(8),range(4),(0x27F,0xE7F)):
        objects=[dict(archetypes[i],coords=(20,30,40)),dict(archetypes[j],coords=(23,34,40))]
        cases.append((objects,mode,(0,0),(25,35,40),cw))
    for mode,cw in itertools.product(range(4),(0x27F,0xE7F)):
        cases.append(([],mode,(0,0),(0,0,0),cw))
        # Fractional distances truncate to a tie; preserve the first object.
        cases.append(([actor(coords=(2,0,0)),actor(coords=(2,1,0))],mode,(0,0),(0,0,0),cw))
        # Integer subtraction must wrap before conversion to floating point.
        cases.append(([actor(coords=(-0x80000000,0,0)),actor(coords=(0x7FFFFFFF,0,0))],mode,(-32768,32767),(-0x80000000,0,0),cw))
    for _ in range(256):
        objects=[dict(rng.choice(archetypes),coords=tuple(rng.randrange(-131072,131073) for _ in range(3))) for _ in range(rng.randrange(1,9))]
        cases.append((objects,rng.randrange(4),(rng.randrange(-100,100),rng.randrange(-100,100)),(123,456,789),rng.choice((0x27F,0xE7F))))
    differences=[]
    for case in cases:
        expected=old.run(*case);actual=new.run(*case)
        if expected!=actual:differences.append({'input':case,'original':expected,'candidate':actual})
    report={'exe_sha256':SHA,'cases':len(cases),'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'selection-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
    if differences:print(json.dumps(differences[:3],indent=2))
    return bool(differences)
if __name__=='__main__':raise SystemExit(main())
