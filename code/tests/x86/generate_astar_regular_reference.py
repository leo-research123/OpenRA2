#!/usr/bin/env python3
"""Execute YR regular AStar, original Infantry passability, costs, corner cutting and path optimization on deterministic diamond maps; only map lookup and RTTI/type getters are fixture boundaries."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_FPCW

SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    arena=0x1000000;cpu.mem_map(arena,0x1000000)
    finder,foot,typ,house,vt,stub,stack,stop,coords,path=[arena+n for n in (0,0x2000,0x3000,0x5000,0x20000,0x21000,0x2F000,0x30000,0x31000,0x32000)]
    slots,cells,nodes,opens,queue,queue_items,visits,alt_visits,costs,alt_costs,subzones,markers=[arena+n for n in (0x100000,0x200000,0x300000,0x490000,0x5A0000,0x5A1000,0x600000,0x601000,0x602000,0x603000,0x604000,0x606000)]
    invalid=arena+0x607000
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<I',cpu.mem_read(a,4))[0]
    def getint(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def at(x,y):return cells+(y*21+x)*0x180
    def valid(x,y):return 0<=x<21 and 0<=y<21 and x+y>8 and x-y<8 and y-x<8 and x+y<=32
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX)
        if address==0x5657A0:
            x,y=struct.unpack('<hh',cpu.mem_read(get(sp+4),4));value=at(x,y) if valid(x,y) else invalid;pop=4
            if value==invalid:cpu.mem_write(invalid+0x24,struct.pack('<hh',x,y))
        else:return
        uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    cpu.hook_add(UC_HOOK_CODE,boundary,begin=0x5657A0,end=0x5657A0)
    cpu.mem_write(stub,b'\xB8'+struct.pack('<I',typ)+b'\xC3');cpu.mem_write(stub+0x10,b'\xB8\x0F\0\0\0\xC3')
    for offset,entry in {0x2C:stub+0x10,0x84:stub,0x1AC:0x51BF90,0x1B0:0x4D9C60,0x184:0x5B3040,0x3F8:0x70E140}.items():put(vt+offset,entry)
    put(foot,vt);put(foot+0x6C0,typ);put(foot+0x21C,house);put(foot+0xB4,-1);put(foot+0xAC,2)
    put(house+0x30,0);put(house+0x5788,1);put(typ+0x67C,0)
    put(0x87F924,slots);put(0x87F7E8+0x6C,441);put(0x87F7E8+0x70,subzones);put(0x89C2DC,21)
    put(0x87F7E8+0xF4,8);put(0x87F7E8+0xF8,12)
    cpu.mem_write(0x89EA40,struct.pack('<f',1));cpu.mem_write(0x89EA40+36*2,struct.pack('<f',0));put(0xA8E7AC,0)
    for i,(x,y) in enumerate(((0,-1),(1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1))):
        cpu.mem_write(0x89F688+4*i,struct.pack('<hh',x,y));put(0x89A304+4*i,x+21*y)
    for offset,value in ((0xC,nodes),(0x10,opens),(0x14,queue),(0x18,visits),(0x1C,alt_visits),(0x20,alt_costs),(0x24,costs),(0x40,markers)):
        put(finder+offset,value)
    put(queue+4,65536);put(queue+8,queue_items);put(queue+0x10,0xFFFFFFFF)
    cpu.mem_write(finder+4,struct.pack('<f',1));cpu.mem_write(finder+8,b'\1');put(finder+0x28,1)
    cpu.mem_write(0x89A300,b'\1') # Path static already initialized; no CRT atexit registration in the harness.
    for y in range(21):
        for x in range(21):
            if valid(x,y):put(slots+(y*512+x)*4,at(x,y))
    for c in (invalid,):put(c+0x44,-1);put(c+0x54,-1);put(c+0x58,-1);cpu.mem_write(c+0x116,b'\xFF\xFF')
    rows=[]
    cpu.mem_write(0xABDC50,bytes(0x148)) # Original fallback Cell, including coordinate scratch.
    for seed in range(80):
        sx,sy=8,6;ex,ey=(sx,sy) if seed==0 else (7+seed%7,7+seed%8)
        cpu.mem_write(foot+0x530,struct.pack('<d',0.125 if seed>=48 else 0));cpu.mem_write(foot+0x8C,bytes([seed>=64]))
        for ty in range(7):
            for tx in range(7):put(house+0x59F0+4*(tx+130*ty),4 if seed>=48 and (tx*5+ty*7+seed)%7==0 else 0)
        for y in range(21):
            for x in range(21):
                if not valid(x,y):continue
                c=at(x,y);cpu.mem_write(c,bytes(0x180));cpu.mem_write(c+0x24,struct.pack('<hh',x,y))
                put(c+0x44,-1);put(c+0x54,-1);put(c+0x58,-1);cpu.mem_write(c+0x116,b'\xFF\xFF')
                hashed=(x*17+y*31+seed*13)%19
                blocked=seed>=8 and hashed<5 and (x,y)!=(sx,sy)
                put(c+0xEC,2 if blocked else 0);put(c+0x140,(0x40000 if seed>=16 and hashed==7 else 0)|(0x300 if seed>=64 else 0))
        for limit in (-1,0,1,2,8,32):
            cpu.mem_write(visits,bytes(0x4000));put(nodes+0x180000,0);put(opens+0x100000,0);put(queue,0)
            cpu.mem_write(queue_items,bytes(0x40004));put(finder+0x28,1);put(finder+0x3C,0)
            cpu.mem_write(coords,struct.pack('<hhhh',sx,sy,ex,ey));cpu.mem_write(path,b'\x77'*8008)
            cpu.mem_write(stack,struct.pack('<IIIIIII',stop,coords,coords+4,foot,path,limit&0xFFFFFFFF,0))
            cpu.reg_write(UC_X86_REG_ECX,finder);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
            try:cpu.emu_start(0x429A90,stop,count=3000000)
            except Exception as exc:raise RuntimeError(f'seed={seed} limit={limit} eip={cpu.reg_read(UC_X86_REG_EIP):#x}') from exc
            assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+28
            result=cpu.reg_read(UC_X86_REG_EAX);length=getint(result+8) if result else 0;cost=getint(result+4) if result else 0
            directions=[getint(path+4*i) for i in range(length+1)] if result else []
            levels=[getint(get(result+0x14)+4*i) for i in range(length-1)] if result else []
            rows.append(' '.join(map(str,[seed,limit,cost,length,*directions,*levels])))
    output.write_text('\n'.join(rows)+'\n');report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'x87_control_word':'0x0E7F','fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original regular AStar cases')
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):parser.add_argument('--'+arg,type=Path,required=True)
    a=parser.parse_args();generate(a.exe,a.output,a.report)
