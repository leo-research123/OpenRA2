#!/usr/bin/env python3
"""Original-entry differential of SHP/SHA and the shared resource-name cache.

Executes actual EXE/DLL instructions. Only file services, allocation and CRT
primitives are substituted. This is not a Windows game integration test.
"""
import argparse
from bisect import bisect_right
import hashlib
import json
from pathlib import Path
import struct
import pefile
from unicorn import UcError
from unicorn.x86_const import *
from compare_image_pal import Machine, HEAP, STACK, SUPPORT, STOP, VTABLE, EXISTS, SIZE, READ, DESTRUCT, PRESERVED, SHA

MANIFEST = Path(__file__).resolve().parents[2] / "tests/fixtures/original_image_entries.json"
POINTS = json.loads(MANIFEST.read_text())["entries"]
RANGES = sorted((int(e["address"],16), int(e["end"],16)) for e in POINTS)
STARTS = [a for a,b in RANGES]
OBJECT, OBJECT2, OUTPUT = SUPPORT + 0x4000, SUPPORT + 0x4100, SUPPORT + 0x4200
NAMES = SUPPORT + 0x5000

def shape(flags=3, offset=32, width=3, height=1, color=(7, 8, 9)):
    return (struct.pack("<4h", 0, width, height, 1)
            + struct.pack("<4hI3BxIi", -2, 4, width, height, flags, *color, 0x87654321, offset)
            + b"\x06\x00\x01\x00\x01\x02")

class Resources(Machine):
    def __init__(self, exe, dll, files, replacement, fail=()):
        super().__init__(exe, dll, {}, False)
        self.replacement = replacement
        self.files, self.open_files, self.strings = files, {}, {}
        self.fail = set(fail)
        self.uc.mem_map(HEAP + 0x100000, 0x700000)
        self.uc.mem_write(HEAP + 0x100000, b"\xa5" * 0x700000)
        self.uc.mem_write(OBJECT, b"\xa5" * 0x300)
        for address, value in [(0xABF00C,0), (0xB077B0,0), (0xB077B4,0),
                               (0xB077B8,0), (0xB077BC,0), (0x83F870,1)]:
            self.write32(address, value)
        self.uc.mem_write(0xB077A0, struct.pack("<4i", 11, 12, 13, 14))
        self.hooks.update({0x473C50:self.exists, 0x473C00:self.size, 0x473B10:self.read,
                           0x7D5408:self.strdup, 0x7C93E8:self.free_name,
                           0x7DCFC4:self.upper, 0x7C8D20:lambda:self.ret(0)})
        for address, name in self.import_names.items():
            handlers = {"strlen":self.strlen, "strcpy":self.strcpy, "strstr":self.strstr,
                        "memmove":self.memcpy}
            if name in handlers:self.hooks[address]=handlers[name]
        if replacement:
            for entry in POINTS:
                site=int(entry["address"],16); target=self.exports[entry["export"]]
                length=len(bytes.fromhex(entry["expected"]))
                self.uc.mem_write(site, b"\xe9"+struct.pack("<I",(target-site-5)&0xffffffff)+b"\x90"*(length-5))

    def text(self, address):
        result=bytearray()
        while True:
            value=self.uc.mem_read(address,1)[0]
            if not value:return bytes(result)
            result.append(value);address+=1

    def name(self, value):
        address=NAMES+len(self.strings)*256
        if value not in self.strings:
            self.strings[value]=address;self.uc.mem_write(address,value.encode()+b"\0")
        return self.strings[value]

    def construct_file(self):
        obj=self.uc.reg_read(UC_X86_REG_ECX)
        name=self.text(self.arg(0)).decode()
        self.open_files[obj]=[name,0]
        # Match an empty CCFile Buffer; inline original cleanup may consume it.
        self.uc.mem_write(obj,bytes(0x6c))
        self.write32(obj,VTABLE)
        self.trace.append(["file",name])
        self.ret(obj,4)

    def active(self):return self.open_files[self.uc.reg_read(UC_X86_REG_ECX)]
    def payload(self):return self.files.get(self.active()[0].upper())

    def exists(self):
        assert self.arg(0)==0
        self.trace.append(["exists",self.active()[0]])
        self.ret(0xABCD0000|int(self.payload() is not None),4)

    def size(self):
        count=len(self.payload()) if self.payload() is not None else 0
        self.trace.append(["size",count]);self.ret(count)

    def read(self):
        dest,count=self.arg(0),self.arg(1)
        active=self.active();payload=self.payload() or b""
        data=payload[active[1]:active[1]+count];active[1]+=len(data)
        self.uc.mem_write(dest,data)
        self.trace.append(["read",count,len(data)])
        self.ret(len(data),8)

    def destroy_inline(self):
        self.trace.append(["close",self.active()[0]])
        del self.open_files[self.uc.reg_read(UC_X86_REG_ECX)]
        self.ret()

    def destroy_virtual(self):
        assert self.arg(0)==0
        self.trace.append(["close",self.active()[0]])
        del self.open_files[self.uc.reg_read(UC_X86_REG_ECX)]
        self.ret(0,4)

    def allocate(self):
        self.ret(self.allocate_block(self.arg(0)))

    def allocate_block(self,size):
        self.allocation_count+=1
        if self.allocation_count in self.fail:
            self.trace.append(["allocate",size,0]);return 0
        assert self.cursor+max(size,16)<HEAP+0x800000
        p=self.cursor;self.cursor+=(max(size,1)+15)&~15
        self.allocations[p]=size
        self.trace.append(["allocate",size,p]);return p

    def deallocate(self):
        p=self.arg(0)
        assert not p or p in self.allocations and p not in self.freed,("invalid delete",hex(p),self.trace)
        if p:self.freed.append(p)
        self.trace.append(["delete",p]);self.ret()

    def strdup(self):
        # Compare allocation/content/release effects, including the original
        # strdup's internal malloc. The candidate copies names inside core.
        data=self.text(self.arg(0))+b"\0";p=self.allocate_block(len(data))
        if p:self.uc.mem_write(p,data)
        self.ret(p)

    def free_name(self):
        self.deallocate()

    def upper(self):
        p=self.arg(0);self.uc.mem_write(p,self.text(p).upper()+b"\0");self.ret(p)
    def strlen(self):self.ret(len(self.text(self.arg(0))))
    def strcpy(self):
        p=self.arg(0);self.uc.mem_write(p,self.text(self.arg(1))+b"\0");self.ret(p)
    def strstr(self):
        p=self.arg(0);i=self.text(p).find(self.text(self.arg(1)));self.ret(0 if i<0 else p+i)

    def on_instruction(self,uc,address,size,user):
        if address==STOP:uc.emu_stop();return
        if self.replacement:
            i = bisect_right(STARTS, address) - 1
            if i >= 0:
                start,end=RANGES[i]
                assert not start<address<end,("original replacement body executed",hex(address),hex(start))
        if address in self.hooks:self.hooks[address]()

    def call(self,entry,ecx=0,edx=0,args=(),kind="void"):
        sp=STACK+0x80000;self.write32(sp,STOP)
        for i,a in enumerate(args):self.write32(sp+4+i*4,a)
        self.uc.reg_write(UC_X86_REG_ESP,sp)
        self.uc.reg_write(UC_X86_REG_ECX,ecx);self.uc.reg_write(UC_X86_REG_EDX,edx)
        for reg,value in PRESERVED.items():self.uc.reg_write(reg,value)
        try:self.uc.emu_start(entry,STOP+1,count=100000000)
        except UcError as exc:raise AssertionError((hex(entry),hex(self.uc.reg_read(UC_X86_REG_EIP)),str(exc),self.trace)) from exc
        assert self.uc.reg_read(UC_X86_REG_EIP)==STOP,("instruction limit",hex(entry))
        assert self.uc.reg_read(UC_X86_REG_ESP)==sp+4+len(args)*4,("stack",hex(entry))
        for reg,value in PRESERVED.items():assert self.uc.reg_read(reg)==value,("saved register",hex(entry),reg)
        value=self.uc.reg_read(UC_X86_REG_EAX)
        return value&255 if kind=="bool" else value if kind=="value" else None

    def snapshot(self):
        return {"trace":self.trace,"objects":bytes(self.uc.mem_read(OBJECT,0x224)).hex(),
                "globals":[self.read32(a) for a in (0xABF00C,0xB077B0,0xB077B4,0xB077B8,0xB077BC,0x83F870)],
                "allocations":[[p,n,hashlib.sha256(self.uc.mem_read(p,n)).hexdigest()]
                               for p,n in self.allocations.items() if p not in self.freed]}

def frame_case(m,flags,index,offset):
    m.uc.mem_write(OBJECT,shape(flags,offset))
    values=[]
    for entry,args,kind in [(0x69E7E0,(OUTPUT,index),"value"),(0x69E860,(OUTPUT,index),"value"),
                            (0x69E900,(index,),"bool"),(0x69E740,(index,),"value")]:
        m.uc.mem_write(OUTPUT,b"\xa5"*16)
        values.append([m.call(entry,OBJECT,args=args,kind=kind),bytes(m.uc.mem_read(OUTPUT,16)).hex()])
    return values

def lifecycle_case(m):
    m.call(0x69E430,OBJECT,args=(m.name("first.shp"),))
    m.call(0x69E430,OBJECT2,args=(m.name("second.sha"),))
    values=[m.call(0x69E580,OBJECT,kind="value"),m.call(0x69E580,OBJECT,kind="value")]
    values.append(m.call(0x69E580,OBJECT2,kind="value"))
    m.call(0x69E090,OBJECT)
    values.append(m.call(0x69E740,OBJECT,args=(0,),kind="value"))
    m.call(0x69E100,OBJECT)
    m.files["FIRST.SHP"]=shape(2,32,4)
    m.call(0x69E090,OBJECT)
    m.write32(OBJECT+0x20,5);m.write32(OBJECT2+0x20,10)
    m.call(0x69E320,4)
    values.append(m.snapshot())
    m.call(0x69E320,5)
    m.call(0x69E210)
    m.call(0x69E500,OBJECT)
    m.call(0x69E500,OBJECT2)
    return values

def cache_case(m):
    values=[]
    for name,forced in [("missing.shp",True),("first.shp",True),("FIRST.SHP",False),
                        ("config.ini",False),("CONFIG.INI",True),("second.sha",True),
                        ("name.shp.backup",False)]:
        values.append(m.call(0x5B40B0,m.name(name),int(forced),kind="value"))
    for name in ("config.ini","missing.pal"):
        values.append(m.call(0x4A38D0,m.name(name),OUTPUT,kind="value"))
        values.append(m.uc.mem_read(OUTPUT,1)[0])
    m.call(0x5B4270,m.name("config.ini"))
    values.append(m.call(0x5B40B0,m.name("config.ini"),0,kind="value"))
    m.call(0x69E180)
    m.call(0x69E210)
    m.call(0x5B4310)
    return values

def missing_case(m):
    m.call(0x69E430,OBJECT,args=(m.name("missing.sha"),))
    results=[m.call(0x69E580,OBJECT,kind="value")]
    results.append(m.call(0x69E740,OBJECT,args=(0,),kind="value"))
    m.files["MISSING.SHA"]=shape()
    results.append(m.call(0x69E580,OBJECT,kind="value"))
    m.call(0x69E210);m.call(0x69E500,OBJECT)
    return results

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe",type=Path,required=True);parser.add_argument("--dll",type=Path,required=True)
    parser.add_argument("--report",type=Path,required=True);args=parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    exe,dll=pefile.PE(str(args.exe)),pefile.PE(str(args.dll))
    cases=[]
    for flags in (0,1,2,3,0x80000002):
        for index in (-1,0,1):
            for offset in (0,32):cases.append((f"frame_{flags}_{index}_{offset}",lambda m,f=flags,i=index,o=offset:frame_case(m,f,i,o),()))
    cases += [("lifecycle",lifecycle_case,()),("cache",cache_case,()),("missing_retry",missing_case,()),
              ("cache_value_allocation_failure",cache_case,(2,)),
              # The two reference names now participate in the same allocation
              # counter; preserve the failure at the first shape-data allocation.
              ("shared_allocation_failure",lifecycle_case,(3,))]
    passed=[]
    for name,operation,fail in cases:
        results=[]
        for replacement in (False,True):
            m=Resources(exe,dll,{"FIRST.SHP":shape(),"SECOND.SHA":shape(0),"CONFIG.INI":b"[S]\nK=V\n",
                                 "NAME.SHP.BACKUP":shape()},replacement,fail)
            value=operation(m);results.append({"values":value,"state":m.snapshot()})
        if results[0]!=results[1]:
            args.report.parent.mkdir(parents=True,exist_ok=True)
            args.report.with_suffix('.failure.json').write_text(json.dumps({"case":name,"original":results[0],"replacement":results[1]},indent=2))
            raise AssertionError(f"differential mismatch: {name}; see failure report")
        passed.append(name)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps({"target_sha256":SHA,"dll_sha256":hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        "scope":"original entry instructions; shared file/allocator/CRT services substituted; no Windows game run",
        "passed":len(passed),"cases":passed},indent=2)+'\n')
    print(f"Passed {len(passed)} SHP/SHA/cache x86 differential cases")

if __name__=="__main__":main()
