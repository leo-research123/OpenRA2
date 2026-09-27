#!/usr/bin/env python3
"""Replay actual native group-attack states through original Foot.ApproachTarget.

Original coordinates, range/ray checks, occupation filtering and candidate loop
execute. Recorded map heights/zones, type/weapon/COM identity, SetDestination and
native AStar replies are boundaries. Every AStar query's endpoints and bridge
flags must match before returning its recorded result. This verifies selection
on the SAME input; it does not verify the producers of those inputs or a whole
original frame. A uniform (-64,-80) cell translation fits the reference arena.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from generate_building_attack_reference import Reference, SHA
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
class Replay(Reference):
 def __init__(self,exe):
  self.grid={};self.paths=[];self.used=[]
  super().__init__(exe)
  self.cpu.mem_map(0x1400000,0x100000)
 def level(self,x,y):return self.grid[x,y][1]
 def boundary(self,cpu,address,size,data):
  sp=cpu.reg_read(UC_X86_REG_ESP)
  if address==0x56D230:
   x,y=struct.unpack('<hh',cpu.mem_read(self.get(sp+4),4));bridge=self.get(sp+12)&255
   result=self.grid[x,y][7 if bridge else 6];pop=12
  elif address==0x578460:result=1;pop=8
  elif address==0x42D170:
   x,y=struct.unpack('<hh',cpu.mem_read(self.get(sp+4),4));a,b=struct.unpack('<hh',cpu.mem_read(self.get(sp+8),4))
   query=[x+64,y+80,a+64,b+80,self.get(sp+16)&255,self.get(sp+20)&255]
   assert self.paths,(query,'unexpected query')
   expected=self.paths.pop(0);assert query==expected[:6],(query,expected)
   self.used.append(expected);result=expected[-1];pop=24
  else:return super().boundary(cpu,address,size,data)
  cpu.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF);cpu.reg_write(UC_X86_REG_EIP,self.get(sp));cpu.reg_write(UC_X86_REG_ESP,sp+4+pop)

def replay(exe,snapshots,report):
 r=Replay(exe);document=json.loads(snapshots.read_text());cells={tuple(c[:2]):c for c in document['base_cells']}
 for idx,wall,crush in document['overlays']:
  address=0x1400000+idx*0x400;r.put(0x1380000+4*idx,address);r.cpu.mem_write(address+0x2A8,bytes([wall,crush]))
 results=[]
 for case in document['cases']:
  frame,id,x,y,z,hx,hy,hz,dx,dy,tx,ty,tz,foundation,speed,zone,bridge=case['input']
  r.grid={};r.used=[];r.paths=[p[:] for p in case['path_queries']]
  cells.update((tuple(c[:2]),c) for c in case['cell_changes'])
  for cx,cy,land,level,flags,occ,alt,overlay,groundzone,bridgezone,cost in cells.values():
   cx-=64;cy-=80;r.grid[cx,cy]=[land,level,flags,occ,alt,overlay,groundzone,bridgezone,cost]
   cell=r.tile(cx,cy);r.put(cell+0xEC,land);r.cpu.mem_write(cell+0x11B,bytes([level]));r.put(cell+0x140,flags)
   r.put(cell+0x124,occ);r.put(cell+0x128,alt);r.put(cell+0x44,overlay)
   r.cpu.mem_write(0x89EA40+36*land+4*speed,struct.pack('<f',cost))
  r.coords(r.obj+0x9C,(x-64*256,y-80*256,z));r.coords(r.driver+36,(hx-64*256,hy-80*256,hz))
  r.coords(r.target+0x9C,(tx-64*256,ty-80*256,tz));r.put(r.btype+0xEF0,foundation)
  r.put(r.typ+0x67C,speed);r.put(r.typ+0x5B4,zone);r.cpu.mem_write(r.obj+0x8C,bytes([bridge]))
  r.put(r.rules+0xDF8,1);r.put(r.weapon+0xB4,1024);r.put(r.weapon+0xB8,0);r.cpu.mem_write(r.bullet+0x296,b'\1\1\1')
  r.put(r.obj+0x2B4,r.target);r.put(r.obj+0x5A4,0 if dx==-1 else r.tile(dx-64,dy-80))
  try:r.call(0x4D5690,(0,))
  except Exception as e:raise RuntimeError(f'case {frame} {id}') from e
  assigned=r.get(r.obj+0x5A4);dest=list(struct.unpack('<hh',r.cpu.mem_read(assigned+0x24,4))) if assigned else [-65,-81]
  dest=[dest[0]+64,dest[1]+80]
  assert dest==case['native_destination'] and not r.paths,(frame,id,dest,case['native_destination'],r.paths)
  results.append({'frame':frame,'id':id,'destination':dest,'path_queries':len(r.used)})
 report.write_text(json.dumps({'exe_sha256':SHA,'scope':__doc__,'snapshots_sha256':hashlib.sha256(snapshots.read_bytes()).hexdigest(),'cases':len(results),'results':results},indent=2)+'\n')
 print(len(results),'actual-scene approach checkpoints match original decisions')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('exe','snapshots','report'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();replay(a.exe,a.snapshots,a.report)
