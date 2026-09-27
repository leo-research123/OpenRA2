#!/usr/bin/env python3
"""YR EVA queue and playback control: complete production Vox bodies vs fixed EXE.

The original circular links execute, not a queue substitute. Compare queue and
voice state, live/orphaned allocations, frees, FIFO/priority selection, callbacks,
parameters, 64-bit deadlines and side filenames. Device creation/playback/state,
clock, vector lookup and allocator/CRT services are explicit boundaries. No device output,
Godot, original-process acceptance, INI parser or heap failure equivalence claim.
"""
import argparse,hashlib,itertools,json,re,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,STACK,STOP,SHA
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/VoxClass.cpp','code/core/src/yrpp/VoxClassQueue.cpp','code/tests/x86/vox_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'vox_probe.dll',
        object_suffix='-vox.obj',
        symbols=True)
VA=dict(Init=0x752290,Shutdown=0x752340,Clear=0x752370,Enqueue=0x752590,Find=0x752680,Play=0x752480,PlayName=0x752700,Update=0x752760,Silence=0x752A40,Stop=0x7529A0,Speaking=0x7529E0,Suppress=0x753570,Unsuppress=0x753580,Pause=0x7535B0,Resume=0x753620,Reset=0x7535D0,Construct=0x752CB0,Destruct=0x752D60,Filename=0x753380,Name=0x753330,DeleteAll=0x7531A0)
class Machine(BaseMachine):
 def __init__(self,*args):
  super().__init__(*args);self.native_crt={};self.g=[0xB1D4A0,0xB1D4C8,0xB1D3C8,0xB1D3F0,0xB1D450,0xB1D4B8,0xB1D4BC,0xB1D4C0,0xB1D4C4,0xB1D4CC,0xB1D4D0,0xB1D3B8,0xB1D3E0,0xB1D3D8,0xB1D428]
  if self.candidate:self.call('Globals',0,BASE+0x700,[]);self.g=list(self.read(BASE+0x700,15))
  self.native_crt={}
  for line in args[2].read_text().splitlines():
   f=line.split()
   if len(f)>=3 and f[1] in ['_strcpy','_strcat','_strncpy','_strlen','_memcpy','_memset','_memmove']:
    self.native_crt[int(f[2],16)]=f[1]
 def string(self,p):return bytes(self.c.mem_read(p,256)).split(b'\0')[0]
 def qput(self,p,v):self.c.mem_write(p,struct.pack('<Q',v&0xffffffffffffffff))
 def qread(self,p):return struct.unpack('<Q',self.c.mem_read(p,8))[0]
 def hook(self,c,pc,size,data):
  sp=c.reg_read(UC_X86_REG_ESP);this=c.reg_read(UC_X86_REG_ECX);edx=c.reg_read(UC_X86_REG_EDX)
  if pc==0x7C8E17:
   n=self.read(sp+4)[0];p=BASE+0x50000+len(self.allocations)*0x200;self.allocations[p]=n;self.c.mem_write(p,b'\xA5'*n);self.trace.append(['new',p,n]);self.ret(p)
  elif pc==0x7C8B3D:
   p=self.read(sp+4)[0];assert p not in self.freed;self.freed.add(p);self.trace.append(['delete',p]);self.ret()
  elif pc==0x7C8D20:
   a,b=self.read(sp+4,2);a=self.string(a).lower();b=self.string(b).lower();self.ret((a>b)-(a<b))
  elif pc==BASE+0x30200:
   target=self.read(self.read(sp+4)[0])[0];items,capacity=self.read(this+4,2);index=-1
   for i in range(capacity):
    if self.read(items+4*i)[0]==target:index=i;break
   self.ret(index,4)
  elif pc==0x407010:self.ret(BASE+0x1000)
  elif pc==0x407860:self.trace.append(['create',this,*self.read(sp+4,2)]);self.ret(BASE+0x2000 if self.create else 0,8)
  elif pc==0x408080:self.trace.append(['stream_name',this,self.string(edx).decode()]);self.ret()
  elif pc in (0x407B40,0x407B50):self.trace.append(['volume',pc,this,edx]);self.ret()
  elif pc==0x407000:self.trace.append(['device']);self.ret(self.available)
  elif pc==0x408070:self.trace.append(['playing',this]);self.ret(self.playing)
  elif pc==0x408140:self.trace.append(['end_time',this]);self.ret(self.end);c.reg_write(UC_X86_REG_EDX,self.end>>32)
  elif pc==0x4093B0:self.trace.append(['clock']);self.ret(self.now);c.reg_write(UC_X86_REG_EDX,self.now>>32)
  elif pc==0x407B60:
   self.trace.append(['play_wav',this,self.string(edx).decode(),self.read(sp+4)[0]]);self.ret(self.success,4)
  elif pc in (0x407F40,0x407A90,0x407FB0,0x408000):self.trace.append(['stream_control',pc,this]);self.ret()
  elif pc in (0x7C8B6A,):self.ret() # not entered by covered allocation paths
  elif pc in self.native_crt:self.crt(self.native_crt[pc],sp)
 def crt(self,name,sp):
  dst,src,n=self.read(sp+4,3)
  if name=='_strlen':self.ret(len(self.string(dst)));return
  if name=='_memset':self.c.mem_write(dst,bytes([src&255])*n)
  elif name in ('_memcpy','_memmove'):self.c.mem_write(dst,bytes(self.c.mem_read(src,n)))
  elif name=='_strncpy':self.c.mem_write(dst,(self.string(src)+bytes(n))[:n])
  else:
   at=dst+(len(self.string(dst)) if name=='_strcat' else 0);self.c.mem_write(at,self.string(src)+b'\0')
  self.ret(dst)
 def invoke(self,name,a=0,b=0,stack=()):
  self.put(STACK,STOP,*stack);self.c.reg_write(UC_X86_REG_ESP,STACK);self.c.reg_write(UC_X86_REG_ECX,a&0xffffffff);self.c.reg_write(UC_X86_REG_EDX,b&0xffffffff)
  self.c.emu_start(self.exports[name] if self.candidate else VA[name],STOP,count=1000000)
  assert self.c.reg_read(UC_X86_REG_EIP)==STOP,(name,'trap')
  assert self.c.reg_read(UC_X86_REG_ESP)==STACK+4*(len(stack)+1),(name,'stack')
  return self.c.reg_read(UC_X86_REG_EAX)
 def setup(self):
  self.reset('vox');self.allocations={};self.freed=set();self.available=1;self.playing=0;self.create=1;self.success=1;self.now=1000;self.end=0
  for p in self.g[1:]:self.put(p,0)
  self.qput(self.g[10],0);self.byte(0xA8ED64,0);self.put(0x87E740,BASE+0x1800);self.put(0x87E750,BASE+0x1900)
  self.put(self.g[0]+4,BASE+0x3000,20);self.put(self.g[0]+16,8)
  self.put(self.g[0],BASE+0x9000);self.put(BASE+0x9010,BASE+0x30200)
  for i in range(8):
   p=BASE+0x10000+i*0x100;self.put(BASE+0x3000+i*4,p);self.c.mem_write(p,b'\xA5'*0x54)
   for off,name in [(0,f'EVA_{i}'),(44,f'Y_{i}'),(53,f'R_{i}'),(62,f'A_{i}')]:self.c.mem_write(p+off,name.encode()+b'\0')
   self.put(p+0x28,0x3F800000);self.put(p+0x48,i%4,1,2)
  self.invoke('Init');self.trace=[]
 def voice(self,i):return BASE+0x10000+i*0x100
 def enqueue(self,i,c,p):self.invoke('Enqueue',self.voice(i),c,(p,))
 def snapshot(self):
  def chain(at):
   result=[];p=self.read(at)[0]
   while p and p!=at:
    assert len(result)<100
    result.append(p);p=self.read(p)[0]
   return result
  heads=[self.g[2],self.g[3],*[self.g[4]+i*12 for i in range(4)]]
  normalize={p:-(i+1) for i,p in enumerate(heads)}
  live=[]
  for p,n in self.allocations.items():
   if p in self.freed:continue
   raw=list(self.read(p,n//4));raw=[normalize.get(v,v) if i<3 else v for i,v in enumerate(raw)]
   live.append([p,raw])
  return dict(trace=self.trace,queues=[chain(h) for h in heads],live=live,freed=sorted(self.freed),
   globals=[self.read(p)[0] for p in self.g[5:10]]+[self.qread(self.g[10])]+[self.read(p)[0] for p in self.g[11:]],
   voices=[bytes(self.c.mem_read(self.voice(i),0x54)).hex() for i in range(8)],
   array=[self.read(self.g[0]+16)[0],list(self.read(BASE+0x3000,20))])
 def sequence(self,requests,playing=1,side=0,success=1,serial=0):
  self.setup();self.put(self.g[1],side);self.put(self.g[7],serial);self.playing=playing;self.success=success
  for i,c,p in requests:self.invoke('Play',i,c,(p,))
  before=self.snapshot();self.trace=[];self.playing=0
  steps=[]
  for _ in range(len(requests)+2):self.invoke('Update');steps.append(self.snapshot());self.trace=[]
  self.invoke('Clear');steps.append(self.snapshot())
  return [before,steps]
 def deadline(self,gap,end,now,paused,quiet,available,stream,playing,current):
  self.setup();self.enqueue(1,1,2);self.qput(self.g[10],gap);self.put(self.g[14],paused);self.byte(0xA8ED64,quiet)
  self.available=available;self.put(self.g[9],BASE+0x2000 if stream else 0);self.playing=playing;self.end=end;self.now=now
  if current:self.put(self.g[8],self.voice(0));self.put(self.voice(0)+0x50,0)
  self.trace=[];self.invoke('Update');return self.snapshot()
 def controls(self,method,value,stream):
  self.setup();self.put(self.g[13],value);self.put(self.g[14],value);self.put(self.g[9],BASE+0x2000 if stream else 0)
  self.enqueue(0,1,1);self.enqueue(1,3,0);self.enqueue(2,0,3);self.put(self.g[8],self.voice(3));self.put(self.voice(3)+0x50,0)
  self.trace=[];result=self.invoke(method,1)
  return [result if method in ('Suppress','Unsuppress','Pause','Resume','Speaking') else None,self.snapshot()]
 def silence(self,index):
  self.setup();self.enqueue(0,0,1);self.enqueue(0,1,3);self.enqueue(0,2,3);self.enqueue(0,3,0);self.enqueue(1,1,2)
  self.put(self.g[8],self.voice(0));self.put(self.voice(0)+0x50,0);self.trace=[]
  self.invoke('Silence',index);return self.snapshot()
 def startup(self,exists,success):
  self.setup();self.put(self.g[9],BASE+0x2000 if exists else 0);self.create=success
  self.put(self.g[13],4);self.put(self.g[14],3);self.put(self.g[7],22);self.qput(self.g[10],777)
  self.trace=[];result=self.invoke('Init');return [result&255,self.snapshot()]
 def admission(self,index,control,priority,stream,suppress,current):
  self.setup();self.playing=1;self.put(self.g[9],BASE+0x2000 if stream else 0);self.put(self.g[13],suppress)
  if current:self.put(self.g[8],self.voice(7));self.put(self.voice(7)+0x50,0)
  self.invoke('Play',index,control,(priority,));return self.snapshot()
 def names(self,name):
  self.setup();self.playing=1
  if name is not None:self.c.mem_write(BASE+0x6000,name.encode()+b'\0')
  self.invoke('PlayName',BASE+0x6000 if name is not None else 0,-1,(-1,));return self.snapshot()
 def definition(self,name,side):
  self.setup();obj=BASE+0x80000;self.c.mem_write(obj,b'\xA5'*0x54);self.c.mem_write(BASE+0x6000,name.encode()+b'\0')
  self.invoke('Construct',obj,0,(BASE+0x6000,));constructed=bytes(self.c.mem_read(obj,0x54)).hex()
  self.put(self.g[1],side);ptr=self.invoke('Filename',obj);offset=ptr-obj
  names=[self.string(self.invoke('Name',i)).decode() for i in (-1,0,8,9)]
  self.invoke('Destruct',obj)
  return [constructed,offset,names,self.read(self.g[0]+16)[0],list(self.read(BASE+0x3000,10)),self.trace]
def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,sym=build(a.compile_db,a.output)
 old,new=[Machine(pefile.PE(str(a.exe)),pefile.PE(str(dll)),sym,b) for b in (False,True)]
 cases={'sequence':[],'deadline':[],'controls':list(itertools.product(('Suppress','Unsuppress','Pause','Resume','Speaking','Reset','Stop','Shutdown','DeleteAll'),(0,1,-1,-0x80000000,0x7FFFFFFF),(0,1))),
  'silence':[(i,) for i in (-1,0,1,7,8)],'startup':list(itertools.product((0,1),repeat=2)),
  'admission':list(itertools.product((-1,0,7,8),(-1,0,1,2,3),(-1,0,1,2,3),(0,1),(0,1,-1),(0,1))),
  'names':[(n,) for n in (None,'','EVA_0','eva_0','Eva_7','absent')],
  'definition':list(itertools.product(('A','TEST_123','X'*39),(-1,0,1,2,9)))}
 for c,p,d,q in itertools.product(range(4),range(4),range(4),range(4)):
  cases['sequence'].append(([(0,c,p),(1,d,q),(0,c,p),(2,1,0)],1,0,1,0))
 for side,success,serial in itertools.product((-1,0,1,2,9),(0,1),(0,99,0x7FFFFFFF,-0x80000000)):
  cases['sequence'].append(([(0,1,0),(1,1,3),(2,1,2),(3,0,1),(4,3,0),(5,2,3)],0,side,success,serial))
 for gap,end in itertools.product((0,500),(0,0xFFFFFFF0,0x100000004,0xFFFFFFFFFFFFFFF0)):
  for delta in (-1,0,1):
   now=(gap+end+delta)&0xffffffffffffffff
   for flags in itertools.product((0,1),repeat=6):cases['deadline'].append((gap,end,now,*flags))
 differences=[]
 for method,inputs in cases.items():
  for case in inputs:
   try:expected=getattr(old,method)(*case);actual=getattr(new,method)(*case)
   except Exception:
    print('FAILED',method,case,old.trace,new.trace,flush=True);raise
   if expected!=actual:differences.append(dict(method=method,input=case,original=expected,candidate=actual))
  print(method,len(inputs),'mismatches',len(differences),flush=True)
 report={'exe_sha256':SHA,'dll_sha256':digest(dll),'cases':{k:len(v) for k,v in cases.items()},'total':sum(map(len,cases.values())),'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
 (a.output/'vox-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
 if differences:print(json.dumps(differences[:1],indent=2))
 return bool(differences)
if __name__=='__main__':raise SystemExit(main())
