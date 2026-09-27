#!/usr/bin/env python3
"""Compile the production TacticalClass::DrawObjects and projection for x86 MS ABI."""
import argparse
import subprocess
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('llvm','lld','xwin','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();root=Path(__file__).resolve().parents[2];build=a.output.resolve()
    build.mkdir(parents=True,exist_ok=True)
    sources=['tests/x86/tactical_objects_probe.cpp','core/src/yrpp/TacticalClassObjects.cpp',
             'core/src/yrpp/TacticalClassProjection.cpp']
    flags=[str(a.llvm/'clang-cl'),'--target=i686-pc-windows-msvc','/nologo','/c','/std:c++20',
           '/EHsc','/MD','/Od','/Ob0','/Oy-','/Gy','/GS-','/DRA2_YRPP_GAME','/clang:-Wno-inconsistent-missing-override',
           '/clang:-Wno-invalid-token-paste','/clang:-Wno-invalid-constexpr','/clang:-Wno-invalid-source-encoding']
    flags+=['/I'+str(root/d) for d in ('core/include','core/src')]
    flags+=['/imsvc'+str(a.xwin/d) for d in ('crt/include','sdk/include/ucrt','sdk/include/shared','sdk/include/um','sdk/include/winrt')]
    with (build/'build-tactical-objects-probe.log').open('w') as log:
        for source in sources:
            subprocess.run(flags+['/Fo'+str(build/(Path(source).stem+'.obj')),'--',str(root/source)],stdout=log,stderr=subprocess.STDOUT,check=True)
        link=[str(a.lld/'lld-link'),'/nologo','/dll','/noentry','/machine:x86','/opt:ref','/out:'+str(build/'tactical_objects_probe.dll')]
        link+=[str(build/(Path(s).stem+'.obj')) for s in sources]+['vcruntime.lib','ucrt.lib']
        link+=['/libpath:'+str(a.xwin/d) for d in ('crt/lib/x86','sdk/lib/ucrt/x86','sdk/lib/um/x86')]
        subprocess.run(link,stdout=log,stderr=subprocess.STDOUT,check=True)

if __name__=='__main__':main()
