#!/usr/bin/env python3
"""Fixed YR WeaponType speed/threat/CRC instructions; field fixtures, no game-process claim."""
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
    obj,bullet,warhead,rules,crc,vt,stack,stop=[0x1000000+n for n in
        (0,0x2000,0x3000,0x4000,0x6000,0x7000,0x18000,0x1F000)]
    cpu.mem_map(obj,0x20000)
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<I',cpu.mem_read(a,4))[0]
    def signed(v):return struct.unpack('<i',struct.pack('<I',v))[0]
    def byte(a,v):cpu.mem_write(a,bytes([v&0xFF]))
    def call(a,this,*args):
        cpu.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*args))
        cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_ECX,this)
        cpu.emu_start(a,stop,count=1000000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop
        assert cpu.reg_read(UC_X86_REG_ESP)==stack+4*(len(args)+1)
        return cpu.reg_read(UC_X86_REG_EAX)
    put(0x8871E0,rules)
    lines=[]
    for gravity in (-6,0,1,6,11):
        for distance in (-256,0,1,256,1280,0x7FFFFFFF):
            for floater in (0,1):
                for rot,present in ((0,1),(7,1),(0,0)):
                    put(rules+0x16B8,gravity);put(obj+0xA0,bullet if present else 0)
                    put(obj+0xA8,71);put(obj+0xB4,distance)
                    put(bullet+0x2DC,rot);byte(bullet+0x295,floater)
                    cpu.reg_write(UC_X86_REG_FPCW,0x027F)
                    call(0x7729F0,obj)
                    lines.append(f'S {gravity} {distance} {floater} {rot} {present} {signed(get(obj+0xA8))}')
    for aa in (0,1):
        for ag in (0,1):
            put(obj+0xA0,bullet);byte(bullet+0x2A4,aa);byte(bullet+0x2A5,ag)
            lines.append(f'T {aa} {ag} {call(0x772A90,obj)}')
    rng=random.Random(0x772AE0)
    for case in range(64):
        cpu.mem_write(obj,bytes(0x160));cpu.mem_write(crc,bytes(16))
        cpu.mem_write(obj+0x24,b'P2_CORPUS\0');cpu.mem_write(obj+0x64,b'P2_CORPUS\0')
        ident=rng.getrandbits(32)
        projectile_id=rng.getrandbits(32) if case%2 else 0
        warhead_id=rng.getrandbits(32) if case%3 else 0
        values=[rng.randrange(-1000,1000) for _ in range(6)]
        counts=[rng.randrange(10) for _ in range(3)]
        mask=rng.getrandbits(16);duration=rng.randrange(256)
        put(obj+0x10,ident);byte(obj+0x20,mask>>15)
        for target,token,offset in [(bullet,projectile_id,0xA0),(warhead,warhead_id,0xAC)]:
            put(obj+offset,target if token else 0);put(target+0x10,token)
            put(target+4,vt)
        # The actual base IRTTI Fetch_ID code executes; only the vtable is a fixture.
        put(vt+0x10,0x410220)
        for offset,value in zip((0x98,0x9C,0xA4,0xA8,0xB0,0xB4),values):put(obj+offset,value)
        for offset,value in zip((0xCC,0xE8,0x104),counts):put(obj+offset,value)
        for i,offset in enumerate((0x130,0x145,0x146,0x147,0x149,0x14D,0x129,0x12E,
                0x14C,0x12D,0x148,0x12F,0x12A,0x12B,0x12C)):byte(obj+offset,(mask>>i)&1)
        byte(obj+0x14E,duration)
        call(0x772AE0,obj,crc)
        before=[get(crc+i) for i in (0,4,8)]
        value=call(0x4A1DE0,crc,0,0)
        lines.append('C '+' '.join(map(str,[ident,projectile_id,warhead_id,*values,*counts,mask,duration,*before,value])))
    output.write_text('\n'.join(lines)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(lines),'speed_cases':180,'threat_cases':4,
        'crc_cases':64,'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(f'{len(lines)} original WeaponType cases')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
