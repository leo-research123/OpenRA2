#!/usr/bin/env python3
"""Execute YR Walk controls/queries; map lookup and Foot stop/occupancy callbacks are fixtures."""
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
    cpu.mem_write(base, pe.get_memory_mapped_image()); cpu.mem_map(0x1000000, 0x10000)
    loco, foot, cell, vt, stub, stack, stop = [0x1000000+n for n in (0,0x1000,0x3000,0x4000,0x5000,0xD000,0xF000)]
    def put(a,v): cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a): return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def coord(a,v): cpu.mem_write(a,struct.pack('<iii',*v))
    def getcoord(a): return list(struct.unpack('<iii',cpu.mem_read(a,12)))
    def call(entry,this,*args):
        cpu.mem_write(stack,struct.pack('<'+'I'*(1+len(args)),stop,*[v&0xFFFFFFFF for v in args]))
        cpu.reg_write(UC_X86_REG_ECX,this); cpu.reg_write(UC_X86_REG_ESP,stack); cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        cpu.emu_start(entry,stop,count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4*(1+len(args))
        return cpu.reg_read(UC_X86_REG_EAX)
    callbacks=[0,0,0,0,0]
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP); pop=0
        if address==0x565730: uc.reg_write(UC_X86_REG_EAX,cell); pop=4
        elif address==stub: callbacks[0]+=1
        elif address==stub+0x10:
            callbacks[1]+=1; callbacks[2:]=getcoord(get(sp+4)); pop=4
        else: return
        uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF); uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for address in (0x565730,stub,stub+0x10): cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    put(vt+0x37C,0x70EFD0); put(vt+0x1D4,0x70C5B0); put(vt+0x1D8,0x70C5C0)
    put(vt+0x54C,stub); put(vt+0xF4,stub+0x10); put(0xA8ED84,500)
    # These COORD_NONE globals are dynamic-initialized to zero, not the PE's 0xFF fill.
    call(0x75AA50,0); call(0x55A680,0)
    put(0xB45C28,104)
    scale=struct.unpack('<d',struct.pack('<Q',0x3FC25E5374344960))[0]
    cpu.mem_write(0xB0CDD8,struct.pack('<d',1.0/scale))
    rows=[]
    for operation in range(9):
      for flags in range(512):
        cpu.mem_write(loco,bytes(0x3C)); call(0x75AA90,loco)
        cpu.mem_write(foot,bytes(0x800)); put(foot,vt); put(loco+8,foot); put(loco+12,foot)
        coord(foot+0x9C,(8*256+100,6*256+100,17)); coord(loco+28,(9*256+180,6*256+100,7))
        coord(loco+40,(8*256+48,6*256+200,128+bool(flags&2)) if flags&4 else (0,0,0))
        cpu.mem_write(loco+52,bytes([bool(flags&1),bool(flags&256),bool(flags&2)]))
        put(foot+0x504,int(bool(flags&8))); cpu.mem_write(foot+0x270,bytes([bool(flags&16),bool(flags&32)]))
        cpu.mem_write(foot+0x578,struct.pack('<d',0.75 if flags&64 else 0.0))
        put(cell+0x140,0x100 if flags&128 else 0)
        cpu.mem_write(foot+0x388,struct.pack('<H',0x8000)); callbacks[:]=[0,0,0,0,0]
        if operation<2: call(0x75ACB0,0,loco+4,*((8*256+200,6*256+200,24) if operation else (0,0,0)))
        elif operation==2: call(0x75ADA0,0,loco+4)
        elif operation==3: call(0x75CB30,0,loco+4)
        elif operation==4: call(0x75CBC0,0,loco+4)
        elif operation==5: call(0x75AE00,0,loco+4,flags*127)
        elif operation in (6,7): call(0x75CA30,0,loco+4,operation-6)
        moving=bool(call(0x75AB40,0,loco+4)&0xFF)
        here=bool(call(0x75CA80,0,loco+4,8*256+200,6*256+200,24)&0xFF)
        state=getcoord(loco+28)+getcoord(loco+40)+list(cpu.mem_read(loco+52,3))+[int(moving),int(here)]+callbacks
        state += [struct.unpack('<H',cpu.mem_read(foot+0x388,2))[0]]
        rows.append(' '.join(map(str,[operation,flags,*state])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Walk state cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'): p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args(); generate(a.exe,a.output,a.report)
