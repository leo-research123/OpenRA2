#!/usr/bin/env python3
"""Beacon state/lifecycle/selection and Display beacon-mode fixed-EXE comparison.

Production method bodies run on both sides. Original sqrt executes; candidate
fesetround changes the same x87/MXCSR mode. Map::UnselectAll and cursor virtuals
are observed boundaries, and raw allocator release is recorded. House alliance
predicates execute their original/candidate implementations. Constructors retain
untouched bytes. No beacon placement, network send, messages, art or draw claim.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,MAP,STACK,STOP,SHA
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/BeaconClassState.cpp','code/core/src/yrpp/BeaconManagerClass.cpp','code/core/src/yrpp/DisplayClassModes.cpp','code/tests/x86/beacon_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'beacon_probe.dll',
        object_suffix='-beacon.obj',
        symbols=True)
class Machine(BaseMachine):
 def hook(self,c,pc,size,data):
  sp=c.reg_read(UC_X86_REG_ESP)
  if pc==0x48DC90:
   self.trace.append(['clear_selection']);self.ret()
  elif pc==0x7C8B3D:self.trace.append(['free',self.read(sp+4)[0]]);self.ret()
  elif pc==0x7CA422:
   to,source,count=self.read(sp+4,3);raw=bytearray();done=False
   for i in range(count):
    unit=bytes(c.mem_read(source+i*2,2)) if not done else b'\0\0';done=done or unit==b'\0\0';raw+=unit
   c.mem_write(to,bytes(raw));self.ret(to)
  elif pc==self.exports.get('BeaconSetRound'):
   c.reg_write(UC_X86_REG_FPCW,(c.reg_read(UC_X86_REG_FPCW)&~0xC00)|0xC00)
   c.reg_write(UC_X86_REG_MXCSR,(c.reg_read(UC_X86_REG_MXCSR)&~0x6000)|0x6000);self.ret(0)
  elif pc==BASE+0x50000:
   cursor,mini=self.read(sp+4,2);self.trace.append(['cursor',cursor,mini&255]);self.ret(pop=8)
  elif pc==BASE+0x50010:self.trace.append(['restore']);self.ret()
 def setup(self):
  self.reset('beacon');self.put(0x89C430,0,0,0);self.trace=[]
 def state(self,method,seed,coord,house,length):
  self.setup();obj=BASE+0x1000
  self.c.mem_write(obj-4,bytes((i*13+seed)%256 for i in range(0x11C)))
  src=BASE+0x500
  self.c.mem_write(src,('文'*length+'\0').encode('utf-16-le'))
  if method=='construct':name,entry,args='ConstructBeacon',0x430210,[]
  elif method=='set':name,entry,args='SetBeacon',0x430590,[*coord,house]
  else:name,entry,args='BeaconText',0x430620,[0 if length<0 else obj+0xE if length==200 else src]
  self.call(name,entry,obj,args)
  return bytes(self.c.mem_read(obj-4,0x11C)).hex()
 def manager(self,method,count,mask):
  self.setup();obj=BASE+0x1000;self.c.mem_write(obj-4,b'\xA5'*0x88)
  for i in range(24):self.put(obj+i*4,BASE+0x10000+i*0x200 if mask>>i&1 else 0)
  self.put(obj+0x60,count)
  name,entry={'construct':('ConstructManager',0x430910),'reset':('ResetManager',0x430980),'destroy':('DestroyManager',0x430930)}[method]
  self.call(name,entry,obj,[])
  return {'state':bytes(self.c.mem_read(obj-4,0x88)).hex(),'trace':self.trace}
 def capacity(self,house,mask):
  self.setup();obj=BASE+0x1000
  for i in range(24):self.put(obj+i*4,BASE+0x10000 if mask>>(i%3)&1 else 0)
  self.call('BeaconCapacity',0x430F30,obj,[house]);return bool(self.c.reg_read(UC_X86_REG_EAX)&255)
 def select(self,count,slot,delta,cw):
  self.setup();obj=BASE+0x1000;target=BASE+0x20000
  self.put(obj+0x60,count)
  # First candidate tests distance; later candidate is nearer and must lose if
  # the first is within the original strict 128 threshold.
  self.put(obj+slot*4,target);self.put(obj+((slot+1)%24)*4,target+0x200)
  self.put(target,*delta);self.put(target+0x200,0,0,0)
  self.byte(target+0xC,0xAD);self.byte(target+0x20C,0xE5)
  self.c.reg_write(UC_X86_REG_FPCW,cw);self.c.reg_write(UC_X86_REG_MXCSR,0x7F80 if cw==0xE7F else 0x1F80)
  self.call('SelectBeacon',0x430F70,obj,[0,0,0])
  return {'result':bool(self.c.reg_read(UC_X86_REG_EAX)&255),'trace':self.trace,'flags':[self.c.mem_read(target+off,1)[0] for off in (0xC,0x20C)],'rounding':self.c.reg_read(UC_X86_REG_FPCW)&0xF00}
 def visible(self,flag,house,current,forward,reverse,defeated):
  self.setup();obj=BASE+0x1000;viewer=BASE+0x20000;owner=BASE+0x40000
  self.byte(obj+0xC,flag);self.put(obj+0x110,house)
  self.put(0xA83D4C,viewer);self.put(viewer+0x30,current);self.put(owner+0x30,house)
  self.put(0xA8022C,BASE+0x500)
  for i in range(8):self.put(BASE+0x500+4*i,owner)
  self.put(viewer+0x5788,(1<<house) if forward else 0)
  self.put(owner+0x5788,(1<<current) if reverse else 0)
  self.byte(owner+0x1F5,defeated)
  self.call('BeaconVisible',0x4308B0,obj,[])
  return bool(self.c.reg_read(UC_X86_REG_EAX)&255)
 def beacon_mode(self,request,repair,beacon,pending,full):
  self.setup();obj=BASE+0x1000
  self.c.mem_write(obj,b'\xA5'*0x1200);self.put(obj,BASE+0x40000)
  self.put(BASE+0x40048,BASE+0x50000);self.put(BASE+0x40050,BASE+0x50010)
  self.byte(obj+0x11B0,repair);self.byte(obj+0x11B4,beacon);self.put(obj+0x11A8,1 if pending else 0)
  self.put(0xA83D4C,BASE+0x7000);self.put(BASE+0x7030,3)
  self.put(0x89C3B0+3*12,1,1,1 if full else 0)
  self.call('BeaconMode',0x4AC960,obj,[request])
  return {'state':bytes(self.c.mem_read(obj,0x1200)).hex(),'trace':self.trace}
def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,symbols=build(a.compile_db,a.output)
 exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
 cases={'state':list(itertools.product(('construct','set','text'),(0,1,255),((0,0,0),(1,2,3),(-0x80000000,0x7FFFFFFF,-1)),(-1,0,7,8),(0,3,127,128,200,-1))),
        'manager':list(itertools.product(('construct','reset','destroy'),(0,1,24),(0,1,0xAAAAAA,0xFFFFFF))),
        'capacity':list(itertools.product(range(8),range(8))),
        'select':list(itertools.product((0,1,24),(0,3,22,23),((0,0,0),(127,0,0),(127,15,0),(128,0,0),(0,0,128),(-0x80000000,0,0),(0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF)),(0x27F,0xE7F))),
        'visible':list(itertools.product((0,1,2,3),(0,3,7),(0,3,7),(0,1),(0,1),(0,1))),
        'beacon_mode':list(itertools.product((-3,-1,0,1,2),(0,1),(0,1),(0,1),(0,1)))}
 differences=[]
 for method,inputs in cases.items():
  for case in inputs:
   expected=getattr(old,method)(*case);actual=getattr(new,method)(*case)
   if expected!=actual:differences.append({'method':method,'input':case,'original':expected,'candidate':actual})
 report={'exe_sha256':SHA,'cases':{k:len(v) for k,v in cases.items()},'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
 (a.output/'beacon-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
 if differences:print(json.dumps(differences[:2],indent=2))
 return bool(differences)
if __name__=='__main__':raise SystemExit(main())
