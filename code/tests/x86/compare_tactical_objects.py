#!/usr/bin/env python3
"""Execute YR DrawObjects and the compiled production method with identical object virtuals.
Checks ordered virtual calls, hook coordinates/clips, IsVisible, stack and saved registers.
Fog and object rendering are boundary doubles; projection and culling execute real code.
"""
import argparse, hashlib, itertools, json, random, struct
from pathlib import Path
import pefile
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_FPCW
from compare_image_resources import Resources, SHA
BASE=0x53000000
TACTICAL=BASE;SCENARIO=BASE+0x2000;VT=BASE+0x3000;LISTS=BASE+0x4000;BUILDINGS=BASE+0x5000
OBJECTS=BASE+0x6000;TYPES=BASE+0x40000
class Machine(Resources):
 def __init__(self,exe,dll=None):
  super().__init__(exe,dll,{},False);self.uc.mem_map(BASE,0x80000);self.events=[];self.candidate=dll is not None
  self.exports={n.lstrip('@_').split('@')[0]:v for n,v in self.exports.items()}
  self.uc.hook_add(UC_HOOK_CODE,self.guard)
  for slot in (0x2C,0x48,0x4C,0xAC,0x104,0x10C,0x110):
   address=VT+0x800+slot;self.uc.mem_write(address,b'\xC3');self.write32(VT+slot,address)
   self.hooks[address]=lambda slot=slot:self.virtual(slot)
  self.hooks[0x43E7B0]=lambda:self.draw('info',8)
  self.hooks[0x5865E0]=self.fog
  self.write32(0x887324,TACTICAL);self.write32(0xA8B230,SCENARIO)
  self.uc.mem_write(0xB0CD48,struct.pack('<d',0.4330127018922193))
 def guard(self,uc,address,size,data):
  if self.candidate:assert not 0x6D8DB0<=address<0x6D97C9,('original DrawObjects entered',hex(address))
 def ints(self,p,n):return list(struct.unpack('<'+'i'*n,self.uc.mem_read(p,4*n)))
 def put(self,p,*v):self.uc.mem_write(p,struct.pack('<'+'i'*len(v),*v))
 def byte(self,p,v):self.uc.mem_write(p,bytes([v]))
 def object(self):return (self.uc.reg_read(UC_X86_REG_ECX)-OBJECTS)//0x2000
 def virtual(self,slot):
  i=self.object();o=self.objects[i];self.events.append([i,hex(slot)])
  if slot==0x2C:self.ret(o['kind'])
  elif slot in (0x48,0x4C,0xAC):
   p=self.arg(0);self.put(p,*o.get('center',o['at']));self.ret(p,8 if slot==0x4C else 4)
  elif slot==0x104:
   self.events[-1]+=[self.ints(self.arg(0),4),self.arg(1),self.arg(2)]
   self.ret(1,12)
  else:self.draw(hex(slot),8,False)
 def draw(self,kind,cleanup,append=True):
  record=[self.object(),kind,self.ints(self.arg(0),2),self.ints(self.arg(1),4)]
  if append:self.events.append(record)
  else:self.events[-1]=record
  self.ret(0,cleanup)
 def fog(self):
  at=self.ints(self.arg(0),3);self.events.append(['fog',at]);self.ret(self.config['fog'],4)
 def run(self,c):
  self.config=c;self.events=[];self.objects=c['objects'];self.uc.reg_write(UC_X86_REG_FPCW,0xE7F)
  self.put(TACTICAL+0xB0,*c['camera'])
  self.uc.mem_write(TACTICAL+0xDE4,struct.pack('<12f',4.266666889,8.533333778,0,0,-4.266666889,8.533333778,0,0,0,0,1,0))
  self.put(0x886FA0,*c['clip']);self.put(0xB0CE28,*c['view'])
  self.write32(SCENARIO,c['memory']<<12);self.write32(0xB73550,c['window']);self.byte(0xA8ED6B,c['debug'])
  layers=[[] for _ in range(5)];buildings=[]
  for i,o in enumerate(self.objects):
   address=OBJECTS+0x2000*i;typ=TYPES+0x1000*i;self.uc.mem_write(address,bytes(0x2000));self.uc.mem_write(typ,bytes(0x1000))
   self.write32(address,VT);self.put(address+0x14,o['flags']);self.put(address+0x9C,*o['at'])
   self.byte(address+0x99,1);self.byte(address+0x74,o.get('onmap',1));self.write32(address+0xC8,typ)
   if o['kind']==4:self.byte(address+0x118,o.get('attached',0));self.byte(typ+0x374,o.get('fogremove',1))
   if o['kind']==36:self.byte(typ+0x2B3,o.get('animated',1));self.byte(address+0xCD,o.get('crumbling',0))
   layers[o['layer']].append(address)
   if o['kind']==6:buildings.append(address)
  for n,items in enumerate(layers):
   p=LISTS+0x200*n
   if items:self.put(p,*items)
   self.write32(0x8A0364+24*n,p);self.write32(0x8A0370+24*n,len(items))
  if buildings:self.put(BUILDINGS,*buildings)
  self.write32(0xA8EB44,BUILDINGS);self.write32(0xA8EB50,len(buildings))
  self.call(self.exports['DrawObjects'] if self.candidate else 0x6D8DB0,TACTICAL,args=(c['forced'],))
  return dict(events=self.events,visible=[int(self.uc.mem_read(OBJECTS+0x2000*i+0x99,1)[0]) for i in range(len(self.objects))])
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for k in ('exe','dll','report'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();assert hashlib.sha256(a.exe.read_bytes()).hexdigest()==SHA
 exe=pefile.PE(str(a.exe));dll=pefile.PE(str(a.dll))
 if not hasattr(dll,'DIRECTORY_ENTRY_IMPORT'):dll.DIRECTORY_ENTRY_IMPORT=[]
 original,candidate=Machine(exe),Machine(exe,dll)
 default=dict(camera=(-100,0),clip=(0,0,640,480),view=(0,0,640,480),memory=0,fog=0,window=1,debug=0,forced=1)
 objects=[dict(kind=k,flags=f,at=(2048+i*256,2048,0),layer=i%5) for i,(k,f) in enumerate(((15,5),(6,1),(4,0),(36,0),(3,0),(1,5),(73,5),(6,1)))]
 cases=[]
 for memory,fog,window,debug,forced in itertools.product((0,1),repeat=5):cases.append(default|dict(memory=memory,fog=fog,window=window,debug=debug,forced=forced,objects=objects))
 for kind,flags in ((15,5),(6,1),(4,0),(36,0),(3,0)):
  for at in ((2048,2048,0),(-100000,0,0),(2048,2048,20000),(2048,2048,-20000),(2048,0,728)):
   for layer in range(5):cases.append(default|dict(objects=[dict(kind=kind,flags=flags,at=at,layer=layer)]))
 for attached,fogremove,animated,crumbling in itertools.product((0,1),repeat=4):
  cases.append(default|dict(fog=1,objects=[dict(kind=4,flags=0,at=(2048,2048,0),layer=3,attached=attached,fogremove=fogremove),dict(kind=36,flags=0,at=(2048,2048,0),layer=2,animated=animated,crumbling=crumbling)]))
 rng=random.Random(0x6D8DB0)
 for n in range(80):
  os=[]
  for i in range(rng.randrange(1,10)):
   k,f=rng.choice(((15,5),(6,1),(4,0),(36,0),(3,0)))
   os.append(dict(kind=k,flags=f,at=tuple(rng.randrange(-4000,9000) for _ in range(3)),center=tuple(rng.randrange(-4000,9000) for _ in range(3)),layer=rng.randrange(5),onmap=rng.randrange(2)))
  cases.append(default|dict(camera=(rng.randrange(-300,300),rng.randrange(-300,300)),objects=os))
 passed=0;failures=[]
 for n,c in enumerate(cases):
  x,y=original.run(c),candidate.run(c)
  if x!=y:failures.append(dict(case=n,input=c,original=x,candidate=y));break
  passed+=1
 a.report.write_text(json.dumps(dict(scope=__doc__,exe_sha256=SHA,passed=passed,total=len(cases),failures=failures),indent=2)+'\n')
 print(f'{passed}/{len(cases)} DrawObjects instruction cases; {len(failures)} differences')
 if failures:print(json.dumps(failures[0],indent=2)[:6000]);raise SystemExit(1)
if __name__=='__main__':main()
