#!/usr/bin/env python3
"""Compare S09's original call site with the real compat initialization step.

Both paths execute the original DynamicVectorClass<Cell>::Clear, including
owned storage release. Only the allocator boundary is controlled. No Radar
object is reconstructed and no compiler-generated virtual table is rebound.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX
from compare_scenario import Machine, ENTRIES, OBJECT, SHA


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--dll', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    exe, dll = pefile.PE(str(args.exe)), pefile.PE(str(args.dll))
    ENTRIES['ScenarioClearRadarCells'] = 0x687C56
    receiver, table, clear = 0x880A0C, 0x7E3890, 0x42F420
    assert struct.unpack('<I', exe.get_data(table - exe.OPTIONAL_HEADER.ImageBase + 12, 4))[0] == clear
    results = []
    for kind in ('empty', 'borrowed', 'owned'):
        states = []
        for replacement in (False, True):
            machine = Machine(exe, dll, replacement)
            # Return after the exact original 687C56..687C60 call sequence.
            if not replacement:
                machine.hooks[0x687C63] = machine.ret
            buffer = OBJECT + 0x8000 if kind != 'empty' else 0
            if buffer:
                machine.uc.mem_write(buffer, bytes(range(12)))
            machine.uc.mem_write(receiver - 4, b'LEFT' + bytes(24) + b'RGHT')
            machine.uc.mem_write(receiver, struct.pack('<IIIBB2xii',
                table, buffer, 3 if buffer else 0, 1, kind == 'owned', 2 if buffer else 0, 19))
            calls = []

            def observe(uc, address, size, user_data):
                calls.append(uc.reg_read(UC_X86_REG_ECX))

            machine.uc.hook_add(UC_HOOK_CODE, observe, begin=clear, end=clear)
            # Repeat to prove that Clear does not destroy the member or free
            # an already released allocation on a second initialization step.
            machine.call('ScenarioClearRadarCells')
            machine.call('ScenarioClearRadarCells')
            assert calls == [receiver, receiver], (kind, replacement, calls)
            assert machine.freed == ([buffer] if kind == 'owned' else []), (kind, machine.freed)
            expected = struct.pack('<IIIBB2xii', table, buffer if kind == 'borrowed' else 0,
                                   0, 1, 0, 0, 19)
            state = bytes(machine.uc.mem_read(receiver - 4, 32))
            assert state == b'LEFT' + expected + b'RGHT', (kind, replacement, state.hex())
            if kind == 'borrowed':
                assert bytes(machine.uc.mem_read(buffer, 12)) == bytes(range(12))
            states.append(state)
        assert states[0] == states[1]
        results.append({'storage': kind, 'calls': 2, 'passed': True})
        print('PASS radar Clear boundary', kind)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({
        'exe_sha256': SHA, 'dll_sha256': hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'scope': __doc__, 'passed': len(results), 'cases': results,
    }, indent=2) + '\n')


if __name__ == '__main__':
    main()
