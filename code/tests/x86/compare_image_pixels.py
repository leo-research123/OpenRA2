#!/usr/bin/env python3
"""Compare actual original and replacement blitter pixels and Z/alpha state.

Non-identity palettes/remaps, transparent skips, clipping into a run, tint,
ring-buffer boundaries and all original vtable slots are exercised. No pixel
algorithm or palette generation is substituted in either run.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
import pefile
from compare_image_convert import ConvertMachine, construct
from compare_image_resources import OBJECT, SHA

ROOT = Path(__file__).resolve().parents[3]
DATA = 0x51000000
DST, SRC, ZDATA, ADATA, ZADJUST, REMAP = [DATA + x for x in (0x1000,0x2000,0x3000,0x4000,0x5000,0x6000)]
ZOBJECT, AOBJECT = DATA + 0x7000, DATA + 0x7100

class PixelMachine(ConvertMachine):
    def __init__(self, exe, dll, replacement, bpp):
        super().__init__(exe, dll, replacement)
        self.uc.mem_map(DATA, 0x10000)
        construct(self, bpp, 1 if bpp == 1 else 7, 0)
        self.write32(OBJECT+0x17c, REMAP)
        self.uc.mem_write(REMAP, bytes((i*13+7)%256 for i in range(256)))
        self.write32(0x887644, ZOBJECT); self.write32(0x87E8A4, AOBJECT)
        for obj, buf in ((ZOBJECT,ZDATA),(AOBJECT,ADATA)):
            self.uc.mem_write(obj, struct.pack('<12I',0,0,64,1,0,0,buf,buf+128,128,0,64,1))

    def render(self, obj, entry, rle, slot, variant):
        rng = random.Random(7301 + variant)
        self.uc.mem_write(DST, bytes(rng.randrange(256) for _ in range(512)))
        indexes = bytes(0 if i%7 in (1,2,3) else (i*19+3)%256 for i in range(96))
        if variant == 5: indexes = bytes(96)
        source = indexes
        if rle:
            data=bytearray();i=0
            while i<len(indexes):
                if indexes[i]:data.append(indexes[i]);i+=1
                else:
                    end=i+1
                    while end<len(indexes) and indexes[end]==0:end+=1
                    data.extend((0,end-i));i=end
            source=bytes(data)
        self.uc.mem_write(SRC, source + b'\xcd'*128)
        self.uc.mem_write(ZDATA, struct.pack('<64H', *[rng.choice((0,50,100,101,255,65535)) for _ in range(64)]))
        self.uc.mem_write(ADATA, struct.pack('<64H', *[rng.randrange(256) for _ in range(64)]))
        self.uc.mem_write(ZADJUST, bytes((i*47)%256 if variant == 8 else (i*11)%51 for i in range(128)))
        ring = 120 if variant == 4 else 0
        length = (0,1,17,31,17,19,29,7,23)[variant]
        lead = (0,0,3,9,3,1,0,0,3)[variant]
        args=[DST+32,SRC,length]
        if rle:args.append(lead)
        args += [100,ZDATA+ring,ADATA+ring,(0,1,500,1000,2000,3000,800,1500,2000)[variant]]
        if rle or slot < 3:args.append(variant%4)
        if rle:args.append(ZADJUST)
        if slot in (2,4):args.append(0xf81f)
        self.call(entry,obj,args=args)
        return [bytes(self.uc.mem_read(a,n)).hex() for a,n in ((DST,512),(ZDATA,128),(ADATA,128),(SRC,256),(ZADJUST,128))]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True);p.add_argument('--dll',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--class',dest='classname')
    a=p.parse_args();assert hashlib.sha256(a.exe.read_bytes()).hexdigest()==SHA
    exe,dll=pefile.PE(str(a.exe)),pefile.PE(str(a.dll))
    contract=json.loads((ROOT/'code/tests/fixtures/original_blitter_entries.json').read_text())
    assert contract['target_sha256']==SHA
    tables={r['address']:r for r in contract['vtables']}
    for table in tables.values():
        actual=struct.unpack('<'+'I'*len(table['slots']),exe.get_data(int(table['address'],16)-exe.OPTIONAL_HEADER.ImageBase,4*len(table['slots'])))
        assert list(actual)==[int(entry,16) for entry in table['slots']]
    passed=[];failures=[];seen=set()
    for group in contract['groups']:
        machines=[PixelMachine(exe,dll,r,group['bpp']) for r in (False,True)]
        for recipe in group['rows']:
            table=tables[recipe['vtable']]
            if table['address'] in seen:continue
            seen.add(table['address'])
            if a.classname and a.classname not in table['name']:continue
            objects=[m.read32(OBJECT+recipe['slot']) for m in machines]
            for slot,entry in enumerate(table['slots'][1:],1):
                for variant in range(9):
                    name=f'{table["name"]}/{slot}/{variant}'
                    values=[]
                    for m,obj in zip(machines,objects):
                        try:
                            target = int(entry, 16)
                            if m.replacement:
                                target = m.read32(m.read32(obj) + slot * 4)
                                assert any(lo <= target < hi for lo, hi in m.code_ranges), 'blitter must dispatch into core DLL'
                            values.append(m.render(obj, target, 'RLE' in table['name'], slot, variant))
                        except Exception as error:values.append({'error':str(error)[:1000]})
                    if values[0]!=values[1] or any(isinstance(v,dict) for v in values):
                        failures.append({'case':name,'entry':entry,'original':values[0],'replacement':values[1]})
                    else:passed.append(name)
            print(table['name'],'tested',len(passed),'passed;',len(failures),'failures',flush=True)
    a.report.write_text(json.dumps({'scope':__doc__,'target_sha256':SHA,'dll_sha256':hashlib.sha256(a.dll.read_bytes()).hexdigest(),
        'passed':len(passed),'failures':failures},indent=2)+'\n')
    if failures:raise SystemExit(f'{len(failures)} pixel mismatches; see {a.report}')

if __name__=='__main__':main()
