#!/usr/bin/env python3
"""Execute original and compiled x86 Rules table readers and name registries.

The real INI parser, Rules code, name lookup, vector growth and cleanup execute
on both sides; only CRT primitives are controlled. Registered animation records
are borrowed inputs (their constructors/LoadINI are outside this test's scope).
This is not an installed Windows game-process integration test.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn.x86_const import UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EIP
from compare_rules import RulesMachine, compare, OBJECT, DATA
from compare_ini import INI, STACK, SHA

GROUND, WEIGHTS, ANIMS, ARGUMENTS, NAVAL = 0x89EA40, 0x81DA8C, 0x81DAD8, 0x89EC28, 0x89ECC0
MOVIES, ANIM_TYPES = 0xABF390, 0x8B4150


def initialize_registries(machine):
    if machine.replacement:
        machine.call(machine.exports['RulesInitializeMovies'], 0)
    else:
        # Fields written by actual static initializer 5BF7D0, excluding atexit.
        machine.uc.mem_write(MOVIES, struct.pack('<IIIBB2xii', 0x7EE0B4, 0, 0, 1, 0, 0, 10))
    # Existing original type records: name field +0x24, case-sensitive stored ID.
    names = [b'Spark', b'NONE', b'<none>', b'IonCloud']
    pointers = DATA + 0x20000
    for i, name in enumerate(names):
        record = pointers + 0x100 + i * 0x100
        machine.uc.mem_write(record + 0x24, name + b'\0')
        machine.write32(pointers + i * 4, record)
    machine.write32(ANIM_TYPES + 4, pointers)
    machine.write32(ANIM_TYPES + 8, len(names))
    machine.uc.mem_write(ANIM_TYPES + 12, b'\1\0')
    machine.write32(ANIM_TYPES + 16, len(names))
    machine.write32(ANIM_TYPES + 20, 10)


def table_values(machine):
    return {
        'ground': [bytes(machine.uc.mem_read(GROUND + i * 36, 33)).hex() for i in range(12)],
        'weights': list(struct.unpack('<19i', machine.uc.mem_read(WEIGHTS, 76))),
        'anims': list(struct.unpack('<19i', machine.uc.mem_read(ANIMS, 76))),
        'arguments': bytes(machine.uc.mem_read(ARGUMENTS, 152)).hex(),
        'naval': bytes(machine.uc.mem_read(NAVAL, 19)).hex(),
    }


def movie_values(machine):
    items = machine.read32(MOVIES + 4)
    count = machine.read32(MOVIES + 16)
    return {
        'capacity': machine.read32(MOVIES + 8), 'count': count,
        'flags': list(machine.uc.mem_read(MOVIES + 12, 2)),
        'increment': machine.read32(MOVIES + 20),
        'names': [machine.string(machine.read32(items + i * 4)).decode() for i in range(count)],
    }


def clear_movies(machine):
    items = machine.read32(MOVIES + 4)
    count = machine.read32(MOVIES + 16)
    addresses = [machine.read32(items + i * 4) for i in range(count)]
    labels = {address: f'name[{i}]' for i, address in enumerate(addresses)}
    labels[items] = 'pointer buffer'
    start = len(machine.freed)
    if machine.replacement:
        machine.call(machine.exports['RulesClearMovies'], 0)
    else:
        # Actual inline movie cleanup from original Exit; stop before the next
        # command-array cleanup, with its established EBX=0 precondition.
        machine.uc.reg_write(UC_X86_REG_EBX, 0)
        machine.uc.reg_write(UC_X86_REG_ESP, STACK + 0xF0000)
        machine.uc.emu_start(0x6BE294, 0x6BE31B, count=2000000)
        assert machine.uc.reg_read(UC_X86_REG_EIP) == 0x6BE31B
    freed = machine.freed[start:]
    assert len(freed) == len(set(freed)), 'double free'
    return [labels[address] for address in freed]


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
    a, b = RulesMachine(original, dll, False), RulesMachine(original, dll, True)
    for machine in (a, b):
        machine.invoke('Construct')
        initialize_registries(machine)
    rows = []
    def record(name):
        rows.append({'case': name, 'passed': True})
        print('PASS', name, flush=True)
    def run(name, text):
        for machine in (a, b): machine.load(text)
        expected, actual = a.invoke(name, INI) & 255, b.invoke(name, INI) & 255
        assert expected == actual, (name, 'return', expected, actual)
        compare(a, b, name)
        assert table_values(a) == table_values(b), (name, table_values(a), table_values(b))
        assert movie_values(a) == movie_values(b), (name, movie_values(a), movie_values(b))
        return expected

    for i, address in enumerate((GROUND, WEIGHTS, ANIMS, ARGUMENTS, NAVAL, MOVIES, ANIM_TYPES)):
        assert b.call(b.exports['RulesTable'], 0, i) == address, ('table binding', i)
    record('seven original-game table bindings resolve to fixed EXE globals')

    for name in ('ColorAdd', 'LandCharacteristics', 'Powerups', 'Movies'):
        result = run(name, b'[Unrelated]\nK=1\n')
        assert result == (name == 'LandCharacteristics')
    record('four readers: absent sections, unchanged tables, return and stdcall stack behavior')

    run('ColorAdd', b'[ColorAdd]\n9=-1,256,511\n2=3,4,5\nAlpha=9,8,7\n')
    assert bytes(a.uc.mem_read(OBJECT + 0x1874, 9)) == bytes([255, 0, 255, 3, 4, 5, 9, 8, 7])
    run('ColorAdd', b'[ColorAdd]\nFirst=22,23,24\n')
    assert bytes(a.uc.mem_read(OBJECT + 0x1874, 6)) == bytes([22, 23, 24, 3, 4, 5])
    run('ColorAdd', b'[ColorAdd]\n' + b''.join(f'{15-i}={i},{i+1},{i+2}\n'.encode() for i in range(16)))
    record('ColorAdd enumeration order, byte wrapping, partial overlay and all 16 slots')

    sections = ('Clear', 'Road', 'Water', 'Rock', 'Wall', 'Tiberium', 'Beach', 'Rough', 'Ice', 'Railroad', 'Tunnel', 'Weeds')
    keys = ('Foot', 'Track', 'Wheel', 'Hover', 'Float', 'Amphibious', 'FloatBeach')
    data = b''
    for i, section in enumerate(sections):
        data += f'[{section}]\nWinged=-9\nBuildable={"yes" if i % 2 else "no"}\n'.encode()
        for j, key in enumerate(keys):
            value = ('-0.75', '50%', '1.0', '1.75', '.125', '0', '80%')[(i + j) % len(keys)]
            data += f'{key}={value}\n'.encode()
    a.queries.clear(); b.queries.clear()
    run('LandCharacteristics', data)
    expected_queries = []
    for i, section in enumerate(sections):
        for key in ('Hover', 'Foot', 'Track', 'Wheel', 'Float', 'Amphibious', 'FloatBeach'):
            repeat = 1 if (i + keys.index(key)) % len(keys) in (2, 3) else 2
            expected_queries.extend([(0x5283D0, section, key)] * repeat)
        expected_queries.append((0x5295F0, section, 'Buildable'))
    assert a.queries == expected_queries, 'original terrain read order or second-read behavior'
    for i in range(12): assert struct.unpack('<f', a.uc.mem_read(GROUND + i * 36 + 16, 4))[0] == 1
    run('LandCharacteristics', b'[Clear]\nFoot=.4\n')
    assert struct.unpack('<3f', a.uc.mem_read(GROUND + 4, 12)) == (1, 1, 1)
    assert a.uc.mem_read(GROUND + 32, 1) == b'\0'
    record('all 12 terrain sections and seven movement keys: caps, negatives, percentages, fixed fallbacks and query order')

    for text, expected in ((None, -1), (b'SPARK', 0), (b'none', 1), (b'<NONE>', -1), (b'unknown', -1), (b'ioncloud', 3)):
        results = [m.invoke('FindAnim', this=m.cstring(text) if text is not None else 0) for m in (a, b)]
        assert results == [expected & 0xFFFFFFFF] * 2, ('animation index', text, results)
    record('animation registry lookup: borrowed original records, case folding, null and <none> sentinel')

    powerups = [b'''[Powerups]
Money=51,Spark,yes,125%
Unit=-2,none,no,-.5
HealBase=7,IonCloud,YES,1.25
Cloak=8,<none>,true,2
Explosion=9,unknown,yes,4
Napalm=10,Spark,yes,1e2
Squad=11,NONE,no,.125
Darkness=12,Spark,yes,0
Reveal=13,Spark,yes,-50%
Armor=14,Spark,yes,2
Speed=15,Spark,yes,2
Firepower=16,Spark,yes,2
ICBM=17,Spark,yes,2
Invulnerability=18,Spark,yes,2
Veteran=19,Spark,yes,2
IonStorm=20,Spark,yes,2
Gas=21,Spark,yes,2
Tiberium=22,Spark,yes,2
Pod=23,Spark,yes,2
''', b'''[Powerups]
Money= 73 , Spark , no , 250% ignored
Unit=4294967295,  IonCloud  ,yes,-1.5
HealBase=bad,Spark,false,bad
Cloak=5,,NONE,,no,,.75
Explosion=9
Napalm=,,,31,,,Spark,yes,50%,ignored
''', b'[Powerups]\nMoney=9\nUnit=3,NONE\nHealBase=4,Spark,false\nCloak=,\n']
    for text in powerups: run('Powerups', text)
    assert a.signed32(WEIGHTS) == 9
    assert a.signed32(ANIMS + 6 * 4) == 1, 'missing key uses 0,NONE, which can resolve a real animation'
    record('all 19 powerups, repeated overlays: four tokens, missing keys/tails, empty tokens, whitespace, overflow and percentages')

    run('Movies', b'[Movies]\n' + b''.join(f'{24-i}=MOVIE_{i:02}\n'.encode() for i in range(24)))
    assert movie_values(a)['capacity'] == 30 and movie_values(a)['count'] == 24
    run('Movies', b'[Movies]\n1=movie_00\n2=<none>\n3=<NONE>\n4=ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n5=ABCDEFGHIJKLMNOPQRSTUVWXYZ012345OTHER\n')
    assert movie_values(a)['names'][-3:] == ['<none>', '<NONE>', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ01234']
    assert movie_values(a)['count'] == 27
    run('Movies', b'[Movies]\nBlank=\n')
    assert movie_values(a)['count'] == 27
    for text, expected in ((None, -1), (b'mOvIe_12', 12), (b'<none>', -1), (b'missing', -1)):
        results = [m.invoke('FindMovie', this=m.cstring(text) if text is not None else 0) for m in (a, b)]
        assert results == [expected & 0xFFFFFFFF] * 2, ('movie index', text, results)
    record('movie append/growth, duplicate suppression, 31-byte truncation, sentinel repetition and lookup')

    releases = [clear_movies(m) for m in (a, b)]
    assert releases[0] == releases[1] == [f'name[{i}]' for i in range(27)] + ['pointer buffer']
    assert movie_values(a) == movie_values(b)
    assert a.read32(MOVIES + 4) == b.read32(MOVIES + 4) == 0
    record('original exit vs core movie cleanup: all names freed in order, then the owned pointer buffer')
    run('Movies', b'[Movies]\n1=AGAIN\n2=SECOND\n')
    for m in (a, b): m.uc.mem_write(MOVIES + 13, b'\0')  # borrowed pointer buffer
    releases = [clear_movies(m) for m in (a, b)]
    assert releases[0] == releases[1] == ['name[0]', 'name[1]']
    assert movie_values(a) == movie_values(b)
    assert a.read32(MOVIES + 4) and b.read32(MOVIES + 4), 'borrowed pointer retained'
    assert clear_movies(a) == clear_movies(b) == []
    record('movie rebuild and borrowed buffer cleanup: names owned, buffer retained, repeated cleanup safe')

    # Intentional local failure behavior, separate from equivalence claims.
    # Fail the first CRT malloc after INI parsing: the copied movie name.
    b.call(b.exports['RulesInitializeMovies'], 0)
    b.load(b'[Movies]\n1=OOM\n')
    saved_hooks = {}
    for descriptor in dll.DIRECTORY_ENTRY_IMPORT:
        for item in descriptor.imports:
            if item.name == b'malloc':
                address = b.read32(item.address)
                saved_hooks[address] = b.hooks[address]
                b.hooks[address] = lambda: b.ret(0)
    assert saved_hooks
    before = movie_values(b)
    assert b.invoke('Movies', INI) & 255 == 0
    assert movie_values(b) == before
    b.hooks.update(saved_hooks)
    b.write32(MOVIES + 20, 0)  # original AddItem failure when growth is disabled
    free_start = len(b.freed)
    assert b.invoke('Movies', INI) & 255 == 0
    assert b.read32(MOVIES + 16) == 0
    assert [b.string(p) for p in b.freed[free_start:]] == [b'OOM']
    b.write32(MOVIES + 20, 10)
    assert b.invoke('Movies', INI) & 255 == 1
    assert clear_movies(b) == ['name[0]', 'pointer buffer']
    record('local safety extension: movie name allocation failure and rejected insertion, cleanup and successful retry')

    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({
        'original_sha256': SHA, 'replacement_sha256': dll_sha,
        'method': __doc__, 'passed': len(rows), 'total': len(rows), 'cases': rows,
        'replacement_crt_calls': b.import_counts,
        'intentional_local_differences': ['ColorAdd rejects more than 16 keys before mutation',
            'Malformed ColorAdd components use initialized fallback bytes',
            'Movie allocation/append failure returns false and releases the uninserted name'],
    }, indent=2) + '\n')


if __name__ == '__main__': main()
