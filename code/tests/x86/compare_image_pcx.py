#!/usr/bin/env python3
"""PCX original-entry instruction differential, including actual BSurface methods.

Only file/allocator/CRT services are doubles. Original BSurface lock, unlock,
queries and deletion consume the replacement's original-layout objects.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import pefile
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from compare_image_resources import Resources, OBJECT, OBJECT2, OUTPUT, SUPPORT, VTABLE, SHA

OPEN, SEEK, CLOSE = SUPPORT+0x1600, SUPPORT+0x1610, SUPPORT+0x1620
FILE = SUPPORT+0x4600
CUSTOM_HASH = SUPPORT+0x1630

def encode_row(data, force_runs=False):
    result=bytearray();i=0
    while i<len(data):
        end=i+1
        while end<len(data) and data[end]==data[i] and end-i<63:end+=1
        n=end-i
        if n>1 or data[i]>=192 or force_runs:result+=bytes([192+n,data[i]])
        else:result.append(data[i])
        i=end
    return bytes(result)

def pcx(width=3,height=2,planes=1,padding=0):
    h=bytearray(128);h[:4]=bytes([10,5,1,8]);struct.pack_into('<4H',h,4,0,0,width-1,height-1)
    h[65]=planes;struct.pack_into('<H',h,66,width+padding)
    data=bytearray(h)
    for y in range(height):
        row=[]
        for p in range(planes):row.extend([(17*x+31*y+73*p)%256 for x in range(width)]+[0]*padding)
        data+=encode_row(row,force_runs=bool(padding))
    if planes==1:data+=b'\x0c'+bytes((i*7)%256 for i in range(768))
    return bytes(data)

class PCXMachine(Resources):
    def __init__(self,exe,dll,files,replacement,fail=()):
        super().__init__(exe,dll,files,replacement,fail)
        self.hooks.pop(0x43AE50,None) # execute actual Buffer destructor
        self.hooks.update({OPEN:self.open,SEEK:self.seek,CLOSE:self.close,
                           0x7C978A:self.atexit})
        for offset,target in [(0x1c,OPEN),(0x28,SEEK),(0x34,CLOSE)]:self.write32(VTABLE+offset,target)
        self.uc.mem_write(0x8A0DD0,struct.pack('<6I',11,3,0,3,5,2))
        self.uc.mem_write(0xB0B930,b'\0')
        self.ignore_palette=False
        self.pcx_hash_callback=None
        self.code_ranges=[(dll.OPTIONAL_HEADER.ImageBase+s.VirtualAddress,
                          dll.OPTIONAL_HEADER.ImageBase+s.VirtualAddress+s.Misc_VirtualSize)
                         for s in (dll.sections if dll is not None else []) if s.Characteristics & 0x20000000]
        self.hooks[CUSTOM_HASH]=self.custom_hash
        if replacement:
            # The replacement must not need either original scratch storage or
            # its one-time guard. Leave both mapped for unrelated original code.
            for start,end in ((0xB0B930,0xB0B930),(0xB0B948,0xB0BC4B)):
                self.uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE,
                                 self.removed_state_access,begin=start,end=end)
        for address,name in self.import_names.items():
            if name=='strcmp':self.hooks[address]=self.strcmp

    def removed_state_access(self,uc,access,address,size,value,user):
        raise AssertionError(('removed PCX state accessed',hex(address),size))
    def custom_hash(self):
        key=self.read32(self.uc.reg_read(UC_X86_REG_ECX))
        self.trace.append(['custom_hash',self.text(key).hex() if key else None])
        self.ret(3) # Force collisions; every operation must use this callback.
    def strcmp(self):
        a,b=self.text(self.arg(0)),self.text(self.arg(1));self.ret((a>b)-(a<b))
    def atexit(self):self.trace.append(['atexit',self.arg(0)]);self.ret(0)
    def open(self):
        self.trace.append(['open',self.active()[0],self.arg(0)]);self.active()[1]=0;self.ret(1,4)
    def seek(self):
        offset=struct.unpack('<i',struct.pack('<I',self.arg(0)))[0];origin=self.arg(1)
        self.active()[1]=(len(self.payload()) if origin==2 else self.active()[1] if origin==1 else 0)+offset
        self.trace.append(['seek',offset,origin]);self.ret(self.active()[1],8)
    def close(self):self.trace.append(['file_close',self.active()[0]]);self.ret()
    def direct_file(self,name):self.open_files[FILE]=[name,0];self.write32(FILE,VTABLE);return FILE
    def surface(self,p):
        if not p:return None
        width,height,bpp=self.read32(p+4),self.read32(p+8),self.read32(p+16)
        data=self.read32(p+20)
        return [width,height,bpp,bytes(self.uc.mem_read(data,width*height*bpp)).hex()]
    def snapshot(self):
        result=super().snapshot()
        # 6BA220 is a bare RET. Removing only its registration and
        # replacing the hash function address are intentional implementation changes.
        empty_exit=['atexit',0x6BA220]
        assert self.trace.count(empty_exit) <= (0 if self.replacement else 1)
        result['trace']=[row for row in result['trace'] if row!=empty_exit]
        if self.pcx_hash_callback is not None:
            raw=bytearray.fromhex(result['objects'])
            assert struct.unpack_from('<I',raw,0x18)[0]==self.pcx_hash_callback
            raw[0x18:0x1c]=bytes(4)
            result['objects']=raw.hex()
        if self.ignore_palette:
            for row in result['allocations']:
                p,n,_=row
                if n==0x30c:
                    raw=bytearray(self.uc.mem_read(p,n));raw[8:776]=bytes(768)
                    row[2]=hashlib.sha256(raw).hexdigest()
        return result

def low_case(m,name,palette=True):
    m.uc.mem_write(OUTPUT,b'\xa5'*768)
    value=m.call(0x630310,m.direct_file(name),OUTPUT if palette else 0,args=(0,0),kind='value')
    result=[m.surface(value),bytes(m.uc.mem_read(OUTPUT,768)).hex()]
    if value:
        # Original virtual deleting destructor, consuming the new BSurface.
        m.call(m.read32(m.read32(value)),value,args=(1,),kind='value')
    return result

def construct_cache(m,custom_hash=False):
    m.call(0x6B9450,OBJECT,kind='value')
    callback=m.read32(OBJECT+0x18)
    if m.replacement:
        assert any(start<=callback<end for start,end in m.code_ranges), 'hash must point to local code'
    else:assert callback==0x6B94E0
    if custom_hash:
        callback=CUSTOM_HASH;m.write32(OBJECT+0x18,callback)
    m.pcx_hash_callback=callback

def cache_case(m,format=1,gray=0,expand=False,custom_hash=False):
    m.ignore_palette=format!=1
    construct_cache(m,custom_hash)
    values=[]
    count=105 if expand else 2
    for i in range(count):
        name=f'pic{i}.pcx';m.files[name.upper()]=pcx()
        values.append(m.call(0x6B9D00,OBJECT,args=(m.name(name),format,gray),kind='bool'))
    name=m.name('pic0.pcx')
    m.uc.mem_write(OUTPUT,b'\xa5'*768)
    p=m.call(0x6BA140,OBJECT,args=(name,OUTPUT if format==1 else 0),kind='value')
    values.append(m.surface(p))
    if format==1:values.append(bytes(m.uc.mem_read(OUTPUT,768)).hex())
    values.append(m.call(0x6BA140,OBJECT,args=(m.name('PIC0.PCX'),0),kind='value'))
    values.append(m.call(0x6BA140,OBJECT,args=(m.name('absent.pcx'),OUTPUT),kind='value'))
    # Same-name replacement frees old Surface before recording the new one.
    m.files['PIC0.PCX']=pcx(4,3)
    values.append(m.call(0x6B9D00,OBJECT,args=(name,format,gray),kind='bool'))
    m.call(0x6B9530,OBJECT)
    return values

def hash_callback_case(m):
    construct_cache(m)
    values=[]
    for text in (None,b'',b'pic0.pcx',b'PIC0.PCX',b'abc',b'\x80\xff',b'a'*260):
        if text is not None:m.uc.mem_write(FILE,text+b'\0')
        m.write32(OUTPUT,0 if text is None else FILE)
        values.append(m.call(m.pcx_hash_callback,OUTPUT,0x12345678,kind='value'))
    m.call(0x6B9530,OBJECT)
    return values

def empty_cache_case(m):
    construct_cache(m)
    m.uc.mem_write(OUTPUT,b'\xa5'*768)
    for name,palette in (('absent.pcx',OUTPUT),('',0)):
        assert m.call(0x6BA140,OBJECT,args=(m.name(name),palette),kind='value')==0
    assert bytes(m.uc.mem_read(OUTPUT,768))==b'\xa5'*768, 'miss must preserve palette output'
    m.call(0x6B9530,OBJECT)
    return True

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--dll',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    exe,dll=pefile.PE(str(args.exe)),pefile.PE(str(args.dll))
    cases=[]
    for planes in (1,3):
        for width,height in ((3,2),(8,4),(129,18)):
            for padding in (0,1):
                data=pcx(width,height,planes,padding)
                cases.append((f'read_{planes}_{width}_{height}_{padding}',lambda m:low_case(m,'test.pcx'),{'TEST.PCX':data},()))
    cases += [('missing',lambda m:low_case(m,'test.pcx'),{},()),
              ('header_failure',lambda m:low_case(m,'test.pcx'),{'TEST.PCX':bytes(128)},()),
              ('surface_allocation_failure',lambda m:low_case(m,'test.pcx'),{'TEST.PCX':pcx()},(1,)),
              ('rgb_row_allocation_failure',lambda m:low_case(m,'test.pcx'),{'TEST.PCX':pcx(3,2,3)},(3,)),
              ('indexed_cache',lambda m:cache_case(m),{},()),
              ('rgb565_cache',lambda m:cache_case(m,2),{},()),
              ('grayscale_cache',lambda m:cache_case(m,1,1),{},()),
              ('dictionary_growth_shrink',lambda m:cache_case(m,1,0,True),{},()),
              ('hash_callback_abi',hash_callback_case,{},()),
              ('empty_cache_miss',empty_cache_case,{},()),
              ('custom_hash_growth_shrink',lambda m:cache_case(m,expand=True,custom_hash=True),{},())]
    passed=[]
    for name,operation,files,fail in cases:
        results=[]
        for replacement in (False,True):
            m=PCXMachine(exe,dll,dict(files),replacement,fail)
            values=operation(m);results.append({'values':values,'state':m.snapshot()})
        if results[0]!=results[1]:
            args.report.parent.mkdir(parents=True,exist_ok=True)
            args.report.with_suffix('.failure.json').write_text(json.dumps({'case':name,'original':results[0],'replacement':results[1]},indent=2))
            raise AssertionError(f'differential mismatch: {name}; see failure report')
        passed.append(name)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps({'target_sha256':SHA,'dll_sha256':hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'scope':'original entry and BSurface instructions; file/allocator/CRT doubles; indeterminate format2 cache palette not compared; no Windows integration',
        'intentional_differences':['hash callback address normalized after local-code check; callback ABI and results exercised',
                                   'registration of empty 6BA220 exit callback omitted; other service calls still compared'],
        'replacement_checks':['no access to original PCX scratch or guard', 'per-object custom hash used for growth, lookup and shrink'],
        'passed':len(passed),'cases':passed},indent=2)+'\n')
    print(f'Passed {len(passed)} PCX x86 differential cases')

if __name__=='__main__':main()
