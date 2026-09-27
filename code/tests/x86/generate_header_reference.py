#!/usr/bin/env python3
"""Replay fixed gamemd scalar CRC and BitFont lock instructions.

CRC has no service stubs. BitFont uses the existing deterministic Surface
callbacks from generate_bitfont_reference. This is instruction replay, not a
Windows game integration test. Outputs contain values, never code bytes.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
from generate_bitfont_reference import Original, SHA, FONT, SURFACE, PIXELS, LOCK, UNLOCK, GET_HEIGHT, GET_WIDTH, PITCH

ENTRIES = {'b': 0x4A1CA0, 's': 0x4A1D30, 'i': 0x4A1D50, 'f': 0x4A1D70, 'd': 0x4A1D90}


def generate(exe, output, report):
    assert hashlib.sha256(exe.read_bytes()).hexdigest() == SHA
    machine = Original(exe, b'')
    rng = random.Random(0x4A1CA0)
    lines, overruns = [], 0
    for kind, entry in ENTRIES.items():
        bits = [0, 1] if kind == 'b' else [0, 1, 0x80000000, 0x7FC12345, 0xFFFFFFFF]
        if kind == 'd': bits += [0x8000000000000000, 0x7FF812345678ABCD, 0x3FF8000000000000]
        if kind != 'b': bits += [rng.getrandbits(64 if kind == 'd' else 32) for _ in range(32)]
        for index in range(4):
            for value in bits:
                crc, staging = rng.getrandbits(32), rng.getrandbits(32)
                machine.cpu.mem_write(FONT, struct.pack('<4I', crc, index, staging, 0xA5A5A5A5))
                args = (value & 0xFFFFFFFF, value >> 32) if kind == 'd' else (value,)
                machine.call(entry, args)
                # Original Value() is inlined; null byte-input takes its same
                # finalization branch without appending anything.
                result = machine.call(0x4A1DE0, (0, 0))
                after = struct.unpack('<4I', machine.cpu.mem_read(FONT, 16))
                overruns += after[3] != 0xA5A5A5A5
                lines.append(' '.join(map(str, [kind, value, crc, index, staging, result, *after[:3]])))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines) + '\n')
    lock_rows = []
    calls = []
    def surface_result(name, value, cleanup=0):
        calls.append(name)
        machine.ret(value, cleanup)
    machine.hooks[LOCK] = lambda: surface_result('Lock', PIXELS, 8)
    machine.hooks[UNLOCK] = lambda: surface_result('Unlock', 0)
    machine.hooks[GET_WIDTH] = lambda: surface_result('GetWidth', 2)
    machine.hooks[PITCH] = lambda: surface_result('GetPitch', 5)
    for height in [1, 2, 256, 257, 258]:
        machine.hooks[GET_HEIGHT] = lambda: surface_result('GetHeight', height)
        for bounds in [(0, 0, 0, 0), (0, 0, 1, height - 1), (5, 5, 6, 6)]:
            calls.clear()
            machine.cpu.mem_write(FONT + 0x30, struct.pack('<4i', *bounds))
            result = machine.call(0x4348F0, (SURFACE,))
            graph_buffer = machine.read32(FONT+12)
            pitch = machine.read32(FONT+16)
            after_bounds = struct.unpack('<4i', machine.cpu.mem_read(FONT+0x30, 16))
            expected_bounds = (0, 0, -1, -1) if bounds[0] == 5 else (0, 0, 1, height-1)
            assert graph_buffer == PIXELS and pitch == 2 and after_bounds == expected_bounds
            assert calls == ['Lock', 'GetPitch', 'GetWidth', 'GetHeight']
            unlocked = machine.call(0x434990, (SURFACE,))
            assert machine.read32(FONT+12) == machine.read32(FONT+16) == 0
            assert calls == ['Lock', 'GetPitch', 'GetWidth', 'GetHeight', 'Unlock']
            lock_rows.append({'height': height, 'bounds_before': bounds, 'bounds_after_lock': after_bounds,
                'graph_buffer_matches_surface': graph_buffer == PIXELS, 'pitch_div2': pitch,
                'unlock_clears_state': True, 'surface_calls': list(calls),
                # Evidence only: neither register is a public return protocol.
                'observed_lock_al_not_api': result & 255, 'observed_unlock_eax_not_api': unlocked})
    calls.clear()
    machine.hooks[LOCK] = lambda: surface_result('Lock', 0, 8)
    machine.hooks[GET_HEIGHT] = lambda: surface_result('GetHeight', 257)
    machine.cpu.mem_write(FONT+0x30, bytes(16))
    machine.call(0x4348F0, (SURFACE,))
    assert machine.read32(FONT+12) == 0 and machine.read32(FONT+16) == 2
    assert struct.unpack('<4i', machine.cpu.mem_read(FONT+0x30, 16)) == (0, 0, 1, 256)
    machine.call(0x434990, (SURFACE,))
    assert machine.read32(FONT+12) == machine.read32(FONT+16) == 0
    assert calls == ['Lock', 'GetPitch', 'GetWidth', 'GetHeight', 'Unlock']
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps({'sha256': SHA, 'crc_cases': len(lines),
        'original_crc_out_of_object_writes': overruns,
        'crc_comparison': 'CRC/Index/StagingBuffer (12 bytes) and Value; native byte +12 overrun intentionally excluded',
        'bitfont_comparison': 'State changes and Surface call order only; EAX/AL residues are observations, not return values',
        'bitfont_failed_surface_lock': {'graph_buffer_is_null': True, 'pitch_div2_after_lock': 2,
            'bounds_after_lock': [0, 0, 1, 256], 'surface_calls': list(calls), 'unlock_clears_state': True},
        'bitfont_cases': lock_rows, 'method': 'Unicorn x86 replay; Surface calls supplied by harness'}, indent=2)+'\n')
    print(f'Generated {len(lines)} native CRC cases and verified {len(lock_rows)} BitFont lock/unlock cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    generate(args.exe, args.output, args.report)
