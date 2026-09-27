#!/usr/bin/env python3
"""Original PlanMgr token clear/destruction and unit clear caller.

Production token, node/member/branch and registry bodies execute together.
Compare graph/global state, packets, effective house counters, owner links,
ordered frees/virtual calls and stack cleanup. Allocator, vector virtual slots,
House control predicate and EventCoords remain declared shared boundaries.
Native integration tests actual units, vectors and destruction separately.
Destroyed object vptr addresses are normalized; no full Techno destructor,
planner scheduler, map retirement or original-process installation claim.
"""
import argparse,itertools,json
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_planning_node import NodeMachine,ROOT,SHA,BASE,UNIT,TOKEN,NODES,MEMBERS,BRANCHES,PACKETS,VECTORS,HEAP,GROW,digest
from compare_planning_input import Machine
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EDX
SOURCES=['code/core/src/yrpp/PlanningNodeClassQueries.cpp','code/core/src/yrpp/PlanningNodeClassLifecycle.cpp','code/core/src/yrpp/PlanningTokenClassState.cpp','code/core/src/yrpp/PlanningTokenClassLifecycle.cpp','code/core/src/yrpp/PlanningBranchClass.cpp','code/core/src/yrpp/GamePlanningGraph.cpp','code/core/src/yrpp/GamePlanningTokens.cpp','code/tests/x86/planning_node_probe.cpp','code/tests/x86/planning_token_probe.cpp']
TOKEN_ARRAY=0xAC4C78;UNIT_ARRAY=0xAC4C40;COUNTS=0xAC4B84;DELETING=0xAC4CF5
HOUSE=BASE+0x25000;PLAYER=BASE+0x26000;EVENT=BASE+0x28000

def build(db,out,game=False):
    return build_probe(db, out, SOURCES[-2:] if game else SOURCES, 'planning_token_game_probe.dll' if game else 'planning_token_probe.dll',
        object_suffix='-plan-token-game.obj' if game else '-plan-token.obj',
        flags=(['/DRA2_YRPP_GAME'] if game else []) + ['/GR-', '/GS-', '/Gy'])

class TokenMachine(NodeMachine):
 def __init__(self,*args):
  self.trace=[];self.units=UNIT_ARRAY;self.deleting=DELETING
  super().__init__(*args)
  if self.candidate:
   Machine.call(self,'TokenGlobals',0,ecx=BASE)
   self.units,self.deleting=self.read(BASE,2)
 def setup_token(self,count=2,members=2,controlled=0,player=True,house=3,value=2):
  super().setup(count,1,2)
  self.controlled=controlled;self.mutation=0;self.node=NODES[1]
  self.put(UNIT+0x21C,HOUSE);self.put(UNIT+0x1000+0x21C,HOUSE)
  self.put(HOUSE+0x30,house);self.put(PLAYER+0x30,4);self.put(0xA83D4C,PLAYER if player else 0)
  self.put(COUNTS,*([value]*24));self.byte(self.deleting,0)
  self.vector(TOKEN_ARRAY,BASE+0x21000,[TOKEN,TOKEN+0x1000,TOKEN])
  self.vector(self.units,BASE+0x21100,[UNIT,UNIT+0x1000,UNIT])
  for i,vec in enumerate(VECTORS):self.vector(vec,BASE+0x20000+i*0x100,list(NODES))
  for p in self.ptrs:self.put(p,NODES[0])
  self.put(TOKEN+0x8C,1,3,1);self.byte(TOKEN+0x1C,1);self.byte(TOKEN+0x98,1);self.byte(TOKEN+0x99,1)
  self.packet(EVENT,71);self.byte(EVENT+0x1D,0)
  for n,node in enumerate(NODES):
   self.put(node+0x10,members)
   self.put(MEMBERS+n*0x100+8,0);self.byte(MEMBERS+n*0x100+12,1)
   self.put(node+0xA8,1,88,0,66);self.put(BRANCHES+n*0x400+0x70,1)
 def hook(self,c,pc,size,data):
  if pc==0x50B6F0:
   self.trace.append(['control',c.reg_read(UC_X86_REG_ECX),self.controlled,self.read(UNIT+0x514)[0]])
   if self.mutation:
    self.put(HOUSE+0x30,77);self.put(PLAYER+0x30,6)
    if self.mutation==2:self.put(0xA83D4C,0)
    if self.mutation==3:self.put(UNIT+0x514,TOKEN+0x1000)
   self.ret(self.controlled);return
  if pc in (BASE+0x30110,BASE+0x30120,BASE+0x30130,0x7C8B3D):
   self.trace.append(['unit_link',self.read(UNIT+0x514)[0]])
  begin=len(self.trace)
  super().hook(c,pc,size,data)
  for row in self.trace[begin:]:
   if row[0] in ('find','resize','clear') and row[1]==self.units:row[1]=UNIT_ARRAY
 def result(self,dtor=False):
  freed={row[1] for row in self.trace if row[0]=='free'}
  for node in NODES:
   if node in freed:self.put(node,0);self.put(node+0x20,0)
  if dtor:self.put(TOKEN+4,0)
  base=super().result()
  base.update({'unit_vector':bytes(self.c.mem_read(self.units,24)).hex(),'token_vector':bytes(self.c.mem_read(TOKEN_ARRAY,24)).hex(),
   'counters':self.read(COUNTS,24),'deleting':self.c.mem_read(self.deleting,1)[0],
   'house':bytes(self.c.mem_read(HOUSE,0x2000)).hex(),'current_player':self.read(0xA83D4C)[0]})
  return base
 def lifecycle(self,kind,count,members,controlled,player,deleting,mutation,owner_null,value):
  self.setup_token(count,members,controlled,player,value=value);self.mutation=mutation;self.byte(self.deleting,deleting)
  if owner_null:self.put(TOKEN,0)
  self.value(('TokenClear','TokenDestruct','TokenDestroy')[kind],(0x636120,0x635F80,0x636310)[kind],ecx=TOKEN)
  return self.result(dtor=kind!=0)
 def clear_unit(self,present,count,members,flag,controlled,mutation):
  self.setup_token(count,members,controlled);self.mutation=mutation
  if present==1:self.put(UNIT+0x514,0)
  self.byte(EVENT+0x1D,flag)
  Machine.call(self,'ClearMember' if self.candidate and self.game and present else 'ClearUnit',0x6386E0,ecx=UNIT if present else 0,edx=0 if flag==-1 else EVENT)
  return self.result()
 def owner(self,house,player,controlled,mutation):
  self.setup_token(house=house,player=player,controlled=controlled);self.mutation=mutation
  value=self.value('OwnerIndex',0x6339B0,ecx=UNIT);result=self.result();result['value']=value;return result
 def decrement(self,house,value):
  self.setup_token(value=value);self.value('CountDrop',0x639F80,ecx=house);return self.result()

def main():
 p=argparse.ArgumentParser();p.add_argument('--original-game',action='store_true');p.add_argument('--exe',type=Path,default=ROOT/'out/reference/RA2MDddcompact/gamemd.exe');p.add_argument('--compile-db',type=Path,default=ROOT/'out/game-ui-x86/compile_commands.json');p.add_argument('--output',type=Path,default=ROOT/'out/radar-instruction-audit');a=p.parse_args()
 assert digest(a.exe)==SHA;dll=build(a.compile_db,a.output,a.original_game)
 old,new=[TokenMachine(pefile.PE(str(a.exe)),pefile.PE(str(dll)),x) for x in (False,True)]
 old.game=new.game=a.original_game
 cases={'lifecycle':[(k,n,m,c,p,d,x,o,v) for k,n,m,c,p,d,x,o,v in itertools.product(range(3),(0,1,3),(1,2),(0,1),(0,1),(0,1),(0,1,2,3),(0,1),(0,2)) if not(o and n)],
 'clear_unit':list(itertools.product((0,1,2),(0,1,3),(1,2),(-1,0,1,2,128,255),(0,1),(0,3))),
 'owner':list(itertools.product((-1,0,3,23,24,0x7FFFFFFF),(0,1),(0,1),(0,1,2,3))),
 'decrement':list(itertools.product((-1,0,1,23,24,0x7FFFFFFF),(-1,0,1,2,128,0x7FFFFFFF,0x80000000)))}
 diffs=[];mismatches=0
 for method,rows in cases.items():
  for row in rows:
   try:x=getattr(old,method)(*row);y=getattr(new,method)(*row)
   except Exception as e:raise RuntimeError((method,row,hex(old.c.reg_read(26)),hex(new.c.reg_read(26)))) from e
   if x!=y:
    mismatches+=1
    if len(diffs)<8:diffs.append({'method':method,'input':row,'fields':[k for k in x if x[k]!=y[k]],'original':x,'candidate':y})
    if mismatches<=3:print('difference',method,row,diffs[-1]['fields'],flush=True)
  print(method,len(rows),'cumulative mismatches',mismatches,flush=True)
 report={'exe_sha256':SHA,'dll_sha256':digest(dll),'cases':{k:len(v) for k,v in cases.items()},'total':sum(map(len,cases.values())),'mismatches':mismatches,'differences':diffs,'boundary':('Outgoing RA2_YRPP_GAME typed calls execute original token lifecycle/accounting and unit-clear bodies. Same declared shared boundaries as native comparison; not full original-process acceptance.' if a.original_game else __doc__),'sources':{s:digest(ROOT/s) for s in (SOURCES[-2:] if a.original_game else SOURCES)}}
 (a.output/('planning-token-game-comparison.json' if a.original_game else 'planning-token-comparison.json')).write_text(json.dumps(report,indent=2)+'\n');return bool(mismatches)
if __name__=='__main__':raise SystemExit(main())
