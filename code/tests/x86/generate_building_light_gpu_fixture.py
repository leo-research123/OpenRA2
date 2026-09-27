#!/usr/bin/env python3
"""Fixed signed-glow and RGB565 cases from 0x005FF850/DSurface glow semantics (not an EXE replay).
Uses TestShapeGpuReference's generic compute-packet runner to test production GLSL.
"""
import argparse,struct
from pathlib import Path

def words(values):return struct.pack('<'+'H'*len(values),*values).hex()
def ints(values):return struct.pack('<'+'i'*len(values),*values).hex()
def generate(output):
 w=h=8;initial=[0x4208]*64;depth=[0x7ff0]*64
 text=['SHP_GPU_PACKETS_V1','8 8',words(initial),words(depth),words([0,127]*32),words([0]*256)]
 cases=[]
 for flags in range(16):
  p=[0]*20;p[:10]=[8,8,8,8,0,0,0,0,8,8];p[10]=1;p[11]=flags;p[19]=5
  mask=[0,64,128,255]*16;result=[]
  for strength in mask:
   channels=[64,64,64]
   for i,c in enumerate(channels):
    if flags&1:channels[i]=((256-strength)*c)>>8
    elif not(flags&(2<<i)):channels[i]=min(255,((256+strength)*c)>>8)
   r,g,b=channels;result.append((r>>3)<<11|(g>>2)<<5|(b>>3))
  cases.append((f'pool_flags_{flags}',p,mask,result))
 for z in (0x7fef,0x7ff0,0x7ff1):
  for strength in (-256,-3,128,300):
   p=[0]*20;p[:10]=[8,8,1,1,1,1,0,0,8,8];p[12]=strength;p[14]=z;p[19]=6
   result=initial.copy()
   if z<0x7ff0:
    c=min(255,64+((strength*64)>>8));result[9]=((c>>3)<<11|(c>>2)<<5|(c>>3))&0xffff
   cases.append((f'glow_z_{z}_strength_{strength}',p,[0],result))
 text.append(str(len(cases)))
 for name,p,mask,result in cases:text += [name,ints(p),ints(mask),words(result),words(depth)]
 output.write_text('\n'.join(text)+'\n');print(len(cases),'RGB565/depth cases')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);generate(p.parse_args().output)
