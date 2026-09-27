#!/usr/bin/env python3
"""Execute YR Infantry -> Foot -> Techno arrival for non-building destinations.

Idle/mission/sight/damage/radio/sensor/drop callbacks and map queries are sinks;
Map.RevealArea3 marks the center to record invocation (its algorithm is tested
separately). No garrison, engineer interaction, transport attachment or C4 target
is supplied. This does not validate their unported effects or a full game frame.
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
    pe=pefile.PE(str(exe));u=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    u.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);u.mem_write(base,pe.get_memory_mapped_image())
    u.mem_map(0x1000000,0x100000)
    obj,other,typ,vt,stub,owner,cells,loco,stack,stop=[0x1000000+n for n in (0,0x1000,0x2000,0x4000,0x5000,0x10000,0x30000,0x40000,0xF0000,0xFF000)]
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
    def byte(a,v):u.mem_write(a,bytes([v&255]))
    def coord(a,x,y,z=0):u.mem_write(a,struct.pack('<iii',x,y,z))
    def tile(x,y):
        assert (x,y) in ((8,6),(9,6))
        return cells+(x-8)*0x148
    calls=[0]*9
    moving=False
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);pop=0;result=0
        if address in (0x5657A0,0x565730):
            arg=get(sp+4)
            x,y=struct.unpack('<hh',uc.mem_read(arg,4)) if address==0x5657A0 else (get(arg)//256,get(arg+4)//256)
            result=tile(x,y);pop=4
        elif address==0x578460:result=1;pop=8
        elif address==0x567DA0:byte(cells+0x120,254);pop=16
        elif address==0x47C520:pass
        elif address==stub:result=typ
        elif address==stub+0x10:calls[0]+=1;result=1;pop=8
        elif address==stub+0x20:calls[1]+=1;result=1
        elif address==stub+0x30:calls[2]+=1;pop=16
        elif address==stub+0x40:calls[3]+=1;pop=20
        elif address==stub+0x50:
            assert get(get(sp+4))==125 and get(sp+20)==1 and get(sp+24)==0
            calls[4]+=1;pop=28
        elif address==stub+0x60:calls[5]+=1
        elif address==stub+0x70:calls[6]+=1;pop=4
        elif address==stub+0x80:calls[7]+=1;result=1;pop=4
        elif address==stub+0x90:calls[8]+=1
        elif address==stub+0xA0:result=int(moving);pop=4
        elif address==stub+0xB0:
            result=get(sp+4);uc.mem_write(result,bytes(uc.mem_read(this+0x9C,12)));pop=4
        elif address==stub+0xC0:pop=8
        elif address==stub+0xD0:
            result=get(sp+4);coord(result,9*256+128,6*256+128);pop=4
        elif address==stub+0xE0:result=11
        else:return
        uc.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for address in (0x5657A0,0x565730,0x578460,0x47C520,0x567DA0,*range(stub,stub+0xF0,0x10)):
        u.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    for offset,n in ((0x84,0),(0x484,1),(0x1EC,2),(0x48C,3),(0x488,4),(0x16C,5),(0x420,6),(0x2E4,7),(0x274,8),(0xEC,9),(0x48,11),(0x164,12),(0x4F0,13),(0x2C,14)):
        put(vt+offset,stub+n*0x10)
    for offset,address in ((0x1B8,0x41BEA0),(0x1BC,0x5F6960),(0x38,0x6F9DB0),(0x184,0x5B3040)):
        put(vt+offset,address)
    put(loco,loco+0x100);put(loco+0x110,stub+0xA0)
    put(cells,vt);put(cells+0x148,vt);put(cells+0x24,8+(6<<16));put(cells+0x148+0x24,9+(6<<16))
    coord(cells+0x148+0x9C,9*256+128,6*256+128)
    put(owner+0x30,0);put(0x8B41B8,0);byte(0xA8E9A0,1);put(0x8871E0,0x1050000)
    rows=[]
    for reason in range(4):
      for mission in (0,2,5,7,8,11,17,25):
       for variant in range(4):
        for flags in range(256):
            u.mem_write(obj,bytes(0x800));u.mem_write(typ,bytes(0x1000));put(obj,vt);put(other,vt)
            put(obj+0x21C,owner);put(obj+0x6C0,typ);put(obj+0x674,loco);put(obj+0x6C,125)
            coord(obj+0x9C,8*256+128,6*256+128);coord(other+0x9C,9*256+128,6*256+128)
            put(obj+0xAC,mission);put(obj+0xB4,5 if flags&4 else -1)
            put(obj+0x5A4,cells+0x148 if flags&1 else 0);put(obj+0x2B4,other if flags&2 else 0);put(other+0x14,7)
            byte(obj+0x90,int(not(flags&8)));byte(typ+0xEC2,bool(flags&16));moving=bool(flags&32)
            byte(obj+0x8C,bool(flags&64));byte(obj+0x418,bool(flags&128))
            byte(obj+0x6B0,1);byte(obj+0x6B2,1);byte(obj+0x41B,1);put(obj+0x520,-1)
            put(cells+0xEC,(0,2,3,2)[variant]);put(cells+0x140,0x100 if variant==3 else 0);byte(cells+0x120,255)
            calls[:]=[0]*9
            put(stack,stop);put(stack+4,reason);u.reg_write(UC_X86_REG_ECX,obj);u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_FPCW,0x0E7F)
            try:u.emu_start(0x519630,stop,count=10000)
            except Exception:
                print('case',reason,mission,variant,flags,'pc',hex(u.reg_read(UC_X86_REG_EIP)));raise
            assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==stack+8
            state=[*calls,*[u.mem_read(obj+o,1)[0] for o in (0x3D5,0x6B0,0x6B2)],struct.unpack('<b',u.mem_read(cells+0x120,1))[0]]
            rows.append(' '.join(map(str,[reason,mission,variant,flags,*state])))
    output.write_text('\n'.join(rows)+'\n');report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Infantry arrival cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
