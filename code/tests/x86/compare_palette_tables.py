#!/usr/bin/env python3
"""Compare every palette byte from original Convert/LightConvert with base-core tables.

Runs original constructors and color updates with existing I/O/CRT harness
boundaries. Core static functions neither construct Surface/Convert nor borrow
the original result. This is a color-table test, not a full lifecycle/vtable test.
"""
import argparse,hashlib,json,struct,itertools
from pathlib import Path
import pefile
from unicorn.x86_const import UC_X86_REG_FPCW
from compare_image_light import LightMachine,ART,SCREEN,SURFACE,INDEXES,OBJECT,SUPPORT,SHA

def attach(machine,probe):
    base=probe.OPTIONAL_HEADER.ImageBase
    machine.uc.mem_map(base,(probe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
    machine.uc.mem_write(base,probe.get_memory_mapped_image())
    return {e.name.decode().lstrip('_').split('@')[0]:base+e.address for e in probe.DIRECTORY_ENTRY_EXPORT.symbols if e.name}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',required=True,type=Path);p.add_argument('--image-dll',required=True,type=Path)
    p.add_argument('--probe',required=True,type=Path);p.add_argument('--report',required=True,type=Path)
    a=p.parse_args();assert hashlib.sha256(a.exe.read_bytes()).hexdigest()==SHA
    exe,image,probe=[pefile.PE(str(x)) for x in (a.exe,a.image_dll,a.probe)]
    rows=[]
    m=LightMachine(exe,image,False);exports=attach(m,probe)
    state=0x30000000;m.uc.mem_map(state,0x10000)
    values=[]
    for i in range(2001):
        rgb=(i,max(i-1,0),i//2)
        for rotation in range(3):values.append((1000,*(rgb[rotation:]+rgb[:rotation])))
    for rgb in sorted(set(itertools.permutations((2000,999,-1)))|{(1,1,1),(2,2,2),(3,3,3),(1000,1000,1000),(3000,3000,3000)}):
        for ambient in (-2000000000,-50,0,1000,40000):values.append((ambient,*rgb))
    for ambient,red,green,blue in values:
        initial=struct.pack('<5I',123,ambient&0xffffffff,red&0xffffffff,green&0xffffffff,blue&0xffffffff)
        m.uc.mem_write(state,initial)
        m.uc.reg_write(UC_X86_REG_FPCW,0xe7f) # initialized game's 53-bit toward-zero arithmetic
        original_result=m.call(0x5558e0,state,state+4,args=(state+8,state+12,state+16),kind='value')
        expected=bytes(m.uc.mem_read(state,20))
        m.uc.mem_write(state,initial)
        core_result=m.call(exports['PaletteProbe_Normalize'],args=(state,),kind='value')
        assert m.uc.reg_read(UC_X86_REG_FPCW)==0xe7f,'core changed caller x87 state'
        actual=bytes(m.uc.mem_read(state,20))
        rows.append(dict(kind='normalize',input=[ambient,red,green,blue],passed=expected==actual and original_result==core_result,
            original=expected.hex(),core=actual.hex()))
    for mode in (-1,0,1,2,3,4):
        for mmx in (0,1):
            for shades in (1,27,53):
                m=LightMachine(exe,image,False,mode,mmx)
                exports=attach(m,probe)
                m.call(0x555da0,OBJECT,args=(ART,SCREEN,SURFACE,1000,870,630,1,INDEXES,shades),kind='value')
                output=0x30000000;m.uc.mem_map(output,0x10000)
                for red,green,blue in ((1000,870,630),(999,567,1234),(1700,999,2000),(0,2000,0)):
                    m.call(0x556090,OBJECT,args=(red,green,blue,0))
                    source=m.read32(OBJECT+0x170)
                    expected=bytes(m.uc.mem_read(source,shades*512))
                    status=m.call(exports['PaletteProbe_Light'],args=(ART,output,shades*256,shades,
                        red,green,blue,INDEXES,mode,mmx),kind='value')
                    actual=bytes(m.uc.mem_read(output,shades*512))
                    rows.append(dict(kind='light',mode=mode,mmx=mmx,shades=shades,rgb=[red,green,blue],
                        passed=status==1 and expected==actual,original_sha256=hashlib.sha256(expected).hexdigest(),
                        core_sha256=hashlib.sha256(actual).hexdigest()))
    # Original Convert uses the current RGB shifts, not LightMode.
    for mode,bits in enumerate(((5,5,5),(5,5,6),(5,6,5),(6,5,5))):
        for shades in (1,3,7,27,53):
            m=LightMachine(exe,image,False,mode,0);exports=attach(m,probe)
            r,g,b=bits
            m.uc.mem_write(0x8a0dd0,struct.pack('<6I',g+b,8-r,0,8-b,b,8-g))
            m.call(0x48e740,OBJECT,args=(ART,SCREEN,SURFACE,shades,0),kind='value')
            expected=bytes(m.uc.mem_read(m.read32(OBJECT+0x170),shades*512))
            output=0x30000000;m.uc.mem_map(output,0x10000)
            status=m.call(exports['PaletteProbe_Convert'],args=(ART,output,shades*256,shades,mode),kind='value')
            actual=bytes(m.uc.mem_read(output,shades*512))
            rows.append(dict(kind='convert',mode=mode,shades=shades,passed=status==1 and expected==actual,
                original_sha256=hashlib.sha256(expected).hexdigest(),core_sha256=hashlib.sha256(actual).hexdigest()))
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(dict(scope=__doc__,exe_sha256=SHA,
        image_dll_sha256=hashlib.sha256(a.image_dll.read_bytes()).hexdigest(),
        probe_sha256=hashlib.sha256(a.probe.read_bytes()).hexdigest(),cases=rows),indent=2)+'\n')
    print(f"{sum(r['passed'] for r in rows)}/{len(rows)} original/base-core normalization and palette cases passed")
    for row in rows:
        if not row['passed']:print(row)
    return 0 if all(row['passed'] for row in rows) else 1
if __name__=='__main__':raise SystemExit(main())
