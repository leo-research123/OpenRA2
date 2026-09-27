#!/usr/bin/env python3
"""YR Doing_AI / Movement_AI state decisions, not the whole Infantry Update.

PlayAnim, idle-entry, destination-setting, UnInit and passability callbacks are
recording fixtures. Original COM Walk identity, Facing and targeting-delay RNG
execute. Death cases use NotHuman with no DeadBodies; sound controls are empty.
Map lookup is a fixture and same-zone returns true (native uses MZone::None).
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
    obj,other,typ,seq,vt,stub,cell,loco,scenario,stack,stop=[0x1000000+n for n in
        (0,0x1000,0x2000,0x4000,0x6000,0x8000,0x9000,0xA000,0x10000,0x30000,0x3F000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def byte(a,v):cpu.mem_write(a,bytes([int(bool(v))]))
    def coord(a,x,y,z):cpu.mem_write(a,struct.pack('<iii',x,y,z))
    def call(entry,this,*args):
        cpu.mem_write(stack,struct.pack('<'+'I'*(1+len(args)),stop,*[v&0xFFFFFFFF for v in args]))
        cpu.reg_write(UC_X86_REG_ECX,this);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        try:cpu.emu_start(entry,stop,count=100000)
        except Exception:
            print('original execution failed',hex(entry),'at',hex(cpu.reg_read(UC_X86_REG_EIP)),
                  'flags',flags,flush=True)
            raise
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4*(1+len(args))
    calls=[0,-1,0,0,0,0,0,0] # play, action, force, random, idle, destination, uninit, docker
    flags=0
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);pop=0;result=0
        if address==0x5657A0:result=cell;pop=4
        elif address==0x56D100:result=1;pop=24
        elif address==stub:
            calls[0]+=1;calls[1:4]=[get(sp+4),get(sp+8),get(sp+12)];pop=12
            put(obj+0x6C4,calls[1]);put(obj+0xF8,0);result=1
        elif address==stub+0x10:calls[6]+=1
        elif address==stub+0x20:result=get(obj+0xAC)
        elif address==stub+0x30:
            result=get(sp+4);coord(result,8*256+192,6*256+64,0);pop=8
            if this==other:calls[7]=int(get(sp+8)==obj)
        elif address==stub+0x40:
            calls[4]+=1;assert get(sp+4)==0 and get(sp+8)==1;pop=8
        elif address==stub+0x50:
            calls[5]+=1;put(obj+0x5A4,get(sp+4));pop=8
        elif address==stub+0x60:result=7 if flags&256 else 0;pop=20
        elif address==stub+0x70:pop=4 # IsOnBridge(nullptr)
        elif address==stub+0x80:pass # leave-map permission
        elif address==stub+0x90: # IPersist::GetClassID stdcall
            cpu.mem_write(get(sp+8),bytes(cpu.mem_read(0x7E9AC0 if flags&16 else 0x7E9A60,16)));pop=8
        elif address in (stub+0xA0,stub+0xB0): # Windows Interlocked calls
            pointer=get(sp+4);result=get(pointer)+(1 if address==stub+0xA0 else -1)
            put(pointer,result);pop=4
        else:return
        uc.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF);uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for descriptor in pe.DIRECTORY_ENTRY_IMPORT:
        for imported in descriptor.imports:
            if imported.name==b'InterlockedIncrement':put(imported.address,stub+0xA0)
            if imported.name==b'InterlockedDecrement':put(imported.address,stub+0xB0)
    for address in (0x5657A0,0x56D100,*range(stub,stub+0xC0,0x10)):
        cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    for offset,slot in [(0x558,0),(0xF8,1),(0x184,2),(0x4C,3),(0x484,4),(0x480,5),(0x1AC,6),(0xBC,7),(0x320,8)]:put(vt+offset,stub+slot*0x10)
    put(vt+0x544,0x4D3710);put(vt+0x1C4,0x5F6A10)
    put(0xA8ED84,500);put(0xA8B230,scenario);put(0xA8E7AC,0)
    call(0x75AA50,0);call(0x55A680,0)
    rows=[]
    for mode in (0,1):
      sequences=range(-1,42) if mode==0 else (-1,0,2,3,6,17,23,24,27,28,33,38,40,41)
      for sequence in sequences:
       for phase in ((4,5) if mode==0 else (4,)):
        for flags in range(64 if mode==0 else 512):
            cpu.mem_write(obj,bytes(0x800));cpu.mem_write(typ,bytes(0x1000));cpu.mem_write(seq,bytes(0x600))
            put(obj,vt);put(other,vt);put(obj+0x6C0,typ);put(typ+0xE3C,seq)
            put(obj+0x6C4,sequence);put(obj+0xF8,phase);byte(obj+0x2A4,1)
            for i in range(42):put(seq+36*i+4,5);put(seq+36*i+12,2)
            byte(typ+0xEAD,1);byte(typ+0xEC9,flags&16);put(typ+0x5B4,-1)
            put(obj+0x180,-1);put(obj+0x188,50);call(0x65C6D0,scenario+0x218,99)
            cpu.mem_write(obj+0x388,struct.pack('<H',0x1234));cpu.mem_write(obj+0x38C,struct.pack('<H',0x1234))
            put(obj+0x390,500);put(obj+0x398,0);cpu.mem_write(obj+0x39C,struct.pack('<H',0x7F00))
            call(0x75AA90,loco);put(loco+0x14,1);put(obj+0x674,loco+4)
            put(get(loco)+12,stub+0x90) # only persist GetClassID is doubled
            coord(obj+0x9C,8*256+192,6*256+64,0);put(cell+0x140,0)
            byte(loco+52,flags&1);calls[:]=[0,-1,0,0,0,0,0,0]
            if mode==0:
                byte(obj+0x6DB,flags&2);byte(obj+0x8D,flags&8);byte(obj+0x68D,1)
                put(obj+0x294,other if flags&16 else 0);put(obj+0x2B4,other if flags&32 else 0)
                put(obj+0xAC,10 if flags&32 else 5);put(obj+0x5A4,0)
                cpu.mem_write(obj+0x578,struct.pack('<d',0.2 if flags&4 else 0.1))
                call(0x520AE0,obj)
            else:
                byte(loco+54,flags&2);byte(obj+0x6DB,flags&4);byte(typ+0xD94,flags&8)
                byte(obj+0x68D,flags&32);put(obj+0x5A4,other if flags&64 else 0)
                put(obj+0xAC,2 if flags&128 else 5);byte(obj+0x6DC,flags&256);byte(obj+0x3D5,1)
                cpu.mem_write(obj+0x578,struct.pack('<d',0.9 if flags&256 else 0.8))
                call(0x520F40,obj)
            out=[get(obj+0x6C4),get(obj+0xF8),cpu.mem_read(obj+0x2A4,1)[0],cpu.mem_read(obj+0x68D,1)[0],
                 struct.unpack('<H',cpu.mem_read(obj+0x388,2))[0],get(obj+0x180),get(obj+0x188),get(scenario+0x21C),
                 round(struct.unpack('<d',cpu.mem_read(obj+0x578,8))[0]*10),int(get(obj+0x5A4)!=0),*calls]
            rows.append(' '.join(map(str,[mode,sequence,phase,flags,*out])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Infantry frame-decision cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
