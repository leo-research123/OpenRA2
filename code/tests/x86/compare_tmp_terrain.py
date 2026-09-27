#!/usr/bin/env python3
"""Execute original TMP land/slope/dimensions queries against compiled methods.

Original 544BE0 / 5471B0 / 547150 consume an image loaded by original 547020.
Core consumes the same disk bytes through ReadTMP. I/O/CRT use the established
compare_tmp harness boundaries; no original query result is substituted.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import pefile
from compare_tmp import Resources,fixture,SHA

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',required=True,type=Path);p.add_argument('--dll',required=True,type=Path)
    p.add_argument('--report',required=True,type=Path);a=p.parse_args()
    assert hashlib.sha256(a.exe.read_bytes()).hexdigest()==SHA
    exe,dll=pefile.PE(str(a.exe)),pefile.PE(str(a.dll));rows=[]
    machine=Resources(exe,dll)
    for terrain in range(16):
        data=bytearray(fixture(terrain));data[24+41]=terrain;data[24+42]=terrain%5
        machine.input=bytes(data)
        tile=machine.malloc(0x30c)
        machine.call(0x5447c0,tile,7,191,2,machine.cstring(b'TMP'),0)
        machine.uc.mem_write(tile+0x2f5,b'fixture.tem\0')
        assert machine.call(0x547020,tile)==len(data)
        source=machine.malloc(len(data));machine.uc.mem_write(source,bytes(data))
        output=machine.malloc(20);width=machine.malloc(4);height=machine.malloc(4)
        for index in [0,1,2,3,100,255]:
            machine.write32(width,123);machine.write32(height,456)
            land=machine.call(0x544be0,tile,index);slope=machine.call(0x5471b0,tile,index)
            ok=machine.call(0x547150,tile,index,width,height)&255
            expected=[land,slope,machine.read32(width),machine.read32(height),ok]
            assert machine.call(machine.exports['TMPProbe_TerrainQueries'],0,source,len(data),index,output)==1
            actual=list(struct.unpack('<5I',machine.uc.mem_read(output,20)))
            rows.append(dict(terrain=terrain,index=index,expected=expected,actual=actual,passed=actual==expected))
    report=dict(scope=__doc__,exe_sha256=SHA,dll_sha256=hashlib.sha256(a.dll.read_bytes()).hexdigest(),cases=rows)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{sum(r['passed'] for r in rows)}/{len(rows)} original TMP terrain query cases passed")
    return 0 if all(r['passed'] for r in rows) else 1
if __name__=='__main__':raise SystemExit(main())
