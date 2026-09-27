#!/usr/bin/env python3
"""Original YR subposition index and infantry bit writes; map/owner lookups are fixtures."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_EAX

SHA = '7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'

def generate(exe, output, report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(exe)); cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    obj, cell, coord, vt, stub, stack, stop = [0x1000000 + n for n in (0,0x2000,0x4000,0x6000,0x8000,0x18000,0x1F000)]
    cpu.mem_map(obj, 0x20000); cpu.reg_write(UC_X86_REG_FPCW, 0x27F)
    def put(a,v): cpu.mem_write(a,struct.pack('<I',v & 0xFFFFFFFF))
    def get(a): return struct.unpack('<I',cpu.mem_read(a,4))[0]
    def call(entry,this,*args):
        cpu.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*args))
        cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_ECX,this)
        cpu.emu_start(entry,stop,count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop
        assert cpu.reg_read(UC_X86_REG_ESP)==stack+4*(len(args)+1)
        return cpu.reg_read(UC_X86_REG_EAX)
    rows=[]
    for y in range(256):
        for x in range(256):
            cpu.mem_write(coord,struct.pack('<iii',x,y,0))
            rows.append(f'S {x} {y} {call(0x4810A0,coord)&0xFF}')
    # Keep real Spot_Index / bit update code, replace map address/floor and owner.
    cpu.mem_write(0x565730,b'\xB8'+struct.pack('<I',cell)+b'\xC2\x04\x00')
    cpu.mem_write(0x578080,b'\xB8'+struct.pack('<I',104)+b'\xC2\x04\x00')
    cpu.mem_write(stub,b'\xB8\x0C\x00\x00\x00\xC3');put(obj,vt);put(vt+0x38,stub)
    put(0xA8F234,416)
    for bridge in (0,0x100,0x400):
        for z in (104,519,520,521):
            for x,y in ((128,128),(64,64),(192,64),(64,192),(192,192)):
                cpu.mem_write(coord,struct.pack('<iii',x,y,z))
                put(cell+0x140,bridge)
                put(cell+0x124,0xA000009C);put(cell+0x128,0xB000009C)
                put(cell+0x54,7);put(cell+0x58,9)
                call(0x5217C0,obj,coord)
                marked=[get(cell+off) for off in (0x124,0x128,0x54,0x58)]
                call(0x521850,obj,coord)
                cleared=[get(cell+off) for off in (0x124,0x128,0x54,0x58)]
                rows.append('O '+' '.join(map(str,[bridge,x,y,z,*marked,*cleared])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(f'{len(rows)} original occupation observations')

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
