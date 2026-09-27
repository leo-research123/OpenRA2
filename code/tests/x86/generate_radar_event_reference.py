#!/usr/bin/env python3
"""Original x86 radar event updates, vertices and DSurface gradient pixels.

Only dirty-background restoration is skipped for Update. Gradient Surface stubs
supply memory, dimensions and locking. No candidate implementation is executed.
"""
import argparse,hashlib,json,math,random,struct
from pathlib import Path
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--images',type=Path);a=p.parse_args()
 if a.images:a.images.mkdir(parents=True,exist_ok=True)
 assert hashlib.sha256(a.exe.read_bytes()).hexdigest()==SHA
 pe=pefile.PE(str(a.exe));cpu=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.OPTIONAL_HEADER.ImageBase
 cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);cpu.mem_write(base,pe.get_memory_mapped_image())
 obj,rules,points,stack,stop=0x1000000,0x1002000,0x1005000,0x1010000,0x101f000
 cpu.mem_map(obj,0x200000)
 def put(address,*values):cpu.mem_write(address,struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values)))
 def words(address,count):return struct.unpack('<'+'I'*count,cpu.mem_read(address,4*count))
 def f(v):return struct.unpack('<I',struct.pack('<f',v))[0]
 def run(entry,args=()):
  put(stack,stop,*args);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_ECX,obj);cpu.reg_write(UC_X86_REG_FPCW,0xe7f)
  cpu.emu_start(entry,stop,count=2000000)
  assert cpu.reg_read(UC_X86_REG_EIP)==stop
  assert cpu.reg_read(UC_X86_REG_ESP)==stack+4*(len(args)+1)
 put(0x8871e0,rules);put(rules+120,f(.05),5,f(1),f(.1))
 cpu.mem_write(0x660540,b'\xc3') # Existing background is redrawn in full by the canvas.
 rows=['# '+SHA,'# U: type initial-radius-bits initial-angle-bits steps; resulting radius/angle/angular-speed/color/color-speed/duration-start/duration/visibility-start/visibility/rotating/visible']
 for t in range(17):
  for radius,angle in [(3,0),(20,math.pi/4),(60,1),(110,.3)]:
   for steps in (1,17,32,128,480,800):
    cpu.mem_write(obj,bytes(64));put(obj,t,0,0,f(radius),f(angle),f(.1),f(0),f(.05),0,0,0,0,0,0,0,0x101)
    for frame in range(1,steps+1):put(0xa8ed84,frame);run(0x65fe00)
    state=words(obj,16);rows.append(' '.join(map(str,['U',t,f(radius),f(angle),steps,*state[3:8],state[9],state[11],state[12],state[14],state[15]&255,(state[15]>>8)&255])))
 rng=random.Random(20260915)
 for _ in range(300):
  radius,angle=f(rng.uniform(0,200)),f(rng.uniform(-6.28,12.56));put(obj+12,radius,angle);run(0x660730,(points,))
  vertices=struct.unpack('<8i',cpu.mem_read(points,32));rows.append(' '.join(map(str,['V',radius,angle,*vertices])))
 # DSurface memory services, no drawing stubs. Fake object +4/+8/+12 = W/H/pixels.
 table,funcs,buffer=0x1020000,0x1021000,0x1030000
 put(obj,table,48,36,buffer)
 # GetRect(out): output {0,0,Width,Height}, EAX = out.
 cpu.mem_write(funcs,bytes.fromhex('8b442404c70000000000c74004000000008b51048950088b510889500cc20400'))
 cpu.mem_write(funcs+64,bytes.fromhex('b802000000c3')) # bytes per pixel
 cpu.mem_write(funcs+80,bytes.fromhex('8b4104d1e0c3')) # pitch
 cpu.mem_write(funcs+96,bytes.fromhex('8b4424080faf410403442404d1e003410cc20800')) # Lock(x,y)
 cpu.mem_write(funcs+128,bytes.fromhex('b801000000c3'))
 for slot,offset in [(0x78,0),(0x70,64),(0x74,80),(0x5c,96),(0x60,128)]:put(table+slot,funcs+offset)
 for addr,value in [(0x8a0dd0,11),(0x8a0dd4,3),(0x8a0dd8,0),(0x8a0ddc,3),(0x8a0de0,5),(0x8a0de4,2)]:put(addr,value)
 clip,start,end,first,last,step,phase=[0x1040000+i*32 for i in range(7)]
 put(clip,0,0,48,36);cpu.mem_write(first,bytes([255,0,255]));cpu.mem_write(last,bytes([128,0,128]))
 for _ in range(160):
  xy=[rng.randrange(-40,80),rng.randrange(-40,70),rng.randrange(-40,80),rng.randrange(-40,70)]
  fs,fp=f(rng.uniform(-.4,.4)),f(rng.uniform(0,1));put(start,*xy[:2]);put(end,*xy[2:]);put(step,fs);put(phase,fp)
  cpu.mem_write(buffer,b'\x34\x12'*(48*36));run(0x4bf750,(clip,start,end,first,last,step,phase))
  value=2166136261
  for byte in cpu.mem_read(buffer,48*36*2):value=((value^byte)*16777619)&0xffffffff
  rows.append(' '.join(map(str,['G',*xy,fs,fp,words(step,1)[0],words(phase,1)[0],value])))
 # Whole event drawing: original 660050 + original gradient/clip, replacing
 # only the Surface's geometry and memory access services, as above.
 surface=0x1041000
 put(surface,table,140,108,buffer);put(table+0x90,0x4bf750);put(0x880a04,surface)
 put(0x880c98,0) # Skip original final dirty rectangle union, not pixel drawing.
 for event_type in range(17):
  for _ in range(12):
   x,y=rng.randrange(-30,170),rng.randrange(-30,138)
   radius,angle=f(rng.uniform(.1,140)),f(rng.uniform(-6.28,12.56))
   speed,value=f(rng.uniform(-.4,.4)),f(rng.uniform(0,1))
   cpu.mem_write(obj,bytes(64));put(obj,event_type,x,y,radius,angle,f(.1),value,speed)
   cpu.mem_write(buffer,b'\x34\x12'*(140*108));run(0x660050)
   if a.images:(a.images/f'event-{event_type*12+_:03}.rgb565').write_bytes(bytes(cpu.mem_read(buffer,140*108*2)))
   pixel_hash=2166136261
   for byte in cpu.mem_read(buffer,140*108*2):pixel_hash=((pixel_hash^byte)*16777619)&0xffffffff
   rows.append(' '.join(map(str,['D',event_type,x,y,radius,angle,speed,value,pixel_hash])))
 # Original foundation vector storage: clear/count and fixed-capacity buffers.
 cpu.mem_write(funcs+160,bytes.fromhex('c7411000000000c3'));put(table+12,funcs+160)
 for i in range(22):put(obj+0x1278+i*24,table,0x1100000+i*0x8000,4096,0x101,0,0)
 for scale in (.01,.13,.3,.675,.9,1,1.8,3.5):
  put(obj+0x1488,f(scale));run(0x6563b0)
  for i in range(22):
   count=words(obj+0x1278+i*24+16,1)[0];value=2166136261
   for byte in cpu.mem_read(0x1100000+i*0x8000,count*8):value=((value^byte)*16777619)&0xffffffff
   rows.append(' '.join(map(str,['F',f(scale),i,count,value])))
 # Shroud/gap queries over original-layout Cells. Native tests construct the
 # same diamond normally; these are function inputs, not lifecycle evidence.
 cpu.mem_write(0x1100000,bytes(0x100000));put(0x87f924,0x1100000,262144);put(0xabde88,104)
 run(0x49f2f0)
 for y in range(25):
  for x in range(25):
   if x+y<=8 or x-y>=8 or y-x>=8 or x+y>24:continue
   address=0x1050000+(y*25+x)*0x148;put(0x1100000+(y*512+x)*4,address)
   put(address+0x24,x|(y<<16));put(address+0x12c,8 if (x*17+y*13)%3 else 0)
   put(address+0x13c,1 if (x+y)%5<2 else 0)
 cpu.mem_write(0xabdc50,bytes(0x148))
 for _ in range(400):
  world=(rng.randrange(-256,7000),rng.randrange(-256,7000),rng.randrange(-105,1500));put(points,*world)
  cpu.reg_write(UC_X86_REG_EDX,0)
  run(0x586360,(points,));shroud=cpu.reg_read(UC_X86_REG_EAX)&255
  run(0x5864a0,(points,));gap=cpu.reg_read(UC_X86_REG_EAX)&255
  rows.append(' '.join(map(str,['S',*world,shroud,gap])))
 a.output.write_text('\n'.join(rows)+'\n');print(len(rows)-2,'original samples')
if __name__=='__main__':main()
