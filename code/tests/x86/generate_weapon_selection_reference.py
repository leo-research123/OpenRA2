#!/usr/bin/env python3
"""Original Techno/Infantry weapon choice, not firing or damage.

Type, weapon-slot, RTTI, air/floor and cell virtual getters are fixtures.
Original selector, turret predicate, house alliance and naval selector execute.
Cases cover guards, generic infantry/cell targets, deployment and naval policy;
building/aircraft-specific selection branches are not in this corpus.
"""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX

SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'

def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    cpu.mem_map(0x1000000,0x30000)
    obj,other,typ,other_type,primary,secondary,wh1,wh2,bullet,cell,vt,stub,owner,enemy,slots,stack,stop=[0x1000000+n for n in
        (0,0x1000,0x2000,0x3000,0x5000,0x5200,0x6000,0x6400,0x6800,0x7000,0x8000,0x9000,0x10000,0x16000,0x20000,0x27000,0x2F000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def byte(a,v):cpu.mem_write(a,bytes([int(bool(v))]))
    flags=0;mode=0;case=0
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);pop=0;result=0
        if address==stub:result=typ if this==obj else other_type
        elif address==stub+0x10:result=slots+4*get(sp+4);pop=4
        elif address==stub+0x20:result=11 if this==other and mode==3 and case==7 else 15
        elif address==stub+0x30:result=int(mode==0 and bool(flags&4))
        elif address==stub+0x40:result=int(this==other and bool(flags&(512 if mode==0 else 1)) and not(mode==3 and case==7))
        elif address==stub+0x50:result=cell
        elif address==stub+0x60:result=owner if flags&16 else enemy
        elif address==stub+0x70:pass # Cell IsOnFloor is original false default.
        else:return
        uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for off,n in ((0x84,0),(0x3F8,1),(0x2C,2),(0x400,3),(0x54,4),(0x1BC,5),(0x3C,6),(0x50,7)):put(vt+off,stub+n*0x10)
    put(vt+0x184,0x5B3040);put(vt+0x2E8,0x6F3820)
    for address in range(stub,stub+0x80,0x10):cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    put(owner+0x30,0);put(enemy+0x30,1);put(owner+0x5788,1)
    rows=[]
    cases=itertools.chain(((0,0,f) for f in range(1024)),
        ((1,n,f) for n in range(8) for f in range(128)),
        ((2,s,f) for s in range(-1,42) for f in range(8)),
        ((3,c,f) for c in range(12) for f in range(64)))
    for mode,case,flags in cases:
        for at,size in ((obj,0x800),(other,0x800),(typ,0x1000),(other_type,0x1000),
                        (primary,0x160),(secondary,0x160),(wh1,0x1D0),(wh2,0x1D0),(bullet,0x300),(cell,0x148)):
            cpu.mem_write(at,bytes(size))
        put(obj,vt);put(other,vt);put(obj+0x6C0,typ);put(obj+0x21C,owner);put(other+0x14,7)
        put(primary+0xAC,wh1);put(secondary+0xAC,wh2);put(primary+0xA0,bullet);put(secondary+0xA0,bullet)
        cpu.mem_write(wh1+0xA0,struct.pack('<d',1.0));cpu.mem_write(wh2+0xA0,struct.pack('<d',1.0))
        put(slots,primary);put(slots+4,secondary);put(typ+0xD50,-1);put(obj+0xAC,5);put(obj+0x140,2)
        target=other;entry=0x6F3330
        if mode==0:
            byte(typ+0xCD5,flags&1);put(typ+0x808,2 if flags&2 else 0)
            put(slots,primary if flags&16 else 0);put(slots+4,secondary if flags&8 else 0);byte(secondary+0x136,flags&32)
            target=other if flags&64 else 0;byte(obj+0x82,flags&128);put(typ+0xD50,2);put(obj+0x138,-1 if flags&256 else 3)
            byte(bullet+0x2A4,flags&512)
        elif mode==1:
            entry=0x6F3820;put(typ+0x600,case);byte(other_type+0xD69,flags&1);byte(other_type+0xD97,flags&2)
            put(other_type+0x67C,3 if flags&4 else 0);byte(other_type+0x694,flags&8);put(other+0x220,1 if flags&16 else 0)
            if flags&32:put(other+0x14,0)
            if flags&64:target=0
        elif mode==2:
            entry=0x5218E0;put(obj+0x6C4,case);byte(typ+0x6AC,flags&1);put(typ+0x6A8,3)
            byte(obj+0x82,flags&2);put(typ+0xD50,2 if flags&4 else -1);target=0
        else:
            byte(bullet+0x2A4,case!=9);byte(other+0x8C,flags&2);byte(obj+0x1CC,flags&32)
            put(cell+0xEC,6 if flags&8 else 2 if flags&4 else 0);put(typ+0x600,case%8)
            if case==1:byte(wh2+0x16C,1)
            if case==2:byte(wh1+0x15B,1)
            if case==3:byte(secondary+0x142,1);byte(other_type+0x5EF,1)
            if case==4:byte(secondary+0x150,1);put(obj+0xAC,16)
            if case==5:cpu.mem_write(wh2+0xA0,struct.pack('<d',0.0))
            if case==6:cpu.mem_write(wh1+0xA0,struct.pack('<d',0.0))
            if case==7:put(other+0x14,0);put(other+0xEC,get(cell+0xEC));put(other+0x140,0x100 if flags&2 else 0);byte(typ+0xCCE,1)
            if case in (7,8):put(typ+0x604,2)
            if case==10:byte(obj+0x82,1);put(typ+0xD50,2)
            if case==11:byte(typ+0xCD5,1)
        put(stack,stop);put(stack+4,target);cpu.reg_write(UC_X86_REG_ECX,obj);cpu.reg_write(UC_X86_REG_ESP,stack)
        cpu.emu_start(entry,stop,count=10000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+8
        result=struct.unpack('<i',struct.pack('<I',cpu.reg_read(UC_X86_REG_EAX)))[0]
        rows.append(f'{mode} {case} {flags} {result}')
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original weapon-selection cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
