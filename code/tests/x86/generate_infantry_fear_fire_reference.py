#!/usr/bin/env python3
"""Original YR fear/posture and firing-frame decisions, not projectile/damage execution.

PlayAnim, firing permission, weapon choice, Fire, Scatter and Uncloak are callbacks.
Real direction math, Facing, CombatDamage and Walk COM query/refcounts execute;
IPersist class identity is a fixture. Incoming-scatter is excluded (weapon speed
1000, Incoming=0); positive effects and full Infantry.Update remain unverified.
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
    cpu.mem_map(0x1000000,0x50000)
    obj,other,typ,seq,vt,stub,loco,owner,rules,weapon,slot,stack,stop=[0x1000000+n for n in
        (0,0x1000,0x2000,0x4000,0x6000,0x7000,0x8000,0xA000,0x20000,0x28000,0x29000,0x40000,0x4F000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def byte(a,v):cpu.mem_write(a,bytes([int(bool(v))]))
    def coord(a,x,y,z):cpu.mem_write(a,struct.pack('<iii',x,y,z))
    def call(entry,this):
        put(stack,stop);cpu.reg_write(UC_X86_REG_ECX,this);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        cpu.emu_start(entry,stop,count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4
    calls=[0,-1,0,0,0,0,0,0] # play,last,scatter,permission,fire,uncloak,stop,set-target
    mode=flags=error=0
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);pop=0;result=0
        if address==stub:
            calls[0]+=1;calls[1]=get(sp+4);pop=12
            assert get(sp+8)==0 and get(sp+12)==0
            if mode==0 or not flags&16384:put(obj+0x6C4,calls[1]);put(obj+0xF8,0);result=1
        elif address==stub+0x10:
            calls[2]+=1;pop=12;assert get(sp+8)==1 and get(sp+12)==0
        elif address==stub+0x20:result=int(bool(flags&32))
        elif address==stub+0x30:result=int(bool(flags&4));pop=4
        elif address==stub+0x40:
            calls[3]+=1;result=error if calls[3]==1 or flags&8 else 0;pop=12
        elif address==stub+0x50:
            calls[4]+=1;pop=8
            if flags&8192:put(obj+0x2B4,0)
        elif address==stub+0x60:calls[5]+=1;pop=4;assert get(sp+4)==0
        elif address==stub+0x70:calls[6]+=1
        elif address==stub+0x80:calls[7]+=1;put(obj+0x2B4,get(sp+4));pop=4
        elif address==stub+0x90:result=slot;pop=4
        elif address==stub+0xA0:result=typ
        elif address==stub+0xB0:
            result=get(sp+4);uc.mem_write(result,bytes(uc.mem_read(this+0x9C,12)));pop=4
        elif address==stub+0xC0:result=15
        elif address==stub+0xD0:
            uc.mem_write(get(sp+8),bytes(uc.mem_read(0x7E9AC0 if flags&1024 else 0x7E9A60,16)));pop=8
        elif address in (stub+0xE0,stub+0xF0):
            pointer=get(sp+4);result=get(pointer)+(1 if address==stub+0xE0 else -1);put(pointer,result);pop=4
        else:return
        uc.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF);uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for address in range(stub,stub+0x100,16):cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    for descriptor in pe.DIRECTORY_ENTRY_IMPORT:
        for imported in descriptor.imports:
            if imported.name==b'InterlockedIncrement':put(imported.address,stub+0xE0)
            if imported.name==b'InterlockedDecrement':put(imported.address,stub+0xF0)
    for off,i in [(0x558,0),(0x174,1),(0x2AC,2),(0x2E4,3),(0x3C0,4),(0x3CC,5),(0x45C,6),(0x500,7),
                  (0x3C8,8),(0x3F8,9),(0x84,10),(0x88,10),(0x48,11),(0x2C,12)]:put(vt+off,stub+16*i)
    put(0x8871E0,rules);put(0xA8B238,0);put(0xA8ED84,500);put(typ+0xA0,100)
    cpu.mem_write(rules+0x16F8,struct.pack('<d',0.75));put(slot,weapon);put(weapon+0xA8,1000)
    call(0x75AA50,0);call(0x55A680,0)
    def reset(sequence):
        cpu.mem_write(obj,bytes(0x800));cpu.mem_write(other,bytes(0x800));cpu.mem_write(seq,bytes(0x600))
        cpu.mem_write(typ,bytes(0x1000));cpu.mem_write(owner,bytes(0x300))
        put(obj,vt);put(other,vt);put(obj+0x21C,owner);put(obj+0x6C0,typ);put(typ+0xE3C,seq);put(typ+0xA0,100)
        put(obj+0x6C4,sequence);put(other+0x14,2);put(other+0x6C,100 if flags&4096 else 10)
        coord(obj+0x9C,8*256+192,6*256+64,0);coord(other+0x9C,11*256+128,9*256+128,0)
        call(0x75AA90,loco);put(loco+0x14,1);put(obj+0x674,loco+4);put(get(loco)+12,stub+0xD0)
        cpu.mem_write(obj+0x388,struct.pack('<H',0x1234));cpu.mem_write(obj+0x38C,struct.pack('<H',0x1234))
        put(obj+0x390,500);put(obj+0x398,0);cpu.mem_write(obj+0x39C,struct.pack('<H',0x7F00))
        calls[:]=[0,-1,0,0,0,0,0,0]
    rows=[]
    for sequence in (-1,0,2,27,28,29,30,40,41):
      for panic in (-1,0,1,49,50,51,200):
       for flags in range(512):
        reset(sequence);put(obj+0x6D4,panic);byte(typ+0xEBC,flags&1);byte(obj+0x6DB,flags&2)
        byte(typ+0xEBF,flags&4);byte(owner+0x1EC,flags&8);put(obj+0x5A4,other if flags&16 else 0)
        put(obj+0x2FC,1 if flags&64 else 0);put(typ+0x684,7);byte(loco+52,flags&128);byte(obj+0x8D,flags&256)
        call(0x5200B0,obj)
        rows.append(' '.join(map(str,[0,sequence,panic,flags,get(obj+0x6D4),get(obj+0x2FC),get(obj+0x6C4),calls[0],calls[1],calls[2]])))
    mode=1
    for sequence in (-1,0,2,27,28,29,30,40,41):
      for error in (0,2,5,9):
       for seed in range(512):
        flags=(seed*4051+sequence*97)&0x7FFF;reset(sequence)
        put(obj+0x2B4,0 if flags&1 else other);byte(obj+0x68D,flags&2);byte(obj+0x6DB,flags&16)
        for i in range(4):put(typ+0xE40+4*i,3+i)
        put(seq+40*36+4,5 if flags&32 else 0);put(seq+41*36+4,5 if flags&64 else 0)
        put(obj+0x5A4,other if flags&128 else 0);put(obj+0x294,other if flags&256 else 0);byte(typ+0xD94,flags&512)
        put(obj+0xF8,(flags>>8)%7);put(weapon+0xA4,-10 if flags&2048 else 10)
        call(0x5206B0,obj)
        out=[get(obj+0x6C4),get(obj+0xF8),cpu.mem_read(obj+0x68D,1)[0],int(get(obj+0x2B4)!=0),int(get(obj+0x5A4)!=0),
             struct.unpack('<H',cpu.mem_read(obj+0x388,2))[0],get(obj+0x390),get(obj+0x398),get(loco+0x14),*calls]
        rows.append(' '.join(map(str,[1,sequence,error,flags,*out])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original fear/firing cases')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
