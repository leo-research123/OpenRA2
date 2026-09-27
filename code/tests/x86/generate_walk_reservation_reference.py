#!/usr/bin/env python3
"""Execute a shared five-infantry reservation sequence in original YR code.

Walk.Mark_Head_To -> Cell.FindInfantrySubposition -> Infantry mark/unmark and
Scenario.Random execute together. Map lookup/flat floor, owner index and the
no-crate result are supplied boundaries. No movement/path/mission loop claimed.
"""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP
from generate_building_attack_reference import Reference, SHA

CELLS = [(24+i,26) for i in range(5)] + [(32,32),(33,32)]
# (actor, operation): 0 reserve common cell, 1 release, 2 arrive, 3 reassign.
STEPS = [(i,0) for i in range(5)]+[(1,1),(4,0),(0,2),(2,3)]+[(i,1) for i in range(5)]
SPOTS = [(64,64),(128,128),(192,64),(64,192),(192,192)]

def generate(exe, output, report):
    r=Reference(exe); cpu=r.cpu
    feet=[0x13A0000+i*0x1000 for i in range(5)]
    drivers=[0x1390000+i*0x100 for i in range(5)]
    at=0x139F000;scenario=0x1383000
    r.put(0xA8B230,scenario);r.put(0xA8F234,416)
    for entry in (0x47B300,0x48E480,0x75AA50):r.call(entry)
    for offset,entry in ((0x184,0x5B3040),(0xF0,0x5217C0),(0xF4,0x521850),(0x38,r.stub+0xF0)):
        r.put(r.vt+offset,entry)
    def boundary(uc,address,size,data):
        sp=uc.reg_read(UC_X86_REG_ESP)
        uc.reg_write(UC_X86_REG_EAX,1 if address==0x481A00 else 0)
        uc.reg_write(UC_X86_REG_EIP,r.get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+(4 if address==0x481A00 else 0))
    # The EIP constant is imported explicitly below; no code bytes replaced.
    from unicorn.x86_const import UC_X86_REG_EIP
    for entry in (0x481A00,r.stub+0xF0):cpu.hook_add(UC_HOOK_CODE,boundary,begin=entry,end=entry)
    rows=[]
    for seed,spot,mask in itertools.product(range(8),range(5),(0,0x80,0x1C,0x20)):
        r.call(0x65C6D0,(seed,),scenario+0x218)
        for cell in CELLS:
            pointer=r.tile(*cell);r.put(pointer+0x124,0);r.put(pointer+0x54,-1)
        for i,(foot,driver) in enumerate(zip(feet,drivers)):
            cpu.mem_write(foot,bytes(0x1000));r.put(foot,r.vt);r.put(foot+0xAC,1);cpu.mem_write(foot+0x90,b'\x01')
            r.coords(foot+0x9C,((24+i)*256+SPOTS[spot][0],26*256+SPOTS[spot][1],0))
            r.call(0x75AA90,(),driver);r.put(driver+8,foot);r.put(driver+12,foot)
            r.call(0x5217C0,(foot+0x9C,),foot)
        r.put(r.tile(32,32)+0x124,mask)
        for step,(i,operation) in enumerate(STEPS):
            foot,driver=feet[i],drivers[i]
            if operation==2:
                head=struct.unpack('<3i',cpu.mem_read(driver+40,12))
                if head!=(0,0,0):r.coords(foot+0x9C,head)
            coord=(0,0,0) if operation in (1,2) else ((33 if operation==3 else 32)*256+SPOTS[spot][0],32*256+SPOTS[spot][1],0)
            r.coords(at,coord);result=r.call(0x75C240,(at,),driver)&0xFF
            state=[result]
            for d in drivers:state.extend(struct.unpack('<3i',cpu.mem_read(d+40,12)))
            for cell in CELLS:
                p=r.tile(*cell);state.extend((r.get(p+0x124),struct.unpack('<i',cpu.mem_read(p+0x54,4))[0]))
            state.extend((r.get(scenario+0x21C),r.get(scenario+0x220)))
            rows.append((seed,spot,mask,step,*state))
    output.write_text(''.join(' '.join(map(str,row))+'\n' for row in rows))
    report.write_text(json.dumps({'exe_sha256':SHA,'cases':len(rows),'scope':__doc__,
        'entries':['0x0075C240','0x00481180','0x005217C0','0x00521850','0x0065C7E0'],
        'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n')
    print(len(rows),'shared original reservation transitions')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for a in ('exe','output','report'):p.add_argument('--'+a,type=Path,required=True)
    a=p.parse_args();generate(a.exe,a.output,a.report)
