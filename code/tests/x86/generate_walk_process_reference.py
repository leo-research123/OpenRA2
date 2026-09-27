#!/usr/bin/env python3
"""Execute YR Walk Process/Movement_AI head-step and arrival branches.

Original direction, quantized math, path shift, bridge transition and Process
dispatch and real Foot.GetDestination -> Walk.Head_To_Coord execute in x86.
Foot coordinate/mark/arrival callbacks, cell lookup,
speed and zero-head occupation are explicit fixtures, not full-game execution.
"""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_FPCW

SHA = '7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'

def generate(exe, output, report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(exe)); cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF)
    cpu.mem_write(base, pe.get_memory_mapped_image()); cpu.mem_map(0x1000000, 0x10000)
    loco, foot, old, new, vt, stub, stack, stop = [0x1000000+n for n in (0,0x1000,0x3000,0x3400,0x4000,0x5000,0xD000,0xF000)]
    def put(a,v): cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a): return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def coord(a,v): cpu.mem_write(a,struct.pack('<iii',*v))
    def xyz(a): return list(struct.unpack('<iii',cpu.mem_read(a,12)))
    def call(entry,this,*args):
        cpu.mem_write(stack,struct.pack('<'+'I'*(1+len(args)),stop,*[v&0xFFFFFFFF for v in args]))
        cpu.reg_write(UC_X86_REG_ECX,this); cpu.reg_write(UC_X86_REG_ESP,stack); cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        cpu.emu_start(entry,stop,count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4*(1+len(args))
        return cpu.reg_read(UC_X86_REG_EAX)
    counters=[0]*7 # marks up/down, set coord, arrival, head cleared, stopped, destination cleared
    speed=0
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP); pop=0; result=0
        if address in (0x5657A0,0x565730):
            arg=get(sp+4)
            cell=struct.unpack('<hh',cpu.mem_read(arg,4)) if address==0x5657A0 else tuple(v//256 for v in xyz(arg)[:2])
            result=old if cell==(8,6) else new;pop=4
        elif address==0x75C240:
            assert xyz(get(sp+4))==[0,0,0]
            coord(loco+40,(0,0,0));counters[4]+=1;pop=4
        elif stub<=address<stub+0x100:
            slot=(address-stub)//0x10
            if slot in (0,1):
                result=get(sp+4);coord(result,xyz(foot+0x9C));pop=4 if slot==0 else 8
            elif slot==2:
                result=get(sp+4);x,y,_=xyz(foot+0x9C);cpu.mem_write(result,struct.pack('<hh',x//256,y//256));pop=4
            elif slot==3:
                mark=get(sp+4);assert mark in (0,1);counters[mark]+=1;cpu.mem_write(foot+0x74,bytes([mark]));result=1;pop=4
            elif slot==4:
                coord(foot+0x9C,xyz(get(sp+4)));counters[2]+=1;pop=4
            elif slot==5:
                x,y,z=xyz(foot+0x9C);tile=old if (x//256,y//256)==(8,6) else new
                level=struct.unpack('<b',cpu.mem_read(tile+0x11B,1))[0]
                put(foot+0xA4,get(sp+4)+104*(level+4*cpu.mem_read(foot+0x8C,1)[0]));pop=4
            elif slot==6: result=speed
            elif slot==7: cpu.mem_write(foot+0x578,bytes(cpu.mem_read(sp+4,8)));pop=8
            elif slot==8:
                assert get(sp+4)==2 and cpu.mem_read(loco+53,1)==b'\x01';counters[3]+=1;pop=4
            elif slot==9:
                assert get(sp+4)==0;counters[6]+=1;coord(loco+28,(0,0,0));pop=8
            elif slot==10: counters[5]+=1
            else: raise AssertionError(slot)
        else: return
        uc.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF);uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for slot,offset in enumerate((0x48,0x4C,0x1B8,0x124,0x1B4,0x1CC,0x538,0x544,0x18C,0x480,0x54C)):
        put(vt+offset,stub+slot*0x10)
    put(vt+0x4C,0x4DBDF0)
    put(vt+0x37C,0x70EFD0)
    for address in (0x5657A0,0x565730,0x75C240,*range(stub,stub+0xB0,0x10)):
        cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    call(0x75AA50,0);call(0x55A680,0);put(0xB45C28,104);put(0xA8ED84,500)
    deltas=[(0,-256),(256,-256),(256,0),(256,256),(0,256),(-256,256),(-256,0),(-256,-256),
            (71,-137),(-93,27),(0,0),(16,0),(17,0),(12,12),(9,-9)]
    rows=[]
    for variant,(dx,dy),speed,flags in itertools.product(range(2),deltas,(0,1,10,37),range(32)):
        cpu.mem_write(loco,bytes(0x3C));call(0x75AA90,loco)
        cpu.mem_write(foot,bytes(0x800));put(foot,vt);put(loco+8,foot);put(loco+12,foot);put(foot+0x674,loco+4);cpu.mem_write(foot+0x684,b"\xFF")
        x,y=8*256+(250 if variant else 128),6*256+(250 if variant else 128)
        coord(foot+0x9C,(x,y,17));coord(loco+40,(x+dx,y+dy,17))
        coord(loco+28,(0,0,0) if flags&16 else (x+dx,y+dy,17))
        cpu.mem_write(loco+52,b'\x01\x00\x00');cpu.mem_write(foot+0x74,b'\x01');cpu.mem_write(foot+0x90,b'\x01')
        cpu.mem_write(foot+0x8C,bytes([bool(flags&2)]));cpu.mem_write(foot+0x68A,b'\x01');cpu.mem_write(foot+0x6B7,b'\x01')
        put(foot+0x504,int(bool(flags&1)));put(foot+0x558,0x00060008)
        cpu.mem_write(foot+0x388,struct.pack('<H',0x1234));cpu.mem_write(foot+0x578,struct.pack('<d',0.5))
        for i in range(24):put(foot+0x5E0+4*i,2 if i<3 else -1)
        for tile in (old,new):
            cpu.mem_write(tile,bytes(0x148));put(tile+0x44,-1)
        cpu.mem_write(old+0x11B,b'\x04');cpu.mem_write(new+0x11B,bytes([0 if flags&8 else 4]))
        put(old+0x140,0x100 if flags&2 else 0);put(new+0x140,0x100 if flags&4 else 0)
        counters[:]=[0]*7
        moving=call(0x75AC80,0,loco+4)&0xFF
        state=xyz(foot+0x9C)+xyz(loco+40)+xyz(loco+28)
        state+=list(cpu.mem_read(loco+52,3))+[moving,cpu.mem_read(foot+0x74,1)[0],cpu.mem_read(foot+0x8C,1)[0]]
        state+=[struct.unpack('<H',cpu.mem_read(foot+0x388,2))[0],int(struct.unpack('<d',cpu.mem_read(foot+0x578,8))[0]*2)]
        state+=[get(foot+0x5E0),get(foot+0x5E4),get(foot+0x5E8),get(foot+0x63C),get(foot+0x558)]
        state+=[cpu.mem_read(foot+0x68A,1)[0],cpu.mem_read(foot+0x6B7,1)[0],*counters]
        rows.append(' '.join(map(str,[variant,dx,dy,speed,flags,*state])))
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Walk Process cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
