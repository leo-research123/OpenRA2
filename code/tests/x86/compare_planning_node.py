#!/usr/bin/env python3
"""YR planning node member/branch lifecycle against production C++.

Uses original-layout actor/token/member/node/branch graphs and original global
vectors. EventCoords, vector virtual Find/Resize/Clear and allocation/free are
explicit shared boundaries; all node/branch caller state changes execute.
Compares ordered callbacks, allocation sizes/content, raw Event bytes, return
values and complete graph/global state. Destructor vptr addresses alone are
normalized. No whole planner, scheduling, token destruction or OOM crash
identity claim. Native integration separately uses actual vectors/allocators.
"""
import argparse,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_planning_input import Machine,ROOT,SHA,BASE,digest
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EAX,UC_X86_REG_ESP
SOURCES=['code/core/src/yrpp/PlanningNodeClassQueries.cpp','code/core/src/yrpp/PlanningNodeClassLifecycle.cpp','code/core/src/yrpp/PlanningTokenClassState.cpp','code/core/src/yrpp/PlanningBranchClass.cpp','code/core/src/yrpp/GamePlanningGraph.cpp','code/tests/x86/planning_node_probe.cpp']
UNIT=BASE+0x1000;TOKEN=BASE+0x3000;NODES=[BASE+x for x in (0x5000,0x6000,0x7000)]
MEMBERS=BASE+0x9000;PACKETS=BASE+0xC000;BRANCHES=BASE+0x10000;TABLE=BASE+0x30000
VECTORS=[0xAC4B30,0xAC4C18,0xAC4C98];PTRS=[0xAC4CCC,0xAC4C38,0xAC4BF0]
HEAP=BASE+0x100000;GROW=BASE+0x120000

def build(db,out,game=False):
    return build_probe(db, out, [SOURCES[-1]] if game else SOURCES, 'planning_node_game_probe.dll' if game else 'planning_node_probe.dll',
        object_suffix='-plan-node-game.obj' if game else '-plan-node.obj',
        flags=(['/DRA2_YRPP_GAME'] if game else []) + ['/GR-', '/GS-', '/Gy'])

class NodeMachine(Machine):
 def __init__(self,*args):
  super().__init__(*args)
  self.ptrs=PTRS
  if self.candidate:
   super().call('Globals',0,ecx=BASE)
   self.ptrs=self.read(BASE,3)
 def value(self,name,pc,args=(),ecx=NODES[1]):
  super().call(name,pc,args=args,ecx=ecx)
  return self.c.reg_read(UC_X86_REG_EAX)
 def vector(self,ptr,items,values,capacity=8,owned=False):
  self.put(ptr,TABLE,items,capacity);self.byte(ptr+12,1);self.byte(ptr+13,owned)
  self.put(ptr+16,len(values),10)
  if values:self.put(items,*values)
 def packet(self,ptr,tag):
  self.c.mem_write(ptr,bytes((i*31+tag)%256 for i in range(111)))
  self.byte(ptr,4);self.put(ptr+32,tag,tag*2,tag*3)
 def setup(self,count=2,position=1,branches=2):
  self.c.mem_write(BASE,bytes([0xA5])*0x40000);self.c.mem_write(HEAP,bytes([0xBC])*0x40000)
  self.trace=[];self.allocs=[];self.heap=HEAP;self.grow=GROW;self.resize=True;self.alias=False;self.mutate=False;self.coord_calls=0
  self.put(TABLE,BASE+0x30100,0,BASE+0x30110,BASE+0x30120,BASE+0x30130)
  for u in range(2):
   unit=UNIT+u*0x1000;token=TOKEN+u*0x1000
   self.put(unit+0x514,token);self.put(token,unit)
   self.vector(token+4,token+0x100,NODES[:count]);self.put(token+0x8C,-1,-1,-1);self.byte(token+0x98,0);self.byte(token+0x99,0)
  for n,node in enumerate(NODES):
   self.vector(node,node+0x200,[MEMBERS+n*0x100,MEMBERS+n*0x100+0x20])
   self.put(node+0x18,77);self.byte(node+0x1C,0)
   self.vector(node+0x20,node+0x300,[BRANCHES+n*0x400+i*0x80 for i in range(branches)])
   self.packet(node+0x38,10);self.put(node+0xA8,0,88,-1,66)
   for u in range(2):
    member=MEMBERS+n*0x100+u*0x20;packet=PACKETS+n*0x200+u*0x80
    self.put(member,UNIT+u*0x1000,packet,-1);self.byte(member+12,0);self.packet(packet,10+n*10)
   for b in range(3):
    branch=BRANCHES+n*0x400+b*0x80;self.packet(branch,10+b*10);self.put(branch+0x70,1,99)
  for i,vec in enumerate(VECTORS):self.vector(vec,BASE+0x20000+i*0x100,[])
  for p in self.ptrs:self.put(p,0)
  self.node=NODES[position];self.member=MEMBERS+position*0x100
 def hook(self,c,pc,size,data):
  sp=c.reg_read(UC_X86_REG_ESP);obj=c.reg_read(UC_X86_REG_ECX)
  if pc==0x633BC0:
   event=c.reg_read(UC_X86_REG_EDX);raw=bytes(c.mem_read(event,111));self.trace.append(['coords',raw.hex()])
   xyz=self.read(event+32,3);target=BASE+0x2F000 if self.alias else obj;self.put(target,*xyz)
   self.coord_calls+=1
   if self.mutate and self.coord_calls==1:self.packet(BRANCHES+0x400,123)
   self.ret(target)
  elif pc==0x7C8E17:
   size=self.read(sp+4)[0];ptr=self.heap;self.heap=(ptr+size+15)&~15;self.allocs.append((ptr,size));self.trace.append(['alloc',ptr,size]);self.ret(ptr)
  elif pc==0x7C8B3D:
   ptr=self.read(sp+4)[0];self.trace.append(['free',ptr]);self.ret()
  elif pc==BASE+0x30130:
   key=self.read(self.read(sp+4)[0])[0];self.trace.append(['find',obj,key]);items=self.read(obj+4)[0];count=self.read(obj+16)[0]
   result=-1
   if not c.mem_read(obj+12,1)[0]:result=0
   else:
    for i in range(count):
     if self.read(items+4*i)[0]==key:result=i;break
   self.ret(result,4)
  elif pc==BASE+0x30110:
   cap,external=self.read(sp+4,2);self.trace.append(['resize',obj,cap,external,self.resize])
   if self.resize:
    old_items,old_cap=self.read(obj+4,2);ptr=self.grow;self.grow+=((cap*4+15)&~15)
    c.mem_write(ptr,bytes([0xA5])*(cap*4))
    if old_items and old_cap:c.mem_write(ptr,bytes(c.mem_read(old_items,min(cap,old_cap)*4)))
    self.put(obj+4,ptr,cap);self.byte(obj+12,1);self.byte(obj+13,1)
   self.ret(self.resize,8)
  elif pc==BASE+0x30120:
   self.trace.append(['clear',obj]);ptr=self.read(obj+4)[0]
   if ptr and c.mem_read(obj+13,1)[0]:self.trace.append(['free',ptr])
   self.put(obj+4,0,0);self.byte(obj+13,0);self.put(obj+16,0);self.ret()
 def result(self,value=None,dtor=False):
  if dtor:
   self.put(self.node,0);self.put(self.node+0x20,0)
  return {'value':value,'trace':self.trace,'graph':bytes(self.c.mem_read(BASE,0x24000)).hex(),
   'globals':[bytes(self.c.mem_read(v,24)).hex() for v in VECTORS],
   'pointers':[self.read(p)[0] for p in self.ptrs],
   'allocations':[(p,n,bytes(self.c.mem_read(p-8,n+24)).hex()) for p,n in self.allocs],
   'growth':bytes(self.c.mem_read(GROW,self.grow-GROW)).hex()}
 def neighbor(self,kind,count,pos,owner,missing):
  self.setup(count,pos)
  if missing:self.put(TOKEN+owner*0x1000+0x100+pos*4,0)
  v=self.value('Next' if kind else 'Previous',0x633F20 if kind else 0x633EA0,(UNIT+owner*0x1000,),self.node)
  return self.result(v)
 def branch(self,add,count,pos,refs,common,match,alias,mutate,grow):
  self.setup(count,pos,2);self.alias=alias;self.mutate=mutate
  for i,r in enumerate(refs):self.put(BRANCHES+pos*0x400+i*0x80+0x70,r)
  self.put(self.node+0xA8,common,88,-1,66);self.packet(self.node+0x38,match)
  if grow:
   self.put(self.node+0x28,2);self.byte(self.node+0x2D,1);self.resize=grow==1
   for i in range(2):self.packet(BRANCHES+pos*0x400+i*0x80,99+i)
  v=self.value('AddBranch' if add else 'FindBranch',0x634E10 if add else 0x634CC0,(self.member if add else UNIT,),self.node)
  return self.result(v)
 def add_member(self,old_count,member_count,grow,fail,initial_pending):
  self.setup(max(old_count,1),old_count,0)
  self.put(TOKEN+0x14,old_count);self.put(self.node+0x10,member_count);self.put(TOKEN+0x8C,initial_pending)
  if grow:
   self.put(TOKEN+12,old_count);self.byte(TOKEN+17,1)
   self.put(self.node+8,member_count);self.byte(self.node+13,1);self.resize=not fail
  v=self.value('AddMember',0x633FA0,(UNIT,PACKETS+0x480),self.node)
  return self.result(v)
 def release(self,loop_only,index,flag,refs,common,dirty,grow):
  self.setup();self.put(self.member+8,index);self.byte(self.member+12,flag)
  self.put(BRANCHES+0x400+0x70,refs);self.put(self.node+0xA8,common,88,1,66);self.byte(self.node+0x1C,dirty)
  if grow:self.put(VECTORS[2]+8,0);self.resize=grow==1
  v=self.value('ReleaseLoop' if loop_only else 'ReleaseBranch',0x6351E0 if loop_only else 0x635060,(self.member,),self.node)
  return self.result(v if loop_only else None)
 def clear(self,count,members,owned,dtor):
  self.setup(branches=count);self.put(self.node+0x10,members);self.byte(self.node+0x2D,owned)
  self.put(self.member+8,0);self.byte(self.member+12,1);self.put(self.node+0xA8,3,88,1,66)
  for i,vec in enumerate(VECTORS):self.vector(vec,BASE+0x20000+i*0x100,[NODES[0],self.node,NODES[2],self.node])
  for i,p in enumerate(self.ptrs):self.put(p,self.node if i!=1 else NODES[0])
  self.value('DestroyNode' if dtor else 'ClearBranches',0x633D30 if dtor else 0x635120,ecx=self.node)
  return self.result(dtor=dtor)
 def remove_member(self,count,index,refs,loop,registered,owned):
  self.setup();self.put(self.node+0x10,count);self.byte(self.node+13,owned)
  self.put(self.member+8,0);self.byte(self.member+12,loop)
  self.put(BRANCHES+0x400+0x70,refs);self.put(self.node+0xA8,1,88,1,66)
  if registered:
   for i,vec in enumerate(VECTORS):self.vector(vec,BASE+0x20000+i*0x100,[NODES[0],self.node,NODES[2]])
   for p in self.ptrs:self.put(p,self.node)
  self.value('RemoveMember',0x6340B0,(UNIT+index*0x1000,),self.node)
  return self.result(dtor=(count==1 and index==0))
 def invalidation(self,branches,count):
  self.setup(branches=count);self.value('Invalidate',0x635DB0,(branches,),self.node);return self.result()
 def loop(self,steps1,steps2,common,branches,match,alias):
  self.setup(3,1,branches);self.alias=alias
  self.put(TOKEN+0x94,steps1);self.put(TOKEN+0x1000+0x94,steps2);self.put(self.node+0xA8,common,88,1,66)
  self.packet(self.node+0x38,match)
  self.value('UpdateLoop',0x6349B0,ecx=self.node);return self.result()
 def registry(self,unregister,mask,duplicates,flagged,grow):
  self.setup();self.byte(self.node+0x1C,flagged)
  for i,vec in enumerate(VECTORS):
   values=[NODES[0],self.node,NODES[2]] if mask&(1<<i) else [NODES[0],NODES[2]]
   if duplicates:values.append(self.node)
   self.vector(vec,BASE+0x20000+i*0x100,values,capacity=len(values) if grow else 8,owned=bool(grow))
  self.resize=grow!=2
  for i,p in enumerate(self.ptrs):self.put(p,self.node if mask&(1<<i) else NODES[0])
  self.value('UnregisterNode' if unregister else 'FlagNode',0x637640 if unregister else 0x6378B0,ecx=self.node)
  return self.result()

def main():
 p=argparse.ArgumentParser();p.add_argument('--original-game',action='store_true');p.add_argument('--exe',type=Path,default=ROOT/'out/reference/RA2MDddcompact/gamemd.exe');p.add_argument('--compile-db',type=Path,default=ROOT/'out/game-ui-x86/compile_commands.json');p.add_argument('--output',type=Path,default=ROOT/'out/radar-instruction-audit');a=p.parse_args()
 assert digest(a.exe)==SHA;dll=build(a.compile_db,a.output,a.original_game)
 old,new=[NodeMachine(pefile.PE(str(a.exe)),pefile.PE(str(dll)),x) for x in (False,True)]
 cases={'neighbor':[(k,c,p,u,m) for k,c,p,u,m in itertools.product((0,1),(1,2,3),range(3),(0,1),(0,1)) if p<c],
 'branch':[(a,c,p,r,q,m,x,t,g) for a,c,p,r,q,m,x,t,g in itertools.product((0,1),(1,2,3),range(3),((0,0),(1,0),(0,1),(-1,2)),(0,1,2),(10,20,30),(0,1),(0,1),(0,1,2)) if p<c],
 'add_member':[(c,m,g,f,p) for c,m,g,f,p in itertools.product((0,1,2),(0,1),(0,1),(0,1),(-1,0,2)) if not(g and f) or c==0],
 'release':list(itertools.product((0,1),(-1,0),(0,1),(0,1,2,0x80000000),(0,1,2),(0,1),(0,1,2))),
 'clear':list(itertools.product((0,1,3),(0,1,2),(0,1),(0,1))),
 'remove_member':list(itertools.product((0,1,2),(0,1,2),(1,2),(0,1),(0,1),(0,1))),
 'invalidation':list(itertools.product((0,1),(0,1,3))),
 'loop':list(itertools.product((-1,0,1,2),(-1,0,1,2),(0,1,2),(0,1,3),(10,20,30),(0,1))),
 'registry':list(itertools.product((0,1),range(8),(0,1),(0,1),(0,1,2)))}
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
 report={'exe_sha256':SHA,'dll_sha256':digest(dll),'cases':{k:len(v) for k,v in cases.items()},'total':sum(map(len,cases.values())),'mismatches':mismatches,'differences':diffs[:8],'boundary':('Compiled outgoing RA2_YRPP_GAME node/member/branch, destructor and registry wrappers execute original EXE bodies with the same declared shared boundaries. Checks stack cleanup, original storage and call ordering. No hook installation or original-process acceptance claim.' if a.original_game else __doc__),'sources':{s:digest(ROOT/s) for s in ([SOURCES[-1]] if a.original_game else SOURCES)}}
 (a.output/('planning-node-game-comparison.json' if a.original_game else 'planning-node-comparison.json')).write_text(json.dumps(report,indent=2)+'\n');return bool(mismatches)
if __name__=='__main__':raise SystemExit(main())
