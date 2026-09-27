#!/usr/bin/env python3
"""Compare the captured macOS core's tile generation with original x86 code.

Uses real Cell constructors and Randomizers on both sides. Compares the return
value, all 64 tile entries, and the complete post-generation RNG state. The cell
is at (0,0), subtile 0; coordinate/subtile selection is covered separately by
compare_cell_variants.py. This is an algorithm check, not an x86 ABI handoff.
The native library must be the macOS arm64 core used for the image capture.
"""
import argparse
import ctypes
import hashlib
import json
import struct
from pathlib import Path

import pefile
from compare_ini import Machine, SHA


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'native-library', 'original-state', 'report'):
        parser.add_argument('--' + name, required=True, type=Path)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    capture = json.loads(args.original_state.read_text())
    machine = Machine(pefile.PE(str(args.exe)))
    scenario = machine.malloc(0x30000)
    machine.write32(0xA8B230, scenario)
    machine.write32(scenario + 0x214, 1000000)
    original_cell = machine.malloc(0x148)
    machine.call(0x47BBF0, original_cell)

    core = ctypes.CDLL(str(args.native_library.resolve()))

    def function(name, result, *parameters):
        call = getattr(core, name)
        call.restype = result
        call.argtypes = list(parameters)
        return call

    def reference(name):
        return ctypes.c_void_p.in_dll(core, name).value

    pointer, integer = ctypes.c_void_p, ctypes.c_int
    create = function('_ZN9CellClass6CreateEv', pointer)
    destroy = function('_ZN9CellClassD0Ev', None, pointer)
    construct = function('_ZN10RandomizerC1Ej', None, pointer, ctypes.c_uint32)
    random = function('_ZN10Randomizer6RandomEv', integer, pointer)
    variant = function('_ZNK9CellClass14GetTileVariantEii', integer, pointer, integer, integer)
    native_random = reference('_ZN10Randomizer6GlobalE')
    table = reference('_ZN9CellClass16TileVariantTableE')
    initialized = ctypes.c_uint8.from_address(reference('_ZN9CellClass27TileVariantTableInitializedE'))
    cell = create()
    assert cell
    rows = []
    generated = {}
    try:
        for seed in (0, 1, 0xFEDCBA98, capture['seed']):
            for advance in (0, 1, 103, 249, 250, 1000):
                for count in (1, 4, 8):
                    construct(native_random, seed)
                    machine.call(0x65C6D0, 0x886B88, seed)
                    for _ in range(advance):
                        random(native_random)
                        machine.call(0x65C780, 0x886B88)
                    initialized.value = 0
                    machine.uc.mem_write(0x89E7C7, b'\0')
                    selected = variant(cell, 0, count)
                    expected_selected = machine.call(0x4814F0, original_cell, 0, count)
                    actual = struct.pack('<i', selected) + ctypes.string_at(table, 256) + ctypes.string_at(native_random + 4, 1008)
                    expected = struct.pack('<I', expected_selected) + bytes(machine.uc.mem_read(0x89E620, 256)) + bytes(machine.uc.mem_read(0x886B8C, 1008))
                    rows.append(dict(seed=f'0x{seed:08X}', advance=advance, count=count,
                                     passed=actual == expected,
                                     original_sha256=hashlib.sha256(expected).hexdigest(),
                                     core_sha256=hashlib.sha256(actual).hexdigest()))
                    if advance == 0 and count == 8:
                        generated[f'0x{seed:08X}'] = list(struct.unpack('<64i', actual[4:260]))
    finally:
        destroy(cell)
    captured_seed = f"0x{capture['seed']:08X}"
    matches_capture = generated[captured_seed] == capture['table']
    report = dict(scope=__doc__, exe_sha256=SHA,
                  core_sha256=hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
                  captured_seed=captured_seed, captured_table_reproduced=matches_capture,
                  generated_tables=generated, cases=rows)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f"{sum(row['passed'] for row in rows)}/{len(rows)} table/RNG cases passed; captured table reproduced: {matches_capture}")
    return 0 if matches_capture and all(row['passed'] for row in rows) else 1


if __name__ == '__main__':
    raise SystemExit(main())
