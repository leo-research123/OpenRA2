#!/usr/bin/env python3
"""Fixed EXE/core Tactical construction comparison and COM conversion checks."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import pefile
from compare_scenario import Machine, ENTRIES, OBJECT, SHA


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--dll', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    a = p.parse_args()
    assert hashlib.sha256(a.exe.read_bytes()).hexdigest() == SHA
    exe, dll = pefile.PE(str(a.exe)), pefile.PE(str(a.dll))
    ENTRIES.update(TacticalConstruct=0x6D1C20, TacticalIdentity=0x6DC450, TacticalDestruct=0x6D1E80)
    states = []
    for candidate in (False, True):
        m = Machine(exe, dll, candidate)
        m.uc.mem_write(OBJECT, bytes(0xE18))
        m.uc.mem_write(0xB0CE08, struct.pack('<2i', 11, 13))
        m.uc.mem_write(0xB0CD60, struct.pack('<4i', 3, 5, 640, 400))
        for address, value in ((0xB0CD88, 0), (0xB0CE98, 0), (0xB0CD78, 60)):
            m.uc.mem_write(address, struct.pack('<d', value))
        # Original initialized sine table inputs for these zero-angle rotations.
        m.uc.mem_write(0x84F084, struct.pack('<f', 0))
        m.uc.mem_write(0x84F084 + 2048 * 4, struct.pack('<f', 1))
        m.call('TacticalConstruct')
        assert m.read32(0x887324) == OBJECT
        assert m.call('TacticalIdentity') == 56
        if candidate:
            lo, hi = dll.OPTIONAL_HEADER.ImageBase, dll.OPTIONAL_HEADER.ImageBase + dll.OPTIONAL_HEADER.SizeOfImage
            for offset in (0, 4, 8, 12):
                assert lo <= m.read32(OBJECT + offset) < hi, 'Tactical COM table outside core DLL'
            assert m.call('LocomotionCastChecks') == 0
        # The four interface vptrs belong to each module; all data is compared.
        states.append(bytes(m.uc.mem_read(OBJECT + 16, 0xE18 - 16)).hex())
        m.call('TacticalDestruct')
        assert m.read32(0x887324) == 0
    if states[0] != states[1]:
        left, right = bytes.fromhex(states[0]), bytes.fromhex(states[1])
        differences = [{'offset': hex(i + 16), 'original': left[i:i+4].hex(), 'core': right[i:i+4].hex()}
                       for i in range(0, len(left), 4) if left[i:i+4] != right[i:i+4]]
        a.report.with_suffix('.failure.json').write_text(json.dumps(differences, indent=2))
        raise AssertionError('Tactical constructor field or matrix mismatch; see failure report')
    a.report.write_text(json.dumps({'exe_sha256': SHA, 'dll_sha256': hashlib.sha256(a.dll.read_bytes()).hexdigest(),
        'tactical_data_bytes_compared': 0xE18 - 16, 'interface_tables': 4,
        'locomotion_cases': ['null', 'query failure', 'null queried interface', 'class ID failure', 'different class ID', 'adjusted pointer', 'smart pointer ownership'],
        'status': 'passed', 'scope': __doc__}, indent=2) + '\n')
    print('Tactical construction/destruction and 7 COM conversion cases passed')


if __name__ == '__main__':
    main()
