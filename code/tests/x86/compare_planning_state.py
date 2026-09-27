#!/usr/bin/env python3
"""YR planning node/branch layout and token query/commit state comparison.

Execute production methods over original-layout graphs. Compare return values,
complete object storage and guards, event bytes, ordered coordinate callbacks
and final writes. The EventCoords query is a declared shared boundary (independently
compared by compare_planning_input.py); constructors normalize only the compiler
vtable address. No node creation/deletion, Event::Execute, planner scheduling,
invalid graph recovery or complete planning implementation is claimed.
"""
import argparse,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_planning_input import Machine,ROOT,SHA,BASE,digest
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EAX
SOURCES=['code/core/src/yrpp/PlanningNodeClassQueries.cpp','code/core/src/yrpp/PlanningTokenClassState.cpp','code/core/src/yrpp/PlanningBranchClass.cpp','code/tests/x86/planning_state_probe.cpp']
TOKEN=BASE+0x1000
NODE=BASE+0x3000
MEMBERS=BASE+0x5000
PACKETS=BASE+0x7000
BRANCHES=BASE+0x9000
OUTPUT=BASE+0xB000

def build(db,out,game=False):
    return build_probe(db, out, [SOURCES[-1]] if game else SOURCES, 'planning_state_game_probe.dll' if game else 'planning_state_probe.dll',
        object_suffix='-plan-state-game.obj' if game else '-plan-state.obj',
        flags=(['/DRA2_YRPP_GAME'] if game else []) + ['/GR-', '/GS-', '/Gy'])

class StateMachine(Machine):
 def hook(self,c,pc,size,data):
  if pc!=0x633BC0:return
  out=c.reg_read(UC_X86_REG_ECX);event=c.reg_read(UC_X86_REG_EDX)
  self.trace.append(['coords',event])
  values=self.read(event+32,3)
  target=OUTPUT+0x300 if self.alias else out
  self.put(target,*values)
  self.ret(target)
 def fresh(self):
  self.c.mem_write(BASE,bytes([0xA5])*0x10000);self.trace=[];self.alias=False
 def value(self,name,pc,args=(),ecx=TOKEN):
  super().call(name,pc,args=args,ecx=ecx)
  return self.c.reg_read(UC_X86_REG_EAX)
 def snapshot(self):return bytes(self.c.mem_read(BASE,0x10000)).hex()
 def constructor(self,token,fill,owner):
  self.fresh();size=0x9C if token else 0x78
  self.c.mem_write(TOKEN,bytes([fill])*size)
  result=self.value('ConstructToken' if token else 'ConstructBranch',0x635F20 if token else 0x633AC0,(owner,) if token else ())
  if token and not self.original_game:self.put(TOKEN+4,0) # only the image-dependent vector vptr
  return result,self.snapshot()
 def graph(self,count=4,null_mask=0):
  self.fresh();self.put(TOKEN,BASE+0x10100);self.put(TOKEN+8,TOKEN+0x100);self.put(TOKEN+0x14,count)
  for i in range(4):
   node=NODE+i*0x200;self.put(TOKEN+0x100+i*4,0 if null_mask&(1<<i) else node)
   self.put(node+4,node+0xC0);self.put(node+0x10,3)
   self.put(node+0x24,node+0xD0);self.put(node+0x30,3)
   for j in range(3):
    member=MEMBERS+i*0x100+j*0x20;packet=PACKETS+i*0x300+j*0x80;branch=BRANCHES+i*0x300+j*0x80
    self.put(node+0xC0+j*4,member);self.put(node+0xD0+j*4,branch)
    self.put(member,BASE+0x10000+j*0x100,packet,0xEEEE,j)
    self.c.mem_write(packet,bytes((x*13+i*47+j*29)%256 for x in range(111)))
    self.put(packet+32,100+i*50+j,200+i*50+j,300+i*50+j)
    self.put(branch+0x70,j+1,71+i*10+j)
   self.put(node+0xA8,23,37+i,43+i,51+i)
  self.put(TOKEN+0x8C,-1,2,2);self.byte(TOKEN+0x98,0);self.byte(TOKEN+0x99,1)
 def node_query(self,kind,count,index,duplicate):
  self.graph();node=NODE;self.put(node+0x10,count)
  if duplicate:self.put(MEMBERS+0x40,BASE+0x10000)
  if kind==0:
   result=self.value('Owner',0x6342D0,(index,),node)
  else:result=self.value('Member',0x6346B0,(0 if index<0 else BASE+0x10000+index*0x100,),node)
  return result,self.snapshot()
 def coordinates(self,kind,alias,offset,invalid):
  self.graph();self.alias=alias
  if invalid:self.put(PACKETS+32,-1,-1,-1)
  values=list(self.read(PACKETS+32,3));values[1]+=offset
  if kind:result=self.value('IsAt',0x634550,values,NODE)
  else:result=self.value('Coords',0x6343C0,(OUTPUT+16,),NODE)
  return result,bytes(self.c.mem_read(OUTPUT,0x40)).hex(),self.trace
 def token_query(self,kind,count,index,steps,length,pending):
  self.graph(count);self.put(TOKEN+0x8C,pending,length,steps)
  if kind==0:result=self.value('TokenNode',0x636E60,(index,))
  elif kind==1:result=self.value('Last',0x636EB0)
  else:result=self.value('Committed',0x636570)
  return result,self.snapshot()
 def event(self,index,alias,owner):
  self.graph();self.put(TOKEN,BASE+0x10000+owner*0x100)
  output=PACKETS+index*0x300+owner*0x80 if alias else OUTPUT+16
  result=self.value('Event',0x636F00,(output,index))
  return result,self.snapshot()
 def commit(self,count,pending,steps,new_loop,null_mask,branch_count):
  self.graph(count,null_mask);self.put(TOKEN+0x8C,pending,2,steps);self.byte(TOKEN+0x98,new_loop)
  for i in range(4):self.put(NODE+i*0x200+0x30,branch_count)
  self.value('Commit',0x636CE0)
  return self.snapshot()

def main():
 p=argparse.ArgumentParser();p.add_argument('--original-game',action='store_true');p.add_argument('--exe',type=Path,default=ROOT/'out/reference/RA2MDddcompact/gamemd.exe');p.add_argument('--compile-db',type=Path,default=ROOT/'out/game-ui-x86/compile_commands.json');p.add_argument('--output',type=Path,default=ROOT/'out/radar-instruction-audit');a=p.parse_args()
 assert digest(a.exe)==SHA;dll=build(a.compile_db,a.output,a.original_game)
 old,new=[StateMachine(pefile.PE(str(a.exe)),pefile.PE(str(dll)),x) for x in (False,True)]
 old.original_game=new.original_game=a.original_game
 cases={'constructor':list(itertools.product((0,1),(0,0xA5,0xFF),(0,BASE+0x10100))),
 'node_query':[(k,c,i,d) for k,c,i,d in itertools.product((0,1),(0,1,3),(-1,0,1,2,3),(0,1)) if k or 0<=i<c],
 'coordinates':list(itertools.product((0,1),(0,1),(-1,0,1),(0,1))),
 'token_query':list(itertools.product((0,1,2),(0,1,3,4),(-1,0,1,3,4,5,8,0x7FFFFFFF),(-1,0,2),(1,2,3),(-2,-1,0,1,3,4))),
 'event':list(itertools.product(range(4),(0,1),range(3))),
 'commit':[(c,p,s,l,m,b) for c,p,s,l,m,b in itertools.product((0,1,3,4),(-1,0,1,3,4),(-1,0,2),(0,1),(0,5,15),(0,1,3)) if s==-1 or s<c]}
 diffs=[]
 for method,rows in cases.items():
  for row in rows:
   try:x=getattr(old,method)(*row);y=getattr(new,method)(*row)
   except Exception as e:raise RuntimeError((method,row,hex(old.c.reg_read(26)),hex(new.c.reg_read(26)))) from e
   if x!=y:diffs.append({'method':method,'input':row,'original':x,'candidate':y})
  print(method,len(rows),'cumulative mismatches',len(diffs),flush=True)
 report={'exe_sha256':SHA,'dll_sha256':digest(dll),'cases':{k:len(v) for k,v in cases.items()},'total':sum(map(len,cases.values())),'mismatches':len(diffs),'differences':diffs[:20],'boundary':('Compiled outgoing RA2_YRPP_GAME constructors, node and token query/commit wrappers execute the original EXE methods. Constructor vptrs are compared without normalization. No original-process installation claim.' if a.original_game else __doc__),'sources':{s:digest(ROOT/s) for s in ([SOURCES[-1]] if a.original_game else SOURCES)}}
 (a.output/('planning-state-game-comparison.json' if a.original_game else 'planning-state-comparison.json')).write_text(json.dumps(report,indent=2)+'\n');return bool(diffs)
if __name__=='__main__':raise SystemExit(main())
