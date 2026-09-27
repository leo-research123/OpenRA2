#!/usr/bin/env python3
"""Render six real theaters through the game scene and compare GPU/software RGB565.

Requires a real installation, a graphical Godot runtime and prebuilt native
ra2_map_drawing_reference. Regular rendering does no readback; this test captures
the final viewport explicitly. It does not compare a full original-game screenshot.
"""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--godot',default='godot');p.add_argument('--dotnet',default='dotnet')
    p.add_argument('--game-data',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--reference',type=Path,required=True)
    a=p.parse_args();project=Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    subprocess.run([a.dotnet,'build',str(project/'ra2opengodot.csproj'),'--nologo'],check=True,timeout=120)
    with (out/'godot-map-window.log').open('w') as log:
        subprocess.run([a.godot,'--path',str(project),'--rendering-method','mobile','res://tests/TestMapWindow.tscn',
            '--','--capture-dir='+str(out),'--game-data='+str(a.game_data.resolve())],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=180)
    text=(out/'godot-map-window.log').read_text();assert 'PASS: TestMapWindow' in text,text
    assert 'ERROR:' not in text,text
    names=[line.split()[1] for line in text.splitlines() if line.startswith('MAP_RENDER ')]
    assert len(names)==6,names
    rows=[]
    for name in names:
        cpu=out/(name+'.rgb565');gpu=out/(name+'.gpu.rgb565')
        with (out/(name+'.reference.log')).open('w') as log:
            subprocess.run([str(a.reference.resolve()),str(a.game_data.resolve()),name,str(cpu),'--ui'],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
        expected,actual=cpu.read_bytes(),gpu.read_bytes();assert len(expected)==len(actual)==1280*720*2
        diffs=[i for i in range(1280*720) if expected[2*i:2*i+2]!=actual[2*i:2*i+2]]
        row=dict(map=name,pixels=1280*720,mismatched=len(diffs),software_sha256=hashlib.sha256(expected).hexdigest(),gpu_sha256=hashlib.sha256(actual).hexdigest(),
            first_differences=[dict(x=i%1280,y=i//1280,software=struct.unpack_from('<H',expected,2*i)[0],gpu=struct.unpack_from('<H',actual,2*i)[0]) for i in diffs[:8]])
        rows.append(row);print(name,len(diffs),'mismatched pixels',flush=True)
    (out/'map-pixel-comparison.json').write_text(json.dumps(dict(scope=__doc__,maps=rows),indent=2)+'\n')
    return 0 if all(r['mismatched']==0 for r in rows) else 1
if __name__=='__main__':raise SystemExit(main())
