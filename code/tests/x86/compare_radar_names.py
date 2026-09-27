#!/usr/bin/env python3
"""Compare full original/compiled Radar DrawNames primitive traces.

Font/device submission, string lookup and CRT formatting are controlled
boundaries. House filtering, naming, counter accumulation and ordering execute.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[3]
SHA='7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
BASE,OBJ,STACK,STOP=0x01000000,0x01000000,0x0101E000,0x0101F000
SOURCES=['code/core/src/yrpp/RadarClassNames.cpp','code/tests/x86/radar_names_probe.cpp']
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def build(database,directory):
    return build_probe(database, directory, SOURCES, 'radar_names_probe.dll',
        object_suffix='.obj')
class Machine:
    def __init__(self,exe,dll,candidate):
        self.c=Uc(UC_ARCH_X86,UC_MODE_32);self.candidate=candidate
        for pe in (exe,dll):
            base=pe.OPTIONAL_HEADER.ImageBase;self.c.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
            self.c.mem_write(base,pe.get_memory_mapped_image())
        self.c.mem_map(BASE,0x400000);self.c.mem_map(0,4096);self.put(0,0xffffffff)
        self.exports={s.name.decode().strip('@').split('@')[0]:dll.OPTIONAL_HEADER.ImageBase+s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols}
        self.c.hook_add(UC_HOOK_CODE,self.hook);self.trace=[]
    def put(self,p,*v):self.c.mem_write(p,struct.pack('<'+'I'*len(v),*(n&0xffffffff for n in v)))
    def read(self,p,n=1):return struct.unpack('<'+'I'*n,self.c.mem_read(p,4*n))
    def wide(self,p):
        out=[]
        for i in range(256):
            c=struct.unpack('<H',self.c.mem_read(p+2*i,2))[0]
            if not c:return ''.join(out)
            out.append(chr(c))
        raise AssertionError('unterminated string')
    def setwide(self,p,s):self.c.mem_write(p,s.encode('utf-16-le')+b'\0\0')
    def ret(self,value=0,pop=0):
        sp=self.c.reg_read(UC_X86_REG_ESP);self.c.reg_write(UC_X86_REG_EIP,self.read(sp)[0])
        self.c.reg_write(UC_X86_REG_ESP,sp+4+pop);self.c.reg_write(UC_X86_REG_EAX,value)
    def hook(self,c,pc,size,data):
        sp=c.reg_read(UC_X86_REG_ESP)
        if pc==BASE+0xD000:
            out=self.read(sp+4)[0];self.put(out,0,0,168,720);self.ret(out,4)
        elif pc==BASE+0xD010:
            a,b,color=self.read(sp+4,3);start=self.read(a,2);end=self.read(b,2)
            self.trace.append(['line',*start,end[0]-start[0]+1,1,color]);self.ret(1,12)
        elif pc==0x72F510:self.ret(BASE+0xF000)
        elif pc==0x4AED70:
            shape,frame,point=self.read(sp+4,3);self.trace.append(['shape',frame,*self.read(point,2)]);self.ret(0,56)
        elif pc==0x68CA50:self.ret(BASE+0x11000)
        elif pc==0x734E60:
            label=c.reg_read(UC_X86_REG_ECX);name=bytes(c.mem_read(label,10))
            self.ret(BASE+(0x8000 if name.startswith(b'TXT_NAME') else 0x8100),8)
        elif pc==0x4A61C0:
            out,text,surface,clip,point,scheme,back,flags=self.read(sp+4,8)
            self.trace.append(['text',self.wide(text),*self.read(point,2),scheme,flags]);self.ret(out)
        elif pc==0x7CA564:
            out,fmt,value=self.read(sp+4,3);f=self.wide(fmt)
            s=self.wide(value) if f=='%s' else f'{struct.unpack("<i",struct.pack("<I",value))[0]:2d}'
            self.setwide(out,s);self.ret(len(s))
        elif pc==0x7CA405:self.ret(len(self.wide(self.read(sp+4)[0])))
        elif pc==0x7CA489:
            out,source=self.read(sp+4,2);self.setwide(out,self.wide(source));self.ret(out)
        elif pc==self.exports['RadarNamesText']:
            text,point,scheme,flags=self.read(sp+4,4);self.trace.append(['text',self.wide(text),*self.read(point,2),scheme,flags]);self.ret()
        elif pc==self.exports['RadarNamesShape']:
            frame,point=self.read(sp+4,2);self.trace.append(['shape',frame,*self.read(point,2)]);self.ret()
        elif pc==self.exports['RadarNamesLine']:
            rect,color=self.read(sp+4,2);self.trace.append(['line',*self.read(rect,4),color]);self.ret()
        elif pc==self.exports['RadarNamesFormat']:
            out,count,fmt,value=self.read(sp+4,4);s=f'{struct.unpack("<i",struct.pack("<I",value))[0]:2d}'
            assert len(s)<count;self.setwide(out,s);self.ret(len(s))
    def run(self,case):
        self.trace=[];self.put(0,0xffffffff)
        self.c.mem_write(OBJ,bytes(0x150C));self.c.mem_write(0x884B8D,bytes([case['active']]))
        self.put(OBJ+0x11E4,0,16,48,16,49,140,108)
        self.put(BASE+0xC000,0,158,168,562);self.put(BASE+0xC010,0,0,1280,720)
        self.put(0x887300,BASE+0xE000);self.put(BASE+0xE000,BASE+0xE100)
        self.put(BASE+0xE100+0x78,BASE+0xD000);self.put(BASE+0xE100+0x30,BASE+0xD010)
        self.put(0x87F6CC,BASE+0xF000);self.put(BASE+0xF004,2);self.put(BASE+0xF174,BASE+0xF200)
        self.c.mem_write(BASE+0xF21C,struct.pack('<H',0x1234))
        self.setwide(BASE+0x8000,'Name:');self.setwide(BASE+0x8100,'Kills:')
        self.put(0xB054D4,BASE+0x10000);self.put(0xB054E0,4)
        for i in range(4):
            p=BASE+0x11000+i*0x400;self.put(BASE+0x10000+4*i,p)
            self.put(p+0x304,BASE+0x12000);self.put(p+0x310,1)
        self.c.mem_write(BASE+0x12000,b'Grey\0')
        self.put(0xA8022C,BASE+0x13000);self.put(0xA80238,len(case['houses']))
        for i,h in enumerate(case['houses']):
            p=BASE+0x20000+i*0x18000;self.put(BASE+0x13000+4*i,p if h else 0)
            if not h:continue
            self.c.mem_write(p,bytes(0x17000));self.put(p+0x34,p+0x17000)
            self.c.mem_write(p+0x171A5,bytes([h['multi']]))
            self.c.mem_write(p+0x1F5,bytes([h['defeated']]))
            self.setwide(p+0x1602A,h['name']);self.put(p+0x16054,h['scheme'])
            self.put(p+0x53E4,*h['units']);self.put(p+0x5438,*h['buildings'])
        self.put(STACK,STOP);self.c.reg_write(UC_X86_REG_ESP,STACK);self.c.reg_write(UC_X86_REG_ECX,OBJ)
        self.c.reg_write(UC_X86_REG_FPCW,0xE7F)
        regs=(UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)
        for i,r in enumerate(regs):self.c.reg_write(r,0x12345000+i)
        entry=self.exports['RadarNamesDraw'] if self.candidate else 0x653FA0
        self.c.emu_start(entry,STOP,count=1000000)
        assert self.c.reg_read(UC_X86_REG_EIP)==STOP
        assert self.c.reg_read(UC_X86_REG_ESP)==STACK+4
        assert all(self.c.reg_read(r)==0x12345000+i for i,r in enumerate(regs))
        return self.trace
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('exe','compile-db','output'):p.add_argument('--'+arg,type=Path,required=True)
    a=p.parse_args();assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True)
    dllpath=build(a.compile_db,a.output.resolve());exe,dll=pefile.PE(str(a.exe)),pefile.PE(str(dllpath))
    machines=[Machine(exe,dll,c) for c in (False,True)];cases=[]
    def house(name,multi=1,defeated=0,scheme=1,overflow=False):
        return dict(name=name,multi=multi,defeated=defeated,scheme=scheme,
                    units=[0x7fffffff if overflow else i for i in range(20)],buildings=[i*3 for i in range(20)])
    for active,name,defeated,multi in itertools.product((0,1),('', 'A','123456789012345678','12345678901234567890','盟军指挥官'),(0,1),(0,1)):
        cases.append(dict(active=active,houses=[None,house(name,multi,defeated),house('Second',scheme=2),house('Overflow',overflow=True)]))
    cases+= [dict(active=1,houses=[]),dict(active=1,houses=[house('Player'+str(i),scheme=i%4) for i in range(12)])]
    failures=[]
    for i,case in enumerate(cases):
        traces=[]
        for m in machines:
            try:traces.append(m.run(case))
            except Exception as error:
                raise RuntimeError(f'case={i} candidate={m.candidate} pc={m.c.reg_read(UC_X86_REG_EIP):#x} trace={m.trace}') from error
        if traces[0]!=traces[1]:failures.append(dict(case=i,input=case,original=traces[0],candidate=traces[1]))
    report=dict(scope=__doc__,exe_sha256=SHA,dll_sha256=digest(dllpath),cases=len(cases),mismatches=len(failures),
                source_sha256={s:digest(ROOT/s) for s in SOURCES},examples=failures[:3],all_compared_contracts_match=not failures)
    (a.output/'names-comparison.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
    print(json.dumps(report,indent=2,ensure_ascii=False));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
