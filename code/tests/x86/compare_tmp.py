#!/usr/bin/env python3
"""Compare core TMP relocation/radar colors with original 547020 + 549E90.

CCFile I/O and CRT memory are harness boundaries. Original and core constructors,
TMP relocation, per-subtile radar construction and core teardown execute as code.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import pefile
from unicorn.x86_const import UC_X86_REG_FPCW
from compare_ini import Machine,SHA,SUPPORT
from test_tile_half_invert import Renderer
from compare_image_pal import initialize_exception_chain

class Resources(Machine):
    load_library=Renderer.load_library
    memcmp=Renderer.memcmp
    def __init__(self, exe, dll, rgb555=False):
        super().__init__(exe)
        self.uc.reg_write(UC_X86_REG_FPCW, 0x27f) # MSVC x87 CRT: 53-bit, round-to-nearest
        self.exports=self.load_library(dll)
        # Real Cell/map-root initialization now consumes the core's scoped
        # services. Map the DLL's loader-owned TLS instead of reading FS:2C
        # through the emulator's default zero segment.
        initialize_exception_chain(self.uc)
        if hasattr(dll,'DIRECTORY_ENTRY_TLS'):
            tls=dll.DIRECTORY_ENTRY_TLS.struct
            self.uc.mem_map(SUPPORT+0x20000,0x20000)
            self.write32(SUPPORT+0x12000+0x2c,SUPPORT+0x20000)
            self.write32(tls.AddressOfIndex,0)
            self.write32(SUPPORT+0x20000,SUPPORT+0x21000)
            self.uc.mem_write(SUPPORT+0x21000,bytes(self.uc.mem_read(
                tls.StartAddressOfRawData,tls.EndAddressOfRawData-tls.StartAddressOfRawData)))
        self.call(self.exports['TMPProbe_ColorMode'],0,int(rgb555))
        # Resolve the additional CRT primitives pulled by real type lifecycle.
        for descriptor in dll.DIRECTORY_ENTRY_IMPORT:
            for entry in descriptor.imports:
                name=(entry.name or b'').decode()
                address=self.read32(entry.address)
                if name=='strncpy': self.hooks[address]=self.strncpy
                if name=='__security_check_cookie': self.hooks[address]=lambda:self.ret()
                if name=='__std_find_trivial_4': self.hooks[address]=self.find4
                if name=='timeGetTime': self.hooks[address]=lambda:self.ret(0)
                # Single-thread loader fixture: the CRT still runs its actual
                # local-static guard; only OS synchronization is substituted.
                if name in ('AcquireSRWLockExclusive','ReleaseSRWLockExclusive','WakeAllConditionVariable'):
                    self.hooks[address]=lambda:self.ret(0,4)
                if name in ('_register_onexit_function','_crt_atexit'):
                    self.hooks[address]=lambda:self.ret(0)
        self.hooks[0x4739f0]=lambda:self.ret(0,4)
        self.hooks[0x473c00]=lambda:self.ret(len(self.input))
        self.hooks[0x473b10]=self.read_file
        self.hooks[0x43ae50]=lambda:self.ret()
        self.hooks[0x431b80]=lambda:self.ret()
        for address,value in zip(range(0x8a0dd0,0x8a0de8,4),[10 if rgb555 else 11,3,0,3,5,3 if rgb555 else 2]): self.write32(address,value)
        if 'TMPProbe_Initialize' in self.exports:
            self.call(self.exports['TMPProbe_Initialize'],0)
    def find4(self):
        start,end=self.arg(0),self.arg(1)
        value=self.arg(2)
        while start<end and self.read32(start)!=value: start+=4
        self.ret(start)
    def read_file(self):
        self.uc.mem_write(self.arg(0), self.input[:self.arg(1)])
        self.ret(min(len(self.input),self.arg(1)),8)
    def compare(self,data):
        self.input=data
        tile=self.malloc(0x30c)
        self.call(0x5447c0,tile,7,191,2,self.cstring(b'TMP'),0)
        self.uc.mem_write(tile+0x2f5,b'fixture.tem\0')
        size=self.call(0x547020,tile)
        assert size==len(data)
        tmp=self.read32(tile+0xa4)
        count=self.read32(tmp)*self.read32(tmp+4)
        expected=bytearray(self.uc.mem_read(tmp,16))+bytearray(self.uc.mem_read(tile+0x2e4,8))
        colors=self.read32(tile+0x2a8)
        for i in range(count):
            block=self.read32(tmp+16+i*4)
            expected+=bytes(self.uc.mem_read(block,52))+bytes(self.uc.mem_read(self.read32(colors+4*i),52)) if block else bytes(104)
        source=self.malloc(len(data)); self.uc.mem_write(source,data)
        output=self.malloc(len(expected));self.uc.mem_write(output,b'\xa5'*len(expected))
        actual_size=self.call(self.exports['TMPProbe_Read'],0,source,len(data),output)
        actual=bytes(self.uc.mem_read(output,len(expected)))
        assert bytes(self.uc.mem_read(source,len(data)))==data, 'core changed disk offsets'
        return actual_size==len(expected) and actual==expected, expected, actual

def fixture(seed):
    data=bytearray(24+52+900+900+12+12)
    struct.pack_into('<6I',data,0,2,1,60,30,24,0)
    struct.pack_into('<10i',data,24,100,200,1852,952,1864,92,195,4,3,7)
    for i in range(6): data[24+43+i]=(seed*61+i*37)%256
    for i in range(900): data[76+i]=i%256;data[976+i]=i%4
    for i in range(12): data[24+1852+i]=i;data[24+1864+i]=5
    return bytes(data)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',required=True,type=Path);p.add_argument('--dll',required=True,type=Path);p.add_argument('--report',required=True,type=Path)
    args=p.parse_args();assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    exe,dll=pefile.PE(str(args.exe)),pefile.PE(str(args.dll)); rows=[]
    for rgb555 in [False,True]:
        for seed in range(8):
            machine=Resources(exe,dll,rgb555);passed,expected,actual=machine.compare(fixture(seed))
            row=dict(seed=seed,rgb555=rgb555,passed=passed,expected=expected.hex(),actual=actual.hex());rows.append(row)
            print(rgb555,seed,passed,flush=True)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(scope=__doc__,exe_sha256=SHA,dll_sha256=hashlib.sha256(args.dll.read_bytes()).hexdigest(),cases=rows),indent=2)+'\n')
    return 0 if all(r['passed'] for r in rows) else 1
if __name__=='__main__': raise SystemExit(main())
