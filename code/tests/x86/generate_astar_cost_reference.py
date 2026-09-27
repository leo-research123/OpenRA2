#!/usr/bin/env python3
"""Execute original AStar step cost (0x429830), including traffic lookahead; only map address/coordinate/type boundaries are fixture data."""
import argparse
import hashlib
import itertools
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
    arena=0x1000000;cpu.mem_map(arena,0x400000)
    finder,body,vt,stub,stack,returned,stop=[arena+n for n in (0,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000)]
    slots=arena+0x10000;cells=arena+0x20000
    offsets=((0,-1),(1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1))
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<I',cpu.mem_read(a,4))[0]
    def at(x,y):return cells+(y*32+x)*0x180
    def slot(x,y):return slots+(y*512+x)*4
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP)
        if address==0x5657A0:
            x,y=struct.unpack('<hh',cpu.mem_read(get(sp+4),4));value=at(x,y);pop=4
        else:return
        uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    cpu.hook_add(UC_HOOK_CODE,boundary,begin=0x5657A0,end=0x5657A0)
    for y in range(16):
        for x in range(16):
            put(slot(x,y),at(x,y));cpu.mem_write(at(x,y)+0x24,struct.pack('<hh',x,y))
    for i,offset in enumerate(offsets):cpu.mem_write(0x89F688+4*i,struct.pack('<hh',*offset))
    put(body,vt);put(vt+0x1B8,0x41BEA0)
    # GetCell returns this fixture's cell; all traffic costs and counters stay original.
    put(vt+0x1BC,stub);cpu.mem_write(stub,b'\xB8'+struct.pack('<I',at(8,6))+b'\xC3')
    cpu.mem_write(body+0x9C,struct.pack('<iii',8*256+128,6*256+128,0));cpu.mem_write(body+0x38C,bytes(20))
    # Capture the actual x87 return, without deriving its value in the harness.
    cpu.mem_write(returned,b'\xDD\x1D'+struct.pack('<I',returned+0x20)+b'\xE9'+struct.pack('<i',stop-returned-11))
    rows=[]
    def sample(move,mode,predicted,bridge,avoid,orientation,direction,near,far,traffic):
        for y in range(3,10):
            for x in range(5,12):
                c=at(x,y);put(c+0x140,0);put(c+0xE4,0);put(c+0xE8,0)
        dx,dy=offsets[direction];start=at(8-dx,6-dy);dest=at(8,6)
        across=(-2,-2,0,1,1,1,0,-2);along=(0,-1024,-1024,-1024,0,512,512,512)
        table=along if orientation else across
        if near:put(get(slot(8,6)+4*table[direction])+0x140,0x100)
        if far:put(get(slot(8,6)+4*table[(direction-4)&7])+0x140,0x100)
        put(dest+0x140,get(dest+0x140)|(0x40000 if predicted else 0)|(0x800 if orientation else 0))
        cpu.mem_write(finder+1,bytes([avoid]));put(finder+0x3C,mode)
        put(body+0x14,2 if traffic==1 else 7);put(body+0x5E0,-1 if traffic==2 else 8 if traffic==4 else 2)
        cpu.mem_write(body+0x578,struct.pack('<d',1.0 if traffic==5 else 0.0))
        cpu.mem_write(body+0x8C,b'\0');put(body+0x388,0x4000);put(body+0x38C,0x4000)
        if traffic:
            put(dest+(0xE8 if bridge else 0xE4),body)
            if traffic==6:put(at(9,6)+0xE4,body) # Ten-step cycle remains costly.
        cpu.mem_write(stack,struct.pack('<IIIIII',returned,slot(8-dx,6-dy),slot(8,6),bridge,move,body))
        cpu.reg_write(UC_X86_REG_ECX,finder);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        try:cpu.emu_start(0x429830,stop,count=50000)
        except Exception as exc:raise RuntimeError(f'{(move,mode,predicted,bridge,avoid,orientation,direction,near,far,traffic)}, eip={cpu.reg_read(UC_X86_REG_EIP):#x}') from exc
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+24
        bits=struct.unpack('<Q',cpu.mem_read(returned+0x20,8))[0]
        rows.append(' '.join(map(str,(move,mode,predicted,bridge,avoid,orientation,direction,near,far,traffic,bits))))
    for move,mode,pred,bridge,avoid,orient,direction,near,far in itertools.product(range(8),range(3),range(2),range(2),range(2),range(2),range(8),range(2),range(2)):
        sample(move,mode,pred,bridge,avoid,orient,direction,near,far,0)
    for traffic,mode,pred,bridge in itertools.product(range(1,7),range(3),range(2),range(2)):
        sample(2,mode,pred,bridge,0,0,2,0,0,traffic)
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original AStar cost cases')
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):parser.add_argument('--'+arg,type=Path,required=True)
    a=parser.parse_args();generate(a.exe,a.output,a.report)
