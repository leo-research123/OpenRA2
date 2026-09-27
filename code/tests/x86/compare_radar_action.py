#!/usr/bin/env python3
"""Full YR 0x6539D0 RTactical caller vs production native Action.

Device event coordinates are supplied at the same point after queued/live mouse
acquisition. Picking, ground/shroud, selected-object queries, cursor, planning,
ConvertAction/LeftRelease, Gadget base and final camera are declared shared
callee boundaries. Production navigation clamp and original redraw body run.
Compare ordered calls, arguments, key, result and complete radar state. Retained
native drag capture is excluded from the original equality assertion and tested
through real host input separately. No claim of device/callee/whole-game proof.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
BASE,MAP,STACK,STOP=0x01000000,0x87F7E8,0x0101E000,0x0101F000
GADGET=BASE+0x3000
SOURCES=['code/core/src/yrpp/RadarClassAction.cpp','code/core/src/yrpp/RadarClassNavigation.cpp','code/tests/x86/radar_action_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'radar_action_probe.dll',
        object_suffix='-action.obj')
class Machine:
 def __init__(self,exe,dll,candidate):
  self.c=Uc(UC_ARCH_X86,UC_MODE_32);self.candidate=candidate
  for pe in (exe,dll):
   base=pe.OPTIONAL_HEADER.ImageBase;self.c.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);self.c.mem_write(base,pe.get_memory_mapped_image())
  self.c.mem_map(BASE,0x20000);self.c.mem_map(0,4096)
  self.entry=next(dll.OPTIONAL_HEADER.ImageBase+s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols if b'RadarAction' in s.name)
  self.c.hook_add(UC_HOOK_CODE,self.hook)
 def put(self,p,*v):self.c.mem_write(p,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
 def read(self,p,n=1):return struct.unpack('<'+'I'*n,self.c.mem_read(p,4*n))
 def cell(self,p):return list(struct.unpack('<hh',self.c.mem_read(p,4)))
 def coords(self,p):return list(struct.unpack('<iii',self.c.mem_read(p,12)))
 def ret(self,value=0,pop=0):
  sp=self.c.reg_read(UC_X86_REG_ESP);self.c.reg_write(UC_X86_REG_EIP,self.read(sp)[0]);self.c.reg_write(UC_X86_REG_ESP,sp+4+pop);self.c.reg_write(UC_X86_REG_EAX,value&0xffffffff)
 def hook(self,c,pc,size,data):
  sp=c.reg_read(UC_X86_REG_ESP);self_=c.reg_read(UC_X86_REG_ECX)
  if pc==0x656750:
   point,cell,target=self.read(sp+4,3);self.trace.append(['pick',self_,*self.read(point,2)])
   c.mem_write(cell,struct.pack('<hh',*self.target));self.put(target,BASE+0xA000 if self.has_object else 0);self.ret(pop=12)
  elif pc in (0x578080,0x586360):
   self.trace.append(['height' if pc==0x578080 else 'shroud',self_,*self.coords(self.read(sp+4)[0])]);self.ret(self.height if pc==0x578080 else self.fog,4)
  elif pc==0x5353D0:
   cell,target=self_,c.reg_read(UC_X86_REG_EDX);self.trace.append(['best',self.cell(cell) if cell else None,target]);self.ret(BASE+0xA100)
  elif pc==BASE+0x18000:
   point,fog,ignore=self.read(sp+4,3);self.trace.append(['cell_action',self_,*self.cell(point),fog,ignore]);self.ret(self.action,12)
  elif pc==BASE+0x18010:
   target,ignore=self.read(sp+4,2);self.trace.append(['object_action',self_,target,ignore]);self.ret(self.action,8)
  elif pc==0x639DA0:self.trace.append(['planner']);self.ret(self.unsupported)
  elif pc in (0x5BDA80,0x5BDC80):self.trace.append(['default' if pc==0x5BDA80 else 'override',self_,*self.read(sp+4,2)]);self.ret(1,8)
  elif pc==0x4AAE90:
   cell,fog,obj,action,mini=self.read(sp+4,5);self.trace.append(['convert',self_,*self.cell(cell),fog,obj,action,mini]);self.ret(1,20)
  elif pc==0x4AB9B0:
   world,cell,obj,action,mini=self.read(sp+4,5);self.trace.append(['release',self_,*self.coords(world),*self.cell(cell),obj,action,mini]);self.ret(pop=20)
  elif pc==0x4E1530:
   flags,key,modifier=self.read(sp+4,3);self.trace.append(['gadget',self_,flags,self.read(key)[0],modifier]);self.ret(pop=12)
  elif pc in (BASE+0x18020,BASE+0x18030):self.ret(self.pointer[0 if pc==BASE+0x18020 else 1])
  elif pc==BASE+0x18040:self.ret(pop=4) # Extension's sticky receiver only.
  elif pc==0x5657A0:
   xy=self.cell(self.read(sp+4)[0]);self.trace.append(['cell',*xy]);c.mem_write(BASE+0x5024,struct.pack('<hh',*xy));self.ret(BASE+0x5000,4)
  elif pc==0x47B3A0:self.ret(self.height,4)
  elif pc==0x6D6070:self.trace.append(['camera',*self.coords(self.read(sp+4)[0])]);self.ret(pop=4)
  elif pc==0x4F42F0:self.trace.append(['redraw',self.read(sp+4)[0]]) # Real original body.
 def run(self,flags=8,mode=1,state=1,point=(70,80),target=(70,90),selected=1,sw=-1,action=1,has_object=0,planning=0,unsupported=-1,aborted=0,fog=0,window=1,sidebar=1,modifier=0):
  c=self.c;c.mem_write(BASE,bytes(0x19000));c.mem_write(MAP,bytes(0x556C));self.trace=[]
  self.target,self.has_object,self.height,self.fog,self.action,self.unsupported=target,has_object,208,fog,action,unsupported
  self.put(MAP,BASE+0x4000);self.put(BASE+0x4000,*([0x4F42F0]*64));self.put(BASE+0x4000+0xD0,0x653F70)
  self.put(MAP+0xEC,7,9,100,100);self.put(MAP+0x123C,BASE+0x6000)
  self.put(MAP+0x149C,16,49,140,108);self.put(MAP+0x14AC,state,mode);self.put(MAP+0x11B8,sw)
  self.put(MAP+0x11F0,16,49);self.put(MAP+0x1200,140,108);self.put(MAP+0x0C,3);self.put(MAP+0x1158,0x1234FFFF)
  self.put(0x886FA0,0,0,856,736);self.put(0x887324,BASE+0x8000);self.put(0x887644,BASE+0x7000);self.put(BASE+0x7024,0x6543)
  self.put(0xB048C0,0xffffffff);self.put(0xA8ECC8,selected);self.put(0xB73550,window)
  for p,v in [(0xAC4CF4,planning),(0xA8ED9D,aborted),(0xA8EB7C,sidebar)]:c.mem_write(p,bytes([v]))
  self.put(BASE+0xA100,BASE+0xB000);self.put(BASE+0xB070,BASE+0x18000,BASE+0x18010)
  self.pointer=(point[0]+(856 if sidebar else 0),point[1]);self.put(0x87F770,BASE+0x2000);self.put(BASE+0x2000,*self.pointer)
  self.put(0x887640,BASE+0xC000);self.put(BASE+0xC000,BASE+0xC100);self.put(BASE+0xC12C,BASE+0x18020,BASE+0x18030)
  self.put(GADGET,BASE+0xD000);self.put(BASE+0xD000,*([BASE+0x18040]*40))
  self.put(BASE+0x9100,*self.pointer,0,flags,modifier);c.mem_write(BASE+0x9114,bytes([window]));self.put(BASE+0x2200,0x12345678)
  self.put(STACK,STOP,flags,BASE+0x2200,modifier);c.reg_write(UC_X86_REG_ESP,STACK);c.reg_write(UC_X86_REG_ECX,GADGET)
  c.emu_start(self.entry if self.candidate else 0x6539D0,STOP,count=100000)
  assert c.reg_read(UC_X86_REG_EIP)==STOP,'trap/instruction limit'
  assert c.reg_read(UC_X86_REG_ESP)==STACK+16,'stack balance'
  return {'result':bool(c.reg_read(UC_X86_REG_EAX)&255),'trace':self.trace,'key':self.read(BASE+0x2200)[0],
   'radar':bytes(c.mem_read(MAP,0x556C)).hex(),'tactical_redrawing':c.mem_read(BASE+0x8D7D,1)[0],'depth':self.read(BASE+0x7024)[0]}
def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll=build(a.compile_db,a.output)
 exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,False),Machine(exe,image,True)
 cases={
  'actions':[dict(flags=f,action=act,has_object=obj,planning=plan,unsupported=bad) for f,act,obj,plan,bad in itertools.product((1,4,8,12,16,64),range(73),(0,1),(0,1),(-1,2))],
  'superweapons':[dict(flags=f,sw=sw,selected=sel,planning=plan,unsupported=bad) for f,sw,sel,plan,bad in itertools.product((1,4,8,12,16,64),range(-2,14),(0,1),(0,1),(-1,0,1,2))],
  'gates':[dict(flags=f,mode=m,state=s) for f,m,s in itertools.product((0,1,2,4,8,16,32,64,128,0x55,0xFF),range(5),range(5))],
  'edges':[dict(flags=f,point=p,target=t,sidebar=side,selected=sel) for f,p,t,side,sel in itertools.product((1,4,8,16,64),((15,49),(16,48),(156,49),(16,157),(16,49),(155,156)),((-1,-1),(0,0),(70,90)),(0,1),(0,1))],
  'release':[dict(flags=f,fog=fog,window=w,aborted=ab,modifier=mod) for f,fog,w,ab,mod in itertools.product((4,8,12),(0,1),(0,1),(0,1),range(8))]
 }
 differences=[]
 for group,inputs in cases.items():
  for case in inputs:
   expected=old.run(**case);actual=new.run(**case)
   if expected!=actual:
    # Keep unchanged full state out of diagnostics, but compare it above.
    differences.append({'group':group,'input':case,'original':{k:v for k,v in expected.items() if v!=actual[k]},'candidate':{k:v for k,v in actual.items() if v!=expected[k]}})
  print(group,len(inputs),'cumulative mismatches',len(differences),flush=True)
 report={'exe_sha256':SHA,'dll_sha256':digest(dll),'cases':{k:len(v) for k,v in cases.items()},'total':sum(map(len,cases.values())),'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
 (a.output/'action-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
 if differences:print(json.dumps(differences[:2],indent=2))
 return bool(differences)
if __name__=='__main__':raise SystemExit(main())
