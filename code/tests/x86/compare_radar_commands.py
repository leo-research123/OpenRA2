#!/usr/bin/env python3
"""Original display ActiveClick, Map clipping and Foot path assignment.

ActiveClick shares/traces House queries, ClipToMap and virtual actor commands;
checks exact order, feedback, follow-cell aliasing and live/snapshot selection.
ClipToMap executes original membership and cell lookup helpers. Foot assignment
uses real original-layout waypoint storage. Full Radar Action, cursor, event
execution and House/Waypoint lifecycle are separate integration requirements.
"""
import argparse,hashlib,itertools,json,random,re,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
BASE,MAP,STACK,STOP=0x01000000,0x87F7E8,0x013E0000,0x013F0000
SOURCES=['code/core/src/yrpp/DisplayClassCommands.cpp','code/core/src/yrpp/MapClassClip.cpp','code/core/src/yrpp/FootClassWaypoints.cpp','code/tests/x86/radar_commands_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'radar_commands_probe.dll',
        object_suffix='-commands.obj',
        symbols=True)
class Machine:
    def __init__(self,exe,dll,symbols,candidate):
        self.c=Uc(UC_ARCH_X86,UC_MODE_32);self.candidate=candidate;self.mode='setup'
        for pe in (exe,dll):
            base=pe.OPTIONAL_HEADER.ImageBase;self.c.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
            self.c.mem_write(base,pe.get_memory_mapped_image())
        self.c.mem_map(BASE,0x400000);self.c.mem_map(0,4096)
        self.exports={re.sub(r'^[@_]|@\d+$','',s.name.decode()):dll.OPTIONAL_HEADER.ImageBase+s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols}
        self.clip=set()
        for line in symbols.read_text().splitlines():
            f=line.split()
            if len(f)>=3 and f[1].startswith('?ClipToMap@'):self.clip.add(int(f[2],16))
        self.c.hook_add(UC_HOOK_CODE,self.hook)
    def put(self,p,*v):self.c.mem_write(p,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
    def read(self,p,n=1):return struct.unpack('<'+'I'*n,self.c.mem_read(p,4*n))
    def cell(self,p):return struct.unpack('<hh',self.c.mem_read(p,4))
    def byte(self,p,v):self.c.mem_write(p,bytes([v&255]))
    def ret(self,v=0,pop=0):
        sp=self.c.reg_read(UC_X86_REG_ESP);self.c.reg_write(UC_X86_REG_EIP,self.read(sp)[0])
        self.c.reg_write(UC_X86_REG_ESP,sp+4+pop);self.c.reg_write(UC_X86_REG_EAX,v&0xffffffff)
    def reset(self,mode):
        self.mode=mode;self.trace=[];self.c.mem_write(BASE,bytes(0x100000));self.put(0,0xffffffff)
        self.put(0xA83D4C,BASE+0x10000)
    def hook(self,c,pc,size,data):
        sp=c.reg_read(UC_X86_REG_ESP);self_=c.reg_read(UC_X86_REG_ECX)
        if self.mode=='click':
            if pc in ({0x586E50}|self.clip):
                out,where=self.read(sp+4,2);self.trace.append(['clip',*self.cell(where)])
                self.c.mem_write(out,struct.pack('<hh',80,100));self.ret(out,8)
            elif pc==0x5023B0:
                self.trace.append(['waypoint',*self.cell(self.read(sp+4)[0])]);self.ret(BASE+0x20000 if self.has_path else 0,4)
            elif pc==0x502460:
                point,path,index=self.read(sp+4,3);self.trace.append(['properties',point]);self.put(path,5);self.byte(index,3);self.ret(1,12)
            elif BASE+0x30000<=pc<=BASE+0x30040:
                actor=(self_-BASE-0x1000)//0x1000;feedback=c.mem_read(0x822CF2,1)[0]
                if pc==BASE+0x30000:
                    path,index=self.read(sp+4,2);self.trace.append(['assign',actor,path,index&255]);self.ret(pop=8)
                elif pc==BASE+0x30010:
                    where,fog,force=self.read(sp+4,3);self.trace.append(['classify_cell',actor,*self.cell(where),fog&255,force&255,feedback]);self.ret(actor+1,12)
                elif pc==BASE+0x30020:
                    obj,force=self.read(sp+4,2);self.trace.append(['classify_object',actor,obj,force&255,feedback]);self.ret(actor+5,8)
                elif pc==BASE+0x30030:
                    action,where,follow,force=self.read(sp+4,4);self.trace.append(['click_cell',actor,action,*self.cell(where),*self.cell(follow),where==follow,force&255,feedback])
                    if self.mutate and actor==0:self.put(0xA8ECC8,1)
                    self.ret(actor&1,16)
                else:
                    action,obj,force=self.read(sp+4,3);self.trace.append(['click_object',actor,action,obj,force&255,feedback])
                    if self.mutate and actor==0:self.put(0xA8ECC8,1)
                    self.ret(actor&1,12)
        elif self.mode=='clip' and pc==0x578460:
            where,by_height=self.read(sp+4,2);self.trace.append([*self.cell(where),by_height&255])
    def call(self,name,original,self_,args):
        self.put(STACK,STOP,*args);self.c.reg_write(UC_X86_REG_ESP,STACK);self.c.reg_write(UC_X86_REG_ECX,self_);self.c.reg_write(UC_X86_REG_EDX,0)
        self.c.emu_start(self.exports[name] if self.candidate else original,STOP,count=2000000)
        assert self.c.reg_read(UC_X86_REG_EIP)==STOP,'instruction limit/trap'
        assert self.c.reg_read(UC_X86_REG_ESP)==STACK+4*(1+len(args)),'stack imbalance'
    def click(self,count,action,object_target,has_path,feedback,debug,mutate):
        self.reset('click');self.has_path=has_path;self.mutate=mutate
        self.byte(0x822CF2,feedback);self.byte(0xA8ED6B,debug);self.put(0xA8ECBC,BASE+0x500);self.put(0xA8ECC8,count)
        for i in range(count):
            obj=BASE+0x1000+i*0x1000;self.put(BASE+0x500+4*i,obj);self.put(obj,BASE+0x20000);self.put(obj+0x14,7 if i!=1 else 3);self.byte(obj+0x430,1)
        for slot,pc in ((0x1A4,0x30000),(0x70,0x30010),(0x74,0x30020),(0x140,0x30030),(0x144,0x30040)):
            self.put(BASE+0x20000+slot,BASE+pc)
        self.call('RadarActiveClick',0x4AE750,MAP,[BASE+0x7000 if object_target else 0,(90<<16)|60,action])
        return {'trace':self.trace,'feedback':self.c.mem_read(0x822CF2,1)[0],'patrol':[self.c.mem_read(BASE+0x1430+i*0x1000,1)[0] for i in range(count)],'count':self.read(0xA8ECC8)[0]}
    def clip_cell(self,map_size,rect,cell,height,slope,local_map):
        self.reset('clip');obj=BASE+0x8000 if local_map else MAP
        self.put(obj+0xEC,7,9,*map_size);self.put(obj+0xFC,*rect)
        self.put(MAP+0x13C,BASE+0x100000);self.c.mem_write(BASE+0x100000,struct.pack('<I',BASE+0x50000)*0x40000)
        self.c.mem_write(0xABDC50,bytes(0x148));self.byte(BASE+0x5011B,height);self.byte(BASE+0x5011C,slope)
        self.c.mem_write(BASE+0x600,struct.pack('<hh',*cell))
        self.call('RadarClip',0x586E50,obj,[BASE+0x604,BASE+0x600])
        return {'cell':self.cell(BASE+0x604),'membership':self.trace}
    def assign_path(self,path,index,coords):
        self.reset('assign');obj=BASE+0x2000
        self.c.mem_write(obj,b'\xA5'*0x700)
        for p in range(12):
            self.put(BASE+0x10210+p*4,BASE+0x20000+p*0x100)
            self.put(BASE+0x20000+p*0x100+0x2C,BASE+0x21000);self.put(BASE+0x20000+p*0x100+0x38,128)
        for i in range(128):self.put(BASE+0x21000+i*12,*coords)
        self.call('RadarAssignPath',0x4DC810,obj,[path,index])
        return {'state':bytes(self.c.mem_read(obj,0x700)).hex()}
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,symbols=build(a.compile_db,a.output)
    exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
    differences=[];counts={};rng=random.Random(0x4AE750)
    cases={'click':list(itertools.product((0,1,3),(0,1,2,3,5,7,20,51,65),(0,1),(0,1),(0,1),(0,1),(0,1))),
           'assign_path':list(itertools.product((-1,0,5,11),(0,3,127),((0,0,0),(128,256,104),(-128,-257,208),(0x7FFFFFFF,-0x80000000,0)))),
           'clip_cell':[]}
    for dims,rect in (((100,100),(2,2,96,92)),((40,180),(2,4,36,160)),((180,40),(4,3,160,32)),((64,60),(9,7,40,35))):
        for height,slope,local in itertools.product((0,3,13),(0,1,4),(0,1)):
            for _ in range(20):cases['clip_cell'].append((dims,rect,(rng.randrange(-20,301),rng.randrange(-20,301)),height,slope,local))
    for method,inputs in cases.items():
        counts[method]=len(inputs)
        for case in inputs:
            expected=getattr(old,method)(*case);actual=getattr(new,method)(*case)
            if expected!=actual:differences.append({'method':method,'input':case,'original':expected,'candidate':actual})
    report={'exe_sha256':SHA,'cases':counts,'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'commands-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
    if differences:print(json.dumps(differences[:3],indent=2))
    return bool(differences)
if __name__=='__main__':raise SystemExit(main())
