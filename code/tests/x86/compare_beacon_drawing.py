#!/usr/bin/env python3
"""Compare actual beacon drawing submissions against the fixed EXE.

Both production DrawRadar bodies execute, including slot order. The candidate
VisibleToPlayer helper delegates to the fixed EXE (independently compared in
phase 14). Coordinate projection and final shape rasterization are shared
observed boundaries. Sprite/palette/frame/position/clip/flags and all original draw args
are compared. Original dirty-rectangle and per-pixel refresh still execute but
are deliberately excluded: approved whole composition replaces those writes.
Simulation frames are nonnegative and initialized periods are positive.
Native integration separately verifies actual projection, palettes and pixels.
"""
import argparse,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,MAP,SHA,digest
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/BeaconClassDrawing.cpp','code/core/src/yrpp/BeaconManagerClassDrawing.cpp','code/tests/x86/beacon_drawing_probe.cpp']
def build(db,out):
    return build_probe(db, out, SOURCES, 'beacon_drawing_probe.dll',
        object_suffix='-beacon-drawing.obj',
        symbols=True)
class Machine(BaseMachine):
 def hook(self,c,pc,size,data):
  sp=c.reg_read(UC_X86_REG_ESP)
  if pc==0x6557F0:
   out,coord,restrict=self.read(sp+4,3)
   assert restrict==1
   x,y,z=self.read(coord,3);self.put(out,x,y);self.ret(out,12)
  elif pc==0x4AED70:
   args=[c.reg_read(UC_X86_REG_ECX),c.reg_read(UC_X86_REG_EDX),*self.read(sp+4,14)];args[4]=list(self.read(args[4],2));args[5]=list(self.read(args[5],4))
   self.trace.append(args);self.ret(pop=56)
  elif pc==0x6562D0:self.ret(pop=4)
 def setup(self,frame,frames,period):
  self.reset('drawing');self.put(0x89C420,5,7,frames,period)
  self.put(0x89C478,BASE+0x90000);self.put(0xA8ED84,frame)
  self.put(0x8809F4,0,0,0,0);self.put(0x880C84,0,0,140,108)
  self.put(0xA83D4C,BASE+0xE0000)
  self.put(BASE+0xE0030,7);self.put(BASE+0xE0000+0x5788,0xFF)
  self.put(0xA8022C,BASE+0x500);self.put(0xB054D4,BASE+0x600);self.put(0xB054E0,8)
  for i in range(8):
   house=BASE+0x10000+i*0x18000;scheme=BASE+0xD0000+i*0x400
   self.put(BASE+0x500+i*4,house);self.put(house+0x30,i);self.put(house+0x5788,0xFF);self.byte(house+0x1F5,0)
   self.put(house+0x16054,i);self.put(BASE+0x600+i*4,scheme);self.put(scheme+0x30C,BASE+0xD8000+i*0x100)
 def draw_beacon(self,frame,frames,point,bounds,house,clear):
  self.setup(frame,frames,4*frames);obj=BASE+0x800
  self.put(obj,*point);self.put(obj+0x110,house)
  self.call('DrawBeacon',0x430650,obj,[BASE+0xF0000,*bounds,clear]);return self.trace
 def draw_manager(self,frame,frames,period,mask,visibility,local,count):
  self.setup(frame,frames,4*frames);obj=BASE+0x1000 if local else 0x89C3B0
  self.put(obj+0x60,count);self.put(obj+0x7C,period)
  for i in range(24):
   beacon=BASE+0x2000+i*0x200;house=i//3
   self.put(obj+i*4,beacon if mask>>i&1 else 0)
   self.put(beacon,i*3,i*2,0);self.put(beacon+0x110,house);self.byte(beacon+0xC,3)
   if visibility==1:self.byte(beacon+0xC,i&1)
   elif visibility==2:self.put(BASE+0xE0000+0x5788,0x55)
   elif visibility==3:self.put(BASE+0x10000+house*0x18000+0x5788,0 if house&1 else 0xFF)
   elif visibility==4:self.byte(BASE+0x10000+house*0x18000+0x1F5,house&1)
  self.call('DrawManager',0x431700,obj,[BASE+0xF0000,0,0,140,108]);return self.trace

def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,symbols=build(a.compile_db,a.output)
 exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
 cases={'draw_beacon':[],'draw_manager':[]}
 for frames in (1,4,11):
  for frame,point,bounds,house,clear in itertools.product((0,frames-1,frames,frames+1,4*frames-1,4*frames,0x7FFFFFFF),((0,0,0),(70,54,0),(139,107,104)),((0,0,140,108),(17,29,80,60)),(0,3,7),(0,1)):
   cases['draw_beacon'].append((frame,frames,point,bounds,house,clear))
 for frame,frames,period,mask,visibility,local,count in itertools.product((0,3,4,5,15,16,0x7FFFFFFF),(1,4),(16,44),(0,1,0x800000,0xAAAAAA,0xFFFFFF),range(5),(0,1),(0,24)):
  cases['draw_manager'].append((frame,frames,period,mask,visibility,local,count))
 differences=[]
 for method,inputs in cases.items():
  for case in inputs:
   expected=getattr(old,method)(*case);actual=getattr(new,method)(*case)
   if expected!=actual:differences.append({'method':method,'input':case,'original':expected,'candidate':actual})
 report={'exe_sha256':SHA,'cases':{k:len(v) for k,v in cases.items()},'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
 (a.output/'beacon-drawing-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
 if differences:print(json.dumps(differences[:2],indent=2))
 return bool(differences)
if __name__=='__main__':raise SystemExit(main())
