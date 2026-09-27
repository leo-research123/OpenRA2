#!/usr/bin/env python3
"""YR Techno.Revealed (0x6F4960), including Object and observer-house predicates.

QueueMission is a recording fixture; tags are absent. No autonomous house update
or trigger execution runs. Compare object discovery flags, owner notifications,
queued mission and boolean return, including null observer failure side effects.
"""
import argparse
import hashlib
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
    obj,owner,player,observer,vt,stub,stack,stop=[0x1000000+n for n in (0,0x1000,0x7000,0xD000,0x14000,0x15000,0x20000,0x2F000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def byte(a,v):cpu.mem_write(a,bytes([int(bool(v))]))
    queued=-1
    def queue(uc,address,size,data):
        nonlocal queued
        sp=uc.reg_read(UC_X86_REG_ESP);queued=get(sp+4);assert get(sp+8)==0
        uc.reg_write(UC_X86_REG_EAX,0);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+12)
    cpu.hook_add(UC_HOOK_CODE,queue,begin=stub,end=stub)
    put(vt+0x184,0x5B3040);put(vt+0x1E8,stub);put(0xA83D4C,player)
    rows=[]
    for who in range(3):
      for flags in range(256):
        cpu.mem_write(obj,bytes(0x800));cpu.mem_write(owner,bytes(0x6000));cpu.mem_write(player,bytes(0x6000));cpu.mem_write(observer,bytes(0x6000))
        put(obj,vt);put(obj+0x21C,owner);byte(obj+0x41B,flags&1);byte(obj+0x41C,flags&2);byte(obj+0x41A,flags&4)
        for house in (player,observer):byte(house+0x1EC,flags&8);byte(house+0x1ED,flags&16)
        put(0xA8B238,0 if flags&32 else 3);put(obj+0xAC,14 if flags&64 else 5);put(0xA8E7AC,int(bool(flags&128)))
        put(stack,stop);put(stack+4,(0,player,observer)[who]);queued=-1
        cpu.reg_write(UC_X86_REG_ECX,obj);cpu.reg_write(UC_X86_REG_ESP,stack)
        cpu.emu_start(0x6F4960,stop,count=10000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+8
        out=[cpu.reg_read(UC_X86_REG_EAX)&0xFF,*[cpu.mem_read(a,1)[0] for a in
             (obj+0x41B,obj+0x41C,owner+0x5778,owner+0x5779,owner+0x1F4)],queued]
        rows.append(' '.join(map(str,[who,flags,*out])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Techno discovery cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
