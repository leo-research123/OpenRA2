#!/usr/bin/env python3
"""Create an isolated ALL01 lighting/rubble fixture and run the production GPU scene."""
import argparse,re,subprocess
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--game-data',type=Path,required=True)
    p.add_argument('--source-map',type=Path,required=True,help='Extracted original ALL01UMD.MAP')
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--godot',default='godot');p.add_argument('--dotnet',default='dotnet')
    a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    root=out/'game-data';root.mkdir(exist_ok=True)
    for source in a.game_data.resolve().iterdir():
        destination=root/source.name
        if not destination.exists():destination.symlink_to(source,target_is_directory=source.is_dir())
    text=a.source_map.read_text(encoding='latin1')
    text,count=re.subn(r'(?m)^([^\n=]+=Americans,)GTGCAN,',r'\1CAWASH01,',text,count=1)
    if count!=1:raise RuntimeError('Source map must contain the ALL01 American giant cannons')
    text+='\n[CAWASH01]\nPowered=no\nHasSpotlight=yes\nLightIntensity=0.25\nLightVisibility=1280\nLightRedTint=0.2\nLightGreenTint=0\nLightBlueTint=-0.1\nLeaveRubble=yes\n'
    (root/'RA2OPEN_LIGHT_TEST.MAP').write_text(text,encoding='latin1')
    project=Path(__file__).resolve().parents[1]
    commands=[('dotnet-build',[a.dotnet,'build',str(project/'ra2opengodot.csproj'),'--nologo']),
        ('godot-import',[a.godot,'--headless','--editor','--import','--quit','--path',str(project)]),
        ('gpu-effects',[a.godot,'--path',str(project),'--rendering-method','mobile',
          'res://tests/TestBuildingEffectsWindow.tscn','--','--game-data='+str(root),'--capture-dir='+str(out)])]
    for name,command in commands:
        with (out/(name+'.log')).open('w') as log:
            subprocess.run(command,check=True,timeout=180,stdout=log,stderr=subprocess.STDOUT)
    log=(out/'gpu-effects.log').read_text()
    if 'PASS: TestBuildingEffectsWindow' not in log or 'ERROR:' in log:raise RuntimeError(log)
    print('PASS: online/offline/destruction/rubble/reload through production GPU')
if __name__=='__main__':main()
