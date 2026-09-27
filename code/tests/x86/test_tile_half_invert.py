#!/usr/bin/env python3
"""Compare the original TMP renderer against the compiled CORE implementation.

Only the baseline executes 547CF0. Candidate calls fail on that rendering entry.
Surface storage and original-layout TMP resource inputs
are fixtures; core executes clipping, Z/Alpha, shade remapping and inversion.
Checks the real 480499 CALL ABI, pixel writes and unmodified source ownership.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP
from compare_ini import Machine, SHA, STACK, STOP, SUPPORT

WIDTH, HEIGHT = 128, 96


class Renderer(Machine):
    def __init__(self, original, library, candidate, mask, x, y, clip, extra, z, mmx, *, flags=(0,0,0), intensity=1000, level=0, use_z=1, ring=0, invert=True, shades=53, translucent=True, alternate=0, lazy_variant=False):
        super().__init__(original)
        self.pixels=self.malloc(WIDTH*HEIGHT*2)
        self.uc.mem_write(self.pixels, struct.pack('<H',0x1234)*(WIDTH*HEIGHT))
        self.zdata=self.malloc(WIDTH*HEIGHT*2)
        self.adata=self.malloc(WIDTH*HEIGHT*2)
        self.uc.mem_write(self.zdata,b''.join(struct.pack('<H',0 if i%7==0 and z else 65535) for i in range(WIDTH*HEIGHT)))
        self.uc.mem_write(self.adata,b''.join(struct.pack('<H',96+i%80) for i in range(WIDTH*HEIGHT)))
        self.zobject,self.aobject=self.malloc(64),self.malloc(64)
        for address,data in [(self.zobject,self.zdata),(self.aobject,self.adata)]:
            self.write32(address+4,300)
            self.write32(address+16,ring*2)
            self.write32(address+36,0x8000)
            self.write32(address+28,data+WIDTH*HEIGHT*2)
            self.write32(address+32,WIDTH*HEIGHT*2)
            self.write32(address+40,WIDTH)
        self.write32(0x887644,self.zobject); self.write32(0x87e8a4,self.aobject)
        self.write32(0x886fa4,0)
        rules=self.malloc(6200);self.write32(0x8871e0,rules);self.uc.mem_write(rules+6114,bytes([translucent]))
        self.uc.mem_write(0xabc55c,bytes([mmx]))
        self.uc.mem_write(0xabc55d,b'\0')
        shifts=[11,3,0,3,5,2] if mask==0xffff else [10,3,0,3,5,3]
        for address,value in zip(range(0x8a0dd0,0x8a0de8,4),shifts): self.write32(address,value)
        self.write32(0x8a0de8,0x7bef if mask==0xffff else 0x3def)
        self.palette=self.malloc(shades*256*2)
        colors=[((i*197)&mask) | (0x8000 if mask==0x7fff else 0) for i in range(shades*256)]
        self.uc.mem_write(self.palette,struct.pack('<'+'H'*len(colors),*colors))
        self.convert=self.malloc(0x1b4)
        self.write32(self.convert+4,2); self.write32(self.convert+0x16c,shades); self.write32(self.convert+0x170,self.palette)
        self.remap=self.malloc(65536*2)
        values=[min(shades-1,(shades-1)*(i//256)*(i%256)//32258)<<8 for i in range(65536)]
        self.uc.mem_write(self.remap,struct.pack('<'+'H'*len(values),*values))
        self.tile=self.malloc(0x30c); self.vtable=self.malloc(256)
        self.surface=self.malloc(16); self.svtable=self.malloc(256)
        self.tmp=self.malloc(24); block=self.malloc(2304)
        self.uc.mem_write(self.tmp,struct.pack('<6I',1,1,60,30,block,0))
        self.write32(block+12,952)
        self.write32(block+36,(1 if extra else 0)|(2 if z else 0))
        self.uc.mem_write(block+52,bytes(1+i%255 for i in range(900)))
        self.uc.mem_write(block+952,bytes(i%4 for i in range(900)))
        if extra:
            for offset,value in [(8,1852),(16,2052),(20,-6),(24,-8),(28,20),(32,10)]: self.write32(block+offset,value)
            self.uc.mem_write(block+1852,bytes(0 if i%5==0 else 1+i%255 for i in range(200)))
            self.uc.mem_write(block+2052,bytes(i%4 for i in range(200)))
        self.write32(self.tile,self.vtable); self.write32(self.tile+188*4,1)
        self.write32(self.surface,self.svtable)
        self.write32(self.tile+0xa4,self.tmp)
        self.zsurface,self.asurface=self.malloc(16),self.malloc(16)
        for obj,surface,data in [(self.zobject,self.zsurface,self.zdata),(self.aobject,self.asurface,self.adata)]:
            self.write32(obj+24,data);self.write32(obj+20,surface);self.write32(surface,self.svtable)
        self.image_fetches=0
        if lazy_variant:
            variant=self.malloc(0x30c);self.write32(variant,self.vtable)
            self.write32(variant+0x2f0,1);self.write32(self.tile+0x2bc,variant);self.write32(self.tile+0x2f0,2)
        for slot,offset,function in [(self.vtable,156,self.get_image),
                (self.svtable,92,self.lock),(self.svtable,96,lambda:self.ret(1)),
                (self.svtable,112,lambda:self.ret(2)),(self.svtable,116,lambda:self.ret(WIDTH*2))]:
            address=SUPPORT+0x9000+len(self.hooks)*16
            self.hooks[address]=function; self.write32(slot+offset,address)
        self.hooks.update({0x420140:lambda:self.ret(self.remap,4),0x420270:lambda:self.ret(0,4),
            0x7bd130:lambda:self.buffer(self.zdata),0x4114b0:lambda:self.buffer(self.adata)})
        self.written=set()
        self.uc.hook_add(UC_HOOK_MEM_WRITE,self.track)
        if candidate:
            exports=self.load_library(library)
            self.call(exports['RA2TileInvert_SetEnabled'],0,int(invert))
            self.hooks[0x547cf0]=lambda:self.unexpected('CORE candidate called original TMP renderer')
            target=exports['RA2TileInvert_Draw']
            assert bytes(self.uc.mem_read(0x480499,5))==bytes.fromhex('e852780c00')
            self.uc.mem_write(0x480499,b'\xe8'+struct.pack('<I',(target-0x48049e)&0xffffffff))
        # The original function uses thiscall; the actual CALL site already has
        # all 0x44 argument bytes on its stack. Stop just after returning to it.
        self.hooks[0x48049e]=lambda:self.ret()
        args=[self.convert,0,self.surface,x,y,*clip,level,intensity,use_z,alternate,*flags,0x7bef]
        assert len(args)==17
        sp=STACK+0xf0000
        for i,value in enumerate([*args,STOP]): self.write32(sp+4*i,value)
        self.uc.reg_write(UC_X86_REG_ESP,sp); self.uc.reg_write(UC_X86_REG_ECX,self.tile)
        preserved=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]
        for register in preserved: self.uc.reg_write(register,0x13572468)
        try: self.uc.emu_start(0x480499,STOP+1,count=15000000)
        except Exception as error: raise RuntimeError(f'EIP {self.uc.reg_read(UC_X86_REG_EIP):x}: {error}') from error
        assert self.uc.reg_read(UC_X86_REG_EIP)==STOP,'instruction budget exhausted'
        assert self.uc.reg_read(UC_X86_REG_ESP)==sp+72,'CALL stack cleanup mismatch'
        assert all(self.uc.reg_read(reg)==0x13572468 for reg in preserved),'nonvolatile register clobber'
        assert self.read32(self.convert+0x170)==self.palette,'original palette pointer not restored'
        assert bytes(self.uc.mem_read(self.palette,len(colors)*2))==struct.pack('<'+'H'*len(colors),*colors),'original palette mutated'
        if candidate: assert self.image_fetches==int(lazy_variant), 'core dispatched back to the foreign resource loader'

    def get_image(self):
        self.image_fetches+=1
        self.write32(self.uc.reg_read(UC_X86_REG_ECX)+0xa4,self.tmp)
        self.ret(self.tmp)

    def lock(self):
        x,y=self.arg(0),self.arg(1)
        surface=self.uc.reg_read(UC_X86_REG_ECX)
        data=self.zdata if surface==self.zsurface else self.adata if surface==self.asurface else self.pixels
        self.ret(data+2*(y*WIDTH+x) if x<WIDTH and y<HEIGHT else 0,8)
    def buffer(self,data):
        offset=self.read32(self.uc.reg_read(UC_X86_REG_ECX)+16)
        self.ret(data+(2*(self.arg(1)*WIDTH+self.arg(0))+offset)%(WIDTH*HEIGHT*2),8)
    def track(self,uc,access,address,size,value,context):
        if self.pixels<=address<self.pixels+WIDTH*HEIGHT*2:
            self.written.update(range((address-self.pixels)//2,(address-self.pixels+size)//2))
    def memcmp(self):
        a=bytes(self.uc.mem_read(self.arg(0),self.arg(2))); b=bytes(self.uc.mem_read(self.arg(1),self.arg(2)))
        self.ret((a>b)-(a<b))
    def load_library(self,library):
        base=library.OPTIONAL_HEADER.ImageBase
        self.uc.mem_map(base,(library.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
        self.uc.mem_write(base,library.get_memory_mapped_image())
        for descriptor in getattr(library,'DIRECTORY_ENTRY_IMPORT',[]):
            for entry in descriptor.imports:
                name=entry.name.decode() if entry.name else str(entry.ordinal)
                address=SUPPORT+0xc000+len(self.hooks)*16
                self.write32(entry.address,address)
                self.hooks[address]={'memcmp':self.memcmp,'memcpy':self.memcpy,'memset':self.memset,'malloc':self.allocate,'free':lambda:self.ret(),'memmove':self.memcpy}.get(name,
                    lambda name=name:self.unexpected('candidate '+name))
        return {e.name.decode():base+e.address for e in library.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
    def state(self):
        return tuple(bytes(self.uc.mem_read(p,WIDTH*HEIGHT*2)) for p in [self.pixels,self.zdata,self.adata])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--dll',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    original,library=pefile.PE(str(args.exe)),pefile.PE(str(args.dll))
    rows=[]
    for mask in [0xffff,0x7fff]:
        for x,y,clip in [(20,20,[0,0,128,96]),(-20,-10,[0,0,128,96]),(90,80,[0,0,128,96]),
                (20,20,[35,27,30,13]),(20,20,[55,0,30,96]),(20,20,[0,0,40,96])]:
            for extra,z,mmx in [(False,False,False),(False,True,False),(True,True,False),(True,False,False)]:
                parameters=(mask,x,y,clip,extra,z,mmx)
                baseline=Renderer(original,library,False,*parameters)
                candidate=Renderer(original,library,True,*parameters)
                before=baseline.state(); after=candidate.state()
                expected=bytearray(before[0])
                for i in baseline.written:
                    if i%WIDTH<x+30:
                        value=struct.unpack_from('<H',expected,2*i)[0]
                        struct.pack_into('<H',expected,2*i,value^mask)
                passed=bool(baseline.written) and bytes(expected)==after[0] and before[1:]==after[1:] and baseline.written==candidate.written
                row=dict(mask=hex(mask),x=x,y=y,clip=clip,extra=extra,z=z,mmx=mmx,passed=passed,
                    writes=len(baseline.written),color_matches=bytes(expected)==after[0],z_alpha_matches=before[1:]==after[1:])
                rows.append(row)
                if not passed: print('FAIL',row,flush=True)
        print(hex(mask),'complete',flush=True)
    for flags,translucent in [((0,0,0),True),((1,0,0),True),((0,1,0),True),((0,0,1),True),((0,0,1),False)]:
        for ring in [0,WIDTH*HEIGHT-1000]:
            kw=dict(flags=flags,ring=ring,invert=False,translucent=translucent)
            baseline=Renderer(original,library,False,0xffff,20,20,[0,0,128,96],True,True,False,**kw)
            candidate=Renderer(original,library,True,0xffff,20,20,[0,0,128,96],True,True,False,**kw)
            passed=baseline.state()==candidate.state() and baseline.written==candidate.written
            row=dict(kind='plain_core_flags_ring',**kw,passed=passed,writes=len(baseline.written))
            rows.append(row)
            if not passed: print('FAIL',row,flush=True)
    for intensity,level,use_z,shades in [(-10,0,1,53),(0,0,1,53),(2000,8,1,53),(100000,0,0,3),(1000,0,1,3)]:
        kw=dict(intensity=intensity,level=level,use_z=use_z,shades=shades,invert=False)
        baseline=Renderer(original,library,False,0xffff,20,20,[0,0,128,96],True,True,False,**kw)
        candidate=Renderer(original,library,True,0xffff,20,20,[0,0,128,96],True,True,False,**kw)
        row=dict(kind='plain_core_lighting',**kw,passed=baseline.state()==candidate.state(),writes=len(baseline.written))
        rows.append(row)
        if not row['passed']: print('FAIL',row,flush=True)
    # The fingerprinted EXE skips the non-wrapping base path when ABC55C=1
    # (548913 branches straight to 54911E). Do not count a shared no-op as a
    # rendering success. Core always uses the functional scalar semantics.
    disabled=Renderer(original,library,False,0xffff,20,20,[0,0,128,96],False,True,True)
    assert not disabled.written, 'recheck the target acceleration branch calibration'
    kw=dict(alternate=3,lazy_variant=True,invert=False)
    baseline=Renderer(original,library,False,0xffff,20,20,[0,0,128,96],True,True,False,**kw)
    candidate=Renderer(original,library,True,0xffff,20,20,[0,0,128,96],True,True,False,**kw)
    rows.append(dict(kind='lazy_variant_resource_borrow',passed=baseline.state()==candidate.state(),resource_fetches=candidate.image_fetches))
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(original_sha256=SHA,dll_sha256=hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        scope=__doc__,disabled_acceleration_probe=dict(address='548913 -> 54911E',writes=len(disabled.written)),passed=sum(r['passed'] for r in rows),total=len(rows),cases=rows),indent=2)+'\n')
    print(sum(r['passed'] for r in rows),'/',len(rows),'passed')
    return 0 if all(r['passed'] for r in rows) else 1

if __name__=='__main__': raise SystemExit(main())
