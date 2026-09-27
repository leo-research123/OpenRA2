#!/usr/bin/env python3
"""Execute original Convert/PAL/alpha and replacement instructions with matched services.

Original vector and blitter virtual methods consume replacement objects. File,
allocator and CRT services are doubles; this is not a Windows integration run.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from compare_image_pcx import PCXMachine
from compare_image_resources import OBJECT, SUPPORT, SHA

ART, SCREEN, SURFACE = SUPPORT+0x8000, SUPPORT+0x8400, SUPPORT+0x8800
CONVERT_VECTOR, ALPHA_VECTOR = 0x89ECF8, 0x88A080
class ConvertMachine(PCXMachine):
    def __init__(self,exe,dll,replacement,fail=()):
        super().__init__(exe,dll,{},replacement,fail)
        self.hooks.pop(0x48E740,None)
        self.uc.mem_write(OBJECT,b'\xa5'*0x400)
        self.uc.mem_write(ART,bytes((i*17+3)%256 for i in range(768)))
        self.uc.mem_write(SCREEN,bytes((i*11+91)%256 for i in range(768)))
        for address,vtable,items in [(CONVERT_VECTOR,0x7E5318,SUPPORT+0x9000),(ALPHA_VECTOR,0x7E2AD0,SUPPORT+0x9400)]:
            self.uc.mem_write(address,struct.pack('<IIIBB2xII',vtable,items,16,1,0,0,10))
        self.uc.mem_write(SURFACE,struct.pack('<8I',0x7E2070,2,2,0,2,0,0,0))
        self.write32(0x887310,SURFACE)
        self.write32(0x8A0DE8,0x39E773EF)
    def snapshot(self):
        r=super().snapshot()
        r['vectors']=[bytes(self.uc.mem_read(a,24)).hex() for a in (CONVERT_VECTOR,ALPHA_VECTOR)]
        r['vector_items']=[bytes(self.uc.mem_read(SUPPORT+o,64)).hex() for o in (0x9000,0x9400)]
        return r

def construct(m,bpp,shades,skip):
    m.write32(SURFACE+16,bpp)
    return m.call(0x48E740,OBJECT,args=(ART,SCREEN,SURFACE,shades,skip),kind='value')

def lifetime(m,bpp,shades,skip):
    values=[construct(m,bpp,shades,skip),m.snapshot()]
    m.call(0x490400,OBJECT)
    return values

def alpha(m,count):
    p=m.call(0x420140,args=(count,),kind='value')
    q=m.call(0x420140,args=(count,),kind='value')
    r=m.call(0x420140,args=(count+1,),kind='value')
    values=[p,q,r,m.snapshot()]
    for item in (p,r,q,0):m.call(0x420270,args=(item,))
    return values

def selection(m):
    for i in range(89):m.write32(OBJECT+8+i*4,0x11000000+i*16)
    result=[]
    # Exhaust all combinations of the 15 relevant selector bits, with masks
    # changed at runtime to prove we read the original globals.
    bits=[0,1,2,3,4,5,6,8,11,12,13,14,15,16,17]
    for mask in (0x3000,0,0x1000):
        m.write32(0x81DC24,mask);m.write32(0x81DC28,mask)
        digest=hashlib.sha256()
        for n in range(1<<len(bits)):
            f=sum(1<<bit for j,bit in enumerate(bits) if n>>j&1)
            for entry in (0x490B90,0x490E50):
                digest.update(struct.pack('<I',m.call(entry,OBJECT,args=(f,),kind='value')))
        result.append(digest.hexdigest())
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True);p.add_argument('--dll',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--case');a=p.parse_args()
    assert hashlib.sha256(a.exe.read_bytes()).hexdigest()==SHA
    exe,dll=pefile.PE(str(a.exe)),pefile.PE(str(a.dll))
    cases=[]
    for bpp in (1,2):
        for shades in (-1,1,3,7):
            for skip in (0,1):cases.append((f'convert_{bpp}_{shades}_{skip}',lambda m,b=bpp,s=shades,k=skip:lifetime(m,b,s,k)))
    cases += [(f'alpha_{count}',lambda m,c=count:alpha(m,c)) for count in (-1,0,1,7,53,256,65536)]
    cases += [('selection_flags',selection)]
    if a.case:cases=[row for row in cases if row[0]==a.case]
    passed=[]
    for name,operation in cases:
        results=[]
        for replacement in (False,True):
            m=ConvertMachine(exe,dll,replacement)
            values=operation(m);results.append({'values':values,'state':m.snapshot()})
        if results[0]!=results[1]:
            a.report.parent.mkdir(parents=True,exist_ok=True)
            a.report.with_suffix('.failure.json').write_text(json.dumps({'case':name,'original':results[0],'replacement':results[1]},indent=2))
            raise AssertionError(f'differential mismatch: {name}; see failure report')
        passed.append(name);print('Passed',name,flush=True)
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps({'target_sha256':SHA,'dll_sha256':hashlib.sha256(a.dll.read_bytes()).hexdigest(),
       'scope':__doc__,'passed':len(passed),'cases':passed},indent=2)+'\n')
if __name__=='__main__':main()
