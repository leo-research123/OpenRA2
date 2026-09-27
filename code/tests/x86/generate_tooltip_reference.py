#!/usr/bin/env python3
"""Execute fixed YR CCToolTip layout/draw and BitText line breaking.
Only mouse/font glyph lookup and final surface/glyph sinks are doubled. No real
map data is required. Native tests consume the resulting rectangles/pixel hashes.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import pefile
from compare_image_resources import Resources,SHA
from unicorn.x86_const import UC_X86_REG_ECX
TEXTS=['A','AAAA AAAA','A\r\nAA','A\tAA','A'*110,'中 A 中','AA AA AA AA AA AA AA AA AA AA AA AA']
def generate(exe,fixture,report):
 assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
 m=Resources(pefile.PE(str(exe)),None,{},False);b=0x54000000;m.uc.mem_map(b,0x40000)
 obj,data,region,font,mouse,mvt,surf,svt,side,glyph,space=[b+x for x in (0,0x1000,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000,0x8000,0x9000,0x9008)]
 m.write32(0x887640,mouse);m.write32(mouse,mvt);m.write32(mvt+0x28,b+0xA000)
 m.uc.mem_write(b+0xA000,b'\xc3');state={'visible':True,'pixels':[],'left':False}
 m.hooks[b+0xA000]=lambda:m.ret(0 if state['visible'] else 0xFFFFFFFF)
 m.write32(0x89C4D0,font);m.write32(font+4,font+0x80);m.uc.mem_write(font+0x80,struct.pack("<9i",5,1,7,9,2,8,b+0x10000,glyph,2));m.write32(font+0x1c,9);m.write32(font+0x28,16);m.write32(font+0x2c,1)
 m.uc.mem_write(glyph,b'\5'+b'\xf8'*7);m.uc.mem_write(space,b'\3'+bytes(7));m.uc.mem_write(b+0x10000,b'\1\0'*0x10000);m.uc.mem_write(b+0x10000+32*2,b'\2\0')
 m.hooks[0x4346C0]=lambda:m.ret(space if (m.arg(0)&0xffff)==32 else glyph,4)
 for pointer in (surf,side):m.write32(pointer,svt)
 m.write32(0x88731c,surf);m.write32(0x887300,side)
 for address,val in ((0x8a0dd0,11),(0x8a0dd4,3),(0x8a0de0,5),(0x8a0de4,2),(0x8a0dd8,0),(0x8a0ddc,3)):m.write32(address,val)
 m.uc.mem_write(0xB0FA1C,bytes((255,255,255)))
 def signed(v):return struct.unpack('<i',struct.pack('<I',v))[0]
 def origin(surface):return 168 if state['left'] and surface==surf else 632 if not state['left'] and surface==side else 0
 def paint(x,y,w,h,color):
  for yy in range(max(0,y),min(480,y+h)):
   for xx in range(max(0,x),min(800,x+w)):state['pixels'][yy*800+xx]=color&0xffff
 def fill(border=False):
  x,y,w,h=struct.unpack('<4i',m.uc.mem_read(m.arg(0),16));x+=origin(m.uc.reg_read(UC_X86_REG_ECX));c=m.arg(1)
  if border:
   paint(x,y,w,1,c);paint(x,y+h-1,w,1,c);paint(x,y,1,h,c);paint(x+w-1,y,1,h,c)
  else:paint(x,y,w,h,c)
  m.ret(1,8)
 def hook(slot,callback):
  p=b+0xA100+slot;m.write32(svt+slot,p);m.uc.mem_write(p,b'\xc3');m.hooks[p]=callback
 hook(0x14,lambda:fill());hook(0x58,lambda:fill(True))
 hook(0x7c,lambda:m.ret(632 if m.uc.reg_read(UC_X86_REG_ECX)==surf else 168))
 def lock():state['surface']=m.arg(0);m.ret(1,4)
 m.hooks[0x4348F0]=lock;m.hooks[0x434990]=lambda:m.ret(0,4)
 def draw_glyph():
  char,x,y,color=[m.arg(i) for i in range(4)];char&=0xffff;x=signed(x);y=signed(y)
  if char==9:
   start=m.read32(font+0x20);next_x=16+x-(16+x-start)%16
  else:
   w=3 if char==32 else 5;next_x=x+w+1
   if char!=32:
    l,t,r,bot=struct.unpack('<4i',m.uc.mem_read(font+0x30,16))
    px,py=max(x,l),max(y,t);right,bottom=min(x+w,r+1),min(y+7,bot+1)
    paint(px+origin(state['surface']),py,right-px,bottom-py,color)
  m.ret(next_x,16)
 m.hooks[0x434120]=draw_glyph
 rows=[];count=0
 for left in (0,1):
  state['left']=left
  tactical=(168 if left else 0,0,632,448);sidebar=(0 if left else 632,0,168,480)
  m.uc.mem_write(0x886fa0,struct.pack('<4i',*tactical));m.uc.mem_write(0x886f90,struct.pack('<4i',*sidebar));m.uc.mem_write(0xa8eb7c,bytes([1-left]))
  for text_id,text in enumerate(TEXTS):
   for x,y in ((220,120),(610,430),(799,479),(632,10),(40,20)):
    for anchored in (0,1):
     for visible in (0,1):
      state['visible']=visible;m.uc.mem_write(0x884B8E,b'\0')
      m.uc.mem_write(data,struct.pack('<4i',x,y,0,0)+text.encode('utf-16le')+b'\0\0')
      m.uc.mem_write(region,struct.pack('<I4iI',500,x,y,50,30,0)+bytes([anchored]));m.write32(obj+4,region)
      ok=m.call(0x478ba0,obj,args=(data,),kind='bool')
      dims=struct.unpack('<4i',m.uc.mem_read(data,16));redraw=m.uc.mem_read(0x884B8E,1)[0]
      rows.append('L '+' '.join(map(str,[left,text_id,x,y,anchored,visible,ok,*dims,redraw])))
      if not ok:continue
      state['pixels']=[0x1234]*(800*480)
      # Unified native composition executes exactly one matching surface pass.
      m.uc.mem_write(obj+0x260,b'\1')
      m.call(0x478e30,obj,args=(data,))
      h=1469598103934665603
      for pixel in state['pixels']:h=((h^pixel)*1099511628211)&0xffffffffffffffff
      rows.append('D '+' '.join(map(str,[left,text_id,*dims,h])))
      count+=1
 fixture.write_text('\n'.join(rows)+'\n')
 report.write_text(json.dumps(dict(scope=__doc__,exe_sha256=SHA,layout_cases=len(rows)-count,draw_cases=count,fixtures_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest()),indent=2)+'\n')
 print(len(rows)-count,'layouts;',count,'draws')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();generate(a.exe,a.fixture,a.report)
