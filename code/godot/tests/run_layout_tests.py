#!/usr/bin/env python3
"""Run the single-canvas UI in real Godot; compare initial GPU/core RGB565.

Fixed original layout expectations and raw mouse sequences live in
TestGameLayout. No desktop automation or semantic radar API is involved.
"""
import argparse,hashlib,json,os,subprocess,tempfile
from pathlib import Path

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--godot',default='godot');p.add_argument('--dotnet',default='dotnet')
 p.add_argument('--game-data',type=Path,required=True);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
 a=p.parse_args();project=Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 subprocess.run([a.dotnet,'build',str(project/'ra2opengodot.csproj'),'--nologo'],check=True,timeout=120)
 env=os.environ.copy();env.pop('RA2_GAME_DATA',None)
 with tempfile.TemporaryDirectory(prefix='ra2-layout-') as temporary:
  with (out/'godot-layout.log').open('w') as log:
   subprocess.run([a.godot,'--path',str(project),'--rendering-method','mobile','res://tests/TestGameLayout.tscn','--',
    '--capture-dir='+str(out),'--game-data='+str(a.game_data.resolve()),'--resource-config='+str(Path(temporary)/'resources.cfg'),
    '--display-config='+str(Path(temporary)/'display.cfg')],cwd=temporary,env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=150)
 log=(out/'godot-layout.log').read_text();assert 'PASS: TestGameLayout' in log and 'ERROR:' not in log,log
 cpu=out/'initial.cpu.rgb565'
 with (out/'reference.log').open('w') as log:
  subprocess.run([str(a.reference.resolve()),str(a.game_data.resolve()),'ALL01UMD.MAP',str(cpu),'--ui'],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
 expected,actual=cpu.read_bytes(),(out/'initial.rgb565').read_bytes();assert len(expected)==len(actual)==1280*720*2
 mismatched=sum(expected[i:i+2]!=actual[i:i+2] for i in range(0,len(expected),2))
 result={'passed':mismatched==0,'pixels':1280*720,'mismatched':mismatched,'software_sha256':hashlib.sha256(expected).hexdigest(),
  'gpu_sha256':hashlib.sha256(actual).hexdigest(),'checks':['original region rectangles','HUD collapse and expand','radar click and drag',
  'outside release','sidebar/map input isolation','focus loss','1600x900 resize'],'scope':__doc__}
 (out/'layout-results.json').write_text(json.dumps(result,indent=2)+'\n');assert result['passed'],result
 print('PASS: original layout, core input, HUD toggling and 921600 GPU/software pixels')
if __name__=='__main__':main()
