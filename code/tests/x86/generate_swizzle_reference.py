#!/usr/bin/env python3
"""Original Swizzle COM/vector/reset instructions; borrowed storage avoids allocator doubles."""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *

SHA="7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"
def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe)); cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF)
    cpu.mem_write(base,pe.get_memory_mapped_image())
    obj,old,new,slots,stack,stop=[0x1000000+n for n in (0,0x1000,0x2000,0x3000,0x18000,0x1F000)]
    cpu.mem_map(obj,0x20000)
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<I',cpu.mem_read(a,4))[0]
    def call(a,*args,this=0):
        cpu.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*args))
        cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_ECX,this)
        cpu.emu_start(a,stop,count=1000000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop
        assert cpu.reg_read(UC_X86_REG_ESP)==stack+4*(len(args)+1)
        return cpu.reg_read(UC_X86_REG_EAX)
    cases=[([],[]),([],[1,2]),([0],[]),([1],[1]),([0xFFFFFFFF,1,0x80000000,1],[1,0x80000000,0xFFFFFFFF])]
    rng=random.Random(0x6CF350)
    for size in range(1,27):
        announcements=[rng.choice((1,7,0x80000000,0xFFFFFFFF)) for _ in range(size)]
        requests=[rng.choice(announcements) for _ in range(size+2)]
        cases.append((requests,announcements))
    lines=[]
    for requests,announcements in cases:
        call(0x6CF180,this=obj)
        for offset,data in [(4,old),(0x1C,new)]:
            put(obj+offset+4,data);put(obj+offset+8,64)
        for i,key in enumerate(requests):
            put(slots+i*4,key)
            assert call(0x6CF240,obj,slots+i*4)==0
            assert get(slots+i*4)==0
        pairs=[(key,0x200+i) for i,key in enumerate(announcements)]
        for key,value in pairs:assert call(0x6CF2C0,obj,key,value)==0
        result=call(0x6CF230,obj)
        assert get(obj+8)==old and get(obj+0x20)==new
        lines.append('S '+' '.join(map(str,(len(requests),len(pairs),result,get(obj+0x14),get(obj+0x2C),get(obj+0xC),get(obj+0x24)))))
        lines.append(' '.join(map(str,requests)))
        lines.extend(f'{key} {value}' for key,value in pairs)
        lines.append(' '.join(str(get(slots+i*4)) for i in range(len(requests))))
    output.write_text('\n'.join(lines)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(cases),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(f'{len(cases)} original swizzle cases')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
