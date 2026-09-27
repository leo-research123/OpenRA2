#!/usr/bin/env python3
"""Execute YR building range and approach decisions with real coordinate dispatch.

Building GetCoords/center/target entries, foundation dimensions, range arithmetic,
quantized math and the approach candidate loop execute original instructions.
Flat map lookup/height, type/weapon getters and COM identity are fixture boundaries.
Approach reachability is a connected flat-map distance fixture; native AStar and
dynamic group occupancy are validated separately, not covered by this corpus.
Range cases include flat ground, a cliff strip and an unowned wall strip, with
M60 projectile flags off/on. Bridges, allied wall transparency, arcing and full
frame simulation are excluded. Approach cases use the connected flat map.
"""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

SHA = '7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'

class Reference:
    def __init__(self, executable):
        assert hashlib.sha256(executable.read_bytes()).hexdigest() == SHA
        pe = pefile.PE(str(executable)); self.cpu = cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        base = pe.OPTIONAL_HEADER.ImageBase
        cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF)
        cpu.mem_write(base, pe.get_memory_mapped_image()); cpu.mem_map(0x1000000, 0x400000)
        self.obj, self.target, self.typ, self.btype, self.weapon, self.bullet, self.slot = [0x1000000+n for n in (0,0x1000,0x3000,0x5000,0x7000,0x8000,0x9000)]
        self.vt, self.bvt, self.cvt, self.stub, self.stack, self.stop = [0x1000000+n for n in (0xA000,0xB000,0xC000,0xD000,0xE000,0xF000)]
        self.cells, self.rules, self.source = 0x1100000, 0x1300000, 0x1009800
        self.terrain = 0
        self.driver, self.dvt = 0x1009900, 0x1009A00
        self.put(self.obj,self.vt); self.put(self.target,self.bvt)
        cpu.mem_write(self.bvt, bytes(cpu.mem_read(0x7E3EBC, 0x600)))
        assert self.get(self.bvt+0x48)==0x447AC0 and self.get(self.bvt+0x58)==0x410540
        for table in (self.vt,self.bvt):
            self.put(table+0x50,self.stub+0x20); self.put(table+0x54,self.stub+0x30)
        self.put(self.vt+0x48,0x5F65A0); self.put(self.vt+0x4C,0x4DBDF0)
        for offset, n in ((0x2C,0),(0x84,1),(0x3F8,4),(0x400,3),(0x2E4,9),(0x168,5),(0x3A8,6),(0x480,10),(0x3C8,11)):
            self.put(self.vt+offset,self.stub+n*0x10)
        self.put(self.cvt+0x48,self.stub+0x80)
        self.put(self.target+0x520,self.btype); self.put(self.slot,self.weapon)
        self.put(self.vt+0x168,0x7012C0) # Real GetWeaponRange, ordinary infantry (not OpenTopped).
        self.put(self.weapon+0xAC,0x1382000)
        self.put(0xA83D84,0x1380000); self.put(0x1380000,0x1381000)
        cpu.mem_write(0x1381000+0x2A8,b"\x01") # OverlayType.Wall
        self.put(self.weapon+0xA0,self.bullet); self.put(self.obj+0x6C0,self.typ)
        self.put(0x8871E0,self.rules); self.put(0xB0EB24,624); self.put(0xB0EB34,104)
        self.put(self.rules+0x1838,2)
        cpu.mem_write(self.rules+0x1840,struct.pack("<dd",1.0,2.0))
        self.put(self.driver,self.dvt);self.put(self.obj+0x674,self.driver)
        self.put(self.driver+8,self.obj);self.put(self.dvt+0x18,0x75AC00)
        cpu.mem_write(self.obj+0x684,b"\xFF") # No tunnel; real Foot destination forwards to Walk head.
        for offset,n in ((0,12),(4,13),(8,13),(12,14)):self.put(self.dvt+offset,self.stub+n*0x10)
        self.put(self.typ+0x5B4,0);self.put(self.typ+0x67C,0)
        cpu.mem_write(self.typ+0xD33,b'\x01\x01');cpu.mem_write(self.obj+0x3D5,b'\x01')
        self.put(self.obj+0xAC,1);self.put(self.target+0x14,3)
        cpu.mem_write(0x89EA40,struct.pack('<f',1.0))
        self.coords(self.target+0x9C,(32*256+128,32*256+128,0))
        for y in range(65):
            for x in range(65):
                cell=self.tile(x,y); self.put(cell,self.cvt)
                cpu.mem_write(cell+0x24,struct.pack('<hh',x,y)); self.put(cell+0x44,-1)
        for address in (*range(self.stub,self.stub+0xF0,0x10),0x565730,0x5657A0,0x578080,0x578460,0x56D230,0x42D170,0x480510):
            cpu.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)

    def put(self,a,v): self.cpu.mem_write(a,struct.pack('<I',v&0xFFFFFFFF))
    def get(self,a): return struct.unpack('<I',self.cpu.mem_read(a,4))[0]
    def coords(self,a,v): self.cpu.mem_write(a,struct.pack('<3i',*v))
    def tile(self,x,y):
        assert 0<=x<65 and 0<=y<65,(x,y)
        return self.cells+(y*65+x)*0x148
    def boundary(self,cpu,address,size,data):
        sp=cpu.reg_read(UC_X86_REG_ESP); this=cpu.reg_read(UC_X86_REG_ECX)
        pop=result=0
        if address==self.stub: result=15
        elif address==self.stub+0x10: result=self.typ
        elif address==self.stub+0x20: result=1
        elif address==self.stub+0x30: pass
        elif address==self.stub+0x40: result=self.slot; pop=4
        elif address==self.stub+0x50: result=self.get(self.weapon+0xB4); pop=4
        elif address==self.stub+0x60:
            # Preserve the real virtual range function, including Building.GetCoords.
            cpu.reg_write(UC_X86_REG_EIP,0x6F77B0); return
        elif address==self.stub+0x70:
            result=self.get(sp+4); cpu.mem_write(result,bytes(cpu.mem_read(this+0x9C,12))); pop=8
        elif address==self.stub+0x80:
            result=self.get(sp+4); x,y=struct.unpack('<hh',cpu.mem_read(this+0x24,4))
            self.coords(result,(x*256+128,y*256+128,self.level(x,y)*104)); pop=4
        elif address==self.stub+0x90: pop=4 # SelectWeapon
        elif address==self.stub+0xA0:self.put(self.obj+0x5A4,self.get(sp+4));pop=8
        elif address==self.stub+0xB0:self.put(self.obj+0x2B4,self.get(sp+4));pop=4
        elif address==self.stub+0xC0:self.put(self.get(sp+12),self.driver);pop=12 # COM QueryInterface
        elif address==self.stub+0xD0:result=1;pop=4
        elif address==self.stub+0xE0:
            cpu.mem_write(self.get(sp+8),bytes(cpu.mem_read(0x7E9A60,16)));pop=8
        elif address==0x578460:
            x,y=struct.unpack('<hh',cpu.mem_read(self.get(sp+4),4));result=int(x+y>32 and abs(x-y)<32 and x+y<=96);pop=8
        elif address==0x56D230:result=2;pop=12
        elif address==0x42D170:
            x,y=struct.unpack('<hh',cpu.mem_read(self.get(sp+4),4));a,b=struct.unpack('<hh',cpu.mem_read(self.get(sp+8),4))
            result=max(abs(x-a),abs(y-b));pop=24
        elif address==0x480510:pop=8 # Wall-destroying warheads are excluded.
        elif address==0x578080:
            x,y=struct.unpack("<ii",cpu.mem_read(self.get(sp+4),8));result=self.level(int(x/256),int(y/256))*104;pop=4
        elif address in (0x565730,0x5657A0):
            arg=self.get(sp+4)
            if address==0x5657A0: x,y=struct.unpack('<hh',cpu.mem_read(arg,4))
            else:
                x,y=struct.unpack('<ii',cpu.mem_read(arg,8)); x=int(x/256); y=int(y/256)
            result=self.tile(x,y); pop=4
        else: raise AssertionError(hex(address))
        cpu.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF)
        cpu.reg_write(UC_X86_REG_EIP,self.get(sp)); cpu.reg_write(UC_X86_REG_ESP,sp+4+pop)
    def call(self,entry,args=(),this=None):
        self.cpu.mem_write(self.stack,struct.pack('<'+'I'*(1+len(args)),self.stop,*[a&0xFFFFFFFF for a in args]))
        self.cpu.reg_write(UC_X86_REG_ESP,self.stack); self.cpu.reg_write(UC_X86_REG_ECX,self.obj if this is None else this)
        self.cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        try:self.cpu.emu_start(entry,self.stop,count=1000000)
        except Exception as e: raise RuntimeError(f'entry {entry:#x}, PC {self.cpu.reg_read(UC_X86_REG_EIP):#x}') from e
        assert self.cpu.reg_read(UC_X86_REG_EIP)==self.stop
        assert self.cpu.reg_read(UC_X86_REG_ESP)==self.stack+4*(1+len(args))
        return self.cpu.reg_read(UC_X86_REG_EAX)

    def level(self,x,y):return 4 if self.terrain==1 and 29<=x<=30 else 0
    def terrain_pattern(self,terrain):
        self.terrain=terrain
        for y in range(65):
            for x in range(65):
                cell=self.tile(x,y)
                self.cpu.mem_write(cell+0x11B,bytes([self.level(x,y)]))
                self.put(cell+0x44,0 if terrain==2 and x==30 else -1)

    def occupy(self,mode):
        for y in range(65):
            for x in range(65):
                blocked=(mode==1 and max(abs(x-32),abs(y-32))>=3) or (mode==2 and x<31) or (mode==3 and (x*17+y*31)%5<3)
                self.cpu.mem_write(self.tile(x,y)+0x124,bytes([4 if blocked else 0]))

def generate_approach(executable,output,report):
    ref=Reference(executable);rows=[]
    for mode in range(4):
      ref.occupy(mode)
      for foundation,direction,query,near,prior,multiplier in itertools.product((0,3,4,6,7,17,20),range(8),(0,1),(0,1),range(4),(1,2)):
        x,y=((3072,0),(3072,1024),(3072,3072),(0,3072),(-3072,3072),(-3072,0),(-3072,-3072),(0,-3072))[direction]
        if near:x//=2;y//=2
        old=((-1,-1),(35,35),(28,32),(39,32))[prior]
        ref.put(ref.rules+0xDF8,multiplier)
        ref.put(ref.btype+0xEF0,foundation);ref.put(ref.weapon+0xB4,1024);ref.put(ref.weapon+0xB8,0)
        ref.cpu.mem_write(ref.bullet+0x296,b"\x01\x01\x01") # M60 / InvisibleLow
        ref.put(ref.obj+0x2B4,ref.target);ref.put(ref.obj+0x5A4,ref.tile(*old) if prior else 0)
        ref.coords(ref.obj+0x9C,(8320+x,8320+y,0))
        try:pointer=ref.call(0x4D5690,(query,))
        except Exception as e:
            sp=ref.cpu.reg_read(UC_X86_REG_ESP)
            raise RuntimeError(f'approach {foundation=} {direction=} {mode=} {query=} {prior=}, return {ref.get(sp):#x}') from e
        dest=struct.unpack('<hh',ref.cpu.mem_read(pointer+0x24,4)) if pointer else (-1,-1)
        assigned=ref.get(ref.obj+0x5A4)
        assigned_coords=struct.unpack('<hh',ref.cpu.mem_read(assigned+0x24,4)) if assigned else (-1,-1)
        rows.append((foundation,x,y,mode,query,*old,multiplier,*dest,*assigned_coords))
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(''.join(' '.join(map(str,row))+'\n' for row in rows))
    report.parent.mkdir(parents=True,exist_ok=True)
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'entries':['0x004D5690','0x006F7220','0x006F77B0','0x00447AC0','0x004834A0','0x007012C0','0x004DBDF0','0x0075AC00'], 'columns':'foundation source_offset_xy occupancy query prior_destination_xy reset_multiplier result_xy assigned_destination_xy','fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original building approach cases')

def generate(executable,output,report):
    ref=Reference(executable); rows=[]
    widths=(1,2,1,2,2,3,3,3,4,3,1,3,4,1,1,2,2,5,4,3,6,0)
    heights=(1,1,2,2,3,2,3,5,2,3,3,1,3,4,5,6,5,3,4,4,4,0)
    for terrain,flags in itertools.product(range(3),(0,1)):
        ref.terrain_pattern(terrain)
        ref.cpu.mem_write(ref.bullet+0x296,bytes([flags,flags,flags]))
        for foundation,direction,edge,z,cell_range in itertools.product((0,3,4,6,7,17,20),range(8),(-257,-1,0,1,257), (0,104,416),(0,1)):
            radius=1024+(widths[foundation]+heights[foundation])*64+edge
            x,y=((radius,0),(radius,64),(radius,radius),(0,radius),(-radius,radius),(-radius,0),(-radius,-radius),(0,-radius))[direction]
            ref.put(ref.btype+0xEF0,foundation);ref.put(ref.weapon+0xB4,1024);ref.put(ref.weapon+0xB8,0)
            ref.cpu.mem_write(ref.weapon+0x134,bytes([cell_range]));ref.coords(ref.obj+0x9C,(8320+x,8320+y,z));ref.coords(ref.source,(8320+x,8320+y,z))
            coordinate=ref.call(0x6F7220,(ref.source,ref.target,ref.weapon))&0xFF
            virtual=ref.call(0x6F77B0,(ref.target,0))&0xFF
            rows.append((foundation,x,y,z,cell_range,terrain,flags,coordinate,virtual))
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(''.join(' '.join(map(str,row))+'\n' for row in rows))
    report.parent.mkdir(parents=True,exist_ok=True)
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,'entries':['0x006F7220','0x006F77B0','0x00447AC0'],'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'original building range cases')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--approach',action='store_true')
    for arg in ('exe','output','report'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();(generate_approach if a.approach else generate)(a.exe,a.output,a.report)
