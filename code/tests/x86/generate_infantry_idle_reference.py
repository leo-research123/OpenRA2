#!/usr/bin/env python3
"""Execute original YR idle/mission-readiness decisions, including real Foot/Techno bases.

Destination/mission/target/scatter callbacks record effects; COM movement-now is
a fixture. Planning tokens, Temporal and waypoint paths are absent. Foot idle
has no driver here; native ownership/piggyback cases are tested separately.
The fixture does not prove a complete frame loop or player command delivery.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_FPCW

SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'

def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    cpu.mem_map(0x1000000,0x40000)
    obj,other,typ,vt,stub,owner,rules,nav,route,loco,scenario,seq,stack,stop=[0x1000000+n for n in
        (0,0x1000,0x2000,0x4000,0x5000,0x6000,0x18000,0x24000,0x25000,0x26000,0x27000,0x29000,0x30000,0x3F000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def byte(a,v):cpu.mem_write(a,bytes([int(bool(v))]))
    def call(entry,this,*args):
        cpu.mem_write(stack,struct.pack('<'+'I'*(1+len(args)),stop,*args))
        cpu.reg_write(UC_X86_REG_ECX,this);cpu.reg_write(UC_X86_REG_ESP,stack)
        cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        cpu.emu_start(entry,stop,count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4*(1+len(args))
        return cpu.reg_read(UC_X86_REG_EAX)&255
    calls=[0,-1,0,0,0,0] # queue count/order/start, destination count/immediate, scatter
    flags=0
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);pop=0;result=0
        if address==stub:result=typ
        elif address==stub+0x10:
            calls[0]+=1;calls[1:3]=[get(sp+4),get(sp+8)];pop=8
        elif address==stub+0x20:
            calls[3]+=1;calls[4]=get(sp+8);put(obj+0x5A4,get(sp+4));pop=8
        elif address==stub+0x30:put(obj+0x2B4,get(sp+4));pop=4
        elif address==stub+0x40:
            calls[5]+=1;assert bytes(cpu.mem_read(get(sp+4),12))==bytes(12)
            assert get(sp+8)==1 and get(sp+12)==0;pop=12
        elif address==stub+0x50:result=int(bool(flags&256))
        elif address==stub+0x60:
            result=get(sp+4);cpu.mem_write(result,bytes(cpu.mem_read(this+0x9C,12)));pop=4
        elif address==stub+0x70:result=int(bool(flags&1));pop=4 # COM stdcall
        elif address==stub+0x80:result=int(bool(flags&2));pop=4 # Is_Moving
        elif address==stub+0x90:
            calls[0]+=1;assert get(sp+4)==0 and get(sp+8)==1;pop=8
        elif address==stub+0xA0:
            calls[1]=get(sp+4);assert get(sp+8)==0 and get(sp+12)==0;pop=12;result=1
        else:return
        uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    cpu.mem_write(vt,bytes(cpu.mem_read(0x7EB058,0x580)))
    for off,slot in [(0x84,0),(0x1E8,1),(0x480,2),(0x3C8,3),(0x174,4),(0x2AC,5),(0x48,6)]:put(vt+off,stub+16*slot)
    for address in range(stub,stub+0xB0,0x10):cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    put(loco,nav);put(nav+0x80,stub+0x70)
    put(0x8871E0,rules);put(0xA8B238,0);put(rules+0x1440,3)
    rows=[]
    for mission in range(-1,32):
      for seed in range(512):
        flags=(seed*4051+(mission+1)*97)&0xFFFF
        cpu.mem_write(obj,bytes(0x800));cpu.mem_write(typ,bytes(0x1000));cpu.mem_write(owner,bytes(0x300))
        cpu.mem_write(0xA8E3A8,bytes(0x400));put(obj,vt);put(other,vt);put(obj+0x6C0,typ);put(obj+0x21C,owner)
        put(obj+0xAC,mission);put(obj+0xB4,-1);put(obj+0x520,-1);put(obj+0x5C4,29 if flags&32768 else -1)
        if flags&32768:
            put(obj+0x5C8,other if flags&2 else 0);put(obj+0x5CC,other if flags&1 else 0);byte(obj+0x5D1,flags&16)
        cpu.mem_write(obj+0x578,struct.pack('<d',0.75));cpu.mem_write(obj+0x9C,struct.pack('<iii',512,768,0))
        cpu.mem_write(other+0x9C,struct.pack('<iii',1024,1280,0))
        put(obj+0x2B4,other if flags&1 else 0);put(obj+0x5A4,other if flags&2 else 0)
        for off,buf,mask in [(0x588,route,8),(0x5AC,nav,4)]:
            put(obj+off+4,buf);put(obj+off+8,8);put(obj+off+16,2 if flags&mask else 0);put(buf,other);put(buf+4,obj)
        put(obj+0x218,other if flags&16 else 0);byte(owner+0x1EC,flags&32);put(obj+0x5D4,other if flags&64 else 0)
        byte(typ+0xD39,flags&128);put(owner+0x24C,4 if flags&256 else 1)
        if mission>=0:byte(0xA8E3A8+32*mission+5,flags&512);byte(0xA8E3A8+32*mission+7,flags&1024)
        put(obj+0x2DC,other if flags&2048 else 0);byte(obj+0x687,flags&4096);byte(obj+0x6B3,flags&8192);byte(obj+0x6B1,flags&16384)
        calls[:]=[0,-1,0,0,0,0]
        result=call(0x51CBA0,obj,0,1)
        out=[result,*calls,int(get(obj+0x5A4)!=0),int(get(obj+0x218)!=0),get(obj+0x598),get(obj+0x5BC),
             int(get(nav)==obj),cpu.mem_read(obj+0x6B3,1)[0],cpu.mem_read(obj+0x687,1)[0],get(obj+0x5C4),
             int(struct.unpack('<d',cpu.mem_read(obj+0x578,8))[0]*100)]
        rows.append(' '.join(map(str,[0,mission,flags,*out])))
    for mission in (-1,1,2,5,6,8,15,21,28):
      for sequence in range(-1,42):
       for flags in range(16):
        cpu.mem_write(obj,bytes(0x800));put(obj,vt);put(obj+0xAC,mission);put(obj+0xB4,-1);put(obj+0x6C4,sequence)
        put(obj+0x674,loco);put(nav+0x80,stub+0x70);byte(obj+0x68D,flags&2);byte(obj+0x8D,flags&4)
        put(obj+0x2B4,other if flags&8 else 0)
        rows.append(' '.join(map(str,[1,mission,flags,sequence,call(0x521B60,obj)])))
    put(0xA8B230,scenario);put(vt+0x484,stub+0x90);put(vt+0x558,stub+0xA0);put(nav+0x10,stub+0x80)
    cpu.mem_write(0xA8E3A8+32*2+16,struct.pack('<d',0.016));put(seq+31*36+4,17)
    for sequence in range(-1,42):
      for flags in range(32):
        cpu.mem_write(obj,bytes(0x800));cpu.mem_write(typ,bytes(0x1000));cpu.mem_write(owner,bytes(0x300))
        put(obj,vt);put(obj+0x6C0,typ);put(obj+0x21C,owner);put(obj+0xAC,2);put(obj+0xB4,5 if flags&4 else -1)
        put(obj+0x674,loco);put(obj+0x6C4,sequence);put(obj+0x5A4,other if flags&1 else 0)
        byte(owner+0x1EC,flags&8);put(typ+0x6C4,0 if flags&16 else -1);put(typ+0xE3C,seq)
        call(0x65C6D0,scenario+0x218,99);calls[:]=[0,-1,0,0,0,0]
        result=call(0x51F660,obj)
        rows.append(' '.join(map(str,[2,sequence,flags,result,calls[0],calls[1],calls[3],calls[4],int(get(obj+0x5A4)!=0),get(scenario+0x21C)])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original infantry idle/readiness cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
