#!/usr/bin/env python3
"""Original radar sight dependencies vs compiled production bodies.

Map cell lookup, z projection, occlusion, device registration and discovery are
explicit shared/trace boundaries. See/radius tests stop at their next reveal
entry; Display tests execute counters and RevealCheck. Memory-fog snapshots
are excluded, consistently with the project's FoggedObject exclusion.
"""
import argparse,hashlib,itertools,json,random,re,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
BASE,MAP,STACK,STOP=0x01000000,0x87F7E8,0x01FFE000,0x01FFF000
PTRS,CELLS,COORD,TECHNO,TYPE,HOUSE,RULES=BASE+0x200000,BASE+0x400000,BASE+0x1000,BASE+0x2000,BASE+0x4000,BASE+0x10000,BASE+0x6000
N=64
SOURCES=['code/core/src/yrpp/'+s+'.cpp' for s in ['TechnoClassSight','AircraftClassSight','MapClassSight','CellClassShroud','DisplayClassVisibility','MapClassRevealCheck','CellSpread']]+['code/tests/x86/radar_sight_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'radar_sight_probe.dll',
        object_suffix='-sight.obj',
        symbols=True)
class Machine:
    def __init__(self,exe,dll,symbols,candidate):
        self.c=Uc(UC_ARCH_X86,UC_MODE_32);self.candidate=candidate;self.mode='setup';self.calls=[];self.cw=0xE7F
        for pe in (exe,dll):
            base=pe.OPTIONAL_HEADER.ImageBase;self.c.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);self.c.mem_write(base,pe.get_memory_mapped_image())
        self.c.mem_map(BASE,0x1000000);self.c.mem_map(0,4096)
        self.exports={re.sub(r'^[@_]|@\d+$','',s.name.decode()):dll.OPTIONAL_HEADER.ImageBase+s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols}
        self.symbols={}
        for line in symbols.read_text().splitlines():
            fields=line.split()
            if len(fields)>=3 and fields[1].startswith('?'):
                try:self.symbols[fields[1]]=int(fields[2],16)
                except ValueError:pass
        self.entry={k:{v for s,v in self.symbols.items() if s.startswith('?'+k+'@')} for k in ['RevealArea1','RevealArea2','MapCellFoggedness','RevealFogShroud','Unshroud']}
        self.c.hook_add(UC_HOOK_CODE,self.hook)
        self.call(0x561910,[]);self.call(0x5638D0,[]);self.call(0x49F2F0,[])
        self.initial=bytes(self.c.mem_read(BASE,0x800000))
    def put(self,p,*v):self.c.mem_write(p,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
    def read(self,p,n=1):return struct.unpack('<'+'I'*n,self.c.mem_read(p,4*n))
    def byte(self,p,v):self.c.mem_write(p,bytes([v&255]))
    def ret(self,v=0,pop=0):
        sp=self.c.reg_read(UC_X86_REG_ESP);self.c.reg_write(UC_X86_REG_EIP,self.read(sp)[0]);self.c.reg_write(UC_X86_REG_ESP,sp+4+pop);self.c.reg_write(UC_X86_REG_EAX,v&0xffffffff)
    def call(self,entry,args,obj=MAP):
        self.put(STACK,STOP,*args);self.c.reg_write(UC_X86_REG_ESP,STACK);self.c.reg_write(UC_X86_REG_ECX,obj);self.c.reg_write(UC_X86_REG_EDX,0)
        self.c.reg_write(UC_X86_REG_FPCW,self.cw);self.c.reg_write(UC_X86_REG_MXCSR,0x7F80 if self.cw==0xE7F else 0x1F80);self.c.emu_start(entry,STOP,count=20000000)
        assert self.c.reg_read(UC_X86_REG_EIP)==STOP,hex(self.c.reg_read(UC_X86_REG_EIP))
        assert self.c.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)*4,'stack imbalance'
        return self.c.reg_read(UC_X86_REG_EAX)&255
    def hook(self,c,pc,size,data):
        sp=c.reg_read(UC_X86_REG_ESP);obj=c.reg_read(UC_X86_REG_ECX)
        if pc==self.exports['SightSetRound']:
            c.reg_write(UC_X86_REG_FPCW,0xE7F);c.reg_write(UC_X86_REG_MXCSR,0x7F80);self.ret()
        elif pc in ({0x5673A0}|self.entry['RevealArea1']) and self.mode=='See':
            args=list(self.read(sp+4,8));self.calls.append(['area1',list(self.read(args[0],3)),*args[1:3],*[x&255 for x in args[3:]]]);self.ret(pop=32)
        elif self.mode in ('AircraftSee','Update','Drop') and pc in ({0x5678E0}|self.entry['RevealArea2']):
            args=list(self.read(sp+4,8));self.calls.append(['area2',list(self.read(args[0],3)),*args[1:3],args[3]&255,args[4],*[x&255 for x in args[5:]]]);self.ret(pop=32)
        elif self.mode.startswith('Area') and pc in ({0x653830,0x4A9DD0,self.exports['SightReveal']}|self.entry['RevealFogShroud']|self.entry['MapCellFoggedness']):
            fog=pc in ({0x4A9DD0}|self.entry['MapCellFoggedness']);args=self.read(sp+4,2 if fog else 3)
            self.calls.append(['fog' if fog else 'reveal',self.read(args[0])[0],args[1],*args[2:]]);self.ret(1,8 if fog else 12)
        elif self.mode.startswith('Area') and pc in ({0x4876F0}|self.entry['Unshroud']):
            self.calls.append(['unshroud',self.read(obj+0x24)[0]]);self.ret()
        elif pc==0x567DA0:
            a=self.read(sp+4,4);self.calls.append(['edge',list(self.read(a[0],3)),*a[1:]]);self.ret(pop=16)
        elif pc==0x6DA7D0:
            cell=self.read(sp+4)[0];self.calls.append(['mark',self.read(cell+0x24)[0]]);self.ret(pop=4)
        elif pc==0x6565A0:
            self.calls.append(['radar',self.read(self.read(sp+4)[0])[0]]);self.ret(pop=4)
        elif pc==0x47C3D0:
            self.calls.append(['nearest',self.read(obj+0x24)[0]]);self.ret(0,pop=12)
        elif pc==BASE+0x100:
            self.calls.append(['type']);self.ret(TYPE)
        elif pc==BASE+0x110:self.calls.append(['mp']);self.ret(self.mp)
        elif pc==BASE+0x120:self.calls.append(['height']);self.ret(self.read(TECHNO+0x800)[0])
    def setup(self,seed):
        self.mode='setup';self.calls=[];self.cw=0xE7F;rng=random.Random(seed);self.c.mem_write(BASE,self.initial)
        self.put(0,0xffffffff);self.put(MAP,BASE+0x800);self.put(MAP+0xEC,0,0,30,30);self.put(MAP+0x13C,PTRS)
        self.c.mem_write(0xB0CD48,struct.pack("<Q",0x3fc25e5374344960));self.put(0xABDE88,104);self.put(0x8871E0,RULES);self.put(0xA83D4C,HOUSE);self.put(0xA8B230,BASE+0xA000);self.put(0x887324,BASE+0xB000)
        self.put(0xA8B238,0,0);self.put(RULES+0x16BC,50);self.c.mem_write(RULES+0x680,struct.pack('<d',1.0));self.byte(RULES+0x17EE,1)
        self.put(HOUSE+0x34,BASE+0x30000);self.put(BASE+0x30000+0xB8,3);self.put(HOUSE+0x30,1)
        self.put(BASE+0x900,BASE+0xA00);self.put(BASE+0xA04,BASE+0x110);self.mp=seed&1
        self.put(BASE+0x700+0x84,BASE+0x100);self.put(TECHNO,BASE+0x700);self.put(TECHNO+0x21C,HOUSE)
        self.byte(TECHNO+0x3D5,1);self.put(TECHNO+0x9C,30*256+128,30*256+128,0);self.put(TYPE+0x5E8,6)
        self.put(BASE+0x800+0x90,self.exports['SightMapCell'] if self.candidate else 0x4A9890)
        self.put(BASE+0x800+0x98,self.exports['SightFog'] if self.candidate else 0x4A9DD0)
        self.put(BASE+0x800+0x94,self.exports['SightReveal'] if self.candidate else 0x653830)
        if seed==0 and hasattr(self,'zero_cells'):
            cells,ptrs=self.zero_cells
        else:
            cells=bytearray(N*N*0x148);ptrs=bytearray(0x100000)
            for y in range(N):
                for x in range(N):
                    i=y*N+x;off=i*0x148;struct.pack_into('<I',ptrs,4*(y*512+x),CELLS+off);struct.pack_into('<hh',cells,off+0x24,x,y)
                    cells[off+0x11B]=rng.randrange(15) if seed&1 else 0
                    cells[off+0x120]=rng.choice([254,255,0,10]);cells[off+0x121]=rng.choice([254,255,0,10])
                    struct.pack_into('<IIIIII',cells,off+0x12C,rng.choice([0,8,16,24]),rng.choice([0,1,2,0xffffffff]),5,0,0,rng.choice([0,1,2,3,0x40,0x60]))
            if seed==0:self.zero_cells=(bytes(cells),bytes(ptrs))
        self.c.mem_write(CELLS,bytes(cells));self.c.mem_write(PTRS,bytes(ptrs));self.c.mem_write(0xABDC50,bytes(0x148));self.put(COORD,30|(30<<16))
    def result(self,value=None):
        return {'value':value,'trace':self.calls,'cells':hashlib.sha256(self.c.mem_read(CELLS,N*N*0x148)).hexdigest()}
    def run_display(self,name,seed):
        self.setup(seed);self.mode=name
        args=[COORD,HOUSE]+([seed&1] if name=='Reveal' else [])
        old={'MapCell':0x4A9890,'Reveal':0x4A9CA0,'Fog':0x4A9DD0}[name]
        return self.result(self.call(self.exports['Sight'+name] if self.candidate else old,args))
    def run_see(self,seed):
        self.setup(0);self.mode='See';self.cw=0x27F if seed&2 else 0xE7F;rng=random.Random(seed)
        self.byte(TECHNO+0x3D5,seed%9!=0);self.byte(BASE+0x30000+0x1A6,seed%4==0);self.put(0xA8B238,seed%5)
        self.put(TECHNO+0xA4,rng.choice([0,49,50,100,500,1200,-50]));self.byte(TECHNO+0x420,rng.choice([0,10,50,127,128,255]))
        self.put(TYPE+0x5E8,rng.choice([0,1,2,3,6,10,12]));self.byte(TYPE+0x2A1,seed&1);self.byte(TYPE+0x2B3,(seed>>1)&1)
        self.c.mem_write(TECHNO+0x150,struct.pack('<f',rng.choice([-1,0,0.99,1,1.5,2,4])));self.c.mem_write(RULES+0x680,struct.pack('<d',rng.choice([0,0.5,1,1.2,2,3])))
        self.call(self.exports['SightSee'] if self.candidate else 0x70ADC0,[seed&1,(seed>>1)&1],TECHNO)
        return {'increase':self.c.mem_read(TECHNO+0x420,1)[0],'trace':self.calls,'fpcw':self.c.reg_read(UC_X86_REG_FPCW)}
    def run_paired(self,name,seed):
        self.setup(0);self.mode=name;self.cw=0x27F if seed&2 else 0xE7F;rng=random.Random(seed)
        self.byte(TECHNO+0x3D5,seed%9!=0);self.byte(BASE+0x30000+0x1A6,seed%4==0)
        self.put(TECHNO+0xA4,rng.choice([0,49,50,100,500,1200,-50]));self.byte(TECHNO+0x420,rng.choice([0,10,50,127,128,255]))
        self.put(TYPE+0x5E8,rng.choice([0,1,2,3,6,10,12]));self.byte(TYPE+0x2A1,seed&1);self.byte(TYPE+0x2B3,(seed>>1)&1)
        self.c.mem_write(TECHNO+0x150,struct.pack('<f',rng.choice([-1,0,0.99,1,1.5,2,4])));self.c.mem_write(RULES+0x680,struct.pack('<d',rng.choice([0,0.5,1,1.2,2,3])))
        self.byte(TECHNO+0x250,seed&1);self.put(TECHNO+0x254,0x123,0x456,0x789,rng.choice([0,1,6,10]))
        args=[(seed>>1)&1,seed-128,(seed>>2)&1,HOUSE+0x10000 if seed&8 else 0]
        if name=='Update':args.append(rng.choice([0,0,0,3,6,11]))
        self.call(self.exports['Sight'+name] if self.candidate else (0x70AF50 if name=='Update' else 0x70B1D0),args,TECHNO)
        return {'increase':self.c.mem_read(TECHNO+0x420,1)[0],'registered':self.c.mem_read(TECHNO+0x250,1)[0],
                'saved':list(self.read(TECHNO+0x254,4)),'trace':self.calls,'fpcw':self.c.reg_read(UC_X86_REG_FPCW)}
    def run_aircraft(self,seed):
        self.setup(0);self.mode='AircraftSee';self.put(TECHNO+0x6C4,TYPE)
        self.put(BASE+0x700+0x1C8,BASE+0x120);self.put(TECHNO+0x800,[-1,0,1,200,256,500][seed%6]);self.put(TYPE+0x5E8,[0,1,6][(seed//6)%3])
        self.put(BASE+0xA000,0x1000 if (seed//18)%2 else 0);self.put(RULES+0xF4,2);self.put(RULES+0x7B4,512)
        self.call(self.exports['SightAircraftSee'] if self.candidate else 0x41ADF0,[seed&1,(seed>>1)&1],TECHNO)
        return {'trace':self.calls}
    def run_area(self,paired,seed):
        self.setup(seed);self.mode='Area2' if paired else 'Area1';rng=random.Random(seed)
        self.put(COORD,rng.choice([7,13,30,43])*256+128,rng.choice([7,30,35,41,45])*256+128,rng.choice([0,104,208,624]))
        flags=[rng.randrange(2) for _ in range(5)];radius=rng.choice([0,1,2,3,6,10,11,20]);self.byte(RULES+0x17EE,seed&1)
        self.put(0xA8B238,seed%5,BASE+0x900 if seed%3 else 0)
        other=BASE+0x40000;self.put(other+0x30,2);self.put(other+0x34,BASE+0x30000)
        owner=HOUSE
        relation=(seed//128)%6
        if relation==1:owner=other
        elif relation==2:owner=other;self.put(other+0x54E4,1<<3)
        elif relation==3:owner=other;self.put(other+0x5788,1<<1);self.byte(RULES+0x17E7,1)
        elif relation==4:owner=other;self.put(other+0x5788,1<<1);self.byte(RULES+0x17E7,0)
        elif relation==5:owner=0
        self.call(self.exports['Sight'+self.mode] if self.candidate else (0x5678E0 if paired else 0x5673A0),[COORD,radius,owner,*flags])
        return self.result()
    def run_counter(self,name,seed):
        self.setup(0);self.mode=name;p=CELLS
        self.put(p+0x12C,[0,8,16,24][seed%4],[-2,-1,0,1,2,3,0x7fffffff,0x80000000][(seed//4)%8],[-1,0,1,5][(seed//32)%4]);self.put(p+0x140,0x4000020 if seed&1 else 0x4000000)
        self.call(self.exports['Sight'+name] if self.candidate else {'Down':0x487630,'Up':0x487690,'Unshroud':0x4876F0}[name],[],p)
        return list(self.read(p+0x12C,6))
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert digest(a.exe)==SHA
    dll,symbols=build(a.compile_db,a.output);exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
    differences=[];counts={}
    def check(kind,seed,fn):
        try:expected=fn(old);actual=fn(new)
        except Exception:
            print('failure',kind,seed,'oldpc',hex(old.c.reg_read(UC_X86_REG_EIP)),'newpc',hex(new.c.reg_read(UC_X86_REG_EIP)),flush=True);raise
        counts[kind]=counts.get(kind,0)+1
        if expected!=actual:differences.append({'kind':kind,'seed':seed,'original':expected,'candidate':actual})
    for name,seed in itertools.product(['Down','Up','Unshroud'],range(128)):check(name,seed,lambda m:m.run_counter(name,seed))
    for seed in range(384):check('See',seed,lambda m:m.run_see(seed))
    for seed in range(144):check('AircraftSee',seed,lambda m:m.run_aircraft(seed))
    for name,seed in itertools.product(['Update','Drop'],range(512)):check(name,seed,lambda m:m.run_paired(name,seed))
    for paired,seed in itertools.product([False,True],range(768)):check('Area2' if paired else 'Area1',seed,lambda m:m.run_area(paired,seed))
    for name,seed in itertools.product(['MapCell','Reveal','Fog'],range(64)):check(name,seed,lambda m:m.run_display(name,seed))
    report={'scope':__doc__,'exe_sha256':SHA,'cases':counts,'mismatches':len(differences),'differences':differences[:12],'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'sight-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(differences)
if __name__=='__main__':raise SystemExit(main())
