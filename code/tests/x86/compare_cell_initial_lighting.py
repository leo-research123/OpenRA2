#!/usr/bin/env python3
"""Original 484180 without LightSource objects/effects versus initial core Cell fields.

Executes the original light calculator and the core's normal Scenario/Cell
construction. Original Scenario lighting fields and empty LightSource registry
are input fixtures; no light/normalization result is substituted. Covers only
initial terrain lighting, not live light sources or Convert cache lifetime.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import pefile
from unicorn.x86_const import UC_X86_REG_FPCW
from compare_tmp import Resources,SHA

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','dll','report'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();assert hashlib.sha256(a.exe.read_bytes()).hexdigest()==SHA
    m=Resources(pefile.PE(str(a.exe)),pefile.PE(str(a.dll)))
    scenario=m.malloc(0x30000);m.write32(0xa8b230,scenario);m.write32(scenario+0x214,1000000)
    for address in (0xabca20,0xa9fab4,0xa9fabc,0xa9fab0,0xa9fac0):m.write32(address,0)
    cell=m.malloc(0x148);m.call(0x47bbf0,cell)
    source=m.malloc(36);output=m.malloc(32);rows=[]
    for light in ((100,100,100,100,0,0),(80,150,100,50,200,100),(0,0,0,0,200,100),
                  (90,1,1,1,50,20),(250,70,100,200,1000,300),(-50,-20,150,300,-100,30)):
        for x,y in ((8,1),(0,0),(-1,-1)):
            for height in (0,3,13,-1):
                ambient,red,green,blue,ground,level=light
                for field,value in ((3403,ambient),(3405,red),(3406,green),(3407,blue),(3408,ground),(3409,level)):
                    m.write32(scenario+4*field,value)
                m.uc.mem_write(cell+0x24,struct.pack('<2h',x,y));m.uc.mem_write(cell+0x11b,bytes([height&255]))
                m.uc.reg_write(UC_X86_REG_FPCW,0xe7f)
                m.call(0x484180,cell,*(output+4*i for i in range(8)))
                expected=bytes(m.uc.mem_read(output,32))
                values=(*light,x,y,height);m.uc.mem_write(source,struct.pack('<9i',*values))
                assert m.call(m.exports['TMPProbe_InitialLight'],0,source,output)==1
                actual=bytes(m.uc.mem_read(output,32))
                rows.append(dict(input=values,original=list(struct.unpack('<8I',expected)),
                    core=list(struct.unpack('<8I',actual)),passed=expected==actual))
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(dict(scope=__doc__,exe_sha256=SHA,
        dll_sha256=hashlib.sha256(a.dll.read_bytes()).hexdigest(),cases=rows),indent=2)+'\n')
    print(f"{sum(r['passed'] for r in rows)}/{len(rows)} initial Cell lighting cases passed")
    return 0 if all(r['passed'] for r in rows) else 1
if __name__=='__main__':raise SystemExit(main())
