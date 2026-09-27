#!/usr/bin/env python3
"""Original RTactical right-press navigation vs production Radar::Navigate.

The original full Action executes its navigation branch. Resolve_Radar_Point,
map cell lookup, floor height and final SetTacticalPosition are shared/trace
boundaries; both callers execute the original GScreen redraw body. This audits
the ordered 16-bit cell clamp, call arguments/order and depth reset, not native
redraw implementation, command dispatch, cursor or camera internals.
"""
import argparse,hashlib,itertools,json,random,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
BASE,OBJ,STACK,STOP=0x01000000,0x87F7E8,0x0101E000,0x0101F000
SOURCES=['code/core/src/yrpp/RadarClassNavigation.cpp','code/tests/x86/radar_navigation_probe.cpp']
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'radar_navigation_probe.dll',
        object_suffix='-navigation.obj')
class Machine:
    def __init__(self,exe,dll,candidate):
        self.c=Uc(UC_ARCH_X86,UC_MODE_32);self.candidate=candidate
        for pe in (exe,dll):
            base=pe.OPTIONAL_HEADER.ImageBase
            self.c.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
            self.c.mem_write(base,pe.get_memory_mapped_image())
        self.c.mem_map(BASE,0x20000);self.c.mem_map(0,4096)
        self.entry=next(dll.OPTIONAL_HEADER.ImageBase+s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols if b'RadarNavigate' in s.name)
        self.c.hook_add(UC_HOOK_CODE,self.hook)
    def put(self,p,*v):self.c.mem_write(p,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
    def read(self,p,n=1):return struct.unpack('<'+'I'*n,self.c.mem_read(p,4*n))
    def cell(self,p):return struct.unpack('<hh',self.c.mem_read(p,4))
    def ret(self,v=0,pop=0):
        sp=self.c.reg_read(UC_X86_REG_ESP);self.c.reg_write(UC_X86_REG_EIP,self.read(sp)[0])
        self.c.reg_write(UC_X86_REG_ESP,sp+4+pop);self.c.reg_write(UC_X86_REG_EAX,v&0xffffffff)
    def hook(self,c,pc,size,data):
        sp=c.reg_read(UC_X86_REG_ESP)
        if pc==0x656750:
            point,cell,obj=self.read(sp+4,3)
            self.trace.append(['pick',*self.read(point,2)])
            self.c.mem_write(cell,struct.pack('<hh',*self.target));self.put(obj,0);self.ret(pop=12)
        elif pc in (0x578080,0x586360):self.ret(0,pop=4) # Pre-navigation shroud check.
        elif pc==0x5657A0:
            xy=self.cell(self.read(sp+4)[0]);self.trace.append(['cell',*xy])
            self.c.mem_write(BASE+0x5024,struct.pack('<hh',*xy));self.ret(BASE+0x5000,pop=4)
        elif pc==0x47B3A0:self.ret(self.height,pop=4)
        elif pc==0x6D6070:
            self.trace.append(['camera',*struct.unpack('<iii',self.c.mem_read(self.read(sp+4)[0],12))]);self.ret(pop=4)
        elif pc==0x4F42F0:self.trace.append(['redraw',self.read(sp+4)[0]]) # Execute original body.
        elif pc==0x4E1530:self.ret(pop=12) # Gadget base call with flags=0.
        elif pc==0x5BDC80:self.ret(pop=8) # Outside-content cursor.
    def run(self,map_size,viewport,target,point,depth,height,sidebar):
        c=self.c;c.mem_write(BASE,bytes(0x19000));c.mem_write(OBJ,bytes(0x150C))
        self.put(0,0xffffffff);self.target=target;self.trace=[];self.height=height
        self.put(OBJ,BASE+0x4000);self.put(BASE+0x4000,*([0x4F42F0]*64))
        self.put(BASE+0x4000+0xD0,0x653F70) # Real wrapper into final camera method.
        self.put(OBJ+0xEC,7,9,*map_size);self.put(OBJ+0x123C,BASE+0x6000)
        self.put(OBJ+0x149C,16,49,140,108);self.put(OBJ+0x14AC,1,1)
        self.put(0x886FA0,0,0,*viewport);self.put(0x887324,BASE+0x8000)
        self.put(0x887644,BASE+0x7000 if depth else 0);self.put(BASE+0x7024,0x6543)
        self.put(0xB048C0,0xffffffff);self.put(0x8809A0,0xffffffff);self.put(0xA8ECC8,0)
        c.mem_write(0xA8EB7C,bytes([sidebar]));self.put(0x87F770,BASE+0x2000)
        self.put(BASE+0x2000,point[0]+(viewport[0] if sidebar else 0),point[1]);self.put(BASE+0x2100,*point)
        self.put(OBJ+0x0C,3);self.put(OBJ+0x1158,0x1234FFFF)
        args=[BASE+0x2100] if self.candidate else [0x10,BASE+0x2200,0]
        self.put(STACK,STOP,*args);c.reg_write(UC_X86_REG_ESP,STACK);c.reg_write(UC_X86_REG_ECX,OBJ if self.candidate else BASE+0x3000);c.reg_write(UC_X86_REG_EDX,0)
        c.emu_start(self.entry if self.candidate else 0x6539D0,STOP,count=100000)
        assert c.reg_read(UC_X86_REG_EIP)==STOP,'instruction limit/trap'
        assert c.reg_read(UC_X86_REG_ESP)==STACK+4*(1+len(args)),'stack imbalance'
        return {'trace':self.trace,'redraw_bitfield':self.read(OBJ+0x0C)[0],
                'redraw_count':self.read(OBJ+0x1158)[0],'tactical_redrawing':c.mem_read(BASE+0x8D7D,1)[0],
                'depth_maximum':self.read(BASE+0x7024)[0]}
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll=build(a.compile_db,a.output)
    exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));original,candidate=Machine(exe,image,False),Machine(exe,image,True)
    cases=[];rng=random.Random(0x6539D0)
    for m,v,d,s in itertools.product(((100,100),(40,180),(180,40),(8,8)),((472,448),(856,736),(1112,688),(1752,1048)),(0,1),(0,1)):
        targets=[(-1,-1),(0,0),(1,1),(m[0],m[0]),(1,m[0]),(m[0],1),(m[0]+m[1]-1,m[1]-1),(m[1]-1,m[0]+m[1]-1),(-32768,32767),(32767,-32768)]
        targets += [(rng.randrange(-16,513),rng.randrange(-16,513)) for _ in range(10)]
        for target in targets:cases.append((m,v,target,(70,80),d,rng.choice((0,104,208,416)),s))
        for point in ((15,49),(16,48),(156,49),(16,157),(16,49),(155,156)):
            cases.append((m,v,(70,90),point,d,104,s))
    differences=[]
    for case in cases:
        expected=original.run(*case);actual=candidate.run(*case)
        if expected!=actual:differences.append({'input':case,'original':expected,'candidate':actual})
    report={'exe_sha256':SHA,'cases':len(cases),'mismatches':len(differences),'differences':differences,'boundary':__doc__,
            'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'navigation-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
    if differences:print(json.dumps(differences[:3],indent=2))
    return bool(differences)
if __name__=='__main__':raise SystemExit(main())
