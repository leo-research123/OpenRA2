#!/usr/bin/env python3
"""Compare normally constructed Mouse/Map roots against fixed original code.

The original write mask excludes padding and unspecified fields, and explicitly
replaces vtable/owned-pointer equality with compiler/ownership checks.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import UC_HOOK_MEM_WRITE
from compare_scenario import Machine, ENTRIES, OBJECT, SUPPORT, SHA


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'dll', 'report'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    exe, dll = pefile.PE(str(args.exe)), pefile.PE(str(args.dll))
    ENTRIES['ScenarioMapRootConstruct'] = 0x5BDA40
    # Constructors may not fall back to an original class or collection ctor.
    forbidden = [0x692290,0x6CFE20,0x6A4E60,0x63F6B0,0x652960,0x4A8730,0x565090,0x4F4220]
    for entry in forbidden: ENTRIES['forbid_'+hex(entry)] = entry
    size = 0x556C
    vtables = [0,0x5518,0x50,0x8C,0xA4,0xBC,0xD4,0x138,0x115C,0x1224,0x125C]
    vtables += [0x1278 + 24*i for i in range(22)]
    cases = []
    for fill in (0,0xA5):
        machines = [Machine(exe,dll,candidate) for candidate in (False,True)]
        mask = bytearray(size)
        def record(uc,access,address,width,value,data):
            begin, end = max(address,OBJECT), min(address+width,OBJECT+size)
            if begin < end: mask[begin-OBJECT:end-OBJECT] = bytes([255])*(end-begin)
        for m in machines:
            m.write32(0xA8B230, SUPPORT + 0x9000)
            m.uc.mem_write(OBJECT, bytes([fill])*size)
            m.uc.mem_write(OBJECT+size, bytes([0xCC])*16)
        hook = machines[0].uc.hook_add(UC_HOOK_MEM_WRITE,record)
        for m in machines:
            assert m.call('ScenarioMapRootConstruct') == OBJECT
        machines[0].uc.hook_del(hook)
        for start in vtables + [0x1258]: mask[start:start+4] = bytes(4)
        left,right = [bytes(m.uc.mem_read(OBJECT,size)) for m in machines]
        differences = [(hex(i),a,b) for i,(a,b,check) in enumerate(zip(left,right,mask)) if check and a!=b]
        assert not differences, differences[:30]
        for m in machines:
            assert bytes(m.uc.mem_read(OBJECT+size,16)) == bytes([0xCC])*16
            assert m.clock_calls == 3
            table = m.read32(OBJECT+0x1258)
            assert table and m.read32(table+8)==256 and m.read32(table+12)==10
            buckets = m.read32(table)
            for index in range(256):
                bucket = buckets+index*24
                assert m.read32(bucket+4)==0 and m.read32(bucket+8)==0
                assert m.read32(bucket+16)==0 and m.read32(bucket+20)==10
            if m.replacement:
                owned_table = table
                for offset in vtables:
                    table = m.read32(OBJECT+offset)
                    assert dll.OPTIONAL_HEADER.ImageBase <= table < dll.OPTIONAL_HEADER.ImageBase+dll.OPTIONAL_HEADER.SizeOfImage
                m.call('ScenarioMapRootDestroy')
                assert sorted(p for p in m.freed if p) == sorted([owned_table, buckets-4]), ('owned hash and bucket array released',
                    list(map(hex,m.freed)),list(map(hex,[owned_table,buckets-4])))
        cases.append({'fill':fill,'compared_bytes':sum(bool(b) for b in mask),'status':'passed'})
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'exe_sha256':SHA,'dll_sha256':hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'cases':cases,'scope':__doc__,'checks':['original initialized fields','two compiler interface vtables',
        'nested collection vtables','radar hash buckets','clock units and order','stack and preserved registers',
        'no original hierarchy constructor fallback'],
        'excluded':['original unspecified fields and padding','art/surface lifecycle','Tab command notification behavior',
        'full game-world resize and pathfinding']},indent=2)+'\n')
    print('2 original/compiled map-root construction cases passed')


if __name__ == '__main__': main()
