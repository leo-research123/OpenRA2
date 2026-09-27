#!/usr/bin/env python3
"""Run original Infantry->Foot speed, house bias and veteran ability instructions.

Only RTTI/type/default-speed virtual getters are fixtures. No arithmetic,
ability or crawling branch is stubbed; this is not a world-update test.
"""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_FPCW

SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'

def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    cpu.mem_map(0x1000000,0x10000)
    foot,typ,house,country,rules,vt,stub,stack,stop=[0x1000000+n for n in (0,0x1000,0x2000,0x3000,0x4000,0x6000,0x7000,0xD000,0xF000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    put(foot,vt);put(typ,vt+0x800);put(foot+0x6C0,typ);put(foot+0x21C,house);put(house+0x34,country);put(0x8871E0,rules)
    cpu.mem_write(stub,b'\xB8'+struct.pack('<I',typ)+b'\xC3');put(vt+0x84,stub)
    cpu.mem_write(stub+0x10,b'\xB8\x0F\x00\x00\x00\xC3');put(vt+0x2C,stub+0x10)
    cpu.mem_write(stub+0x20,b'\xB8\x10\x00\x00\x00\xC3');put(vt+0x800+0x2C,stub+0x20)
    put(vt+0x38C,0x70EFE0);cpu.mem_write(rules+0x678,struct.pack('<d',1.3))
    rows=[]
    for speed,bias,mult,percent,rank,abilities,posture in itertools.product(
        (0,1,2,3,4,5,6,10,11,37),range(3),range(3),range(3),range(3),range(4),range(4)):
        put(typ+0x678,speed);cpu.mem_write(country+0x128,struct.pack('<f',(0.5,1.0,1.25)[bias]))
        cpu.mem_write(foot+0x580,struct.pack('<d',(0.5,1.0,1.5)[mult]));cpu.mem_write(foot+0x578,struct.pack('<d',(0.0,0.5,1.0)[percent]))
        cpu.mem_write(foot+0x150,struct.pack('<f',float(rank)))
        cpu.mem_write(typ+0x29C,bytes([bool(abilities&1)]));cpu.mem_write(typ+0x2AE,bytes([bool(abilities&2)]))
        cpu.mem_write(foot+0x6DB,bytes([bool(posture&1)]));cpu.mem_write(typ+0xEBD,bytes([bool(posture&2)]))
        put(stack,stop);cpu.reg_write(UC_X86_REG_ECX,foot);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        cpu.emu_start(0x521D80,stop,count=10000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4
        result=struct.unpack('<i',struct.pack('<I',cpu.reg_read(UC_X86_REG_EAX)))[0]
        rows.append(' '.join(map(str,[speed,bias,mult,percent,rank,abilities,posture,result])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Infantry speed cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
