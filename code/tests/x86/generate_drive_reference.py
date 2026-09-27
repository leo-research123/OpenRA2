#!/usr/bin/env python3
"""Run original YR Drive controls and every normal track's transformed samples.

The original executable supplies track data, Smooth_Turn, rotation, Move_To,
Stop_Moving and movement queries. Map lookup, current speed and UnitType are
explicit boundary fixtures. This is not a complete original game replay.
"""
import argparse, hashlib, json, struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_FPCW
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def generate(exe,output,report):
 assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
 pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
 cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image());cpu.mem_map(0x1000000,0x20000)
 loco,foot,cell,vt,kind,buf,point,face,stack,stop=[0x1000000+n for n in (0,0x1000,0x3000,0x4000,0x6000,0x8000,0x8100,0x8200,0x1D000,0x1F000)]
 def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
 def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
 def xyz(a):return struct.unpack('<iii',cpu.mem_read(a,12))
 def coord(a,v):cpu.mem_write(a,struct.pack('<iii',*v))
 def call(entry,this,*args):
  cpu.mem_write(stack,struct.pack('<'+'I'*(1+len(args)),stop,*[v&0xFFFFFFFF for v in args]));cpu.reg_write(UC_X86_REG_ECX,this);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
  cpu.emu_start(entry,stop,count=100000)
  assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4*(1+len(args))
  return cpu.reg_read(UC_X86_REG_EAX)
 speed=0
 def boundary(uc,address,size,data):
  sp=uc.reg_read(UC_X86_REG_ESP);pop=0
  if address==0x565730:result=cell;pop=4
  elif address==0x1009000:result=speed
  elif address==0x1009010:result=kind
  else:return
  uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF);uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
 for address in (0x565730,0x1009000,0x1009010):cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
 for offset,address in ((0x37C,0x70EFD0),(0x380,0x4DE770),(0x1D4,0x70C5B0),(0x1D8,0x70C5C0),(0x538,0x1009000),(0x84,0x1009010)):put(vt+offset,address)
 call(0x55A680,0);put(0xA8ED84,500);put(0xB45C28,104);put(0x8A07C4,416)
 rows=[]
 for operation in range(3):
  for flags in range(64):
   cpu.mem_write(loco,bytes(0x70));call(0x4AF540,loco)
   cpu.mem_write(foot,bytes(0x900));put(foot,vt);put(loco+8,foot);put(loco+12,foot)
   coord(foot+0x9C,(2176,1664,17));coord(loco+0x34,(2432,1664,7));coord(loco+0x40,(2200,1700,17) if flags&1 else (0,0,0))
   speed=10 if flags&2 else 0;cpu.mem_write(loco+0x50,struct.pack('<d',0.75));put(cell+0x140,0x100 if flags&4 else 0)
   put(foot+0x504,int(bool(flags&8)));cpu.mem_write(foot+0x270,bytes([bool(flags&16),bool(flags&32)]));put(foot+0x6A8,0)
   if operation==0:call(0x4AFD40,0,loco+4,2300,1777,24)
   elif operation==1:call(0x4AFE00,0,loco+4)
   else:call(0x4B0EF0,0,loco+4,0x4000)
   moving=call(0x4AFB80,0,loco+4)&255;now=call(0x4AFC20,0,loco+4)&255
   rows.append(' '.join(map(str,[operation,flags,*xyz(loco+0x34),*xyz(loco+0x40),moving,now,struct.unpack('<d',cpu.mem_read(loco+0x50,8))[0],struct.unpack('<H',cpu.mem_read(foot+0x388,2))[0]])))
 for track in range(72):
  raw=cpu.mem_read(0x7E7B28+track*12,1)[0]
  if not raw:continue
  points=get(0x7E7A28+raw*16);index=0
  while True:
   x,y,direction=struct.unpack('<iii',cpu.mem_read(points+index*12,12))
   put(loco+0x58,track);coord(loco+0x40,(1000,2000,0));cpu.mem_write(point,struct.pack('<ii',x,y));put(face,direction)
   call(0x4B4780,loco,buf,point,face);rx,ry=struct.unpack('<ii',cpu.mem_read(buf,8))
   rows.append(' '.join(map(str,[3,track,index,x,y,direction,rx,ry,get(face)])))
   if index and not x and not y:break
   index+=1;assert index<256
 output.write_text('\n'.join(rows)+'\n')
 report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
 print(len(rows),'original Drive cases')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
 a=p.parse_args();generate(a.exe,a.output,a.report)
