#!/usr/bin/env python3
"""Compare original 4814F0 pattern selection and RNG state with compiled core.

Original and core construct real Cells, tile types and Randomizers. Original
CCFile/CRT use the existing TMP harness; Scenario identity storage and the
original catalog's single-entry registration are fixture boundaries.
The cache starts uninitialized for each case; neither path receives the other's
pattern or random state. This does not verify world drawing or all TMP loading.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import pefile
from compare_tmp import Resources,fixture,SHA


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','dll','report'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();assert hashlib.sha256(a.exe.read_bytes()).hexdigest()==SHA
    m=Resources(pefile.PE(str(a.exe)),pefile.PE(str(a.dll)))
    scenario=m.malloc(0x30000);m.write32(0xa8b230,scenario);m.write32(scenario+0x214,1000000)
    random=0x886b88 # 481521 uses the object's address, not a pointer slot
    cell=m.malloc(0x148);m.call(0x47bbf0,cell)
    data=fixture(3);m.input=data
    tile=m.malloc(0x30c);m.call(0x5447c0,tile,0,191,2,m.cstring(b'TMP'),0)
    catalog=m.malloc(4);m.write32(catalog,tile);m.write32(0xa8ed2c,catalog);m.write32(0xa8ed38,1)
    m.uc.mem_write(tile+0x2f5,b'fixture.tem\0');assert m.call(0x547020,tile)==len(data)
    source=m.malloc(len(data));m.uc.mem_write(source,data)
    output=m.malloc(317*4);rows=[]
    for seed in (0,1,0xfedcba98):
        for x,y in ((0,0),(1,1),(3,4),(7,7),(8,9),(511,511),(-1,-1),(-32768,32767)):
            for height in (0,1,2,255):
                for count in (1,4,8):
                    m.call(0x65c6d0,random,seed)
                    m.uc.mem_write(0x89e7c7,b'\0')
                    m.uc.mem_write(cell+0x24,struct.pack('<2h',x,y))
                    m.uc.mem_write(cell+0x11a,bytes([height]))
                    result=m.call(0x4814f0,cell,0,count)
                    expected=struct.pack('<I',result)+bytes(m.uc.mem_read(0x89e620,256))+bytes(m.uc.mem_read(random+4,1008))
                    assert m.call(m.exports['TMPProbe_CellVariant'],0,source,len(data),seed,x,y,height,count,output)==1
                    actual=bytes(m.uc.mem_read(output,len(expected)))
                    rows.append(dict(seed=seed,x=x,y=y,height=height,count=count,passed=actual==expected,
                        original_sha256=hashlib.sha256(expected).hexdigest(),core_sha256=hashlib.sha256(actual).hexdigest()))
                    if expected!=actual:
                        rows[-1].update(original=expected.hex(),core=actual.hex())
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(dict(scope=__doc__,exe_sha256=SHA,
        dll_sha256=hashlib.sha256(a.dll.read_bytes()).hexdigest(),cases=rows),indent=2)+'\n')
    print(f"{sum(r['passed'] for r in rows)}/{len(rows)} Cell variant / RNG cases passed")
    return 0 if all(r['passed'] for r in rows) else 1
if __name__=='__main__':raise SystemExit(main())
