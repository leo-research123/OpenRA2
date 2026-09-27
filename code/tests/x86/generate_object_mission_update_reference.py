#!/usr/bin/env python3
"""YR Object.Update and Mission.Update instruction corpora (not Techno/Infantry frames).

Object height, Mark, arrival and damage are explicit callbacks; original layer
Submit/Remove execute with fixed-capacity vectors and a FindItemIndex callback.
Sounds are disabled and no flaming animation object is used. Mission
handlers record the selected slot and can change frame/mission/health; the real
Object.Update runs first, including a falling-arrival callback in selected cases.
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
    cpu.mem_map(0x1000000,0x20000)
    obj,vt,stub,rules,parachute,stack,stop=[0x1000000+n for n in (0,0x1000,0x2000,0x4000,0x8000,0x10000,0x1F000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def byte(a,v):cpu.mem_write(a,bytes([int(bool(v))]))
    def call(entry):
        put(stack,stop);cpu.reg_write(UC_X86_REG_ECX,obj);cpu.reg_write(UC_X86_REG_ESP,stack)
        cpu.reg_write(UC_X86_REG_FPCW,0x0E7F);cpu.emu_start(entry,stop,count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4
    calls=[0]*7 # mark count/encoded order, arrival, damage, submit, remove, dispatch slot
    mode=flags=0;delay=0
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);pop=0;result=0
        if address==stub:pass # no ObjectType, ambient disabled
        elif address==stub+0x10:result=3 if cpu.mem_read(obj+0x8D,1)[0] else 2
        elif address==stub+0x20:result=get(obj+0xA4)
        elif address==stub+0x30:put(obj+0xA4,get(sp+4));pop=4
        elif address==stub+0x40:
            calls[0]+=1;calls[1]=calls[1]*4+get(sp+4)+1;pop=4;result=1
        elif address==stub+0x50:
            calls[2]+=1;assert get(sp+4)==2;pop=4
            if mode==0:
                if flags&32:byte(obj+0x81,1)
                if flags&128:byte(obj+0x90,0)
            else:
                if flags&16:byte(obj+0x90,0)
                if flags&32:put(obj+0x6C,0)
                if flags&64:put(obj+0xAC,2)
        elif address==stub+0x60:
            calls[3]+=1;pop=28
            assert get(get(sp+4))==get(obj+0x6C) and get(sp+8)==0 and get(sp+20)==1 and get(sp+24)==1
        elif address==stub+0x70:result=15
        elif address==0x4A9720:calls[4]+=1;return # execute original Submit/Remove
        elif address==0x4A9770:calls[5]+=1;return
        elif address==stub+0x80:
            vector=uc.reg_read(UC_X86_REG_ECX);needle=get(get(sp+4));pop=4;result=-1
            for i in range(get(vector+16)):
                if get(get(vector+4)+4*i)==needle:result=i;break
        elif stub+0x100<=address<stub+0x2C0:
            calls[6]=(address-stub-0x100)//16
            result=delay
            if flags&128:put(0xA8ED84,get(0xA8ED84)+7);put(obj+0xAC,0);put(obj+0x6C,0)
        else:return
        uc.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF);uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for off,slot in [(0x88,0),(0x78,1),(0x1C8,2),(0x1CC,3),(0x124,4),(0x18C,5),(0x16C,6),(0x2C,7),(0x1D0,2)]:put(vt+off,stub+slot*16)
    for i in range(28):put(vt+0x204+4*i,stub+0x100+16*i)
    for address in [*range(stub,stub+0x2C0,16),0x4A9720,0x4A9770]:cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    put(0x8871E0,rules);put(rules+494*4,-10);put(rules+495*4,-30)
    put(vt+0x600+16,stub+0x80)
    def reset():
        cpu.mem_write(obj,bytes(0x800));put(obj,vt);put(obj+0x64,-1);put(obj+0x6C,25);put(obj+0x88,parachute)
        byte(obj+0x90,1);byte(parachute+0x195,1);calls[:]=[0]*6+[-1]
        put(obj+0x94,-1)
        for i in range(5):
            vector=0x8A0360+24*i;put(vector,vt+0x600);put(vector+4,parachute+0x800+0x100*i)
            put(vector+8,16);put(vector+16,0)
    rows=[]
    for z in (-1,0,1,16,31,256):
      for rate in (-40,-30,-10,-1,0,2,15):
       for flags in range(256):
        reset();put(obj+0xA4,z);put(obj+0x2C,rate);byte(obj+0x8D,flags&1);byte(obj+0x81,flags&2)
        byte(obj+0x74,flags&4);byte(obj+0x84,flags&8);byte(obj+0x8F,flags&16);put(obj+0x6C,0 if flags&64 else 25)
        put(obj+0x94,3);put(0x8A0360+3*24+16,1);put(get(0x8A0360+3*24+4),obj);call(0x5F3E70)
        out=[get(obj+0xA4),get(obj+0x2C),cpu.mem_read(obj+0x8D,1)[0],cpu.mem_read(obj+0x81,1)[0],
             cpu.mem_read(obj+0x90,1)[0],cpu.mem_read(parachute+0x195,1)[0],get(obj+0x94),*calls[:4],
             get(0x8A0360+2*24+16),get(0x8A0360+3*24+16)]
        rows.append(' '.join(map(str,[0,z,rate,flags,*out])))
    mode=1
    timers=[(-1,0,500),(-1,3,500),(-1,-2,500),(490,10,500),(490,11,500),(490,0,500),
            (510,2,500),(0x7FFFFFF0,32,-0x7FFFFFF0),(0x7FFFFFF0,33,-0x7FFFFFF0)]
    for mission in range(-2,34):
      for timer,(start,left,frame) in enumerate(timers):
       for flags in range(256):
        reset();byte(obj+0x90,flags&1);put(obj+0x6C,25 if flags&2 else 0);byte(obj+0x8D,flags&4)
        byte(obj+0x81,flags&8);put(obj+0xAC,mission);put(obj+0xC8,start);put(obj+0xD0,left)
        put(0xA8ED84,frame);delay=(-3,0,1,450)[(flags>>6)&3];call(0x5B3060)
        out=[calls[2],calls[6],get(obj+0xC8),get(obj+0xD0),get(obj+0xAC),get(obj+0x6C),get(0xA8ED84)]
        rows.append(' '.join(map(str,[1,mission,timer,flags,*out])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Object/Mission frame cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
