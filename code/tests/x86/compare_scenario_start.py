#!/usr/bin/env python3
"""Compare 683AB0 startup control with the compiled replacement.

Both sides execute their real CCINI parsing and movie lookup. File transport,
media/UI/audio dependencies and the still-unported 684620 world loader are
controlled boundaries. This validates the controller, not world startup.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn.x86_const import *
from compare_scenario_ini import IniMachine, OBJECT, SUPPORT, ENTRIES, SHA

ENTRIES['ScenarioStart'] = 0x683AB0
FILE_VTABLE, SURFACE, SURFACE_VTABLE = SUPPORT + 0xB000, SUPPORT + 0xB100, SUPPORT + 0xB200


class StartMachine(IniMachine):
    def __init__(self, original, dll, replacement, case, mock_world=True):
        super().__init__(original, dll, replacement)
        # ReadCCFile supplies a real FileStraw. Do not inherit the generic
        # test-input BufferStraw shortcut used by the standalone INI fixtures.
        del self.hooks[0x4A2780]
        self.case, self.file, self.file_open = case, 0, False
        self.events, self.tracking = [], False
        self.hooks.update({0x7DCFC4: self.uppercase,
            0x4790B0: self.request_disc, 0x4790A0: self.current_disc,
            0x69AC30: self.mission_disc, 0x69ACC0: self.first_disc, 0x4790E0: self.force_disc,
            0x4739F0: self.file_construct, 0x43AE50: self.ret, 0x431B80: self.file_destroy,
            0x720EA0: lambda: self.theme('stop'), 0x720BB0: lambda: self.theme('play'),
            0x720B20: lambda: self.theme('queue'), 0x721210: self.find_theme,
            0x5BF260: self.movie, 0x684620: self.load_world,
            0x4B6C30: lambda: self.event('dropships'), 0x560BF0: self.resize,
            0x72DEF0: lambda: self.event('menu'), 0x5FB160: self.options,
            0x4F4780: self.present, 0x7781A0: lambda: self.event('ime'),
            0x78ABF0: self.online,
            SUPPORT + 0x1860: lambda: self.event('hide'),
            SUPPORT + 0x1880: lambda: self.event('show')})
        if replacement and mock_world:
            self.hooks[self.call('ScenarioAcquireFileBoundary')] = self.acquire_file
            self.hooks[self.call('ScenarioReleaseFileBoundary')] = self.release_file
            # The startup-only suite controls the world boundary on both
            # sides. Resolve the compiler's exported tail-jump to the actual
            # shared-core entry; the world suite executes that entry itself.
            entry = self.exports['ScenarioReadWorld']
            code = bytes(self.uc.mem_read(entry, 5))
            assert code[0] == 0xE9, code.hex()
            self.hooks[entry + 5 + struct.unpack('<i', code[1:])[0]] = self.load_world
        operations = {0: lambda: self.file_destroy(deleting=True),
            5: self.file_exists, 6: self.file_has_handle, 7: self.file_open_read,
            9: self.file_read, 13: self.file_close}
        for slot, operation in operations.items():
            address = SUPPORT + 0x9000 + slot * 0x20
            self.write32(FILE_VTABLE + slot * 4, address); self.hooks[address] = operation
        self.write32(SURFACE, SURFACE_VTABLE)
        self.write32(SURFACE_VTABLE + 24, SUPPORT + 0x9300)
        self.hooks[SUPPORT + 0x9300] = self.fill
        self.write32(0x88730C, SURFACE)
        # The target formats an unused Brief movie filename. Track its bytes
        # only as CRT work, not a presentation or playback event.
        for desc in original.DIRECTORY_ENTRY_IMPORT:
            for item in desc.imports:
                if item.name == b'wsprintfA':
                    self.write32(item.address, SUPPORT + 0x9400)
                    self.hooks[SUPPORT + 0x9400] = self.unused_brief_name
        self.call('ScenarioConstruct')
        self.clock_calls, self.clock, self.step = 0, 16000, 16
        self.write32(0xA8B238, case['mode'])
        self.write32(0xA8B254, case.get('mission', -1))
        self.write32(0xA8B8CC, OBJECT + 0xD000)
        self.write32(0xA8B8D8, 1)
        self.write32(OBJECT + 0xD000, OBJECT + 0xD100)
        self.write32(0xA83CFC, OBJECT + 0xD800); self.write32(0xA83D08, 1)
        self.write32(OBJECT + 0xD800, OBJECT + 0xDA00)
        self.write32(OBJECT + 0xDA00 + 0x98, 2)
        self.uc.mem_write(OBJECT + 0xDA00 + 0x9C, b'Campaign.map\0')
        self.uc.mem_write(0xA8ED5D, bytes([case.get('alternate', 0)]))
        self.uc.mem_write(0xA83E48, b'Alternate.map\0')
        self.write32(0xA8DAB4, case.get('depth', 3))
        self.uc.mem_write(0xA8ED5C, b'\0'); self.uc.mem_write(0xA8E378, b'\0')
        self.uc.mem_write(0x886FB8, struct.pack('<2i', *(case.get('resolution', (640,480)))))
        self.uc.mem_write(0xA8EB84, struct.pack('<2i', 800,600))
        self.write32(OBJECT + 0x614, 900 if case.get('ticking') else -1)
        self.write32(OBJECT + 0x61C, 42)
        self.write32(0xABF394, OBJECT + 0xE000); self.write32(0xABF3A0, 3)
        for i, name in enumerate((b'opening',b'brief',b'action')):
            self.write32(OBJECT + 0xE000 + 4*i, self.cstring(name))
        self.input = (b'[Basic]\nIntro=opening\nBrief=brief\n',
                      b'[Basic]\nIntro=unknown\nBrief=brief\n',
                      b'[Basic]\nIntro=unknown\n', b'')[case['movie']]
        self.position = 0
        self.tracking = True

    def event(self, name): self.events.append([name]); self.ret()

    def time(self):
        if getattr(self, 'tracking', False): self.events.append(['clock'])
        super().time()

    def uppercase(self):
        self.uc.mem_write(self.arg(0), self.string(self.arg(0)).upper() + b'\0')
        self.ret(self.arg(0))

    def request_disc(self):
        disc = self.uc.reg_read(UC_X86_REG_ECX)
        self.events.append(['disc', disc]); self.write32(0x81C1D0, disc); self.ret()

    def current_disc(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == 60
        self.events.append(['current disc', 60]); self.ret(2)

    def mission_disc(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == OBJECT + 0xD100 and self.arg(0) == 2
        self.events.append(['mission disc', 2]); self.ret(self.case.get('has_disc', True), cleanup=4)

    def first_disc(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == OBJECT + 0xD100
        self.events.append(['first disc']); self.ret(1)

    def force_disc(self):
        self.events.append(['force', self.arg(0), self.read32(0xA8DAB4)])
        self.ret(self.case['failure'] != 1, cleanup=4)

    def acquire_file(self):
        self.position = 0
        self.file = self.malloc(0x6C)
        self.events.append(['file', self.string(self.arg(1)).decode()])
        self.uc.mem_write(self.file, bytes(0x6C)); self.write32(self.file, FILE_VTABLE)
        self.write32(self.arg(2), self.file); self.ret(1)

    def release_file(self):
        assert self.arg(1) == self.file and not self.file_open
        self.events.append(['release']); self.ret()

    def file_construct(self):
        self.position = 0
        self.file = self.uc.reg_read(UC_X86_REG_ECX)
        self.events.append(['file', self.string(self.arg(0)).decode()])
        self.uc.mem_write(self.file, bytes(0x6C)); self.write32(self.file, FILE_VTABLE)
        self.ret(self.file, cleanup=4)

    def file_destroy(self, deleting=False):
        assert self.uc.reg_read(UC_X86_REG_ECX) == self.file and not self.file_open
        self.events.append(['release']); self.ret(cleanup=4 if deleting else 0)

    def file_exists(self): self.ret(1, cleanup=4)
    def file_has_handle(self): self.ret(self.file_open)
    def file_open_read(self):
        assert self.arg(0) == 1
        self.file_open = True; self.events.append(['open']); self.ret(1, cleanup=4)

    def file_read(self):
        assert self.file_open
        count = min(self.arg(1), len(self.input) - self.position)
        if count: self.uc.mem_write(self.arg(0), self.input[self.position:self.position+count])
        self.position += count; self.ret(count, cleanup=8)

    def file_close(self): self.file_open = False; self.events.append(['close']); self.ret()

    def theme(self, action):
        assert self.uc.reg_read(UC_X86_REG_ECX) == 0xA83D10
        self.events.append([action + ' theme', self.arg(0)]); self.ret(cleanup=4)

    def find_theme(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == 0xA83D10
        self.events.append(['find theme', self.string(self.arg(0)).decode()]); self.ret(10, cleanup=4)

    def movie(self):
        assert [self.arg(i) for i in range(3)] == [1,1,1]
        self.events.append(['movie', self.uc.reg_read(UC_X86_REG_ECX), self.uc.reg_read(UC_X86_REG_EDX)])
        self.ret(cleanup=12)

    def load_world(self):
        self.events.append(['world', self.string(self.uc.reg_read(UC_X86_REG_ECX)).decode()])
        self.write32(OBJECT + 0x1444, self.case.get('action', 2))
        self.write32(OBJECT + 0x1438, 1)
        self.write32(OBJECT + 0x34D0, self.case.get('dropships', 1))
        self.write32(OBJECT + 0x1C70, self.case.get('theme', 15))
        if self.case.get('live_mode'): self.write32(0xA8B238, 4)
        self.ret(self.case['failure'] != 2)

    def resize(self):
        self.events.append(['resize', self.uc.reg_read(UC_X86_REG_ECX), self.uc.reg_read(UC_X86_REG_EDX)])
        self.ret()

    def options(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == 0xA8EB60
        self.event('options')

    def fill(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) == SURFACE and self.arg(0) == 0
        self.events.append(['fill']); self.ret(1, cleanup=4)

    def present(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) & 255 == 1
        assert self.uc.reg_read(UC_X86_REG_EDX) == SURFACE and self.arg(0) == 0
        self.events.append(['present']); self.ret(cleanup=4)

    def online(self):
        assert self.uc.reg_read(UC_X86_REG_ECX) & 255 == 0
        assert bytes(self.uc.mem_read(self.uc.reg_read(UC_X86_REG_EDX), 2)) == b'\0\0'
        self.event('online')

    def unused_brief_name(self):
        assert self.string(self.arg(1)) == b'%s.VQA'
        value = self.string(self.arg(2)) + b'.VQA'
        self.uc.mem_write(self.arg(0), value + b'\0'); self.ret(len(value))

    def run(self):
        name = self.cstring(b'Mixed.map')
        if self.case.get('campaign', -1) != -1: name = 0
        if self.case.get('alias'):
            name = OBJECT + 0x125C; self.uc.mem_write(name, b'Alias.map\0')
        result = self.call('ScenarioStart', args=(self.case.get('campaign', -1),),
                           ecx=name, edx=self.case['briefing']) & 255
        return {'result': result, 'events': self.events,
                'filename': self.string(OBJECT + 0x125C).decode(),
                'campaign': self.read32(OBJECT + 0x34CC), 'depth': self.read32(0xA8DAB4),
                'started': bytes(self.uc.mem_read(0xA8ED5C,1)).hex(),
                'active': bytes(self.uc.mem_read(0xA8E378,1)).hex(),
                'timer': [self.read32(OBJECT + 0x614), self.read32(OBJECT + 0x61C)],
                'clock_calls': self.clock_calls}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path); parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == SHA
    original, dll = pefile.PE(str(args.exe)), pefile.PE(str(args.dll))
    cases = [dict(mode=mode,failure=failure,briefing=briefing,movie=movie)
             for mode in (0,3,4,5) for failure in range(3) for briefing in (False,True) for movie in range(4)]
    base = dict(mode=0,failure=0,briefing=True,movie=0)
    cases += [dict(base,campaign=0,alternate=flag) for flag in (0,1,2)]
    cases += [dict(base,mode=5,mission=0,has_disc=available) for available in (False,True)]
    cases += [dict(base,alias=True), dict(base,ticking=True,theme=-1,dropships=0,resolution=(800,600)),
              dict(base,mode=5,live_mode=True),dict(base,action=-1),
              dict(base,depth=0x7FFFFFFF),dict(base,depth=0x7FFFFFFF,failure=1)]
    results=[]
    for index, case in enumerate(cases):
        a,b = (StartMachine(original,dll,replacement,case).run() for replacement in (False,True))
        assert a == b, (index,case,a,b)
        assert a['result'] == (case['failure']==0)
        results.append({'case':case, 'passed':True})
        print('PASS', index, case)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps({'exe_sha256':SHA,
        'probe_sha256':hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        'scope':'683AB0 controller and real INI preflight; 684620 world loader is controlled, not implemented',
        'passed':len(results),'cases':results},indent=2)+'\n')


if __name__ == '__main__': main()
