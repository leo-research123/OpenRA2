#!/usr/bin/env python3
"""Execute original INI enum, UUBlock and SHA/CRC routines and compare core.

Extends compare_ini.Machine: only CRT/string primitives and external streams
are replaced. All enum decisions, Base64 packets, SHA rounds and INI state
transitions execute from the fingerprinted EXE. No Windows process handoff.
"""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile

import pefile
from unicorn.x86_const import UC_X86_REG_EDX
from compare_ini import Machine, INI, STRAW, PIPE, SHA


class ValuesMachine(Machine):
    def __init__(self, original):
        super().__init__(original)
        self.hooks.update({0x7c8d20: self.compare, 0x7c8ef4: self.sprintf, 0x7c9cc2: self.strtok,
                          0x7ca530:self.sscanf, 0x7ca405:self.wcslen, 0x7ca422:self.wcsncpy})
        self.token = 0
        self.section, self.key = self.cstring(b'S'), self.cstring(b'K')

    def compare(self):
        a, b = self.string(self.arg(0)).lower(), self.string(self.arg(1)).lower()
        self.ret((a > b) - (a < b))

    def sprintf(self):
        fmt = self.string(self.arg(1))
        assert fmt in [b'%d',b'%d,%d,%d',b'%02d:%02d:%02d'], repr(fmt)
        numbers=[]
        for i in range(fmt.count(b'%')):
            number=self.arg(2+i)
            numbers.append(number if number<0x80000000 else number-0x100000000)
        data = fmt % tuple(numbers)
        self.uc.mem_write(self.arg(0), data+b'\0'); self.ret(len(data))

    def sscanf(self):
        text,fmt=self.string(self.arg(0)).decode(),self.string(self.arg(1)).decode()
        assert fmt in ['%d,%d','%d,%d,%d','%02d:%02d:%02d','%x'],repr(fmt)
        pos=count=0
        for piece in re.findall(r'%\d*[dx]|[^%]+',fmt):
            if not piece.startswith('%'):
                if not text[pos:].startswith(piece): break
                pos+=len(piece); continue
            while pos<len(text) and text[pos].isspace(): pos+=1
            width=int(piece[1:-1] or 0)
            remaining=text[pos:pos+width] if width else text[pos:]
            match=re.match(r'[+-]?(?:0[xX])?[0-9a-fA-F]+' if piece[-1]=='x' else r'[+-]?\d+',remaining)
            if not match: break
            self.write32(self.arg(2+count),int(match[0],16 if piece[-1]=='x' else 10))
            pos+=len(match[0]); count+=1
        self.ret(count)

    def wide(self,p):
        result=bytearray()
        while p:
            unit=bytes(self.uc.mem_read(p,2))
            if unit==b'\0\0': break
            result.extend(unit); p+=2
        return result

    def wcslen(self): self.ret(len(self.wide(self.arg(0)))//2)
    def wcsncpy(self):
        count=self.arg(2)*2
        self.uc.mem_write(self.arg(0),bytes(self.wide(self.arg(1)))[:count].ljust(count,b'\0')); self.ret(self.arg(0))

    def strtok(self):
        p = self.arg(0) or self.token
        delimiters = self.string(self.arg(1))
        while p and self.uc.mem_read(p, 1)[0] in delimiters: p += 1
        if not p or not self.uc.mem_read(p, 1)[0]: self.token = 0; self.ret(); return
        start = p
        while self.uc.mem_read(p, 1)[0] and self.uc.mem_read(p, 1)[0] not in delimiters: p += 1
        self.token = p + 1 if self.uc.mem_read(p, 1)[0] else 0
        self.uc.mem_write(p, b'\0'); self.ret(start)

    def clear(self):
        self.call(0x5257c0, INI, 0, 0)
        self.uc.mem_write(INI+64,b'\0')
        self.output = bytearray()

    def load(self, text):
        self.clear()
        self.input, self.position = text, 0
        self.call(0x525a60, INI, STRAW, 0)

    def enum(self, address, fallback, value):
        self.clear()
        if value is not None:
            self.call(0x528660, INI, self.section, self.key, self.cstring(value.encode()))
        result = self.call(address, INI, self.section, self.key, fallback)
        return result if result < 0x80000000 else result - 0x100000000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--corpus',type=Path,default=Path(__file__).parents[1]/'fixtures/ini_enum_corpus.json')
    args=parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    machine=ValuesMachine(pefile.PE(str(args.exe)))
    rows=[]
    def native(*arguments):
        return subprocess.check_output([str(args.native.resolve()), *map(str,arguments)],text=True).strip()
    def record(name, expected, actual):
        passed=expected==actual
        rows.append(dict(name=name,passed=passed,**({} if passed else dict(expected=expected,actual=actual))))
        if not passed: print('FAIL',name,repr(expected),repr(actual),flush=True)
    corpus=json.loads(args.corpus.read_text()); assert corpus['sha256']==SHA
    for reader in corpus['readers']:
        name,address=reader['name'],int(reader['address'],16)
        inputs=[(0,text) for text in reader['names']]
        inputs += [(0,text.swapcase()) for text in reader['names']]
        inputs += [(fallback,None) for fallback in reader['defaults']]
        inputs += [(0,text) for text in ['', '???', ' , ', ',Normal,Strong', 'Normal,Strong', 'Subterannean', '  Unknown  ']]
        for fallback,value in inputs:
            expected=machine.enum(address,fallback,value)
            arguments=['--enum',name,str(fallback)]+([] if value is None else [value])
            record(f'{name}:{fallback}:{value!r}',expected,int(native(*arguments)))
        print(name,len(inputs),'cases',flush=True)
    scalar_cases={
        '2':['','1,2','-2147483648,2147483647',' 1, -2 trailing','1,2,3'],
        '3':['','1,2,3','-1,256,511','1, 2, -3 trailing'],
        'Bytes':['','1,2,3','-1,256,511','255,255,255'],
        'Color':['','1,2,3','-1,256,511','123','1,bad,4','bad'],
        'Time':['','01:02:03','123:45:56','bad','-1:02:03','99:99:99',' 3:4:5','12','00:00:01'],
        'WriteTime':['0','59','60','223439','-1','-60','2147483647','-2147483648'],
        'Unicode':['','41,4e2d,d83d,de00,',',41,,42,','ffff,12345,','0x41,+42,'],
        'Abilities':['','FASTER','FASTER,STRONGER','FASTER,,STRONGER, FASTER,unknown','FASTER,STRONGER,FIREPOWER,SCATTER,ROF,SIGHT,CLOAK,TIBERIUM_PROOF,VEIN_PROOF,SELF_HEAL,EXPLODES,RADAR_INVISIBLE,SCATTER,FEARLESS,C4,TIBERIUM_HEAL,GUARD_AREA,CRUSHER']}
    scalar_addresses={'2':0x529880,'3':0x529ca0,'Bytes':0x474b50,'Color':0x474c70,'Time':0x52a760,
        'WriteTime':0x52a940,'Unicode':0x528f00,'Abilities':0x477640}
    for name,inputs in scalar_cases.items():
        for value in inputs:
            machine.clear()
            machine.call(0x528660,INI,machine.section,machine.key,machine.cstring(value.encode()))
            address=scalar_addresses[name]; output=machine.malloc(256)
            if name=='Time':
                result=machine.call(address,INI,machine.section,machine.key,77)
                expected=str(result if result<0x80000000 else result-0x100000000)
            elif name=='WriteTime':
                machine.call(address,INI,machine.section,machine.key,int(value))
                machine.call(0x528a10,INI,machine.section,machine.key,machine.cstring(b''),output,128)
                expected=machine.string(output).decode()
            elif name=='Unicode':
                default=machine.cstring('default\0'.encode('utf-16le'))
                count=machine.call(address,INI,machine.section,machine.key,default,output,128)
                units=struct.unpack('<'+'H'*count,machine.uc.mem_read(output,2*count))
                expected=','.join(map(str,[count,*units]))
            else:
                ints=name in ['2','3']
                default=machine.cstring(struct.pack('<3i',7,8,9) if ints else bytes([3])*18 if name=='Abilities' else bytes([7,8,9]))
                machine.call(address,INI,output,machine.section,machine.key,default)
                count=2 if name=='2' else 18 if name=='Abilities' else 3
                values=struct.unpack('<'+('i' if ints else 'B')*count,machine.uc.mem_read(output,count*(4 if ints else 1)))
                expected=','.join(map(str,values))
            record(f'scalar-{name}-{value}',expected,native('--scalar',name,value))
    for value in ['', 'POWER,FACTORY,BARRACKS,RADAR,TECH,PROC', 'POWER,POWER,unknown']:
        for empty in [False,True]:
            machine.clear(); machine.call(0x528660,INI,machine.section,machine.key,machine.cstring(value.encode()))
            source,output=machine.malloc(28),machine.malloc(28)
            machine.uc.mem_write(output,b'\xcd'*28)
            machine.call(0x477db0,source,0,0)
            if not empty:
                data=machine.malloc(40); machine.write32(data,9)
                machine.write32(source+4,data); machine.write32(source+8,10)
                machine.uc.mem_write(source+12,b'\1\1'); machine.write32(source+16,1)
            machine.write32(source+20,23); machine.write32(source+24,77)
            fields=struct.unpack('<7I',machine.uc.mem_read(source,28))
            machine.uc.reg_write(UC_X86_REG_EDX,INI)
            machine.call(0x4770e0,output,machine.section,machine.key,*fields)
            result=struct.unpack('<7I',machine.uc.mem_read(output,28))
            # Original does not initialize the TypeList extension; core explicitly
            # initializes it to zero. Compare the defined DynamicVector state.
            assert result[6]==0xcdcdcdcd
            values=struct.unpack('<'+'i'*result[4],machine.uc.mem_read(result[1],result[4]*4)) if result[4] else ()
            expected=','.join(map(str,[result[2],result[4],result[5],*values]))
            record(f'prerequisites-{empty}-{value}',expected,native('--prerequisites',value,*(['empty'] if empty else [])))
    randomizer=random.Random(474)
    with tempfile.TemporaryDirectory(prefix='ini-values-') as directory:
        path=Path(directory)/'input'
        for length in [0,1,2,3,7,8,40,41,42,43,50,51,52,53,54,55,56,57,62,63,64,65,100,512,1024]:
            # Exercise SHA padding both sides of 56 bytes and multiple blocks.
            text=b'[S]\nK='+b'x'*length+b'\n'; path.write_bytes(text); machine.load(text)
            crc=machine.call(0x476d80,INI)
            digest=bytes(machine.uc.mem_read(INI+65,20))
            machine.call(0x526470,INI,PIPE)
            assert digest==hashlib.sha1(machine.output).digest(), 'original SHA differs from standard within this size range'
            record(f'digest-{length}',digest.hex()+' '+str(crc),native('--digest',path))
        plain=b'[S]\nK=value\n'
        digest=hashlib.sha1(b'[S]\r\nK=value\r\n').digest()
        for name,text in [('empty',b''),('missing',plain),('matching',plain+b'[Digest]\n1='+base64.b64encode(digest)+b'\n'),
                ('mismatch',plain+b'[Digest]\n1='+base64.b64encode(bytes(20))+b'\n')]:
            path.write_bytes(text); machine.clear(); machine.input,machine.position=text,0
            status=machine.call(0x4741f0,INI,PIPE,1,0)
            digested=machine.uc.mem_read(INI+64,1)[0]
            sha=bytes(machine.uc.mem_read(INI+65,20)).hex() if digested else '-'
            machine.call(0x526470,INI,PIPE)
            expected=f'{status} {digested} {sha} {machine.output.hex()}'.strip()
            record(f'read-digest-{name}',expected,native('--read-digest',path))
        for length in [0,1,2,3,4,50,51,52,53,54,70,71,127,128,512,1024]:
            data=randomizer.randbytes(length); path.write_bytes(data); machine.clear()
            machine.call(0x526e80,INI,machine.cstring(b'B'),machine.cstring(data),length)
            machine.call(0x526470,INI,PIPE)
            record(f'UU-encode-{length}',machine.output.hex(),native('--uuencode',path))
        for i,raw in enumerate([b'YWJjZA==',b'YW\n1=JjZA==',b'====',b'A',b'AB',b'ABC',b'!!!!',b'!YWJ',b'Y!WJ',b'YW!J',b'YWJ!',b'=AAA',b'YQ==Yg==',b'YWJj'*40]):
            text=b'[B]\n9='+raw+b'\n'; path.write_bytes(text)
            for capacity in [1,2,3,4,128]:
                machine.load(text); output=machine.malloc(capacity)
                count=machine.call(0x526fb0,INI,machine.cstring(b'B'),output,capacity)
                expected=bytes(machine.uc.mem_read(output,count)).hex()
                record(f'UU-decode-{i}-{capacity}',expected,native('--uudecode',path,capacity))
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(original_sha256=SHA,native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(),
        scope=__doc__,passed=sum(r['passed'] for r in rows),total=len(rows),cases=rows),indent=2)+'\n')
    print(sum(r['passed'] for r in rows),'/',len(rows),'passed')
    return 0 if all(r['passed'] for r in rows) else 1


if __name__=='__main__': raise SystemExit(main())
