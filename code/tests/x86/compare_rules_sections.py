#!/usr/bin/env python3
"""Differential for complete General, AudioVisual and CombatDamage readers.

Rules, INI, type/sound lookup and list operations execute as actual original
or compiled production instructions. Type/sound registries contain borrowed
records. The two SHP resource requests use a recorded fixture boundary; this
test does not repeat the image module's cache/IO tests or type construction.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

import pefile
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EDX
from compare_rules_readers import ReaderMachine, compare_state, FACTORIES
from compare_rules import ENTRIES, OBJECT, DATA
from compare_ini import INI, SHA

ENTRIES.update(General=0x66D530, AudioVisual=0x6691E0, CombatDamage=0x66BBB0)
FACTORIES.add(0x772FA0)


class SectionMachine(ReaderMachine):
    def __init__(self, original, dll, replacement):
        self.strings, self.shapes = [], []
        super().__init__(original, dll, replacement)
        self.string_reader = self.call(self.exports['RulesStringReaderAddress'], 0) if replacement else 0x528A10
        self.registry(0x887568, 15)  # WeaponTypeClass
        sounds = DATA + 0x60000
        for index, name in enumerate((b'KnownA', b'KnownB')):
            record = sounds + 0x100 + index * 0x200
            self.write32(sounds + index * 4, record)
            self.write32(record, self.cstring(name))
        self.write32(0xB1D37C, sounds)
        self.write32(0xB1D380, 2)
        self.write32(0xB1D388, 2)
        self.hooks[0x5B40B0] = self.shape

    def hook(self, uc, address, size, data):
        if address == getattr(self, 'string_reader', 0x528A10):
            self.strings.append((self.string(self.arg(0)).decode(), self.string(self.arg(1)).decode(),
                self.string(self.arg(2)).decode(), self.arg(4)))
        super().hook(uc, address, size, data)

    def shape(self):
        name = self.string(self.uc.reg_read(UC_X86_REG_ECX)).decode()
        flag = self.uc.reg_read(UC_X86_REG_EDX) & 255
        self.shapes.append((name, flag))
        assert name in ('BOMBCURS.SHP', 'CHRONOSK.SHP') and flag == 0
        self.ret(DATA + 0x65000 + (0x100 if name.startswith('CHRONO') else 0))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.original.read_bytes()).hexdigest() == SHA
    dll_bytes = args.dll.read_bytes()
    dll_sha = hashlib.sha256(dll_bytes).hexdigest()
    original, dll = pefile.PE(str(args.original)), pefile.PE(data=dll_bytes)
    cases, inventory = [], {}
    for section in ('CombatDamage', 'AudioVisual', 'General'):
        a, b = SectionMachine(original, dll, False), SectionMachine(original, dll, True)
        def run(text):
            for machine in (a, b):
                machine.load(text)
                machine.resolutions.clear(); machine.loads.clear(); machine.shapes.clear()
                machine.strings.clear()
            expected, actual = a.invoke(section, INI) & 255, b.invoke(section, INI) & 255
            assert expected == actual, (section, 'return', expected, actual, text)
            try: compare_state(a, b, section)
            except AssertionError as error: raise AssertionError((text, error)) from error
            assert a.shapes == b.shapes
            assert [(s, k, n) for s, k, _, n in a.strings] == [(s, k, n) for s, k, _, n in b.strings], (section, 'string query order', a.strings, b.strings)
            return expected
        assert not run(b'[Unrelated]\nKey=1\n')
        a.queries.clear(); a.strings.clear()
        assert run(f'[{section}]\nUnrelated=1\n'.encode())
        queries = list(dict.fromkeys(a.queries))
        strings = list(dict.fromkeys(a.strings))
        scalar_keys = {(s, k) for _, s, k in queries}
        special = [(s, k, d, n) for s, k, d, n in strings if (s, k) not in scalar_keys]
        inventory[section] = {'scalars': queries, 'strings': special}
        for address, name, key in queries:
            for value in (('yes', 'no') if address == 0x5295F0 else
                    ('1.75', '-.125') if address == 0x5283D0 else ('37', '-1')):
                run(f'[{name}]\n{key}={value}\n'.encode())
        for name, key, default, capacity in special:
            value = '-1,256,511' if re.fullmatch(r'\d+,\d+,\d+', default) else 'KnownA'
            run(f'[{name}]\n{key}={value}\n'.encode())
        # Exercise simultaneous named references and then a partial overlay.
        text = f'[{section}]\n'.encode()
        for name, key, default, capacity in special:
            if name == section:
                value = '4,5,6' if re.fullmatch(r'\d+,\d+,\d+', default) else 'KnownB'
                text += f'{key}={value}\n'.encode()
        run(text)
        run(f'[{section}]\nUnrelated=2\n'.encode())
        label = f'{section}: absent/base/partial overlays; all {len(queries)} scalar and {len(special)} string queries observed in original execution'
        cases.append({'case': label, 'passed': True}); print('PASS', label, flush=True)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'original_sha256': SHA,
        'replacement_sha256': dll_sha,
        'method': __doc__, 'cases': cases, 'inventory': inventory,
        'normalization': 'same allocation/vtable identity and unspecified TypeList unknown_18 exclusions as compare_rules_readers'}, indent=2) + '\n')


if __name__ == '__main__':
    main()
