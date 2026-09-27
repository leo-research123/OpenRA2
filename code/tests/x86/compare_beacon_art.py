#!/usr/bin/env python3
"""Startup LoadArt order, fallback, global pointers and manager metadata.

Production LoadArt executes, sharing RawFile ctor/read/dtor and FileSystem lookup
as observed resource boundaries. Covers raw success, fallback success, complete
failure, signed SHP metadata and preservation of untouched bytes. ReleaseArt is
a native lifecycle extension, called outside each oracle case and tested with
real native resources separately. This probe does not verify filesystem I/O,
SHP decoding, allocator failure or exception branches.
"""
import argparse,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,SHA,digest
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/BeaconManagerClassArt.cpp','code/tests/x86/beacon_art_probe.cpp']
def build(db,out):
    return build_probe(db, out, SOURCES, 'beacon_art_probe.dll',
        object_suffix='-beacon-art.obj',
        flags=['/GS-', '/GR-', '/Gy'],
        symbols=True)
class Machine(BaseMachine):
 def string(self,p):
  value=bytearray()
  while self.c.mem_read(p,1)!=b'\0':value+=self.c.mem_read(p,1);p+=1
  return value.decode()
 def hook(self,c,pc,size,data):
  sp=c.reg_read(UC_X86_REG_ESP);obj=c.reg_read(UC_X86_REG_ECX)
  if pc==0x65CA80:
   name=self.string(self.read(sp+4)[0]);self.files[obj]=name;self.trace.append(['construct',name]);self.ret(obj,4)
  elif pc==0x65CA00:self.trace.append(['destroy',self.files[obj]]);self.ret()
  elif pc==0x4A3890:
   name=self.files[obj];i=int(name=='RDRBEACN.SHP');self.trace.append(['raw_read',name]);self.ret(BASE+0x5000+i*0x100 if self.modes[i]==0 else 0)
  elif pc==0x5B40B0:
   name=self.string(obj);i=int(name=='RDRBEACN.SHP');self.trace.append(['lookup',name,c.reg_read(UC_X86_REG_EDX)&255]);self.ret(BASE+0x5000+i*0x100 if self.modes[i]==1 else 0)
  elif pc==0x7C8B3D:self.ret()
 def art(self,beacon_mode,radar_mode,metadata,seed):
  if self.candidate:self.call('ReleaseBeaconArt',0,BASE+0x1000,[])
  self.reset('art');self.files={};self.modes=(beacon_mode,radar_mode)
  obj=BASE+0x1000;self.c.mem_write(obj-4,bytes((i*13+seed)%256 for i in range(0x88)))
  for i in range(2):self.c.mem_write(BASE+0x5000+i*0x100,struct.pack('<Hhhh',0,*metadata[i*3:i*3+3]))
  self.call('LoadBeaconArt',0x4309D0,obj,[])
  pointers=[]
  if self.candidate:
   for i in range(2):self.call('BeaconArtPointer',0,obj,[i]);pointers.append(self.c.reg_read(UC_X86_REG_EAX))
  else:pointers=list(self.read(0x89C474,2))
  return {'trace':self.trace,'state':bytes(self.c.mem_read(obj-4,0x88)).hex(),'pointers':pointers}
def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,symbols=build(a.compile_db,a.output)
 exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
 cases=list(itertools.product(range(3),range(3),((48,60,24,15,15,12),(0,0,0,0,0,0),(32767,-32768,-1,-1,32767,-32768)),(0,0xFF,0xA5)))
 differences=[]
 for case in cases:
  expected=old.art(*case);actual=new.art(*case)
  if expected!=actual:differences.append({'input':case,'original':expected,'candidate':actual})
 report={'exe_sha256':SHA,'cases':len(cases),'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
 (a.output/'beacon-art-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
 if differences:print(json.dumps(differences[:2],indent=2))
 return bool(differences)
if __name__=='__main__':raise SystemExit(main())
