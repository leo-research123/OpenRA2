#!/usr/bin/env python3
"""Run pinned YR Mirage state/query, firing, retaliation and acquisition bodies.

Original 0x7468C0, 0x746750, 0x7465B0, 0x7465F0, 0x746720,
0x70ED80, 0x70EE30, 0x741340, 0x7087C0 and 0x6F7CA0 execute.
Map lookup, house relations, locomotor moving, nearby infantry, weapon queries,
base projectile creation, radar notification and threat score are controlled
dependency boundaries. Disguise checks, timers, random tree selection, disguise
detection RNG, firing blink and retaliation/acquisition branches are NOT mocked.
DrawIt 0x73CEC0 also executes through VXL/SHP dispatch, with renderer callbacks
captured; DrawVoxel 0x706640 outputs are captured before cached surface blit.
The fixtures do not validate projectile simulation, threat ranking
or final rasterized pixels.
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

SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'

def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    cpu.mem_map(0x400000,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(0x400000,pe.get_memory_mapped_image())
    base=0x1000000;cpu.mem_map(base,0x60000)
    obj,typ,vt,loco,lvt,cell,infantry,ring,owner,viewer,other,actor,actor_type,scenario,rules,choices,weapon,slot,bullet,mission,buffer,stub,stack,stop=[base+n for n in
        (0,0x2000,0x4000,0x5000,0x5100,0x6000,0x7000,0x8000,0x9000,0xA000,0xB000,0xC000,0xE000,0x10000,0x14000,0x18000,0x19000,0x1A000,0x1B000,0x1C000,0x1D000,0x20000,0x50000,0x5F000)]
    trees=[base+0x21000+i*0x1000 for i in range(4)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def byte(a,v):cpu.mem_write(a,bytes([int(bool(v))]))
    def call(entry,this=0,*args):
        cpu.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*[x&0xFFFFFFFF for x in args]))
        cpu.reg_write(UC_X86_REG_ECX,this);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        try:cpu.emu_start(entry,stop,count=1000000)
        except Exception as e:raise RuntimeError(f'{entry:#x} stopped at {cpu.reg_read(UC_X86_REG_EIP):#x}') from e
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4*(len(args)+1)
        return cpu.reg_read(UC_X86_REG_EAX)
    state={}
    def allied(a,b):
        return a==b or (a==owner and b==viewer and state.get('allied',False)) or (a==viewer and b==other and state.get('disguise_ally',False))
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);result=0;pop=0
        if address in (0x5657A0,0x565730):result=cell;pop=4
        elif address==0x486750:result=0
        elif address==0x47EC40:result=infantry if state.get('nearby',0) else 0;pop=4
        elif address==0x4F9A50:result=allied(this,get(sp+4));pop=4
        elif address==0x4F9A90:result=state.get('nearby',0)==2 if get(sp+4)==infantry else allied(this,get(get(sp+4)+0x21C));pop=4
        elif address==0x4870F0:result=state.get('sensor',False);pop=4
        elif address==0x50B6F0:result=state.get('local',False) if this==owner else False
        elif address==0x50B730:result=state.get('human',False) if this==viewer else state.get('local',False)
        elif address==0x65AD30:result=actor if state.get('linked',False) else 0;pop=4
        elif address==0x5B3A00:result=mission
        elif address==0x6F3970:result=10;pop=4
        elif address==0x6FDD50:result=bullet if state.get('bullet',False) else 0;pop=8
        elif address==0x707480:state["voxel_flags"]=get(sp+16);pop=24
        elif address==stub+0x100:result=0
        elif address==stub+0x110:result=get(sp+4);pop=4
        elif address==0x6D9EF0:pop=12
        elif address==0x4DB250:pop=8
        elif address==stub+0xD0:pop=8 # VisualCharacter
        elif address in (stub+0xE0,stub+0xF0):state["draw"]=1 if address==stub+0xE0 else 2;pop=32
        elif address==stub:result=state.get('moving',False);pop=4 # COM stdcall this
        elif address==stub+0x10:result=get(sp+4);cpu.mem_write(result,bytes(cpu.mem_read(this+0x9C,12)));pop=4
        elif address==stub+0x20:result=get(sp+4);put(result,40|(40<<16));pop=4
        elif address==stub+0x30:result=cell
        elif address==stub+0x40:pass # radar refresh
        elif address==stub+0x50:result=actor_type if this==actor else typ
        elif address==stub+0x60:result=slot;pop=4
        elif address==stub+0x70:result=1 # WhatAmI=Unit
        elif address==stub+0x80:result=0;pop=4 # SelectWeapon
        elif address==stub+0x90:result=0;pop=8 # GetFireErrorWithoutRange
        elif address==stub+0xA0:result=1 # IsArmed/IsStrange
        elif address==stub+0xB0:result=0 # IsEngineer/height/air
        elif address==stub+0xC0:result=state.get('fire_error',0);pop=12
        else:return
        uc.reg_write(UC_X86_REG_EAX,int(result)&0xFFFFFFFF);uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF);uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    hooks=(0x5657A0,0x565730,0x486750,0x47EC40,0x4F9A50,0x4F9A90,0x4870F0,0x50B6F0,0x50B730,0x65AD30,0x5B3A00,0x6F3970,0x6FDD50,0x6D9EF0,0x4DB250,0x707480,*range(stub,stub+0x120,0x10))
    for address in hooks:cpu.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
    cpu.mem_write(vt,bytes(cpu.mem_read(0x7F5C70,0x600)))
    for off,index in ((0x48,1),(0x1B8,2),(0x1BC,3),(0x49C,4),(0x84,5),(0x3F8,6),(0x2C,7),(0x2E4,8),(0x3BC,9),(0x2AC,10),(0x80,10),(0x330,11),(0x1C8,11),(0x54,11),(0x3C0,12),(0x68,13),(0x554,14),(0x558,15),(0x1D4,16),(0x1D8,16),(0x160,16),(0x464,17)):put(vt+off,stub+index*0x10)
    put(loco,lvt);put(lvt+0x10,stub);put(0xA8B230,scenario);put(0x8871E0,rules)
    put(rules+0xFFC,choices);put(rules+0x1008,4);put(rules+0x1014,20)
    for i,t in enumerate(trees):put(choices+4*i,t)
    for i,(x,y) in enumerate(((0,-1),(1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1))):cpu.mem_write(0x89F688+i*4,struct.pack('<hh',x,y))
    # Irrelevant downstream ranking is fixed positive; disguise admission and
    # its RNG are still the unchanged full Evaluate_Object function.
    cpu.mem_write(0x70CD10,b'\xD9\xE8\xC2\x08\x00') # fld1; ret 8
    cpu.mem_write(0x6F79A0,b'\xD9\xE8\xC2\x04\x00') # fld1; ret 4
    put(slot,weapon);put(weapon+0x9C,1);put(weapon+0x13C,15);put(bullet+0xAC,typ)
    byte(actor_type+0xD9A,1);byte(typ+0x231,1);byte(typ+0xD30,0)
    put(owner+0x30,0);put(viewer+0x30,1);put(other+0x30,2)
    put(owner+0x34,other+0x400);put(viewer+0x34,other+0x400)
    byte(mission+8,1);put(0xA8B238,0);put(0xA83D4C,viewer)
    put(rules+0xE14,choices+0x100);put(choices+0x100,15);put(choices+0x104,5);put(choices+0x108,2)
    def reset():
        for a in (obj,actor):
            cpu.mem_write(a,bytes(0x800));put(a,vt);put(a+0x6C4,typ if a==obj else actor_type)
            put(a+0x21C,owner if a==obj else viewer);put(a+0x674,loco);put(a+0xAC,5)
            put(a+0x6C0,-1);put(a+0x6CC,-1);put(a+0x6C8,0)
            put(a+0x6C,100);put(a+0x70,100);byte(a+0x90,1);byte(a+0x3D5,1);byte(a+0x41A,1)
            cpu.mem_write(a+0x9C,struct.pack('<iii',10368,10368,0))
        put(obj+0x1DC,900);put(obj+0x1E0,990);put(obj+0x1E8,0);put(obj+0x1EC,990);put(obj+0x1F4,0)
        put(obj+0x518,trees[0]);put(obj+0x51C,0);byte(ring+0x19D,0)
        state.clear();put(0xA83D4C,viewer);put(0xA8ED84,1000);call(0x65C6D0,scenario+0x218,12345)
    def identity(v):return 0 if not v else 1+trees.index(v) if v in trees else 5 if v==typ else 6 if v==owner else 7 if v==viewer else 8
    rows=[]
    for flags,frame in itertools.product(range(512),(0,1,7,8,9,20,21)):
        reset();state.update(moving=bool(flags&2),linked=bool(flags&8),nearby=1 if flags&16 else 2 if flags&32 else 0,local=bool(flags&256))
        call(0x65C6D0,scenario+0x218,12345+flags)
        byte(obj+0x1D8,flags&1);byte(typ+0xD32,flags&4);put(obj+0x2C8,ring if flags&128 else 0)
        put(obj+0x1E0,0);put(obj+0x1E8,20 if flags&64 else 0);put(0xA8ED84,frame)
        call(0x7468C0,obj)
        rows.append(['U',flags,frame,cpu.mem_read(obj+0x1D8,1)[0],identity(get(obj+0x518)),identity(get(obj+0x51C)),get(obj+0x1DC),get(obj+0x1E0),get(obj+0x1E8),cpu.mem_read(ring+0x19D,1)[0],get(scenario+0x21C),get(scenario+0x220)])
    for flags,age in itertools.product(range(256),(0,3,4,7,8,11,12,15,16,47,48,51,52,55,56,59,60,63,64,255,256)):
        reset();state.update(local=bool(flags&2),allied=bool(flags&4),sensor=bool(flags&8),disguise_ally=bool(flags&64))
        put(0xA83D4C,owner if flags&2 else viewer);byte(obj+0x1D8,flags&1);put(obj+0x1DC,1000-age)
        put(obj+0x51C,viewer if flags&16 else other if flags&32 else 0);put(obj+0x1F4,12 if flags&128 else 0)
        values=[call(0x746750,obj,viewer)&0xFF,call(0x70EE30,obj,viewer)&0xFF,call(0x70ED80,obj,0x2E00),identity(call(0x7465B0,obj,0)),identity(call(0x7465F0,obj,0))]
        rows.append(['Q',flags,age,*values])
    for flags,difficulty,seed in itertools.product(range(64),range(3),range(24)):
        reset();state.update(human=bool(flags&2),sensor=bool(flags&4),allied=bool(flags&8))
        byte(obj+0x1D8,flags&1);put(obj+0x1F4,12 if flags&16 else 0);byte(actor_type+0xD31,flags&32)
        put(viewer+0x184,difficulty);call(0x65C6D0,scenario+0x218,seed)
        retaliation=call(0x7087C0,actor,obj,0)&0xFF
        acquisition=call(0x6F7CA0,actor,0,2,4096,obj,buffer,0xFFFFFFFF,buffer+16)&0xFF
        rows.append(['A',flags,difficulty,seed,retaliation,acquisition,get(scenario+0x21C),get(scenario+0x220)])
    # Ordinary on-ground Mirage; capture the real draw dispatch, not a
    # reimplementation of the visibility predicate in Python.
    byte(0xB1CF9C,1);byte(typ+0x236,1);put(typ+0xB0,trees[0])
    for flags,age in itertools.product(range(16),(0,8,48,56,64)):
        reset();state.update(local=bool(flags&2),sensor=bool(flags&4),draw=0)
        byte(obj+0x1D8,flags&1);put(0xA83D4C,owner if flags&2 else viewer)
        put(obj+0x1DC,1000-age);put(obj+0x1F4,12 if flags&8 else 0)
        cpu.mem_write(obj+0x684,b'\xFF');byte(typ+0x236,not bool(flags&16))
        call(0x73CEC0,obj,buffer,buffer+16)
        rows.append(['D',flags,age,state['draw']])
    # Full DrawVoxel 0x706640 with an already populated cache. Capture its
    # output flags at DrawVoxelShadow/cache blit 0x707480; no flag mock.
    cache=choices+0x200;entry=choices+0x240
    put(cache+4,1);put(cache+16,entry);put(entry,7);put(entry+4,trees[0])
    for flags,age in itertools.product(range(8),(0,3,4,7,8,11,12,15,16,47,48,51,52,55,56,59,60,63,64,255,256)):
        reset();state.update(local=bool(flags&2),voxel_flags=-1)
        byte(obj+0x1D8,flags&1);put(obj+0x1DC,1000-age);put(obj+0x1F4,12 if flags&4 else 0)
        call(0x706640,obj,buffer,0,7,cache,buffer+16,buffer+32,buffer+48,1000,0,0)
        assert state['voxel_flags']!=-1
        rows.append(['V',flags,age,state['voxel_flags']])
    for disguised,whenstill,success in itertools.product((0,1),(0,1),(0,1)):
        reset();byte(obj+0x1D8,disguised);byte(typ+0xD32,whenstill);state['bullet']=success
        result=call(0x741340,obj,actor,0)
        rows.append(['F',disguised,whenstill,success,int(result==bullet),cpu.mem_read(obj+0x1D8,1)[0],get(obj+0x1EC),get(obj+0x1F4)])
    output.write_text('\n'.join(' '.join(map(str,r)) for r in rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'scope':__doc__,'cases':{k:sum(r[0]==k for r in rows) for k in 'UQAFDV'},'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Mirage cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','output','report'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
