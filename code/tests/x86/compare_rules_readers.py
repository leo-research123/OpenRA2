#!/usr/bin/env python3
"""Execute new Rules readers and real original registry lookups in x86.

Registered records are borrowed fixtures, as in compare_rules_tables.py.
Factory lookup and Warhead LoadFromINI (absent content section) execute in the
original EXE. This validates Rules orchestration, not construction/content of
every dependent game type or a Windows-process replacement installation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_FPCW
from compare_rules import RulesMachine, ENTRIES, OBJECT, DATA, SIZE
from compare_ini import INI, SUPPORT, SHA

TYPE_READERS = {
    'InfantryTypes': (0x672280, 0x524CB0, 0xA8E348),
    'Countries': (0x6722F0, 0x512680, 0xA83C98),
    'VehicleTypes': (0x672360, 0x7480D0, 0xA83CE0),
    'AircraftTypes': (0x6723D0, 0x41CEF0, 0xA8B218),
    'SuperWeaponTypes': (0x6725F0, 0x6CEEF0, 0xA8E330),
    'BuildingTypes': (0x672660, 0x4653C0, 0xA83C68),
    'TerrainTypes': (0x6726D0, 0x71E2A0, 0xA8E318),
    'SmudgeTypes': (0x6727D0, 0x6B5910, 0xA8EC18),
    'OverlayTypes': (0x672840, 0x5FEC70, 0xA83D80),
    'Animations': (0x6728B0, 0x428B80, 0x8B4150),
    'VoxelAnims': (0x672920, 0x74B960, 0xA8EB28),
    'Warheads': (0x672990, 0x75E3B0, 0x8874C0),
    'Particles': (0x672A00, 0x645820, 0xA83D98),
    'ParticleSystems': (0x672A70, 0x644890, 0xA83D68),
}
ENTRIES.update({name: row[0] for name, row in TYPE_READERS.items()})
ENTRIES.update(Radiation=0x66CF70, CrateRules=0x66B900, SpecialWeapons=0x668FB0, AI=0x672AE0)
FACTORIES = {row[1] for row in TYPE_READERS.values()} | {0x46C790}


class ReaderMachine(RulesMachine):
    def __init__(self, original, dll, replacement):
        self.resolutions, self.loads = [], []
        super().__init__(original, dll, replacement)
        self.uc.mem_map(SUPPORT + 0x20000, 0x20000)
        self.write32(SUPPORT + 0x12000 + 0x2C, SUPPORT + 0x20000)
        if hasattr(dll, 'DIRECTORY_ENTRY_TLS'):
            tls = dll.DIRECTORY_ENTRY_TLS.struct
            self.write32(tls.AddressOfIndex, 0)
            self.write32(SUPPORT + 0x20000, SUPPORT + 0x21000)
            self.uc.mem_write(SUPPORT + 0x21000, bytes(self.uc.mem_read(
                tls.StartAddressOfRawData, tls.EndAddressOfRawData - tls.StartAddressOfRawData)))
        self.warhead_loader = self.read32(0x7F6B30 + 100)
        for i, (_, factory, array) in enumerate(TYPE_READERS.values()):
            self.registry(array, i)
        self.registry(0xA83C80, 14)  # BulletTypeClass
        self.write32(0xB1D388, 0)  # Sound lookup: an unknown sound preserves the prior index.
        self.invoke('Construct')

    def registry(self, array, index):
        table = DATA + 0x40000 + index * 0x1000
        for i, name in enumerate((b'KnownA', b'KnownB')):
            record = table + 0x100 + i * 0x400
            self.write32(table + i * 4, record)
            self.write32(record, 0x7F6B30)  # Warhead vtable is consumed only for the warhead registry.
            self.uc.mem_write(record + 0x24, name + b'\0')
        self.write32(array + 4, table)
        self.write32(array + 8, 2)
        self.uc.mem_write(array + 12, b'\1\0')
        self.write32(array + 16, 2)
        self.write32(array + 20, 10)

    def hook(self, uc, address, size, data):
        if address in FACTORIES:
            self.resolutions.append((hex(address), self.string(uc.reg_read(UC_X86_REG_ECX)).decode()))
        if address == getattr(self, 'warhead_loader', None):
            self.loads.append((uc.reg_read(UC_X86_REG_ECX), self.arg(0)))
        super().hook(uc, address, size, data)


def compare_state(a, b, label):
    assert a.uc.reg_read(UC_X86_REG_FPCW) == b.uc.reg_read(UC_X86_REG_FPCW), (label, 'persistent x87 control word')
    left, right = bytearray(a.uc.mem_read(OBJECT, SIZE)), bytearray(b.uc.mem_read(OBJECT, SIZE))
    mask = bytearray(a.mask)
    for offset in a.lists:
        # Compare owned/borrowed flags, capacity/count/increment and live
        # elements. Normalize allocation/vtable identity, and the original
        # unspecified TypeList::unknown_18 (copied from uninitialized temps).
        mask[offset:offset + 4] = bytes(4)
        mask[offset + 4:offset + 8] = bytes(4)
        mask[offset + 24:offset + 28] = bytes(4)
        count = a.read32(OBJECT + offset + 16)
        assert count == b.read32(OBJECT + offset + 16), (label, 'count', hex(offset))
        if count:
            aa, bb = a.read32(OBJECT + offset + 4), b.read32(OBJECT + offset + 4)
            assert a.uc.mem_read(aa, count * 4) == b.uc.mem_read(bb, count * 4), (label, 'items', hex(offset))
    differences = [hex(i) for i in range(SIZE) if mask[i] and left[i] != right[i]]
    assert not differences, (label, [(i, left[int(i, 16)], right[int(i, 16)]) for i in differences[:40]])
    assert a.resolutions == b.resolutions, (label, 'factory trace', a.resolutions, b.resolutions)
    assert a.loads == b.loads, (label, 'virtual LoadFromINI trace', a.loads, b.loads)


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
    a, b = ReaderMachine(original, dll, False), ReaderMachine(original, dll, True)
    cases = []
    def record(label):
        cases.append({'case': label, 'passed': True})
        print('PASS', label, flush=True)
    def run(name, text):
        for machine in (a, b):
            machine.load(text)
            machine.resolutions.clear(); machine.loads.clear()
        expected, actual = a.invoke(name, INI) & 255, b.invoke(name, INI) & 255
        assert expected == actual, (name, 'return', expected, actual)
        compare_state(a, b, name)
        return expected
    for name in TYPE_READERS:
        assert not run(name, b'[Unrelated]\nK=1\n')
        assert run(name, f'[{name}]\n9=KnownB\n2=knowna\n1=KnownB\nNone=NONE\nOther=<none>\n'.encode())
        assert [name for _, name in a.resolutions][:3] == ['KnownB', 'knowna', 'KnownB']
    record('14 stdcall type-list readers: absent sections, INI order, duplicate/case-folded names, both sentinels; actual original factories')
    for name in ('Radiation', 'CrateRules', 'SpecialWeapons', 'AI'):
        assert not run(name, b'[Unrelated]\nK=1\n')
    record('four member readers: absent sections, unchanged state and false returns')
    assert run('Radiation', b'''[Radiation]
RadDurationMultiple=-3
RadApplicationDelay=19
RadLevelMax=217
RadLevelDelay=4
RadLightDelay=8
RadLevelFactor=125%
RadLightFactor=.75
RadTintFactor=-.25
RadColor=-1,256,511
RadSiteWarhead=knowna
''')
    run('Radiation', b'[Radiation]\nRadLevelMax=73\nRadSiteWarhead=NONE\n')
    record('Radiation: all fields, percent/negative values, byte wrapping, partial overlays and real warhead resolution')
    run('CrateRules', b'''[CrateRules]
FreeMCV=yes
WoodCrateImg=KnownA
CrateImg=KnownB
WaterCrateImg=knowna
HealCrateSound=Missing
CrateMinimum=-7
CrateMaximum=29
CrateRadius=1.25
CrateRegen=125%
UnitCrateType=KnownB
SoloCrateMoney=2719
SilverCrate=Money
WoodCrate=Unit
WaterCrate=Armor
''')
    for value in ('-1', '-100%', '-.125', '.123', '50%', '16777216', '-16777216'):
        run('CrateRules', f'[CrateRules]\nCrateRadius={value}\n'.encode())
    record('CrateRules: all fields, original enum lookup, unknown sound fallback, distance sentinel/truncation/32-bit wrap')
    run('SpecialWeapons', b'''[SpecialWeapons]
NukeWarhead=KnownA
NukeProjectile=KnownB
NukeDown=KnownA
MutateWarhead=KnownB
MutateExplosionWarhead=KnownA
EMPulseWarhead=KnownB
EMPulseProjectile=KnownA
''')
    assert len(a.loads) == 2
    run('SpecialWeapons', b'[SpecialWeapons]\nNukeWarhead=NONE\nEMPulseProjectile=<none>\n')
    assert len(a.loads) == 2
    record('SpecialWeapons: seven typed references and actual virtual warhead content-reader dispatch (absent content sections)')
    keys = ('BuildConst BuildPower BuildRefinery BuildBarracks BuildTech BuildWeapons '
        'AlliedBaseDefenses SovietBaseDefenses ThirdBaseDefenses BuildDefense BuildPDefense '
        'BuildAA BuildHelipad BuildRadar ConcreteWalls NSGates EWGates BuildNavalYard BuildDummy NeutralTechBuildings').split()
    text = b'[AI]\n' + b''.join(f'{key}=KnownA,,KnownB,NONE,knowna,<none>\n'.encode() for key in keys)
    text += b'AIForcePredictionFudge=1,,bad,4294967295,-2\nAttackInterval=75%\nCompEasyBonus=yes\n'
    run('AI', text)
    run('AI', b'[AI]\nBuildConst=NONE\nAIForcePredictionFudge=3,2,1\n')
    run('AI', b'[AI]\nAttackDelay=.75\n')
    record('AI: all 20 building lists, actual factories, duplicates/null filtering, integer token parsing and repeated overlays')
    # Discover every original scalar read and exercise each key independently.
    a.queries.clear()
    run('AI', b'[AI]\nAttackDelay=.25\n')
    scalar_queries = list(dict.fromkeys(a.queries))
    for address, section, key in scalar_queries:
        value = 'yes' if address == 0x5295F0 else ('1.75' if address == 0x5283D0 else '37')
        run('AI', f'[{section}]\n{key}={value}\n'.encode())
    record(f'AI: all {len(scalar_queries)} scalar queries observed in original instructions')
    report = {'original_sha256': SHA, 'replacement_sha256': dll_sha,
        'method': __doc__, 'passed': len(cases), 'cases': cases,
        'normalization': 'module/allocation pointer identity and unspecified TypeList unknown_18; compare remaining target-written bytes and live elements',
        'dependency_scope': 'existing registry records; factory lookup and absent-section warhead LoadFromINI execute; dependent type construction/content and Windows game integration are not covered'}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
