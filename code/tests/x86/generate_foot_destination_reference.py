#!/usr/bin/env python3
"""Original Foot virtual destination -> real Walk head query, including tube exit.

Only object/location, driver fields and one tube entry are supplied. The output
is selected via the Infantry vtable slot 0x4C. No pathfinding is replaced here.
"""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
from generate_building_attack_reference import Reference, SHA

def generate(exe,output,report):
 r=Reference(exe);u=r.cpu;driver=0x1390000;tube=0x1391000;out=0x1392000
 for entry in (0x4D3100,0x75AA50):r.call(entry)
 r.call(0x75AA90,(),driver);r.put(driver+8,r.obj);r.put(driver+12,r.obj);r.put(r.obj+0x674,driver+4)
 r.put(0x8B413C,0x1393000);r.put(0x1393000,tube);u.mem_write(tube+0x28,struct.pack('<hh',17,29))
 entry=r.get(0x7EB058+0x4C);assert entry==0x4DBDF0
 rows=[]
 for tunneling,moving,location,head,docker in itertools.product((0,1),(0,1),((100,200,17),(23066,29120,0),(-129,-511,104)),((0,0,0),(256,512,0),(23488,29120,0),(-192,-256,416),(100,200,17)),(0,1)):
  u.mem_write(r.obj+0x684,bytes([0 if tunneling else 255]));u.mem_write(driver+0x34,bytes([moving]))
  r.coords(r.obj+0x9C,location);r.coords(driver+40,head)
  assert r.call(entry,(out,r.target if docker else 0))==out
  actual=struct.unpack('<3i',u.mem_read(out,12));rows.append((tunneling,moving,*location,*head,docker,*actual))
 output.write_text(''.join(' '.join(map(str,row))+'\n' for row in rows))
 report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'vtable':'0x007EB058','slot':'0x4C','entries':['0x004DBDF0','0x0075AC00'], 'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
 print(len(rows),'original Foot destination cases')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('exe','output','report'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();generate(a.exe,a.output,a.report)
