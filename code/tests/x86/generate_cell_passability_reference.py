#!/usr/bin/env python3
"""Run original Cell.RecalcPassability 0x483C80: terrain/overlay/occupier precedence.
The sole controlled query is occupier RTTI; no passability algorithm is replaced.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def generate(exe,output,report):
 assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
 pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
 cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image());cpu.mem_map(0x1000000,0x400000)
 cell,overlay,body,typ,house,scenario,vt,stub,stack,stop,table=[0x1000000+x for x in (0,0x1000,0x2000,0x4000,0x7000,0x9000,0x10000,0x11000,0x80000,0x90000,0x100000)]
 def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
 put(0x87F924,table);put(table+4*(8+512*6),cell);put(0xA83D84,vt+0x100);put(vt+0x100,overlay);put(0xA8B230,scenario)
 cpu.mem_write(0x87F7E8+0xEC,struct.pack('<8i',0,0,8,12,0,0,8,12));cpu.mem_write(cell+0x24,struct.pack('<2h',8,6))
 put(body,vt);put(vt+0x2C,stub);put(body+0xC8,typ);put(body+0x520,typ);put(body+0x21C,house)
 state={}
 def boundary(uc,a,s,u):
  sp=uc.reg_read(UC_X86_REG_ESP);uc.reg_write(UC_X86_REG_EAX,state['rtti']);uc.reg_write(UC_X86_REG_EIP,struct.unpack('<I',uc.mem_read(sp,4))[0]);uc.reg_write(UC_X86_REG_ESP,sp+4)
 cpu.hook_add(UC_HOOK_CODE,boundary,begin=stub,end=stub)
 lines=[]
 for land,flags,cost,occupier,theater in itertools.product(range(12),range(8),range(4),range(7),range(2)):
  put(cell+0x44,0 if flags else -1);put(cell+0xEC,land);put(cell+0xE4,body if occupier else 0);put(cell+0x4C,99)
  cpu.mem_write(overlay+0x22D,bytes([flags==1]));cpu.mem_write(overlay+0x2A8,bytes([flags==2]));put(overlay+0x298,0)
  cpu.mem_write(overlay+0x2B4,bytes([flags==4,flags==3]));put(scenario+0x1258,theater)
  for i in range(12):cpu.mem_write(0x89EA48+36*i,struct.pack('<f',(1.0,0.0,0.01,0.0101)[cost]))
  cpu.mem_write(typ+0x16BF,bytes([occupier==3,occupier in (1,2)]));cpu.mem_write(house+0x1FA,bytes([occupier==2]));put(body+0x618,0)
  put(typ+0x2A8,7 if occupier==5 else 3);put(typ+0x2AC,7 if occupier==6 else 3);state['rtti']=6 if occupier<4 else 36
  cpu.mem_write(stack,struct.pack('<I',stop));cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_ECX,cell);cpu.emu_start(0x483C80,stop,count=10000)
  assert cpu.reg_read(UC_X86_REG_EIP)==stop
  result=struct.unpack('<I',cpu.mem_read(cell+0x4C,4))[0];lines.append(f'{land} {flags} {cost} {occupier} {theater} {result}')
 output.write_text('\n'.join(lines)+'\n');report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(lines),'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n');print(len(lines),'cell passability cases')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('exe','output','report'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();generate(a.exe,a.output,a.report)
