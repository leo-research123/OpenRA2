#!/usr/bin/env python3
"""Execute YR PlayAnim; virtual air/stop/storage and map/audio boundaries are fixtures."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_FPCW

SHA = '7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'

def generate(exe, output, report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(exe)); cpu = Uc(UC_ARCH_X86, UC_MODE_32); base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF)
    cpu.mem_write(base, pe.get_memory_mapped_image()); cpu.mem_map(0x1000000, 0x40000)
    obj, typ, seq, vt, stub, cell, scenario, stack, stop = [0x1000000 + n for n in (0,0x2000,0x4000,0x6000,0x8000,0x9000,0x10000,0x30000,0x3F000)]
    def put(a, v): cpu.mem_write(a, struct.pack('<I', v & 0xFFFFFFFF))
    def get(a): return struct.unpack('<i', cpu.mem_read(a,4))[0]
    def byte(a, v): cpu.mem_write(a, bytes([v]))
    def call(entry, this, *args):
        cpu.mem_write(stack, struct.pack('<'+'I'*(1+len(args)), stop, *[v&0xFFFFFFFF for v in args]))
        cpu.reg_write(UC_X86_REG_ECX,this);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        cpu.emu_start(entry,stop,count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop
        assert cpu.reg_read(UC_X86_REG_ESP)==stack+4*(1+len(args))
        return int(bool(cpu.reg_read(UC_X86_REG_EAX)&0xFF))
    stops = [0]
    def boundary(uc, address, size, data):
        sp=uc.reg_read(UC_X86_REG_ESP); pop=0
        if address==0x5657A0: uc.reg_write(UC_X86_REG_EAX,cell);pop=4
        elif address==0x7509E0: pop=4 # sound index -1: no observable playback
        elif address==stub+0x60: stops[0]+=1
        else: return
        uc.reg_write(UC_X86_REG_EIP, get(sp)&0xFFFFFFFF);uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for address in (0x5657A0,0x7509E0,stub+0x60):cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    put(obj,vt);put(obj+0x6C0,typ);put(typ+0xE3C,seq);put(0xA8B230,scenario);put(0xA8ED84,500)
    # Immutable stubs, mutable data: avoid translated-block caching across cases.
    cpu.mem_write(stub,b'\x8B\x81\xC0\x06\x00\x00\xC3');put(vt+0x84,stub)
    cpu.mem_write(stub+0x20,b'\xA0'+struct.pack('<I',stub+0x100)+b'\xC3');put(vt+0x54,stub+0x20)
    cpu.mem_write(stub+0x40,b'\x8B\x44\x24\x04\xC2\x04\x00');put(vt+0x1B8,stub+0x40)
    cpu.mem_write(stub+0x60,b'\xC3');put(vt+0x500,stub+0x60)
    cpu.mem_write(stub+0x80,b'\xDD\x05'+struct.pack('<I',stub+0x108)+b'\xC3');put(vt+0x2B4,stub+0x80)
    cpu.mem_write(stub+0x108,struct.pack('<d',1.0))
    for action in range(42):put(seq+36*action,100+10*action);put(seq+36*action+4,action%7+1)
    for off in (0x56C,0x570,0xEA4,0xEA8):put(typ+off,-1)
    rows=[]
    def sample(previous, requested, force=0, variant=0, random=0, speed=4):
        # flags: falling, cannot-crawl, amphibious-water, on-bridge, flying,
        # panic, loaded-slave, dead, missing-sequence.
        put(obj+0x6C4,previous);put(obj+0xF8,19);put(obj+0x100,31);put(obj+0x108,7);put(obj+0x10C,7)
        byte(obj+0x8D,bool(variant&1));byte(typ+0xEBD,not bool(variant&2));byte(obj+0x8C,bool(variant&8))
        put(typ+0x5B4,3 if variant&4 else 0);put(cell+0xEC,6);put(obj+0x6E8,2)
        byte(stub+0x100,bool(variant&16));put(obj+0x6D4,200 if variant&32 else 0)
        put(obj+0x2DC,obj if variant&64 else 0);put(typ+0x800,10 if variant&64 else 0)
        put(obj+0x6C,0 if variant&128 else 100);byte(obj+0x6DB,1)
        put(seq+36*requested+4,0 if variant&256 else requested%7+1)
        put(0xA8EB60,speed);call(0x65C6D0,scenario+0x218,99);stops[0]=0
        result=call(0x51D6F0,obj,requested,force,random)
        state=[result,get(obj+0x6C4),get(obj+0xF8),get(obj+0x100),get(obj+0x108),get(obj+0x10C),cpu.mem_read(obj+0x6DB,1)[0],get(obj+0x6E8),stops[0],get(scenario+0x21C)]
        rows.append(' '.join(map(str,[previous,requested,force,variant,random,speed,*state])))
        put(seq+36*requested+4,requested%7+1)
    for previous in range(-1,42):
        for requested in range(42):
            for force in (0,1): sample(previous,requested,force)
    for variant in (1,2,4,12,16,24,32,64,128,256):
        for action in range(42):sample(33 if variant==1 else -1,action,1,variant)
    for speed in range(8):
        for action in range(42):sample(-1,action,1,0,1,speed)
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original PlayAnim cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
