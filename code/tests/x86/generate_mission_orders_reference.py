#!/usr/bin/env python3
"""Original Mission order state transitions. Object memory and Ready virtual are fixtures."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX

SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    obj,vt,stub,stack,stop=[0x1000000+n for n in (0,0x2000,0x4000,0x18000,0x1F000)]
    cpu.mem_map(obj,0x20000)
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def call(entry,*args):
        cpu.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*[a&0xFFFFFFFF for a in args]))
        cpu.reg_write(UC_X86_REG_ECX,obj);cpu.reg_write(UC_X86_REG_ESP,stack)
        cpu.emu_start(entry,stop,count=10000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop
        assert cpu.reg_read(UC_X86_REG_ESP)==stack+4*(len(args)+1)
        return int(bool(cpu.reg_read(UC_X86_REG_EAX)&0xFF))
    put(obj,vt);put(vt+0x200,stub);put(vt+0x1EC,0x5B3570);put(0xA8ED84,500)
    # Mutate data, not already translated Unicorn code, between readiness cases.
    cpu.mem_write(stub,b'\xA0'+struct.pack('<I',stub+0x100)+b'\xC3')
    rows=[]
    def sample(op,current,queued,suspended,requested,start,ready):
        for offset,value in zip((0xAC,0xB0,0xB4,0xBC,0xC0,0xC4,0xC8,0xD0),
                (current,suspended,queued,19,11,17,13,23)):put(obj+offset,value)
        cpu.mem_write(obj+0xB8,b'\x01');cpu.mem_write(stub+0x100,bytes([ready]))
        if op=='Q':result=call(0x5B35E0,requested,start)
        elif op=='N':result=call(0x5B3570)
        elif op=='F':call(0x5B2FD0,requested);result=0
        elif op=='O':call(0x5B3650,requested,0,0);result=0
        else:result=call(0x5B36B0)
        state=[get(obj+o) for o in (0xAC,0xB0,0xB4)]
        state+=[cpu.mem_read(obj+0xB8,1)[0]]
        state+=[get(obj+o) for o in (0xBC,0xC0,0xC4,0xC8,0xD0)]
        rows.append(op+' '+' '.join(map(str,[current,queued,suspended,requested,start,ready,result,*state])))
    for current in (-1,1,2,3,5,13,19,28):
        for queued in (-1,1,2,5):
            for suspended in (-1,8):
                for requested in (-1,0,1,2,3,5,13,28):
                    for start in (0,1):
                        for ready in (0,1):sample('Q',current,queued,suspended,requested,start,ready)
                    for op in ('F','O'):sample(op,current,queued,suspended,requested,0,0)
                for op in ('N','R'):sample(op,current,queued,suspended,-1,0,0)
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original mission order cases')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
