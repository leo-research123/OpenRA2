#!/usr/bin/env python3
"""Execute Infantry/Foot/Techno/Radio/Object expiry; map and SetTarget/FindIndex are fixture boundaries."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX

SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    cpu.mem_map(0x1000000,0x40000)
    obj,target,other,typ,vt,cell,house,enemy,stub,buffer,stack,stop=[0x1000000+n for n in (0,0x2000,0x4000,0x6000,0x8000,0x9000,0xA000,0xB000,0xC000,0x10000,0x30000,0x3F000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<I',cpu.mem_read(a,4))[0]
    def byte(a,v):cpu.mem_write(a,bytes([int(v)]))
    def token(value):return {0:0,target:1,other:2,cell:3}.get(value,-99)
    assignments=[0]
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);pop=4
        if address in (0x565730,0x5657A0):uc.reg_write(UC_X86_REG_EAX,cell)
        elif address==stub:
            put(this+0x2B4,get(sp+4));assignments[0]+=1
        elif address==stub+0x20:
            sought=get(get(sp+4));found=-1
            for index in range(get(this+0x10)):
                if get(get(this+4)+4*index)==sought:found=index;break
            uc.reg_write(UC_X86_REG_EAX,found&0xFFFFFFFF)
        else:return
        uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for address in (0x565730,0x5657A0,stub,stub+0x20):cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    put(vt+0x28,0x51AA10);put(vt+0x2C,0x523340);put(vt+0x3C,0x6F9DC0);put(vt+0x48,0x5F65A0)
    put(vt+0xC4,stub+0x40);cpu.mem_write(stub+0x40,b'\x31\xC0\xC3')
    put(vt+0x184,0x5B3040);put(vt+0x3C8,stub);put(vt+0x1FC,0x5B3A10);put(vt+0x1F8,0x5B36B0)
    put(stub+0x110,stub+0x20);put(0xA8ED84,500);put(0xA8E7AC,0)
    rows=[]
    fields=(0x2B4,0x2B8,0x218,0x500,0x5A0,0x5A4,0x5A8,0x5C8,0x5CC,0x5D8)
    for removed in (0,1):
      for sensed in (0,1):
       for friendly in (0,1):
        for capture in (0,1):
         for occupier in (0,1):
          for alive in (0,1):
           for health in (0,100):
            for selling in (0,1):
             cpu.mem_write(obj,bytes(0x800));cpu.mem_write(target,bytes(0x800));cpu.mem_write(other,bytes(0x800))
             for value in (obj,target,other):put(value,vt);put(value+0x14,7);put(value+0x6C0,typ)
             put(obj+0x21C,house);put(target+0x21C,house if friendly else enemy)
             byte(target+0x90,alive);put(target+0x6C,health);put(target+0xAC,19 if selling else 5)
             put(obj+0xAC,8 if capture else 2);put(obj+0xB0,-1);byte(typ+0xEB4,occupier)
             put(obj+0x180,0);put(obj+0x188,0);put(house+0x30,0);cpu.mem_write(cell+0x7C,struct.pack('<H',sensed))
             put(target+0x9C,8*256+128);put(target+0xA0,6*256+128);put(target+0x5D8,other)
             for off in fields:put(obj+off,target)
             def vector(off,values):
                 storage=buffer+off*4;put(obj+off,stub+0x100);put(obj+off+4,storage);put(obj+off+8,len(values));put(obj+off+0x10,len(values))
                 for i,v in enumerate(values):put(storage+4*i,v)
             for off in (0x588,0x5AC,0x458,0x470):vector(off,[other,target,target,other])
             vector(0x440,[10,20,30,40]);assignments[0]=0
             cpu.mem_write(stack,struct.pack('<III',stop,target,removed));cpu.reg_write(UC_X86_REG_ECX,obj);cpu.reg_write(UC_X86_REG_ESP,stack)
             cpu.emu_start(0x51AA10,stop,count=100000)
             assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+12
             state=[token(get(obj+off)) for off in fields]+[assignments[0]]
             for off in (0x588,0x5AC,0x458,0x470,0x440):
                 count=get(obj+off+0x10);state.append(count)
                 for i in range(4):
                     value=get(get(obj+off+4)+4*i) if i<count else 0
                     state.append(value if off==0x440 else token(value))
             rows.append(' '.join(map(str,[removed,sensed,friendly,capture,occupier,alive,health,selling,*state])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Infantry expiration cases')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
