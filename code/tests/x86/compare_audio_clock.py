#!/usr/bin/env python3
"""Original 0x4093F0 64-bit audio time extension over shared 32-bit authority.

Executes the production extension, not QPC selection or the backend. Covers
same-time reads, low/high/counter wrap and a changing read serial retry. Neither
side advances the supplied time on a read. Test fixtures own this full lifetime.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,STACK,STOP,SHA
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/AudioClock.cpp','code/tests/x86/audio_clock_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'audio_clock_probe.dll',
        object_suffix='-audioclock.obj',
        symbols=True)
class Machine(BaseMachine):
 def __init__(self,*args):
  super().__init__(*args);self.last=0x816320;self.serial=0x87E84C
  if self.candidate:self.call('Globals',0,BASE+0x700,[]);self.last,self.serial=self.read(BASE+0x700,2)
 def hook(self,c,pc,size,data):
  if pc==BASE+0x30000:
   if self.retry and self.reads==0:self.put(self.serial,self.read(self.serial)[0]+1)
   self.reads+=1;self.ret(self.now)
 def run(self,last,serial,times,retry):
  self.put(self.last,last&0xffffffff,last>>32);self.put(self.serial,serial);self.put(0x7E1530,BASE+0x30000)
  out=[]
  for now in times:
   self.reads=0;self.now=now;self.retry=retry;self.call('Extended',0x4093F0,0,[])
   value=self.c.reg_read(UC_X86_REG_EAX)|(self.c.reg_read(UC_X86_REG_EDX)<<32)
   out.append([value,self.read(self.serial)[0],list(self.read(self.last,2)),self.reads])
  return out
p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,sym=build(a.compile_db,a.output)
old,new=[Machine(pefile.PE(str(a.exe)),pefile.PE(str(dll)),sym,b) for b in (False,True)]
cases=list(itertools.product((0,0x100000000,0x100000001,0x1FFFFFFF0,0xFFFFFFFFFFFFFFF0),(0,1,0x7FFFFFFF,0xFFFFFFFF),((0,0,1),(1000,1000,1001),(0xFFFFFFF0,0xFFFFFFFF,0,5),(0x80000000,0x7FFFFFFF,0,0)),(0,1)))
diff=[]
for case in cases:
 e=old.run(*case);v=new.run(*case)
 if e!=v:diff.append(dict(input=case,original=e,candidate=v))
r={'exe_sha256':SHA,'cases':len(cases),'mismatches':len(diff),'differences':diff,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
(a.output/'audio-clock-comparison.json').write_text(json.dumps(r,indent=2)+'\n');print('Audio clock',len(cases),'cases,',len(diff),'differences')
raise SystemExit(bool(diff))
