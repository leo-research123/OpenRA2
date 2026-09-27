#!/usr/bin/env python3
"""Execute original Foot Can_Reach with two real-format cells, no algorithm stubs."""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX

SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'

def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    cpu.mem_map(0x1000000,0x10000)
    dest,source,level,bridge,stack,stop=[0x1000000+n for n in (0,0x1000,0x2000,0x2010,0xD000,0xF000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    rows=[]
    for dh,sh,df,sf,ds,ss,mode,facing,alt in itertools.product(range(6),range(6),range(4),range(2),range(2),range(2),range(3),(-1,2),range(2)):
        initial=(-1,sh,sh+4)[mode]
        cpu.mem_write(dest+0x11B,bytes([dh,ds]));cpu.mem_write(source+0x11B,bytes([sh,ss]))
        put(dest+0x140,df*0x100);put(source+0x140,sf*0x100);put(level,initial);cpu.mem_write(bridge,bytes([alt]))
        cpu.mem_write(stack,struct.pack('<IIIIII',stop,dest,facing&0xFFFFFFFF,level,bridge,source))
        cpu.reg_write(UC_X86_REG_ESP,stack);cpu.emu_start(0x4D9C60,stop,count=10000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+24
        rows.append(' '.join(map(str,[dh,sh,df,sf,ds,ss,initial,facing,alt,cpu.reg_read(UC_X86_REG_EAX),get(level),cpu.mem_read(bridge,1)[0]])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Foot reach cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
