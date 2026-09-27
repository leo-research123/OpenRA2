#!/usr/bin/env python3
"""Execute original RegionThreat 0x585F40 without algorithm hooks; arrays and initialized quadrant offsets are fixtures."""
import argparse, hashlib, itertools, json, struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def generate(exe,output,report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    pe=pefile.PE(str(exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+0xFFF)&~0xFFF);cpu.mem_write(base,pe.get_memory_mapped_image())
    arena=0x1000000;cpu.mem_map(arena,0x40000);house,records,stack,stop=arena,arena+0x20000,arena+0x30000,arena+0x3F000
    def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    put(0x87F890,records);put(0x87F8A8,records)
    cpu.mem_write(0xABD460,struct.pack('<8h',0,0,4,0,0,4,4,4))
    rows=[]
    for seed in range(4):
        for y in range(130):
            for x in range(130):put(house+0x57E4+4*(130*y+x),((x*37+y*53+seed*17)%257)-128)
        for level,fx,fy,tx,ty in itertools.product((0,1,2,3),range(2,5),range(2,5),range(1,6),range(1,6)):
            fr=fx+1+130*(fy+1);tr=tx+1+130*(ty+1);put(records+32,fr);put(records+36+32,tr)
            cpu.mem_write(stack,struct.pack('<5I',stop,house,level,0,1));cpu.reg_write(UC_X86_REG_ESP,stack)
            cpu.emu_start(0x585F40,stop,count=10000)
            assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+20
            result=cpu.reg_read(UC_X86_REG_EAX);result=result if result<0x80000000 else result-0x100000000
            rows.append(' '.join(map(str,(seed,level,fx,fy,tx,ty,result))))
    output.write_text('\n'.join(rows)+'\n');report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original map region threat cases')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','output','report'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
