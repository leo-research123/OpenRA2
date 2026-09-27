#!/usr/bin/env python3
"""Execute original BitFont instructions to generate portable regression values.

Only CCFile byte I/O, allocation and Win32 SetRect are supplied by the harness.
Font parsing, fallback, measurement, lookup, clipping and drawing run from the
matching user-provided EXE. The output contains no original font or code bytes.
"""
import argparse
import hashlib
import random
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EIP, UC_X86_REG_ESP

SHA = "7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6"
BASE, HEAP = 0x1000000, 0x2000000
FONT, NAME, TEXT, WIDTH, HEIGHT = BASE, BASE+0x100, BASE+0x1000, BASE+0x2000, BASE+0x2004
PIXELS, SURFACE, VTABLE, FILE_VTABLE = BASE+0x3000, BASE+0x4000, BASE+0x5000, BASE+0x5100
STACK, STOP = BASE+0xe0000, BASE+0xff000
SETRECT, DESTROY, EXISTS, OPEN, READ, SEEK = [BASE+0x6000+16*i for i in range(6)]
LOCK, PITCH, GET_WIDTH, GET_HEIGHT, UNLOCK = [BASE+0x6100+16*i for i in range(5)]
CODES = [32,65,88,0x4e2d]
BITMAPS = bytes([3,0,0,0,0,0,0, 5,0x70,0,0x88,0,0xf8,0,
                 9,0x80,0x80,0x41,0,0x22,0, 9,0xff,0x80,0x80,0x80,0xff,0x80])

def fixture(ranges):
    data = bytearray(struct.pack('<7I', 0x744e6f46 if ranges else 0x546e6f66,9,2,3,4,4,7))
    if ranges:
        data += struct.pack('<2I',40,96) + b'\xdd'*4
        for i, code in enumerate(CODES): data += struct.pack('<3I',i,code,code)
        data += b'\xee'*8
    else:
        data += bytes(0x20000)
        for i,code in enumerate(CODES): struct.pack_into('<H',data,28+2*code,i+1)
    return bytes(data)+BITMAPS

def fnv(data):
    value=2166136261
    for byte in data: value=((value^byte)*16777619)&0xffffffff
    return value

class Original:
    def __init__(self, exe, payload):
        self.cpu=Uc(UC_ARCH_X86,UC_MODE_32)
        pe=pefile.PE(str(exe)); base=pe.OPTIONAL_HEADER.ImageBase
        self.cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
        self.cpu.mem_write(base,pe.get_memory_mapped_image())
        self.cpu.mem_map(BASE,0x100000); self.cpu.mem_map(HEAP,0x800000)
        self.payload=payload; self.position=0; self.cursor=HEAP; self.live={}
        self.write32(0x7e14b4,SETRECT)
        self.write32(SURFACE,VTABLE)
        for offset,target in [(92,LOCK),(96,UNLOCK),(116,PITCH),(124,GET_WIDTH),(128,GET_HEIGHT)]:
            self.write32(VTABLE+offset,target)
        for offset,target in [(0,DESTROY),(20,EXISTS),(28,OPEN),(36,READ),(40,SEEK)]:
            self.write32(FILE_VTABLE+offset,target)
        self.cpu.mem_write(NAME,b'test.fnt\0')
        self.hooks={0x7c8e17:self.allocate,0x7c9430:self.allocate,
            0x7c8b3d:self.free,0x7c93e8:self.free,0x4739f0:self.construct_file,
            DESTROY:self.destroy_file,EXISTS:lambda:self.ret(1,4),OPEN:self.open,
            READ:self.read,SEEK:self.seek,SETRECT:self.setrect,
            LOCK:lambda:self.ret(PIXELS,8),UNLOCK:lambda:self.ret(1),
            PITCH:lambda:self.ret(48),GET_WIDTH:lambda:self.ret(24),GET_HEIGHT:lambda:self.ret(10)}
        self.cpu.hook_add(UC_HOOK_CODE,self.instruction)

    def write32(self,address,value): self.cpu.mem_write(address,struct.pack('<I',value&0xffffffff))
    def read32(self,address): return struct.unpack('<I',self.cpu.mem_read(address,4))[0]
    def arg(self,index): return self.read32(self.cpu.reg_read(UC_X86_REG_ESP)+4+4*index)
    def ret(self,value=0,cleanup=0):
        stack=self.cpu.reg_read(UC_X86_REG_ESP)
        self.cpu.reg_write(UC_X86_REG_EIP,self.read32(stack))
        self.cpu.reg_write(UC_X86_REG_ESP,stack+4+cleanup)
        self.cpu.reg_write(UC_X86_REG_EAX,value&0xffffffff)
    def instruction(self,cpu,address,size,context):
        if address in self.hooks: self.hooks[address]()
    def allocate(self):
        size=self.arg(0); address=self.cursor
        self.cursor+=(max(size,1)+15)&~15
        assert self.cursor<HEAP+0x800000
        self.live[address]=size
        self.cpu.mem_write(address,b'\xa5'*max(size,1)); self.ret(address)
    def free(self):
        address=self.arg(0)
        if address: del self.live[address]
        self.ret()
    def construct_file(self):
        self.file=self.cpu.reg_read(UC_X86_REG_ECX)
        self.write32(self.file,FILE_VTABLE); self.ret(self.file,4)
    def destroy_file(self):
        if self.arg(0)&1: del self.live[self.file]
        self.ret(self.file,4)
    def open(self): self.position=0; self.ret(1,4)
    def read(self):
        target,count=self.arg(0),self.arg(1)
        data=self.payload[self.position:self.position+count]; self.position+=len(data)
        if data: self.cpu.mem_write(target,data)
        self.ret(len(data),8)
    def seek(self):
        offset,mode=self.arg(0),self.arg(1)
        self.position=(0 if mode==0 else self.position if mode==1 else len(self.payload))+offset
        self.ret(self.position,8)
    def setrect(self):
        target=self.arg(0)
        self.cpu.mem_write(target,struct.pack('<4I',*[self.arg(i) for i in range(1,5)]))
        self.ret(1,20)
    def call(self,address,args=(),obj=FONT):
        self.cpu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*[v&0xffffffff for v in args]))
        self.cpu.reg_write(UC_X86_REG_ESP,STACK); self.cpu.reg_write(UC_X86_REG_ECX,obj)
        self.cpu.reg_write(UC_X86_REG_EDX,0xdeadbeef)
        self.cpu.emu_start(address,STOP,count=4000000)
        assert self.cpu.reg_read(UC_X86_REG_EIP)==STOP,hex(address)
        assert self.cpu.reg_read(UC_X86_REG_ESP)==STACK+4*(len(args)+1),hex(address)
        return self.cpu.reg_read(UC_X86_REG_EAX)

def generate(exe,output):
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==SHA
    lines=[]
    for ranges in [False,True]:
        machine=Original(exe,fixture(ranges)); machine.call(0x433880,(NAME,))
        data=machine.read32(FONT+4)
        values=struct.unpack('<9I',machine.cpu.mem_read(data,36))
        assert values[:6]==(9,2,3,4,4,7) and values[8]==4
        assert bytes(machine.cpu.mem_read(values[7],28))==BITMAPS
        assert bytes(machine.cpu.mem_read(machine.read32(FONT+8),7))==bytes([9,0x7f,0x7f,0xbe,0xff,0xdd,0xff])
        machine.call(0x434a40,(0,)); assert not machine.live
    machine=Original(exe,fixture(False)); machine.call(0x433880,(NAME,))
    random_=random.Random(0x433cf0)
    texts=['','A','A A','AAA','A\r\nA','\t',' \tA ','AA AA AA','A\r\rA','A\n\rA','?中X']
    texts += [''.join(random_.choice(' AAX?中\t\n\r') for _ in range(random_.randrange(1,50))) for _ in range(35)]
    for text in texts:
        encoded=text.encode('utf-16-le')
        machine.cpu.mem_write(TEXT,encoded+b'\0\0')
        for maximum in [0,10,16,32,80]:
            result=machine.call(0x433cf0,(TEXT,WIDTH,HEIGHT,maximum))&255
            width,height=machine.read32(WIDTH),machine.read32(HEIGHT)
            units=','.join(f'{v:x}' for v in struct.unpack('<'+'H'*(len(encoded)//2),encoded)) or '-'
            lines.append(f'M {maximum} {units} {result} {width} {height}')
    bounds=[(0,0,23,9),(3,2,7,2),(2,1,19,8),(0,0,-1,-1)]
    positions=[(-4,-2),(0,0),(3,1),(22,8),(30,20)]
    for char in [65,88,63,0x4e2d,9]:
        for x,y in positions:
            for rectangle in bounds:
                machine.cpu.mem_write(PIXELS,struct.pack('<240H',*([0x1234]*240)))
                machine.cpu.mem_write(FONT+0x30,struct.pack('<4i',*rectangle))
                machine.call(0x4348f0,(SURFACE,))
                result=machine.call(0x434120,(char,x,y,0x4567))
                hash_=fnv(bytes(machine.cpu.mem_read(PIXELS,480)))
                result=result if result<0x80000000 else result-0x100000000
                lines.append('D '+' '.join(map(str,[char,x,y,*rectangle,result,hash_])))
                machine.call(0x434990,(SURFACE,))
                assert machine.read32(FONT+12)==machine.read32(FONT+16)==0
    machine.call(0x434a40,(0,)); assert not machine.live
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text('\n'.join(lines)+'\n')
    print(f'Original BitFont: two loaders and destructors, {len(texts)*5} measurements, 100 raster cases; {output}')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True); parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(); generate(args.exe,args.output)
