#!/usr/bin/env python3
"""Execute YR Infantry passability and real reach/weapon/alliance helpers; map address and RTTI/type/vehicle/track queries are fixtures."""
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
# Flags are shared with infantry_passability_tests.cpp (fixture inputs only).
ALLY,MOVING,FROZEN,TRACKS,NAV,CLOAK,DISGUISE,WARP,IRON,DEST,CELL,ARCHIVE,ENGINEER,C4,THIEF,TARGET,TETHER,BRIDGE,BAD_GROUND,INIT,ARMED,HEALER,WALL_WEAPON,REPAIR_HUT,GATE,OPEN,INVISIBLE,LASER,FIRESTORM,FIRE_ACTIVE,HUMAN=[1<<n for n in range(31)]

def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    arena=0x1000000;cpu.mem_map(arena,0x50000)
    actor=arena;objects=[arena+0x1000*(i+1) for i in range(3)]
    typ,building,weapon,warhead,overlay,cell,source,vt,lv,stub,house,enemy,stack,stop=[arena+n for n in (0x5000,0x7000,0x9000,0xA000,0xB000,0xD000,0xE000,0xF000,0x10000,0x11000,0x18000,0x20000,0x40000,0x4F000)]
    locos=[arena+0x12000+i*0x100 for i in range(3)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    def byte(a,v):cpu.mem_write(a,bytes([int(bool(v))]))
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);pop=0
        if address in (0x565730,0x5657A0):
            p=get(sp+4)
            if address==0x565730:x,y=struct.unpack('<ii',cpu.mem_read(p,8));x=int(x/256);y=int(y/256)
            else:x,y=struct.unpack('<hh',cpu.mem_read(p,4))
            value=cell if (x,y)==(8,6) else source;pop=4
        elif address==stub:value=get(this+0xF00)
        elif address==stub+0x10:value=get(this+0xF04)
        elif address==stub+0x20:value=get(this+0xF08)
        elif address==stub+0x30:value=get(get(sp+4)+0x80);pop=4
        else:return
        uc.reg_write(UC_X86_REG_EAX,value&0xFFFFFFFF);uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF);uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    for entry in (0x565730,0x5657A0,stub,stub+0x10,stub+0x20,stub+0x30):cpu.hook_add(UC_HOOK_CODE,boundary,begin=entry,end=entry)
    for offset,entry in {0x2C:stub,0x3C:0x6F9DC0,0x48:0x5F65A0,0x80:stub+0x20,0x84:stub+0x10,0x88:stub+0x10,
       0xC4:0x41C010,0xC8:0x5227F0,0x160:0x41BF40,0x184:0x5B3040,0x1B0:0x4D9C60,0x1B8:0x41BEA0,
       0x1D4:0x70C5B0,0x2AC:0x701120,0x320:0x4DA1D0,0x3F4:0x70E1A0,0x3F8:0x70E140}.items():put(vt+offset,entry)
    put(lv+0x10,0x75AB30);put(lv+0xA4,stub+0x30)
    put(house+0x30,0);put(enemy+0x30,1);put(house+0x5788,1);put(enemy+0x5788,2)
    put(0xA83D84,arena+0x15000);put(arena+0x15000,overlay);put(0x8B4148,0)
    put(0xA8ED84,500);put(0xA8B238,0);byte(0xA8E9A0,1)
    # Native CellClass neighbour offsets, initialized by original globals startup.
    for i,(x,y) in enumerate(((0,-1),(1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1))):cpu.mem_write(0x89F688+4*i,struct.pack('<hh',x,y))
    cpu.mem_write(0x87F7E8+0xEC,struct.pack('<iiiiiiii',0,0,8,12,0,0,8,12))
    cpu.mem_write(cell+0x24,struct.pack('<hh',8,6));cpu.mem_write(source+0x24,struct.pack('<hh',7,6))
    cpu.mem_write(cell+0x116,b'\xFF\xFF');cpu.mem_write(source+0x116,b'\xFF\xFF')
    put(overlay+0x2A0,3);put(weapon+0xAC,warhead)
    rows=[];seen=set()
    def sample(kind,flags,mission=2,count=1,occupation=0,infantry_owner=-1,overlay_kind=0):
        count=count if kind else 0
        key=(kind,flags,mission,count,occupation,infantry_owner,overlay_kind)
        if key in seen:return
        seen.add(key)
        cpu.mem_write(actor,bytes(0x1000));put(actor,vt);put(actor+0xF00,15);put(actor+0xF04,typ);put(actor+0x6C0,typ)
        put(actor+0x21C,house);put(actor+0xAC,mission);put(actor+0xB4,-1)
        byte(actor+0x418,flags&TETHER);put(actor+0x150,0)
        byte(typ+0xEC3,flags&ENGINEER);byte(typ+0xEC2,flags&C4);byte(typ+0xEC6,flags&THIEF)
        put(typ+0x67C,0);put(typ+0x898,weapon if flags&(ARMED|HEALER) else 0)
        put(weapon+0xA4,-10 if flags&HEALER else 10);put(weapon+0x98,0);byte(warhead+0x144,flags&WALL_WEAPON)
        cpu.mem_write(0x89EA40,struct.pack('<f',0.0 if flags&BAD_GROUND else 1.0))
        put(0xA8E7AC,int(bool(flags&INIT)));byte(house+0x1EC,flags&HUMAN)
        put(cell+0x140,0x100 if flags&BRIDGE else 0);put(cell+0x124,occupation);put(cell+0x128,occupation)
        put(cell+0x54,infantry_owner);put(cell+0x58,infantry_owner);put(cell+0xEC,0)
        put(cell+0x44,0 if overlay_kind else -1);byte(overlay+0x2AA,overlay_kind==1);byte(overlay+0x2A8,overlay_kind in (2,3))
        cpu.mem_write(cell+0x11E,bytes([0x30 if overlay_kind==3 else 0]));put(cell+0x50,0 if flags&ALLY else 1)
        rtti=(0,15,1,2,6,36)[kind]
        for i,obj in enumerate(objects):
            cpu.mem_write(obj,bytes(0x1000));put(obj,vt);put(obj+0xF00,rtti);put(obj+0xF04,building if kind==4 else typ)
            put(obj+0xF08,int(kind==2));put(obj+0x14,2 if kind==5 else 3 if kind==4 else 7)
            put(obj+0x6C0,typ);put(obj+0x520,building);put(obj+0x21C,0 if kind==5 else house if flags&ALLY else enemy)
            cpu.mem_write(obj+0x9C,struct.pack('<iii',8*256+192,6*256+64,0));put(obj+0xAC,24 if flags&OPEN else 5)
            byte(obj+0x90,1);byte(obj+0x74,1);put(obj+0x6C,100);put(obj+0x220,2 if flags&CLOAK else 0)
            byte(obj+0x1D8,flags&DISGUISE);put(obj+0x51C,house);byte(obj+0x270,flags&WARP)
            put(obj+0x18C,-1);put(obj+0x194,10 if flags&IRON else 0)
            put(obj+0x674,locos[i]+4);put(locos[i]+4,lv);byte(locos[i]+0x34,flags&MOVING);put(locos[i]+4+0x80,int(bool(flags&TRACKS)))
            byte(obj+0x6B6,flags&FROZEN);put(obj+0x5A4,cell if flags&NAV else 0)
            put(obj+0x30,objects[i+1] if i+1<count else 0)
            byte(obj+0x368,0);byte(obj+0x369,flags&OPEN);put(obj+0x618,8 if flags&LASER else 0)
        put(actor+0x5A4,objects[0] if flags&DEST else cell if flags&CELL else 0)
        put(actor+0x2B4,objects[0] if flags&TARGET else 0);put(actor+0x218,objects[0] if flags&ARCHIVE else 0)
        for offset,flag in ((0x16B6,REPAIR_HUT),(0x16B7,GATE),(0x1701,INVISIBLE),(0x16BF,LASER),(0x16C0,FIRESTORM)):byte(building+offset,flags&flag)
        byte(house+0x1FA,flags&FIRE_ACTIVE);byte(enemy+0x1FA,flags&FIRE_ACTIVE)
        put(cell+0xE4,objects[0] if count else 0);put(cell+0xE8,objects[0] if count else 0)
        cpu.mem_write(stack,struct.pack('<IIIIII',stop,cell,0xFFFFFFFF,0xFFFFFFFF,source,0))
        cpu.reg_write(UC_X86_REG_ECX,actor);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        try:cpu.emu_start(0x51BF90,stop,count=100000)
        except Exception as exc:raise RuntimeError(f'case {key}, eip={cpu.reg_read(UC_X86_REG_EIP):#x}') from exc
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+24
        rows.append(' '.join(map(str,[*key,cpu.reg_read(UC_X86_REG_EAX)])))
    for count,occ,owner,bridge,bad,armed in itertools.product(range(4),(0,4,8,12,16,20,24,28,32,60),(-1,0,1),range(2),range(2),range(2)):
        sample(1,ALLY|(BRIDGE if bridge else 0)|(BAD_GROUND if bad else 0)|(ARMED if armed else 0),count=count,occupation=occ,infantry_owner=owner)
    attributes=(ALLY,MOVING,FROZEN,TRACKS,NAV,CLOAK,DISGUISE,WARP,IRON)
    for kind,mask,armed in itertools.product(range(1,6),range(1<<len(attributes)),(0,ARMED,HEALER)):
        sample(kind,sum(flag for i,flag in enumerate(attributes) if mask&(1<<i))|armed)
    for kind,mission,destination,special,ally,obstruction in itertools.product(range(1,6),(-1,5,7,8,9,11,17,25),(0,DEST,CELL,TARGET,ARCHIVE),(0,ENGINEER,C4,THIEF),range(2),(0,WARP,IRON,BAD_GROUND,TETHER)):
        sample(kind,ARMED|destination|special|(ALLY if ally else 0)|obstruction,mission)
    for mask,ally,armed in itertools.product(range(128),range(2),range(2)):
        flags=sum(flag for i,flag in enumerate((REPAIR_HUT,GATE,OPEN,INVISIBLE,LASER,FIRESTORM,FIRE_ACTIVE)) if mask&(1<<i))
        sample(4,flags|(ALLY if ally else 0)|(ARMED if armed else 0))
    for kind,overlay_kind,ally,weapon_flag,wall,bad,bridge,human in itertools.product((0,1,2,4),(1,2,3),range(2),(0,ARMED,HEALER),range(2),range(2),range(2),range(2)):
        sample(kind,weapon_flag|(ALLY if ally else 0)|(WALL_WEAPON if wall else 0)|(BAD_GROUND if bad else 0)|(BRIDGE if bridge else 0)|(HUMAN if human else 0),overlay_kind=overlay_kind)
    output.write_text('\n'.join(rows)+'\n')
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original Infantry passability cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
