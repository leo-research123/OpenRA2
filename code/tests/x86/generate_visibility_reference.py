#!/usr/bin/env python3
"""Original Tactical.GetOcclusion for every neighbor mask, plus live CellSpread.

Map.GetCellAt resolves a nine-cell fixture; all classification/table instructions
execute in the pinned EXE. The spread table is read after its real CRT initializer.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX

SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));u=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    u.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);u.mem_write(base,pe.get_memory_mapped_image());u.mem_map(0x1000000,0x10000)
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    def tile(x,y):return 0x1000000+((y-5)*3+(x-7))*0x148
    def lookup(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);x,y=struct.unpack('<hh',uc.mem_read(get(sp+4),4))
        assert 7<=x<=9 and 5<=y<=7
        uc.reg_write(UC_X86_REG_EAX,tile(x,y));uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+8)
    u.hook_add(UC_HOOK_CODE,lookup,begin=0x5657A0,end=0x5657A0)
    stack,stop,arg=0x100D000,0x100F000,0x100C000;put(arg,8+(6<<16))
    rows=[]
    for fog in range(2):
      for center in range(4):
       for mask in range(256):
        for y in range(5,8):
         for x in range(7,10):put(tile(x,y)+0x12C,0);put(tile(x,y)+0x140,0)
        field,bit=(0x140,2) if fog else (0x12C,8)
        put(tile(8,6)+field,center if fog else ((center&1)*8+(center//2)*16))
        for i,(dx,dy) in enumerate(((1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1),(0,-1))):
            put(tile(8+dx,6+dy)+field,0 if mask&(1<<i) else bit)
        put(stack,stop);put(stack+4,arg);put(stack+8,fog);u.reg_write(UC_X86_REG_ESP,stack)
        u.emu_start(0x6D8700,stop,count=10000);assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==stack+12
        value=u.reg_read(UC_X86_REG_EAX)&255;rows.append(f'0 {fog} {center} {mask} {value if value<128 else value-256}')
    put(stack,stop);u.reg_write(UC_X86_REG_ESP,stack);u.emu_start(0x561910,stop,count=10000)
    assert u.reg_read(UC_X86_REG_EIP)==stop
    for i in range(12):rows.append(f'1 {i} {get(0x7ED3D0+4*i)}')
    for i in range(369):
        x,y=struct.unpack('<hh',u.mem_read(0xABD490+4*i,4));rows.append(f'2 {i} {x} {y}')
    output.write_text('\n'.join(rows)+'\n');report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original visibility/table cases')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
