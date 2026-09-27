#!/usr/bin/env python3
"""Outgoing RA2_YRPP_GAME wrappers preserve original event-by-value ABI.

Runs original 0x6521C0/0x637DD0/0x639FD0 behind compiled wrappers. Checks
112-byte x86 stdcall cleanup, event-copy semantics, frame/flag/queue writes and
feedback calls. Not entry patch installation or original-process acceptance.
"""
import argparse,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_planning_input import Machine,ROOT,SHA,BASE,QUEUE,digest
from unicorn.x86_const import UC_X86_REG_ECX
SOURCES=['code/tests/x86/planning_input_game_probe.cpp','code/core/src/yrpp/GamePlanningInput.cpp']
def build(db,out):
    return build_probe(db, out, SOURCES, 'planning_input_game_probe.dll',
        object_suffix='-plan-game.obj',
        flags=['/DRA2_YRPP_GAME', '/GS-', '/Gy'])
class GameMachine(Machine):
 def submission(self,operation,event_type,count,tail,frame):
  self.setup(count=count,tail=tail);self.put(0xA8ED84,frame)
  event=bytearray((i*71+19)%256 for i in range(112));event[0]=event_type
  # Direct GAME QueueClass.Add shares AddEvent's result when Frame already
  # matches. This instantiates and exercises the actual WinMM import branch.
  if operation==2:struct.pack_into('<I',event,3,frame)
  self.c.mem_write(BASE+0x500,bytes(event))
  if self.candidate:result=self.call(('OriginalEnqueue','OriginalSubmit','OriginalRingAdd')[operation],0,ecx=BASE+0x500)
  else:result=self.call('',0x637DD0 if operation==1 else 0x6521C0,raw=event)
  return {'value':result,'trace':self.trace,'queue':self.queue_state(),'event':bytes(self.c.mem_read(BASE+0x500,112)).hex()}
 def rejection(self,event_type,reported):
  self.setup(reported=reported);self.byte(BASE+0x500,event_type)
  self.call('OriginalReject',0x639FD0,ecx=BASE+0x500)
  return {'trace':self.trace,'flag':self.c.mem_read(0xAC4C08,1)[0]}
def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,default=ROOT/'out/reference/RA2MDddcompact/gamemd.exe');p.add_argument('--compile-db',type=Path,default=ROOT/'out/game-ui-x86/compile_commands.json');p.add_argument('--output',type=Path,default=ROOT/'out/radar-instruction-audit');a=p.parse_args()
 assert digest(a.exe)==SHA;dll=build(a.compile_db,a.output)
 old,new=[GameMachine(pefile.PE(str(a.exe)),pefile.PE(str(dll)),x) for x in (False,True)]
 cases={'submission':list(itertools.product((0,1,2),(0,4,6,7,9,46,127,255),(0,127,128,129),(0,127),(0,0xF1234567))),
        'rejection':list(itertools.product((0,4,6,7,9,46,255),(0,1)))}
 diffs=[]
 for method,rows in cases.items():
  for row in rows:
   x=getattr(old,method)(*row);y=getattr(new,method)(*row)
   if x!=y:diffs.append({'method':method,'input':row,'original':x,'candidate':y})
  print(method,len(rows),'cumulative mismatches',len(diffs),flush=True)
 report={'exe_sha256':SHA,'dll_sha256':digest(dll),'cases':{k:len(v) for k,v in cases.items()},'total':sum(map(len,cases.values())),'mismatches':len(diffs),'differences':diffs,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
 (a.output/'planning-game-calls-comparison.json').write_text(json.dumps(report,indent=2)+'\n');return bool(diffs)
if __name__=='__main__':raise SystemExit(main())
