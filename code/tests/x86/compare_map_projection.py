#!/usr/bin/env python3
"""Compare compiled Tactical projection through the real compat state binding."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from compare_scenario import Machine, ENTRIES, OBJECT, SUPPORT, SHA
from unicorn.x86_const import UC_X86_REG_FPCW


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--dll', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    exe, dll = pefile.PE(str(args.exe)), pefile.PE(str(args.dll))
    ENTRIES.update(ScenarioTacticalHeight=0x6D20E0, ScenarioTacticalScreen=0x6D1F10, ScenarioTacticalClient=0x6D2140)
    machines = [Machine(exe, dll, candidate) for candidate in (False, True)]
    coord, point = SUPPORT + 0x4000, SUPPORT + 0x4100
    count = 0
    for line in args.fixture.read_text().splitlines():
        kind, *values = line.split()
        values = list(map(int, values))
        for m in machines:
            m.uc.reg_write(UC_X86_REG_FPCW, 0xE7F)
            m.uc.mem_write(0xB0CD48, struct.pack('<Q', 0x3FC25E5374344960))
            m.uc.mem_write(point - 4, bytes([0xA5]) * 16)
            if kind == 'H':
                result = m.call('ScenarioTacticalHeight', ecx=values[0])
                assert result == (values[1] & 0xFFFFFFFF), (kind, values, result, m.replacement)
            else:
                m.uc.mem_write(coord, struct.pack('<3i', *values[:3]))
                if kind == 'P':
                    assert m.call('ScenarioTacticalScreen', (point, coord)) == point
                    expected = values[3:5]
                else:
                    m.uc.mem_write(OBJECT + 0xB0, struct.pack('<2i', *values[3:5]))
                    m.uc.mem_write(0xB0CE28, struct.pack('<4i', 37, 53, *values[5:7]))
                    visible = m.call('ScenarioTacticalClient', (coord, point)) & 0xFF
                    assert visible == values[9], (kind, values, visible, m.replacement)
                    expected = values[7:9]
                actual = list(struct.unpack('<2i', m.uc.mem_read(point, 8)))
                assert actual == expected, (kind, values, actual, m.replacement)
                assert bytes(m.uc.mem_read(point - 4, 4)) == bytes([0xA5]) * 4
                assert bytes(m.uc.mem_read(point + 8, 4)) == bytes([0xA5]) * 4
        count += 1
    # Read live scale values, not a baked native constant in the game target.
    for scale in (0.0, 0.25, 0.5):
        for height in (-104, 0, 104, 727, 728, 2147483647):
            results = []
            for m in machines:
                m.uc.mem_write(0xB0CD48, struct.pack('<d', scale))
                results.append(m.call('ScenarioTacticalHeight', ecx=height))
            assert results[0] == results[1], (scale, height, results)
            count += 1
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'exe_sha256': SHA,
        'dll_sha256': hashlib.sha256(args.dll.read_bytes()).hexdigest(), 'cases': count,
        'status': 'passed', 'scope': __doc__,
        'checks': ['original outputs', 'live compat scale/viewport', 'stack balance',
                   'preserved registers', 'output canaries', 'no original projection fallback']}, indent=2) + '\n')
    print(f'{count} original/compiled projection cases passed')


if __name__ == '__main__':
    main()
