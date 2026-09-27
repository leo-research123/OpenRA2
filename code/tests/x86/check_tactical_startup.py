#!/usr/bin/env python3
"""Calibrate host Tactical constructor inputs from actual YR startup/table code."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from compare_scenario import Machine, ENTRIES, OBJECT, SUPPORT, SHA


def initialize(m):
    previous = m.replacement
    m.replacement = False
    for entry in (0x6D1830, 0x6D18A0, 0x6D18C0, 0x6D1BB0):
        ENTRIES['startup'] = entry
        m.call('startup')
    m.replacement = previous


def inputs(m):
    values = []
    for source in (0xB0CD88, 0xB0CE98):
        angle = struct.unpack('<f', struct.pack('<f', struct.unpack('<d', m.uc.mem_read(source,8))[0]))[0]
        for method in (0x4CACB0, 0x4CAD00):
            low, high = struct.unpack('<2I',struct.pack('<d',angle))
            # Caller-only ABI shim: pass a rounded float as double, execute the
            # actual table lookup, and store ST(0) at float precision. Distinct
            # code addresses avoid stale translated blocks in the emulator.
            code = b'\x68'+struct.pack('<I',high)+b'\x68'+struct.pack('<I',low)
            code += b'\xb8'+struct.pack('<I',method)+b'\xff\xd0\x83\xc4\x08\xd9\x1d'
            code += struct.pack('<I',SUPPORT+0x9000)+b'\xc3'
            address = SUPPORT+0x8000+64*len(values)
            m.uc.mem_write(address,code)
            ENTRIES['table'] = address
            m.call('table')
            values.append(struct.unpack('<I',m.uc.mem_read(SUPPORT+0x9000,4))[0])
    scale = 60 / struct.unpack('<d',m.uc.mem_read(0xB0CD78,8))[0]
    values.append(struct.unpack('<I',struct.pack('<f',scale))[0])
    return values


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','dll','report'): parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    exe,dll=pefile.PE(str(args.exe)),pefile.PE(str(args.dll))
    ENTRIES.update(TacticalConstruct=0x6D1C20, TacticalDestruct=0x6D1E80)
    machines=[Machine(exe,dll,candidate) for candidate in (False,True)]
    states=[]
    bits=None
    for m in machines:
        initialize(m)
        m.uc.mem_write(OBJECT,bytes(0xE18))
        if m.replacement:
            m.uc.mem_write(SUPPORT+0xA000,struct.pack('<5I',*bits))
            m.call('TacticalConstructWithParameters',(SUPPORT+0xA000,))
        else:
            bits=inputs(m)
            m.call('TacticalConstruct')
        states.append(bytes(m.uc.mem_read(OBJECT+16,0xE18-16)))
        m.call('TacticalDestruct')
        assert m.read32(0x887324)==0
    if states[0]!=states[1]:
        print('input bits', [hex(x) for x in bits])
        print('differing DWORDs',[(hex(i+16),states[0][i:i+4].hex(),states[1][i:i+4].hex())
              for i in range(0,len(states[0]),4) if states[0][i:i+4]!=states[1][i:i+4]])
        raise AssertionError('calibrated Tactical constructor differs')
    args.report.write_text(json.dumps({'exe_sha256':SHA,'dll_sha256':hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'input_float_bits':dict(zip(('sin_x','cos_x','sin_z','cos_z','scale'),map(hex,bits))),
        'matrix_float_bits':[hex(x) for x in struct.unpack('<24I',states[0][0xDB4-16:0xE14-16])],
        'data_bytes_compared':0xE18-16,'status':'passed','scope':__doc__},indent=2)+'\n')
    print('Calibrated Tactical construction passed:',[hex(x) for x in bits])


if __name__=='__main__': main()
