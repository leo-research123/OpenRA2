#!/usr/bin/env python3
"""Execute YR Foot.Per_Cell_Process and its Techno base, Cell/House threat math.

Map lookups/bounds/building lookup and virtual sensor/weapon/posture callbacks
are fixtures. Tags, planning paths, bridges and cloaked-neighbor scans are absent.
Compare real adjacency migration, threat maps, path cancellation and callback
counts. This is an arrival contract test, not complete Infantry or frame execution.
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
NEIGHBORS=((0,-1),(1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1))

def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    cpu.mem_map(0x1000000,0x100000)
    obj,other,typ,vt,stub,owner,enemy,house_array,cells,stack,stop=[0x1000000+n for n in
        (0,0x1000,0x2000,0x4000,0x5000,0x10000,0x30000,0x50000,0x60000,0xF0000,0xFF000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def byte(a,v):cpu.mem_write(a,bytes([int(bool(v))]))
    def coord(a,x,y,z):cpu.mem_write(a,struct.pack('<iii',x,y,z))
    def tile(x,y):
        assert 0<=x<32 and 0<=y<32
        return cells+(y*32+x)*0x148
    calls=[0]*8
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);pop=0;result=0
        if address in (0x5657A0,0x565730):
            arg=get(sp+4)
            x,y=struct.unpack('<hh',cpu.mem_read(arg,4)) if address==0x5657A0 else (get(arg)//256,get(arg+4)//256)
            result=tile(x,y);pop=4
        elif address==0x578460:result=1;pop=8
        elif address==0x47C520:pass
        elif address==stub:result=typ
        elif address==stub+0x10:calls[0]+=1;pop=4
        elif address==stub+0x20:calls[1]+=1;pop=4
        elif address==stub+0x30:calls[2]+=1;result=1;pop=4
        elif address==stub+0x40:
            calls[3]+=1;result=get(sp+4);coord(result,9*256+128,6*256+128,0);pop=4
        elif address==stub+0x50:calls[4]+=1;result=1;pop=8
        elif address==stub+0x60:calls[5]+=1;result=300;pop=4
        elif address==stub+0x70:calls[6]+=1;put(obj+0x5A4,0);pop=8
        elif address==stub+0x80:calls[7]+=1
        elif address==stub+0x90:result=24
        else:return
        uc.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for address in (0x5657A0,0x565730,0x578460,0x47C520,*range(stub,stub+0xA0,0x10)):
        cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    for offset,n in ((0x84,0),(0x4EC,1),(0x4E8,2),(0x2E4,3),(0x4F0,4),(0x164,5),(0x168,6),(0x480,7),(0x420,8),(0x2C0,9)):put(vt+offset,stub+n*0x10)
    for offset,address in ((0x1B8,0x41BEA0),(0x1BC,0x5F6960),(0x38,0x6F9DB0),(0x184,0x5B3040)):put(vt+offset,address)
    # GetCoords is a known abstract virtual copy helper; record fixture avoids unrelated identity setup.
    def coords(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);out=get(sp+4)
        cpu.mem_write(out,bytes(cpu.mem_read(this+0x9C,12)));uc.reg_write(UC_X86_REG_EAX,out)
        uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+8)
    put(vt+0x48,stub+0xA0);cpu.hook_add(UC_HOOK_CODE,coords,begin=stub+0xA0,end=stub+0xA0)
    put(0xA8022C,house_array);put(0xA80238,2);put(house_array,owner);put(house_array+4,enemy)
    put(owner+0x30,0);put(enemy+0x30,1);put(owner+0x5788,1);put(enemy+0x5788,0)
    put(0x8B41B8,0)
    # The PE image contains pre-initialization (-1,-1) entries. Run the real
    # CRT initializer, rather than treating those bytes as the live direction table.
    put(stack,stop);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.emu_start(0x49F2F0,stop,count=1000)
    for y in range(32):
        for x in range(32):put(tile(x,y)+0x24,x+(y<<16));put(tile(x,y)+0x44,-1)
    rows=[]
    for reason in range(4):
      for flags in range(256):
        cpu.mem_write(obj,bytes(0x800));cpu.mem_write(typ,bytes(0x1000));put(obj,vt);put(other,vt)
        put(obj+0x21C,owner);put(typ+0x5F0,2 if flags&4 else 0);byte(typ+0x5E4,flags&128)
        coord(obj+0x9C,8*256+128,6*256+128,0);coord(other+0x9C,9*256+128,6*256+128,0)
        put(obj+0x55C,(6 if flags&2 else 8)+(6<<16) if flags&1 else 0);put(obj+0x508,9)
        put(obj+0x2B4,other if flags&8 else 0);put(other+0x14,7);put(obj+0xAC,1 if flags&16 else 2)
        put(obj+0x598,1 if flags&32 else 0);put(obj+0x5BC,1);put(obj+0x5A4,other);put(obj+0x5E0,3)
        byte(obj+0x90,not(flags&64));byte(obj+0x6B0,1);byte(obj+0x6B2,1);byte(obj+0x41B,1);put(obj+0x520,-1)
        for y in range(32):
            for x in range(32):cpu.mem_write(tile(x,y)+0x122,b'\x07')
        cpu.mem_write(enemy+0x57E4,struct.pack('<I',200)*(130*130));calls[:]=[0]*8
        put(stack,stop);put(stack+4,reason);cpu.reg_write(UC_X86_REG_ECX,obj);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        cpu.emu_start(0x4D85D0,stop,count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+8
        old=(6 if flags&2 else 8,6)
        out=[get(obj+0x55C),get(obj+0x508),get(obj+0x5E0),int(get(obj+0x5A4)!=0),
             *[cpu.mem_read(obj+o,1)[0] for o in (0x3D5,0x6B0,0x6B2)],*calls]
        for x,y in (old,(8,6)):out += [cpu.mem_read(tile(x+dx,y+dy)+0x122,1)[0] for dx,dy in NEIGHBORS]
        for x,y in (old,(8,6)):
            region=x//4+130*(y//4)+131
            out += [get(enemy+0x57E4+4*(region+d)) for d in (-131,-130,-129,-1,0,1,129,130,131)]
        rows.append(' '.join(map(str,[reason,flags,*out])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Foot arrival cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
