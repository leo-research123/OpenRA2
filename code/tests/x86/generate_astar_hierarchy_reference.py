#!/usr/bin/env python3
"""Execute original three-level AStar 0x42C290 and RegionThreat over deterministic adjacency graphs; no algorithm hooks."""
import argparse, hashlib, itertools, json, struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    arena=0x1000000;cpu.mem_map(arena,0x100000)
    finder,foot,house,subzones,records,connections,finals,opened,costs,pool,queue,qitems,edges,coords,stack,stop=[arena+n for n in (0,0x2000,0x4000,0x20000,0x30000,0x40000,0x50000,0x51000,0x52000,0x60000,0x90000,0x91000,0xA0000,0xB0000,0xE0000,0xF0000)]
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(a):return struct.unpack('<i',cpu.mem_read(a,4))[0]
    put(foot+0x21C,house);put(foot+0x5D4,0)
    put(0x87F7E8+0x6C,441);put(0x87F7E8+0x70,subzones);put(0x87F7E8+0xF4,8);put(0x87F7E8+0xF8,12)
    cpu.mem_write(0xABD460,struct.pack('<8h',0,0,4,0,0,4,4,4));cpu.mem_write(coords,struct.pack('<4h',8,6,9,7))
    put(finder+0x28,7);put(finder+0x64,pool);put(finder+0x68,queue);put(queue+4,10000);put(queue+8,qitems)
    for level in range(3):
        put(0x87F7E8+0x90+24*level,records+0x1000*level)
        put(finder+0x40+4*level,finals+0x100*level);put(finder+0x4C+4*level,opened+0x100*level);put(finder+0x58+4*level,costs+0x100*level)
        put(finder+0x78+24*level,edges+0x100*level)
        cpu.mem_write(subzones+10*(8+21*6)+2*level,struct.pack('<h',1));cpu.mem_write(subzones+10*(9+21*7)+2*level,struct.pack('<h',6))
    for y in range(130):
        for x in range(130):put(house+0x57E4+4*(130*y+x),(x*37+y*53)%127)
    rows=[]
    for seed in range(12):
        for level in range(3):
            for node in range(8):
                r=records+0x1000*level+36*node;c=connections+0x1000*level+0x100*node
                destinations=[n for n in range(1,8) if n!=node and ((node*11+n*7+seed*5)%9<3 or n==node+1)]
                put(r+4,c);put(r+16,len(destinations))
                for i,n in enumerate(destinations):put(c+8*i,n);cpu.mem_write(c+8*i+4,bytes([(node+n+seed)%2]))
                parent=node if node in (1,6) or seed%2==0 else 2+node%3
                cpu.mem_write(r+24,struct.pack('<H',parent));put(r+28,0 if node in (1,6) else (node+seed)%8)
                put(r+32,(node%4+2)+1+130*(node//4+3))
        for movement,avoid,ban in itertools.product(range(13),(0,1),range(3)):
            cpu.mem_write(finals,bytes(0x3000));cpu.mem_write(pool,bytes(0x27100));put(queue,0);cpu.mem_write(qitems,bytes(40004))
            cpu.mem_write(finder+0xBC,bytes(3012));cpu.mem_write(foot+0x530,struct.pack('<d',0.25 if avoid else 0))
            for level in range(3):put(finder+0x84+24*level,int(ban!=0));put(edges+0x100*level,(1<<16|2) if ban==1 else (2<<16|6))
            cpu.mem_write(stack,struct.pack('<5I',stop,coords,coords+4,movement,foot));cpu.reg_write(UC_X86_REG_ECX,finder);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
            cpu.emu_start(0x42C290,stop,count=3000000)
            assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+20
            result=cpu.reg_read(UC_X86_REG_EAX)&255;values=[seed,movement,avoid,ban,result]
            for level in range(3):
                count=get(finder+0xC74+4*level);values.append(count)
                values.extend(struct.unpack('<'+'H'*count,cpu.mem_read(finder+0xBC+1000*level,2*count)) if count else [])
                values.extend(get(finals+0x100*level+4*n) for n in range(8))
            rows.append(' '.join(map(str,values)))
    output.write_text('\n'.join(rows)+'\n');report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original hierarchical AStar cases')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','output','report'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
