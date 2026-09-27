#!/usr/bin/env python3
"""Execute the original Infantry -> Techno -> range -> Walk firing-permission chain.

Ordinary GI/M60 against a live ground building; real base checks, stance gates,
speed threshold, rearm/ammo priority and both range overloads execute. Type and
weapon slot getters, normal visibility, map lookup/flat floor remain fixtures.
No animation advancement, projectile creation or full frame loop is claimed.
"""
import argparse,hashlib,itertools,json,math,struct
from pathlib import Path
from generate_building_attack_reference import Reference, SHA
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP

def generate(exe,output,report):
 r=Reference(exe);u=r.cpu
 # Keep observed building/infantry identity entries, while supplying weapon/type slots.
 u.mem_write(r.vt,bytes(u.mem_read(0x7EB058,0x600)))
 for off,entry in [(0x84,r.stub+0x10),(0x88,r.stub+0x10),(0x3F8,r.stub+0x40),(0x48,0x5F65A0),(0x3A8,0x6F77B0)]:r.put(r.vt+off,entry)
 house=0x13A0000;driver=0x1390000;warhead=0x1382000
 r.put(r.obj+0x21C,house);r.put(r.target+0x21C,house);u.mem_write(house+0x1EC,b'\1')
 r.put(r.obj+0x2FC,-1);r.put(r.obj+0x14,7);r.put(r.obj+0xAC,1);r.put(r.obj+0xB4,-1);r.put(r.obj+0x6C4,0);r.put(r.obj+0x6C,125)
 r.put(r.weapon+0xA4,15);r.put(r.weapon+0xB4,1024);r.put(r.weapon+0xAC,warhead)
 u.mem_write(warhead+0xA0,struct.pack('<d',1.0));r.put(r.typ+0xA0,125);r.put(r.btype+0xA0,1000)
 r.put(r.btype+0xEF0,6);r.put(r.target+0x6C,1000);r.put(r.obj+0x2EC,-1);r.put(r.obj+0x2F4,0)
 r.call(0x75AA50);r.call(0x55A680);r.call(0x75AA90,(),driver);r.put(driver+8,r.obj);r.put(driver+12,r.obj);r.put(r.obj+0x674,driver+4)
 r.coords(r.obj+0x9C,(8000,8320,0));u.mem_write(r.bullet+0x296,b'\1\1\1');u.mem_write(r.bullet+0x2A5,b'\1')
 # Display/house visibility fixtures only. Real range, base permission and locomotor fire permission run.
 for table in [r.vt,r.bvt]:
  r.put(table+0x68,r.stub+0xF0)
 r.put(r.bvt+0x84,r.stub+0x100)
 def extra(cpu,addr,size,data):
  sp=cpu.reg_read(UC_X86_REG_ESP);ret=r.get(sp);pop=8 if addr==r.stub+0xF0 else 0
  cpu.reg_write(UC_X86_REG_EAX,0 if pop else r.btype);cpu.reg_write(UC_X86_REG_EIP,ret);cpu.reg_write(UC_X86_REG_ESP,sp+4+pop)
 for a in [r.stub+0xF0,r.stub+0x100]:u.hook_add(UC_HOOK_CODE,extra,begin=a,end=a)
 r.put(0xA8ED84,500)
 rows=[]
 for seq,dest,speed,dist,ammo,rearm,moving in itertools.product(range(-1,42),range(2),range(6),(0,1024,1408,1409,1792),(-1,0,7),range(2),range(2)):
  r.put(r.obj+0x6C4,seq);r.put(r.obj+0x5A4,r.tile(28,33) if dest else 0)
  u.mem_write(r.obj+0x578,struct.pack('<d',(0.0,0.05,0.1,math.nextafter(0.1,1.0),0.5,1.0)[speed]))
  r.coords(r.obj+0x9C,(8576-dist,8576,0));r.put(r.obj+0x2FC,ammo)
  r.put(r.obj+0x2EC,500);r.put(r.obj+0x2F4,15 if rearm else 0);u.mem_write(driver+0x34,bytes([moving]))
  base=r.call(0x6FC0B0,(r.target,0,1));infantry=r.call(0x51C8B0,(r.target,0,1))
  rows.append((seq,dest,speed,dist,ammo,rearm,moving,base,infantry))
 output.write_text(''.join(' '.join(map(str,row))+'\n' for row in rows))
 report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
  'entries':['0x0051C8B0','0x006FC0B0','0x006F7220','0x006F77B0'],
  'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
 print(len(rows),'original integrated firing-permission cases')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('exe','output','report'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();generate(a.exe,a.output,a.report)
