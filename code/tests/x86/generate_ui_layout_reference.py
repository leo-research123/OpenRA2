"""Generate geometry from fixed original x86 code, independently of the candidate."""
from pathlib import Path
import hashlib, json, struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
root=Path(__file__).resolve().parents[3]
exe=root/'out/reference/RA2MDddcompact/gamemd.exe'
sha=hashlib.sha256(exe.read_bytes()).hexdigest()
assert sha=='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
pe=pefile.PE(str(exe)); cpu=Uc(UC_ARCH_X86,UC_MODE_32); base=pe.OPTIONAL_HEADER.ImageBase
cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);cpu.mem_write(base,pe.get_memory_mapped_image())
scenario,point,stack,stop=0x1000000,0x1010000,0x1030000,0x103f000
cpu.mem_map(scenario,0x40000)
def put(a,*vs):cpu.mem_write(a,struct.pack('<'+'I'*len(vs),*(v&0xffffffff for v in vs)))
def read(a,n):return struct.unpack('<'+'i'*n,cpu.mem_read(a,n*4))
def call(entry,args=()):
 put(stack,stop,*args);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_ECX,point)
 cpu.emu_start(entry,stop,count=10000)
 assert cpu.reg_read(UC_X86_REG_EIP)==stop
 assert cpu.reg_read(UC_X86_REG_ESP)==stack+4*(len(args)+1)
 return cpu.reg_read(UC_X86_REG_EAX)
put(0xa8b230,scenario);cpu.mem_write(0xa8eb7c,b'\x01');cpu.mem_write(0xa8ed6b,b'\x00')
rows=[]
for w,h in [(640,480),(800,600),(1024,768),(1280,720),(1600,900),(1920,1080),(2560,1440),(3840,2160)]:
 for side in range(3):
  put(scenario+0x34b8,side);put(0xa8eb84,w,h);call(0x72ad90)
  view=read(point,4);put(0x886fa0,*view);put(0x886f90,w-168,158,168,h-158)
  call(0x6a5090);call(0x6a5130,(0,));count=call(0x6ac430)
  rows.append({'resolution':[w,h],'side_index':side,'view':view,'sidebar_body':read(0x886f90,4),
   'cameo_origin':read(0xb0b4f4,2),'cameo_pitch':read(0xb0b4fc,2),'cameo_count_raw':count,
   'cameo_height':read(0xb0b504,1)[0],'scroll_origin':read(0xb0b508,2),
   'over_original_60_button_capacity':count>60})
result={'exe_sha256':sha,'scope':__doc__,'entries':['72AD90','6A5090','6A5130','6AC430'],
 'inputs':{'sidebar_on_right':True,'editor_mode':False,'initial_sidebar_y':158},'observations':rows}
output=root/'code/tests/fixtures/game_ui_layout_reference.txt'
lines=['# Original gamemd '+sha, '# width height side; view[4]; sidebar[4]; cameo_origin[2]; pitch[2]; raw_count; scroll[2]']
for r in rows:
 values=[*r['resolution'],r['side_index'],*r['view'],*r['sidebar_body'],*r['cameo_origin'],*r['cameo_pitch'],r['cameo_count_raw'],*r['scroll_origin']]
 lines.append(' '.join(map(str,values)))
output.write_text('\n'.join(lines)+'\n')
for row in rows:
 if row['side_index']==0:print(row)
