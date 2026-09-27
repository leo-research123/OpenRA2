#!/usr/bin/env python3
"""Execute YR flaming AI, destination search and cell eligibility instructions.

Map storage, flat floor lookup, object coordinate access and deferred deletion
are fixtures. Original direction/math, RNG, search, movement, retry and frame
decisions execute unchanged. The harness advances the death stage at rate 1;
generic Object falling and generic Anim Update are tested separately.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_FPCW

SHA = '7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
ADJ = ((0,-1),(1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1))

def generate(exe, output, report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest() == SHA
    pe=pefile.PE(str(exe)); cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    cpu.mem_map(0x400000,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF)
    cpu.mem_write(0x400000,pe.get_memory_mapped_image());cpu.mem_map(0x1000000,0x400000)
    obj,typ,vt,stub,scenario,shape,stack,stop=[0x1000000+n for n in (0,0x1000,0x2000,0x3000,0x4000,0x8000,0xF000,0xFF00)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def coord(a,v):cpu.mem_write(a,struct.pack('<iii',*v))
    def xyz(a):return list(struct.unpack('<iii',cpu.mem_read(a,12)))
    def byte(a,v):cpu.mem_write(a,bytes([v]))
    def call(entry,this=0,*args):
        cpu.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*args))
        cpu.reg_write(UC_X86_REG_ECX,this);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        cpu.emu_start(entry,stop,count=1000000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4*(len(args)+1)
    cells={};dead=False
    def cell(x,y):
        if (x,y) not in cells:
            at=0x1100000+len(cells)*0x200;cells[x,y]=at
            cpu.mem_write(at,bytes(0x200));cpu.mem_write(at+0x24,struct.pack('<hh',x,y));put(at+0x44,-1)
        return cells[x,y]
    def at_coord(a):
        x,y,_=xyz(a);return cell(x//256,y//256)
    def boundary(uc,address,size,data):
        nonlocal dead
        sp=uc.reg_read(UC_X86_REG_ESP);pop=0;result=0
        if address==0x565730:result=at_coord(get(sp+4));pop=4
        elif address==0x5657A0:result=cell(*struct.unpack('<hh',cpu.mem_read(get(sp+4),4)));pop=4
        elif address==0x578080:result=cpu.mem_read(at_coord(get(sp+4))+0x11B,1)[0]*104;pop=4
        elif address==0x578460:result=1;pop=8 # all scanned cells are inside the fixture map
        elif address==0x481810:
            at=uc.reg_read(UC_X86_REG_ECX);x,y=struct.unpack('<hh',cpu.mem_read(at+0x24,4));dx,dy=ADJ[get(sp+4)]
            result=cell(x+dx,y+dy);pop=4
        elif address==stub:result=get(sp+4);coord(result,xyz(obj+0x9C));pop=4
        elif address==stub+0x10:coord(obj+0x9C,xyz(get(sp+4)));pop=4
        elif address==stub+0x20:result=get(obj+0xA4)-cpu.mem_read(at_coord(obj+0x9C)+0x11B,1)[0]*104
        elif address==stub+0x30:result=shape
        elif address==stub+0x40:dead=True
        else:return
        uc.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF);uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF);uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for address in (0x565730,0x5657A0,0x578080,0x578460,0x481810,*range(stub,stub+0x50,0x10)):
        cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    for offset,index in ((0x48,0),(0x1B4,1),(0x1C8,2),(0x6C,3),(0xF8,4)):put(vt+offset,stub+index*0x10)
    call(0x421E50);call(0x421E60)
    put(0x89A1C0,104);put(0x89A1B4,416);put(0xA8B230,scenario)
    for i,xy in enumerate(ADJ):cpu.mem_write(0x89F688+i*4,struct.pack('<hh',*xy))
    cpu.mem_write(shape,struct.pack('<hhhh',0,1,1,124));put(typ+0x350,6)
    rows=[]
    for mode in range(10):
        cells.clear();dead=False;cpu.mem_write(obj,bytes(0x200));put(obj,vt);put(obj+0xC8,typ)
        coord(obj+0x9C,(40*256+128,40*256+128,416 if mode in (4,5) else 0))
        call(0x65C6D0,scenario+0x218,12345+mode)
        if mode in (1,7,8):put(cell(42,40)+0xEC,2)
        if mode==2:
            for dx,dy in ADJ:put(cell(40+dx,40+dy)+0x124,0xE0)
        if mode==3:put(cell(41,40)+0xEC,6)
        if mode in (3,4,5,6):coord(obj+0x108,(41*256+128,40*256+128,0))
        if mode==4:
            for x in range(39,44):put(cell(x,40)+0x140,0x100)
        if mode==5:byte(obj+0x8D,1)
        if mode==6:put(obj+0x114,7)
        if mode==7:put(cell(41,40)+0x124,0xE0)
        if mode==8:put(cell(42,40)+0x140,0x100)
        if mode==9:
            for dx,dy in ADJ:byte(cell(40+dx,40+dy)+0x11B,3)
        for frame in range(200):
            put(0xA8ED84,frame);call(0x425670,obj)
            rows.append([mode,frame,*xyz(obj+0x9C),*xyz(obj+0x108),get(obj+0x114),get(obj+0xAC),get(obj+0xC0),
                         cpu.mem_read(obj+0x19A,1)[0],cpu.mem_read(obj+0x8D,1)[0],int(dead),get(scenario+0x21C),get(scenario+0x220)])
            if dead:break
            if cpu.mem_read(obj+0x19A,1)[0]:put(obj+0xAC,get(obj+0xAC)+1)
    output.write_text('\n'.join(' '.join(map(str,row)) for row in rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'rows':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original flaming animation states')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
