#!/usr/bin/env python3
"""Original message storage/lifecycle and BitFont measurement comparison.

Production MessageList, TextLabel construction, Gadget/Link and font methods
execute. Original and candidate use original-layout synthetic font data. Glyph
lookup is a shared synthetic-width boundary; GetTextDimension uses the EXE
implementation (native measurement was calibrated separately). The
allocator uses deterministic shared storage; system ticks, session mode, and
sound submission are observed boundaries. Label vptr identities are normalized;
no TextLabel drawing, editing/IME or full production message integration claim.
Valid bounded strings, initialized lists, positive font tabs and progressing
line widths are compared; allocation and exception failures are not emulated.
"""
import argparse,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,SHA,digest
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/MessageListClass.cpp','code/core/src/yrpp/TextLabelClass.cpp','code/core/src/yrpp/GadgetClass.cpp','code/core/src/yrpp/LinkClass.cpp','code/core/src/yrpp/BitFontMeasurement.cpp','code/tests/x86/messages_probe.cpp']
def build(db,out):
    return build_probe(db, out, SOURCES, 'messages_probe.dll',
        object_suffix='-messages.obj',
        flags=['/GS-', '/GR-', '/Gy'],
        symbols=True)
class Machine(BaseMachine):
 def hook(self,c,pc,size,data):
  sp=c.reg_read(UC_X86_REG_ESP)
  if pc==0x7C8E17:
   size=self.read(sp+4)[0];p=BASE+0x10000+len(self.allocations)*0x100
   self.allocations.append((p,size));self.trace.append(['allocate',size]);c.mem_write(p,b'\xA5'*size);self.ret(p)
  elif pc==0x7C8B3D:self.trace.append(['free',self.read(sp+4)[0]]);self.ret()
  elif pc==0x6C8C40:self.ret(self.tick)
  elif pc==0x750920:
   self.trace.append(['sound',c.reg_read(UC_X86_REG_ECX),c.reg_read(UC_X86_REG_EDX),*self.read(sp+4,2)]);self.ret(pop=8)
 def setup(self,seed=0xA5):
  self.reset('messages');self.allocations=[];self.tick=200
  self.put(0x887338,100,0xA5A5A5A5,1000);self.put(0x8B3E88,0,0,0,0)
  self.put(0x8871E0,BASE+0xA0000);self.put(BASE+0xA06AC,74)
  self.put(0xA8B238,0);self.byte(0xA8D1F8,1);self.byte(0xA8D1F9,0)
  self.list=BASE+0x2000;self.c.mem_write(self.list-4,bytes((i*17+seed)%256 for i in range(0x14A4)))
  self.font=BASE+0x8000;self.put(self.font+4,BASE+0x8100,BASE+0x9000)
  self.put(self.font+0x1C,19,0,0,32,1)
  self.put(BASE+0x8100,4,1,10,19,1,11,BASE+0x100000,BASE+0x9000,1)
  self.c.mem_write(BASE+0x100000,b'\1\0'*65536);self.c.mem_write(BASE+0x9000,b'\4'+b'\xF0'*10)
  self.put(0x89C4D0,self.font)
  if self.candidate:self.call('BindFont',0,self.font,[])
 def state(self):
  labels=[]
  for p,n in self.allocations:
   raw=bytearray(self.c.mem_read(p,n));raw[:4]=bytes(4);labels.append(raw.hex())
  return {'list':bytes(self.c.mem_read(self.list-4,0x14A4)).hex(),'labels':labels,'trace':self.trace,'focused':self.read(0x8B3E90)[0]}
 def construct(self,seed):
  self.setup(seed);self.call('MessageConstruct',0x5D39D0,self.list,[]);return self.state()
 def init(self,count,chars,edit,overflow,width):
  self.setup();self.call('MessageConstruct',0x5D39D0,self.list,[])
  self.call('MessageInit',0x5D3A60,self.list,[11,27,count,chars,37,*edit,overflow,20,98,width]);return self.state()
 def text(self,p,s):self.c.mem_write(p,(s+'\0').encode('utf-16-le'))
 def add(self,count,width,name,text,mode,silent,timeout,flags):
  self.setup();self.call('MessageConstruct',0x5D39D0,self.list,[])
  self.call('MessageInit',0x5D3A60,self.list,[11,27,count,98,37,-1,-1,0,20,98,width])
  self.put(0xA8B238,mode);self.byte(0xA8D1F8,flags&1);self.byte(0xA8D1F9,flags>>1)
  self.text(BASE+0x500,name);self.text(BASE+0x800,text)
  self.call('MessageAdd',0x5D3BA0,self.list,[BASE+0x500 if name else 0,33,BASE+0x800,3,0x4046,timeout,silent])
  result=self.c.reg_read(UC_X86_REG_EAX)
  state=self.state();state['result']=result;return state
 def sequence(self,count,timeout,when,edit):
  self.setup();self.call('MessageConstruct',0x5D39D0,self.list,[])
  self.call('MessageInit',0x5D3A60,self.list,[11,27,count,98,37,-1,-1,0,20,98,400])
  for i in range(5):
   self.text(BASE+0x800,'message '+str(i));self.call('MessageAdd',0x5D3BA0,self.list,[0,i%3,BASE+0x800,3,0x4046,timeout+i if timeout!=-1 else -1,1])
  # Edit state without an edit widget is valid for count/position only.
  self.byte(self.list+0x19,edit);self.byte(self.list+0x1A,edit)
  values=[]
  for id in (-1,0,1,2,3):
   for name,pc in [('MessageGet',0x5D3F60),('MessageGetLabel',0x5D3F90)]:
    self.call(name,pc,self.list,[id]);values.append(self.c.reg_read(UC_X86_REG_EAX))
  self.call('MessageCount',0x5D4AA0,self.list,[]);values.append(self.c.reg_read(UC_X86_REG_EAX))
  self.call('MessageY',0x5D4BF0,self.list,[])
  self.byte(self.list+0x19,0);self.call('MessageWidth',0x5D4AD0,self.list,[281])
  self.tick=when;self.call('MessageManage',0x5D4430,self.list,[]);values.append(self.c.reg_read(UC_X86_REG_EAX))
  state=self.state();state['results']=values
  self.call('MessageDestroy',0x5D3A40,self.list,[]);state['after_destroy']=self.state();return state
 def fit(self,text,width,limit,word,spacing,tab):
  self.setup();self.text(BASE+0x500,text);self.put(self.font+0x28,tab,spacing)
  self.call('FontFit',0x433F50,self.font,[BASE+0x500,width,limit,word]);return self.c.reg_read(UC_X86_REG_EAX)
 def trim(self,text,minimum,maximum,direction,dest):
  self.setup();self.text(BASE+0x500,text);self.c.mem_write(BASE+0x800,b'\xA5'*200)
  self.call('MessageTrim',0x5D4B20,self.list,[BASE+0x800 if dest else 0,BASE+0x500,minimum,maximum,direction])
  return {'result':self.c.reg_read(UC_X86_REG_EAX),'source':bytes(self.c.mem_read(BASE+0x500,100)).hex(),'dest':bytes(self.c.mem_read(BASE+0x800,100)).hex()}
def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,symbols=build(a.compile_db,a.output)
 exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
 cases={'construct':[(0,),(0xA5,),(0xFF,)],
 'init':list(itertools.product((-1,0,6,14,20),(-1,0,98,112,200),((-1,4),(5,-1),(5,6)),(0,1),(0,640))),
 'add':list(itertools.product((0,1,6),(100,640),('', 'Name'),('Hello','First word second word\nthird\tline','A'*120),(0,3,4,5),(0,1),(-1,0,225),(0,3))),
 'sequence':list(itertools.product((0,1,3,14),(-1,0,3),(199,200,201,203,204,205),(0,1))),
 'fit':list(itertools.product(('', 'abc', 'word wrap test','first\r\nsecond','a\tb','中文 宽度'),(0,1,4,5,9,10,15,100),(0,1,5),(0,1),(-1,0,1),(8,32))),
 'trim':list(itertools.product(('abcdefghi','abc defgh','a\tb\rc\nd e'),(0,1,3,5),(5,7,12),(0,1),(0,1)))}
 differences=[]
 for method,inputs in cases.items():
  for case in inputs:
   try:expected=getattr(old,method)(*case);actual=getattr(new,method)(*case)
   except Exception as e:print('FAILED',method,case,'old',hex(old.c.reg_read(UC_X86_REG_EIP)),'new',hex(new.c.reg_read(UC_X86_REG_EIP)));raise
   if expected!=actual:differences.append({'method':method,'input':case,'original':expected,'candidate':actual})
 report={'exe_sha256':SHA,'cases':{k:len(v) for k,v in cases.items()},'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
 (a.output/'messages-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='differences'},indent=2))
 if differences:print(json.dumps(differences[:1],indent=2))
 return bool(differences)
if __name__=='__main__':raise SystemExit(main())
