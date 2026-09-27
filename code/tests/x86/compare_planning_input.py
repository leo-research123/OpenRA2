#!/usr/bin/env python3
"""Original planning input submission/rejection and Techno click callers.

Production Event typed constructor, submission, queue writes and caller bodies
execute. Compare original defined wire bytes, caller state, full out-list,
read-only timestamps and ordered callbacks. Target conversion, attack predicate,
actor virtual feedback/type/capability, random and message/audio are declared
shared boundaries. Event::Execute and planner node creation are not executed.
Only native caller-initialized unused event bytes are normalized for callers;
standalone construction/submission compare every byte and surrounding guards.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
BASE,STACK,STOP,QUEUE=0x01000000,0x013E0000,0x013F0000,0xA802C8
SOURCES=['code/core/src/yrpp/EventClassConstruction.cpp','code/core/src/yrpp/EventClassQueue.cpp','code/core/src/yrpp/GamePlanningInput.cpp','code/core/src/yrpp/TechnoClassClick.cpp','code/core/src/yrpp/GamePlanningCommands.cpp','code/core/src/yrpp/PlanningNodeClassQueries.cpp','code/tests/x86/planning_input_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'planning_input_probe.dll',
        object_suffix='-plan-input.obj')
class Machine:
 def __init__(self,exe,dll,candidate):
  self.c=Uc(UC_ARCH_X86,UC_MODE_32);self.candidate=candidate
  for pe in (exe,dll):
   base=pe.OPTIONAL_HEADER.ImageBase;self.c.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);self.c.mem_write(base,pe.get_memory_mapped_image())
  self.c.mem_map(BASE,0x400000);self.c.mem_map(0,4096)
  self.exports={s.name.decode().lstrip('_@').split('@')[0]:dll.OPTIONAL_HEADER.ImageBase+s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols}
  self.c.hook_add(UC_HOOK_CODE,self.hook)
 def put(self,p,*v):self.c.mem_write(p,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
 def read(self,p,n=1):return struct.unpack('<'+'I'*n,self.c.mem_read(p,4*n))
 def byte(self,p,v):self.c.mem_write(p,bytes([v&255]))
 def string(self,p):
  b=bytearray()
  while p:
   x=self.c.mem_read(p,1);p+=1
   if not x[0]:break
   b.extend(x)
  return b.decode(errors='replace')
 def ret(self,v=0,pop=0):
  sp=self.c.reg_read(UC_X86_REG_ESP);self.c.reg_write(UC_X86_REG_EIP,self.read(sp)[0]);self.c.reg_write(UC_X86_REG_ESP,sp+4+pop);self.c.reg_write(UC_X86_REG_EAX,v&0xffffffff)
 def query_hook(self,c,pc):
  sp=c.reg_read(UC_X86_REG_ESP);obj=c.reg_read(UC_X86_REG_ECX)
  if pc in (0x6E6E20,0x6E7C20,0x6E6F20):
   ident=self.read(obj)[0];kind=c.mem_read(obj+4,1)[0]
   name={0x6E6E20:'abstract',0x6E7C20:'as_cell',0x6E6F20:'as_techno'}[pc]
   self.trace.append([name,ident,kind])
   if pc==0x6E6E20:self.ret(BASE+ident*0x1000 if self.valid&(1<<(ident-1)) else 0)
   elif pc==0x6E7C20:self.ret(BASE+0x7000 if self.resolution==1 else 0)
   else:self.ret(BASE+0x2000 if self.resolution==2 else 0)
   return True
  if pc==BASE+0x31000:
   out=self.read(sp+4)[0];ident=(obj-BASE)//0x1000;self.trace.append(['coords',ident])
   self.put(out,ident*100,ident*200,ident*300)
   if self.alias:self.put(BASE+0x28000,ident*400,ident*500,ident*600);out=BASE+0x28000
   self.ret(out,4);return True
  if pc==BASE+0x31010:
   out=self.read(sp+4)[0];self.trace.append(['map_coords',obj])
   if obj==BASE+0x1000:
    xy=(40+self.shift*(self.map_reads!=0),50);self.map_reads+=1
   else:xy=self.remote
   c.mem_write(out,struct.pack('<hh',*xy));self.ret(out,4);return True
  return False
 def hook(self,c,pc,size,data):
  if self.query_hook(c,pc):return
  sp=c.reg_read(UC_X86_REG_ESP);self_=c.reg_read(UC_X86_REG_ECX)
  if pc==BASE+0x30000:
   self.trace.append(['clock']);self.ret(self.timestamp)
  elif pc==0x731BF0:self.trace.append(['attack_move']);self.ret(self.attack)
  elif pc==0x6E6AB0:
   obj=self.read(sp+4)[0];self.trace.append(['target',obj]);index=(obj-BASE)//0x1000 if obj else 0
   c.mem_write(self_,struct.pack('<IB',index*0x1234567,index+10 if obj else 0));self.ret(self_,4)
   if self.mutate:self.put(0xA8ED84,self.read(0xA8ED84)[0]+1)
  elif pc==BASE+0x30100:self.trace.append(['can_attack']);self.ret(self.capable)
  elif pc==BASE+0x30200:
   self.trace.append(['type']);self.ret(BASE+0x20000);self.type_reads+=1
   if self.type_reads==2 and self.mutate:self.put(BASE+0x204B0,2)
  elif BASE+0x30300<=pc<=BASE+0x30360:
   n=(pc-BASE-0x30300)//16;extra=self.read(sp+4)[0] if n in (0,6) else None
   self.trace.append(['voice',n,extra]);self.ret(0,4 if n in (0,6) else 0)
  elif pc==0x65C780:self.trace.append(['random']);self.ret(self.random)
  elif pc==0x734E60:
   file,line=self.read(sp+4,2);self.trace.append(['string',self.string(self_),c.reg_read(UC_X86_REG_EDX),self.string(file),line]);self.ret(BASE+0x28000,8)
  elif pc==0x730A90:self.trace.append(['message',self_,c.reg_read(UC_X86_REG_EDX)]);self.ret()
  elif pc==0x750920:self.trace.append(['sound',self_,c.reg_read(UC_X86_REG_EDX),*self.read(sp+4,2)]);self.ret(pop=8)
 def setup(self,planning=0,feedback=0,house=0,count=0,tail=0,reported=0,mutate=0):
  self.c.mem_write(BASE,bytes(0x40000));self.c.mem_write(QUEUE,bytes([0xA5])*0x398C)
  self.put(QUEUE,count,63,tail);self.put(0xA8ED84,0xF1234567);self.byte(0xAC4CF4,planning);self.byte(0x822CF2,feedback);self.byte(0xAC4C08,reported)
  self.put(0xA83D4C,BASE+0x9000);self.put(BASE+0x9030,house);self.put(0x8871E0,BASE+0xA0000);self.put(BASE+0xA0700,77)
  self.put(0x7E1530,BASE+0x30000);self.trace=[];self.timestamp=0xFFFFFFF7;self.attack=0;self.capable=0;self.random=0xF1234567;self.mutate=mutate;self.type_reads=0
  self.put(BASE+0x1000,BASE+0x10000)
  self.put(BASE+0x104C0,BASE+0x30100);self.put(BASE+0x10084,BASE+0x30200)
  for i,slot in enumerate((0x354,0x358,0x35C,0x364,0x368,0x36C,0x370)):self.put(BASE+0x10000+slot,BASE+0x30300+16*i)
  self.put(BASE+0x204A4,BASE+0x25000);self.put(BASE+0x204B0,3);self.put(BASE+0x25000,101,202,303)
 def call(self,name,pc,args=(),ecx=0,raw=None,pop=None,edx=0):
  data=raw if raw is not None else struct.pack('<'+'I'*len(args),*(x&0xffffffff for x in args))
  self.put(STACK,STOP);self.c.mem_write(STACK+4,bytes(data))
  self.c.reg_write(UC_X86_REG_ESP,STACK);self.c.reg_write(UC_X86_REG_ECX,ecx);self.c.reg_write(UC_X86_REG_EDX,edx)
  self.c.emu_start(self.exports[name] if self.candidate else pc,STOP,count=1000000)
  assert self.c.reg_read(UC_X86_REG_EIP)==STOP,'trap/instruction limit'
  assert self.c.reg_read(UC_X86_REG_ESP)==STACK+4+(len(data) if pop is None else pop),'stack balance'
  return self.c.reg_read(UC_X86_REG_EAX)&255
 def queue_state(self,kind=None):
  data=bytearray(self.c.mem_read(QUEUE,0x398C));count,head,tail=self.read(QUEUE,3)
  if kind and count==self.start_count+1:
   start=12+111*self.start_tail
   # Exclude original caller's undefined stack bytes, not constructor writes.
   defined={0,2,3,4,5,6}
   if self.house>=0:
    defined.update(range(7,30) if kind=='mission' else range(7,12));defined.discard(13)
   for i in range(111):
    if i not in defined:data[start+i]=0
  return data.hex()
 def clicked(self,mission,planning,feedback,attack,capable,house,count,tail,shape,mutate):
  self.setup(planning,feedback,house,count,tail,mutate=mutate);self.attack=attack;self.capable=capable;self.house=house;self.start_count=count;self.start_tail=tail
  args=[mission,*[BASE+(i+2)*0x1000 if shape&(1<<i) else 0 for i in range(3)]]
  result=self.call('ClickMission',0x6FFBE0,args,BASE+0x1000)
  return {'value':result,'trace':self.trace,'queue':self.queue_state('mission'),'frame':self.read(0xA8ED84)[0]}
 def clicked_event(self,event_type,planning,reported,house,count,tail):
  self.setup(planning,house=house,count=count,tail=tail,reported=reported);self.house=house;self.start_count=count;self.start_tail=tail
  result=self.call('ClickEvent',0x6FFE00,[event_type],BASE+0x1000)
  return {'value':result,'trace':self.trace,'queue':self.queue_state('target'),'flag':self.c.mem_read(0xAC4C08,1)[0]}
 def submission(self,operation,event_type,count,tail,frame):
  self.setup(count=count,tail=tail);self.put(0xA8ED84,frame)
  event=bytearray((i*71+19)%256 for i in range(112));event[0]=event_type;self.c.mem_write(BASE+0x500,bytes(event))
  result=self.call('SubmitPlan' if operation else 'Enqueue',0x637DD0 if operation else 0x6521C0,raw=event)
  return {'value':result,'trace':self.trace,'queue':self.queue_state(),'event':bytes(self.c.mem_read(BASE+0x500,112)).hex()}
 def rejection(self,event_type,reported):
  self.setup(reported=reported);self.byte(BASE+0x500,event_type);self.call('Reject',0x639FD0,ecx=BASE+0x500,pop=0)
  return {'trace':self.trace,'flag':self.c.mem_read(0xAC4C08,1)[0]}
 def constructor(self,house,mission,frame,seed):
  self.setup();self.put(0xA8ED84,frame);self.c.mem_write(BASE+0x500,bytes([seed])*120)
  args=[house,0xF1234567,0x12345678,mission,0x89ABCDEF,0x80,0x1234567,0xFF,0xFEDCBA98,3]
  self.call('MissionEvent',0x4C6860,args,BASE+0x504)
  return bytes(self.c.mem_read(BASE+0x500,120)).hex()
 def query_setup(self):
  self.setup();self.valid=7;self.alias=0;self.resolution=0;self.remote=(40,50);self.shift=0;self.map_reads=0
  for i in range(1,4):self.put(BASE+i*0x1000,BASE+0x10000)
  self.put(BASE+0x10058,BASE+0x31000);self.put(BASE+0x101B8,BASE+0x31010)
  self.put(BASE+0x600,4);self.put(BASE+0x800,4)
 def coords_query(self,event_type,flags,valid,alias):
  self.query_setup();self.valid=valid;self.alias=alias;self.byte(BASE+0x600,event_type)
  for i,offset in enumerate((14,24,19)):
   self.put(BASE+0x600+offset,i+1);self.byte(BASE+0x600+offset+4,3 if flags&(1<<i) else 0)
  self.put(BASE+0x900,0xDEAD1234,0xDEAD1234,0xDEAD1234)
  self.call('EventCoords',0x633BC0,ecx=BASE+0x900,edx=BASE+0x600)
  return {'coords':self.read(BASE+0x900,3),'trace':self.trace,'returned':self.c.reg_read(UC_X86_REG_EAX)}
 def guard_query(self,present,mission,kind,resolution,shift,remote):
  self.query_setup();self.resolution=resolution;self.shift=shift;self.remote=remote
  self.c.mem_write(BASE+0x7024,struct.pack('<hh',*remote))
  result=self.call('LocalGuard',0x638B70,[2,kind],BASE+0x1000 if present else 0,edx=mission)
  return {'result':result,'trace':self.trace}
 def command_query(self,event_type,mission,nodes,old_mission,old_kind,owner):
  self.query_setup();self.byte(BASE+0x600,event_type);self.byte(BASE+0x60C,mission)
  self.put(BASE+0x1514,BASE+0x3000 if nodes>=0 else 0)
  self.put(BASE+0x3000,BASE+0x1000*(owner+1));self.put(BASE+0x3008,BASE+0x3400);self.put(BASE+0x3014,max(nodes,0))
  self.put(BASE+0x3400,BASE+0x4800,BASE+0x4000)
  self.put(BASE+0x4004,BASE+0x4400);self.put(BASE+0x4010,2)
  self.put(BASE+0x4400,BASE+0x4500,BASE+0x4510)
  self.put(BASE+0x4500,BASE+0x1000,BASE+0x800);self.put(BASE+0x4510,BASE+0x2000,BASE+0x800)
  self.byte(BASE+0x80C,old_mission);self.byte(BASE+0x812,old_kind)
  self.call('CheckCommand',0x638CE0,ecx=BASE+0x1000,edx=BASE+0x600)
  return {'result':self.c.reg_read(UC_X86_REG_EAX),'trace':self.trace,'current':bytes(self.c.mem_read(BASE+0x600,111)).hex(),'previous':bytes(self.c.mem_read(BASE+0x800,111)).hex()}
 def member_query(self,owners,owner):
  self.query_setup();self.put(BASE+0x4004,BASE+0x4400);self.put(BASE+0x4010,len(owners))
  for i,value in enumerate(owners):self.put(BASE+0x4400+4*i,BASE+0x4500+16*i);self.put(BASE+0x4500+16*i,value)
  self.call('FindMember',0x634290,[owner],BASE+0x4000)
  return self.c.reg_read(UC_X86_REG_EAX)
def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,default=ROOT/'out/reference/RA2MDddcompact/gamemd.exe');p.add_argument('--compile-db',type=Path,default=ROOT/'out/game-ui-x86/compile_commands.json');p.add_argument('--output',type=Path,default=ROOT/'out/radar-instruction-audit');a=p.parse_args()
 assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll=build(a.compile_db,a.output)
 old,new=[Machine(pefile.PE(str(a.exe)),pefile.PE(str(dll)),x) for x in (False,True)]
 cases={
  'coords_query':list(itertools.product((0,4,6),range(8),range(8),(0,1))),
  'guard_query':list(itertools.product((0,1),(-1,1,2,11,12),(0,3,255),range(3),(0,1),((40,50),(41,50),(-32768,32767)))),
  'command_query':list(itertools.product((0,4,6,7,9),(2,11),(-1,0,2),(1,2,7,8,9,11,255),(0,3,11,255),(0,1))),
  'member_query':list(itertools.product(((),(0,),(1,),(1,2),(2,1,1)),(0,1,2,3))),
  'constructor':list(itertools.product((-1,-128,0,127,128,255,256),(-1,0,1,2,7,29,255,256),(0,0xF1234567),(0xA5,0x5A))),
  'submission':list(itertools.product((0,1),(0,4,6,7,9,46,127,255),(0,127,128,129),(0,127),(0,0xF1234567))),
  'rejection':list(itertools.product(range(256),(0,1))),
  'clicked_event':list(itertools.product((0,4,6,7,9,46),(0,1),(0,1),(-1,0,127,128),(0,128),(0,127))),
  'clicked':list(itertools.product(range(-1,31),(0,1),(0,1),(0,1),(0,1),(0,),(0,128),(0,),(0,7),(0,1)))
 }
 differences=[]
 for method,rows in cases.items():
  for row in rows:
   expected=getattr(old,method)(*row);actual=getattr(new,method)(*row)
   if expected!=actual:
    differences.append({'method':method,'input':row,'original':expected,'candidate':actual})
    if len(differences)<=2:print('difference',method,row,expected if method=='constructor' else {k:v for k,v in expected.items() if k!='queue'},actual if method=='constructor' else {k:v for k,v in actual.items() if k!='queue'},flush=True)
  print(method,len(rows),'cumulative mismatches',len(differences),flush=True)
 report={'exe_sha256':SHA,'dll_sha256':digest(dll),'cases':{k:len(v) for k,v in cases.items()},'total':sum(map(len,cases.values())),'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
 (a.output/'planning-input-comparison.json').write_text(json.dumps(report,indent=2)+'\n');return bool(differences)
if __name__=='__main__':raise SystemExit(main())
