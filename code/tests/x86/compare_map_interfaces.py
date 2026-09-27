#!/usr/bin/env python3
"""Original Abstract QueryInterface semantics on a normally constructed Cell."""
import argparse
import hashlib
import json
from pathlib import Path
import pefile
from compare_scenario import Machine, ENTRIES, OBJECT, SUPPORT, SHA


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','dll','report'): parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    exe,dll=pefile.PE(str(args.exe)),pefile.PE(str(args.dll))
    ENTRIES.update(ScenarioCellConstruct=0x47BBF0,ScenarioAbstractQuery=0x410260)
    cases=[]
    for guid in (0x7F7C90,0x7F7C80,0x7F7C70,0x7E9AE0,None):
        for null_output in (False,True):
            observations=[]
            for candidate in (False,True):
                m=Machine(exe,dll,candidate)
                m.write32(0xA8B230,SUPPORT+0x9000)
                cell=m.call('ScenarioCellConstruct',ecx=OBJECT)
                iid,out=SUPPORT+0x8000,SUPPORT+0x8100
                m.uc.mem_write(iid,exe.get_data(guid-0x400000,16) if guid else bytes([0xAB])*16)
                m.write32(out,0xDEADBEEF)
                destination=0 if null_output else out
                callargs=(iid,destination) if candidate else (cell,iid,destination)
                result=m.call('ScenarioAbstractQuery',callargs,ecx=cell)
                target=m.read32(out)
                expected=0x80004003 if null_output else 0 if guid else 0x80004002
                assert result==expected
                relative=target if target in (0,0xDEADBEEF) else target-cell
                assert relative==(0xDEADBEEF if null_output else 4 if guid==0x7E9AE0 else 0)
                assert m.read32(cell+0x1C)==0, 'AddRef does not change RefCount'
                observations.append((result,relative))
            assert observations[0]==observations[1]
            cases.append({'guid_source':hex(guid) if guid else 'unsupported','null_output':null_output,'status':'passed'})
    args.report.write_text(json.dumps({'exe_sha256':SHA,'dll_sha256':hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'cases':cases,'scope':__doc__},indent=2)+'\n')
    print('10 original/compiled map-object COM query cases passed')


if __name__=='__main__': main()
