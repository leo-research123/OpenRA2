#!/usr/bin/env python3
"""Run pinned gamemd target packing and abstract geometry instructions.

Only virtual coordinate/type getters are doubled. The actual constructors,
direction/distance algorithms and CRT math execute in x86 emulation. This is
not a game-process integration test. The fixture contains values, not code.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"

def generate(exe, output, report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(exe))
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage+0xFFF) & ~0xFFF)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    obj, other, data, result, vt, stub, stack, stop = [0x1000000+n for n in
        (0,0x1000,0x2000,0x3000,0x4000,0x5000,0x18000,0x1F000)]
    cpu.mem_map(obj,0x20000)
    # Original gameplay configures x87 double precision, round-to-nearest.
    cpu.reg_write(UC_X86_REG_FPCW,0x027F)
    def put(address, value): cpu.mem_write(address,struct.pack('<I',value & 0xFFFFFFFF))
    # Coordinates returned by pointer, and WhatAmI reports Abstract.
    cpu.mem_write(stub,b'\x8D\x81\x00\x01\x00\x00\xC2\x04\x00')
    cpu.mem_write(stub+0x10,b'\xB8\x34\x00\x00\x00\xC3')
    put(vt+0x48,stub); put(vt+0x2C,stub+0x10)
    put(obj,vt); put(other,vt)
    def call(address, this, *args):
        cpu.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*args))
        cpu.reg_write(UC_X86_REG_ESP,stack); cpu.reg_write(UC_X86_REG_ECX,this)
        cpu.emu_start(address,stop,count=1000000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop
        assert cpu.reg_read(UC_X86_REG_ESP)==stack+4*(len(args)+1)
        return cpu.reg_read(UC_X86_REG_EAX)
    # Initialize the original no-cell static before invoking target constructors.
    call(0x6E69E0,0)
    lines=[]
    for x,y in [(0,0),(-1,-1),(-2,-3),(32767,32767),(-32768,-32768),(12,25)]:
        cpu.mem_write(data,struct.pack('<2h',x,y)); cpu.mem_write(result,b'\xA5'*5)
        call(0x6E6B20,result,data)
        ident,kind=struct.unpack('<iB',cpu.mem_read(result,5))
        lines.append(f'C {x} {y} {ident} {kind}')
    coords=[(0,-256,0),(256,0,0),(0,256,0),(-256,0,0),(-255,-257,0),
            (3,4,12),(0,0,0),(65536,-65536,12),(2147483647,-2147483648,0)]
    rng=random.Random(0x410170)
    coords += [tuple(rng.randrange(-1000000,1000000) for _ in range(3)) for _ in range(128)]
    for xyz in coords:
        cpu.mem_write(data,struct.pack('<3i',*xyz))
        call(0x6E6B70,result,data)
        ident,kind=struct.unpack('<iB',cpu.mem_read(result,5))
        lines.append('Q '+' '.join(map(str,(*xyz,ident,kind))))
        cpu.mem_write(obj+0x100,struct.pack('<3i',0,0,0))
        cpu.mem_write(other+0x100,struct.pack('<3i',*xyz))
        call(0x5F3DB0,obj,result,other)
        direction=struct.unpack('<I',cpu.mem_read(result,4))[0]
        d2=call(0x5F6440,obj,other); d3=call(0x5F6360,obj,other)
        lines.append('G '+' '.join(map(str,(*xyz,direction,d2,d3))))
    output.write_text('\n'.join(lines)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'samples':len(lines),
        'x87_control_word':'0x027F','scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(f'Generated {len(lines)} P1 original samples')

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','output','report'): parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args(); generate(args.exe,args.output,args.report)
