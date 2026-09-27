#!/usr/bin/env python3
"""Execute YR 0x56C510/0x56CB90 and 0x581F90/0x5824A0 on 40 diamond maps.
Only allocation/deallocation are controlled; original vectors, flood fills,
movement-zone labeling and hash-ordered subzone graph construction execute.
"""
import argparse, hashlib, json, struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));lines=[]
    for seed in range(40):
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
        cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
        cpu.mem_map(0,0x1000);cpu.mem_map(0x1000000,0x4000000)
        def invalid(uc,access,address,size,value,user):
            print('invalid-memory',seed,hex(uc.reg_read(UC_X86_REG_EIP)),hex(address),size,flush=True);return False
        cpu.hook_add(UC_HOOK_MEM_INVALID,invalid)
        cursor=0x1200000;stack=0x1100000;stop=0x1000000;world=0x87F7E8;zones=0x1010000;subs=0x1020000
        def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
        def get(a):return struct.unpack('<I',cpu.mem_read(a,4))[0]
        def allocate(size):
            nonlocal cursor
            p=cursor;cursor+=(max(1,size)+15)&~15
            assert cursor<0x5000000
            return p
        def boundary(uc,address,size,user):
            sp=uc.reg_read(UC_X86_REG_ESP)
            if address==0x7C8E17:uc.reg_write(UC_X86_REG_EAX,allocate(get(sp+4)))
            uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
        for entry in (0x7C8E17,0x7C8B3D):cpu.hook_add(UC_HOOK_CODE,boundary,begin=entry,end=entry)
        def vector(address,vt):cpu.mem_write(address,struct.pack('<6I',vt,0,0,1,0,16))
        for index in range(4):
            h=allocate(16);b=allocate(256*24);cpu.mem_write(h,struct.pack('<4I',b,0,256,16))
            put(world+(0x14 if index==0 else 0x80+4*(index-1)),h)
            for i in range(256):vector(b+24*i,0x7ED540 if index==0 else 0x7ED520)
        put(world+0x68,zones);put(world+0x6C,441);put(world+0x70,subs)
        put(world+0x13C,0x2000000)
        cpu.mem_write(world+0xEC,struct.pack('<8i',0,0,8,12,0,0,8,12));put(world+0x60,0)
        for level in range(3):vector(world+0x8C+24*level,0x7ED4A0)
        for y in range(21):
            for x in range(21):
                valid=x+y>8 and x-y<8 and y-x<8 and x+y<=32
                hashed=(x*17+y*31+seed*13)%19
                p=0 if seed<4 else (hashed+seed)%8
                if not valid:p=7
                h=0 if seed<8 else (x//(2+seed%3)+y//3+seed)%5
                cpu.mem_write(zones+4*(x+21*y),struct.pack('<BBH',p,h,0))
                if valid:
                    c=0x2200000+0x200*(x+21*y);put(0x2000000+4*(x+512*y),c)
                    cpu.mem_write(c+0x11B,bytes([h]))
        def call(address,args=()):
            cpu.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*args));cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_ECX,world)
            cpu.emu_start(address,stop,count=10000000)
            assert cpu.reg_read(UC_X86_REG_EIP)==stop,(seed,hex(address),hex(cpu.reg_read(UC_X86_REG_EIP)))
            assert cpu.reg_read(UC_X86_REG_ESP)==stack+4+4*len(args)
            return cpu.reg_read(UC_X86_REG_EAX)
        best=call(0x56C510);count=get(world+0x4C);lines.append(f'{seed} {best} {count}')
        lines.append(' '.join(str(struct.unpack('<H',cpu.mem_read(zones+4*i+2,2))[0]) for i in range(441)))
        for movement in range(13):lines.append(' '.join(map(str,struct.unpack('<'+'H'*count,cpu.mem_read(get(world+0x18+4*movement),2*count)))))
        for level in (2,1,0):
            call(0x581F90,(level,));count=get(world+0x74+4*level);lines.append(str(count))
            lines.append(' '.join(str(struct.unpack('<h',cpu.mem_read(subs+10*i+2*level,2))[0]) for i in range(441)))
            records=get(world+0x90+24*level)
            for i in range(count):
                r=records+36*i;n=get(r+16);data=[struct.unpack('<H',cpu.mem_read(r+24,2))[0],get(r+28),get(r+32),n]
                for j in range(n):c=get(r+4)+8*j;data.extend((get(c),cpu.mem_read(c+4,1)[0]))
                lines.append(' '.join(map(str,data)))
    output.write_text('\n'.join(lines)+'\n');report.write_text(json.dumps({'exe_sha256':SHA,'cases':40,'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print('40 original map zone/subzone graphs')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','output','report'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
