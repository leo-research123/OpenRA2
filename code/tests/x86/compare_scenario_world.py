#!/usr/bin/env python3
"""684620/686730/684370 differential. World INI initialization (686B20),
starting-unit creation (686890) and world finishing (684C30) are controlled
boundaries in this suite; all three tested controllers execute real code.
The compat provider leaves WinSock address services null. Reference cases use
the original's null-transport path; extra candidate cases prove that even a
populated original WinSock slot is ignored by that provider.
Modem masks use a real core probe object and normal virtual dispatch into the
actual compat callback. Additional cases use the production no-device class
and poison the old EXE modem slot; no serial hardware support is claimed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn.x86_const import *
from compare_scenario_start import StartMachine, FILE_VTABLE, OBJECT, SUPPORT, ENTRIES, SHA

ENTRIES.update(ScenarioReadWorld=0x684620, ScenarioReadFile=0x686730, ScenarioWaitForPlayers=0x684370)
MANAGER, PALETTE, UDP, MODEM = SUPPORT+0xB600, SUPPORT+0xB700, SUPPORT+0xB800, SUPPORT+0xBA00


class WorldMachine(StartMachine):
    def __init__(self, original, dll, replacement, case, mock_initialize=True):
        super().__init__(original,dll,replacement,case,mock_world=False)
        del self.hooks[0x684620]
        self.uc.reg_write(UC_X86_REG_FPCW,0x27F)
        self.fractions=list(case.get('fractions',[1.0]))
        self.player_progress=case.get('player_progress',{})
        self.hooks.update({0x642A60:self.begin,0x552A40:self.manager,
            0x552B10:lambda:self.manager_event('prepare manager'),0x643E80:self.attach,
            0x642B10:self.side,0x552CC0:lambda:self.manager_event('prepare surface'),
            0x72B380:self.palette,0x642C20:self.bar,0x552BE0:self.position_query,
            0x642C80:self.configure_text,0x552C90:self.extent,0x642DF0:self.set_extent,
            0x6433D0:self.fraction,0x643C50:self.set_progress,0x642AD0:lambda:self.event('finish progress'),
            0x552AA0:lambda:self.event('destroy manager'),0x554370:lambda:self.manager_event('release graphics'),
            0x597A10:self.seed,0x598960:self.generate,0x686890:self.units,
            0x686B20:self.world_ini,0x684C30:lambda:self.event('finish world'),
            0x577F30:self.shroud,0x4A3C30:self.fade,0x5D3490:self.error_dialog,
            0x5D3A60:self.messages,0x50D610:self.finish_house,0x69AE90:self.progress,
            0x48D080:lambda:self.event('pump'),0x54F000:lambda:self.event('input'),
            0x643E90:self.player_fraction,0x5DA750:self.drop,0x697E70:self.is_host,
            0x664610:self.delay,0x69B170:lambda:self.event('online finished'),
            0x7350C0:lambda:self.ret(SUPPORT+0x100),0x734E60:self.text,
            SUPPORT+0x9500:self.clear_addresses,SUPPORT+0x9520:self.add_address,
            SUPPORT+0x9540:lambda:self.ret(case.get('modem_status',-1))})
        if mock_initialize and replacement:
            entry = self.exports['ScenarioInitializeWorldINI']
            code = bytes(self.uc.mem_read(entry, 5))
            assert code[0] == 0xE9, code.hex()
            self.hooks[entry + 5 + struct.unpack('<i', code[1:])[0]] = self.world_ini
            # The file chain now constructs core objects. Control the same
            # transport dependency at the real service callbacks instead of
            # assuming the candidate calls the old EXE constructor.
            self.hooks[self.call('ScenarioAcquireFileBoundary')] = self.acquire_file
            self.hooks[self.call('ScenarioReleaseFileBoundary')] = self.release_file
        elif not mock_initialize:
            del self.hooks[0x686B20]
        # Original CRT swprintf is a cdecl call; replacement uses bounded
        # Microsoft wide formatting. Both format the original numeric game ID.
        self.hooks[0x7CA564] = self.wide_sprintf
        for desc in dll.DIRECTORY_ENTRY_IMPORT:
            for item in desc.imports:
                if item.name == b'__stdio_common_vswprintf':
                    self.hooks[self.read32(item.address)] = self.wide_vsprintf
        self.write32(UDP,UDP+0x40);self.write32(UDP+0x40+48,SUPPORT+0x9500);self.write32(UDP+0x40+44,SUPPORT+0x9520)
        self.write32(0x887628,UDP if case.get('udp',True) else 0)
        self.write32(MODEM,MODEM+0x40);self.write32(MODEM+0x40+12,SUPPORT+0x9540)
        self.write32(0xB72D20,MODEM if 'modem_status' in case else 0)
        if replacement:
            self.modem_storage = self.malloc(self.call('ScenarioModemProbeSize')) if 'modem_status' in case else 0
            signals = -1 if case.get('no_device_modem') else case.get('modem_status',0)
            self.call('ScenarioConstructModemProbe',ecx=self.modem_storage,edx=signals & 0xFFFFFFFF)
            if self.modem_storage:
                vtable = self.read32(self.modem_storage)
                image_base = dll.OPTIONAL_HEADER.ImageBase
                assert image_base <= vtable < image_base + dll.OPTIONAL_HEADER.SizeOfImage, hex(vtable)
        self.uc.mem_write(0xA8ED6B,bytes([case.get('armageddon',False)]))
        self.uc.mem_write(0xA8DBA0,bytes([case.get('delay',False)]))
        self.write32(0xB779C4,case.get('tournament',0));self.write32(0xB779D4,12345)
        self.write32(0xA8E7AC,case.get('init_depth',2));self.write32(0xA8ED84,77)
        self.write32(OBJECT+0x3598,0xACBDCE00);self.write32(OBJECT+0x34CC,case.get('campaign',-1))
        self.write32(OBJECT+0x34B8,99)
        self.uc.mem_write(0xA8B8C1,b'\x01\x01')
        self.uc.mem_write(0x886FA0,struct.pack('<4i',10,20,640,480))
        self.rules=self.malloc(0x1B78);self.write32(0x8871E0,self.rules)
        self.uc.mem_write(self.rules+0x14AE,bytes([case.get('shroud',True)]))
        self.nodes=[];self.world_houses=[]
        count=case.get('players',3)
        house_count=case.get('houses',count)
        self.node_array=self.malloc(32);self.house_array=self.malloc(32)
        self.write32(0xA8DA78,self.node_array);self.write32(0xA8DA84,count)
        self.write32(0xA8022C,self.house_array);self.write32(0xA80238,house_count)
        country=self.malloc(0x1B0); self.write32(country+0xBC,2)
        country_array=self.malloc(4);self.write32(country_array,country)
        self.write32(0xA83C9C,country_array);self.write32(0xA83CA8,1)
        for i in range(count):
            node=self.malloc(0x85)
            self.nodes.append(node)
            self.write32(self.node_array+4*i,node)
            self.write32(node+0x4B,case.get('country',0) if i==0 else 0)
            self.write32(node+0x6F,i);self.write32(node+0x2C,0x04030201+i)
            self.uc.mem_write(0xA8E2B2+2*i,struct.pack('<H',1234+i))
        for i in range(house_count):
            house=self.malloc(0x160B8);self.world_houses.append(house)
            self.write32(self.house_array+4*i,house)
        self.write32(0xA8E2AC,17);self.uc.mem_write(0xA8E258,b'X'*42)
        self.clock=case.get('clock',16000);self.step=case.get('clock_step',16);self.clock_calls=0

    def world_state(self,result):
        return dict(result=result,events=self.events,frame=self.read32(0xA8ED84),
            filename=self.string(OBJECT+0x125C).decode(),flags=self.read32(OBJECT+0x3598),
            side=self.read32(OBJECT+0x34B8),depth=self.read32(0xA8E7AC),
            random=bytes(self.uc.mem_read(OBJECT+0x34BD,1)).hex(),
            session=bytes(self.uc.mem_read(0xA8B8C1,2)).hex(),clock_calls=self.clock_calls,
            started=bytes(self.uc.mem_read(0xA8ED5C,1)).hex()+bytes(self.uc.mem_read(0xA8E378,1)).hex(),
            timer=[self.read32(OBJECT+0x614),self.read32(OBJECT+0x61C)],
            players=self.read32(0xA8DA84),host=self.read32(0xA8E2AC),
            hostname=bytes(self.uc.mem_read(0xA8E258,42)).hex())

    def run(self,entry='ScenarioReadWorld'):
        name=self.cstring(b'Mixed.SeD' if self.case.get('random') else b'Mixed.map')
        args=(self.case.get('campaign',-1),) if entry=='ScenarioStart' else ()
        return self.world_state(self.call(entry,args=args,ecx=name,edx=self.case['briefing'])&255)

    def doubles(self,index): return struct.unpack('<d',self.uc.mem_read(self.uc.reg_read(UC_X86_REG_ESP)+4+4*index,8))[0]
    def double_return(self,value,cleanup=0):
        # Separate return stubs avoid reusing Unicorn's translated RET with a
        # different callee-pop size when moving from aggregate to peer queries.
        data=SUPPORT+0xE800;stub=SUPPORT+0xE000+cleanup*16
        self.uc.mem_write(data,struct.pack('<d',value))
        self.uc.mem_write(stub,b'\xdd\x05'+struct.pack('<I',data)+b'\xc2'+struct.pack('<H',cleanup))
        self.uc.reg_write(UC_X86_REG_EIP,stub)

    def begin(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==0xAC4F58 and self.arg(3)==0
        self.events.append(['begin',self.doubles(0),self.arg(2)&255]);self.ret(cleanup=16)
    def manager(self): self.events.append(['get manager']);self.ret(MANAGER)
    def manager_event(self,name):
        assert self.uc.reg_read(UC_X86_REG_ECX)==MANAGER;self.event(name)
    def attach(self): self.events.append(['attach',bool(self.arg(0))]);self.ret(cleanup=4)
    def side(self): self.events.append(['side',self.arg(0)]);self.ret(cleanup=4)
    def palette(self): self.events.append(['palette']);self.ret(PALETTE)
    def bar(self):
        assert self.arg(1)==0
        self.events.append(['bar',self.string(self.arg(0)).decode(),self.arg(2)]);self.ret(cleanup=12)
    def position_query(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==MANAGER
        self.events.append(['position']);self.uc.mem_write(self.arg(0),struct.pack('<2i',84,7));self.ret(self.arg(0),cleanup=4)
    def configure_text(self):
        self.events.append(['text',self.arg(0),self.arg(1),self.wstring_at(self.arg(2)) if self.arg(2) else None,self.arg(3)&255,self.arg(4)&255]);self.ret(cleanup=20)
    def extent(self): self.events.append(['extent']);self.ret(406)
    def set_extent(self): self.events.append(['set extent',self.arg(0)]);self.ret(cleanup=4)
    def fraction(self):
        value=self.fractions.pop(0) if len(self.fractions)>1 else self.fractions[0]
        self.events.append(['fraction', 'nan' if value!=value else value]);self.double_return(value)
    def set_progress(self):
        assert bytes(self.uc.mem_read(self.uc.reg_read(UC_X86_REG_ESP)+16,8))==b'\xff'*8
        self.events.append(['set progress',self.arg(0),self.doubles(1)]);self.fractions=[1.0];self.ret(cleanup=20)
    def clear_addresses(self): self.event('clear addresses')
    def add_address(self): self.events.append(['address',self.string(self.arg(0)).decode(),self.arg(1)&0xffff]);self.ret(cleanup=8)
    def seed(self): self.events.append(['seed',self.string(self.arg(0)).decode()]);self.ret(self.case.get('seed_success',True),cleanup=4)
    def generate(self):
        assert self.arg(0)==self.arg(1)==0
        self.events.append(['generate']);self.uc.mem_write(OBJECT+0x125C,b'Generated.map\0');self.ret(cleanup=8)
    def units(self): self.events.append(['starting units',self.uc.reg_read(UC_X86_REG_ECX)&255]);self.ret()
    def world_ini(self):
        assert self.position==len(self.input)
        self.events.append(['world ini',self.string(OBJECT+0x125C).decode(),self.uc.reg_read(UC_X86_REG_EDX)&255]);self.ret(self.case.get('ini_success',True))
    def shroud(self): self.events.append(['shroud',self.arg(0)]);self.ret(cleanup=4)
    def fade(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==0x885780 and self.uc.reg_read(UC_X86_REG_EDX)==7 and self.arg(0)==0x48D080
        self.events.append(['fade']);self.ret(cleanup=4)
    def text(self):
        label=self.string(self.uc.reg_read(UC_X86_REG_ECX)).decode()
        if label=='GUI:SkirmishGame': value='Skirmish fixture'
        else:
            self.events.append(['string',label,self.arg(1)])
            value={'TXT_GAME_ID':'Game %u','TXT_OK':'OK','TXT_UNABLE_READ_SCENARIO':'Unable to read scenario'}[label]
        data=value.encode('utf-16le')+b'\0\0';address=self.malloc(len(data));self.uc.mem_write(address,data);self.ret(address,cleanup=8)
    def wstring_at(self,address):
        data=b''
        while (pair:=bytes(self.uc.mem_read(address,2)))!=b'\0\0':data+=pair;address+=2
        return data.decode('utf-16le')
    def wide_sprintf(self):
        value=self.wstring_at(self.arg(1)) % self.arg(2)
        self.uc.mem_write(self.arg(0),value.encode('utf-16le')+b'\0\0');self.ret(len(value))
    def wide_vsprintf(self):
        value=self.wstring_at(self.arg(4)) % self.read32(self.arg(6))
        assert len(value)<self.arg(3)
        self.uc.mem_write(self.arg(2),value.encode('utf-16le')+b'\0\0');self.ret(len(value))
    def format(self,destination,fmt,args,capacity):
        if fmt in (b'%d.%d.%d.%d',b'%u.%u.%u.%u'):
            value=b'.'.join(str(self.read32(args+4*i)).encode() for i in range(4))
            assert len(value)<capacity;self.uc.mem_write(destination,value+b'\0');self.ret(len(value))
        else:super().format(destination,fmt,args,capacity)
    def error_dialog(self):
        assert self.arg(2)==self.arg(3)==0 and self.arg(4)&255==0
        assert self.wstring_at(self.read32(self.uc.reg_read(UC_X86_REG_ECX)))==' '
        self.events.append(['error',self.wstring_at(self.arg(0)),self.wstring_at(self.arg(1))]);self.ret(cleanup=20)
    def messages(self): self.events.append(['messages']+[self.arg(i) for i in range(11)]);self.ret(cleanup=44)
    def finish_house(self): self.events.append(['house',self.world_houses.index(self.uc.reg_read(UC_X86_REG_ECX))]);self.ret()
    def progress(self): self.events.append(['progress',self.arg(0)]);self.ret(cleanup=4)
    def delay(self): self.events.append(['delay',self.arg(0),self.arg(1)&255]);self.ret(cleanup=8)
    def player_fraction(self):
        slot=self.arg(0);self.events.append(['player fraction',slot]);self.double_return(self.player_progress.get(slot,0.0),cleanup=4)
    def drop(self):
        house=self.uc.reg_read(UC_X86_REG_ECX);assert self.uc.reg_read(UC_X86_REG_EDX)==1
        self.events.append(['drop',house])
        self.nodes=[node for node in self.nodes if self.read32(node+0x6F)!=house]
        self.write32(0xA8DA84,len(self.nodes))
        for i,node in enumerate(self.nodes):self.write32(self.node_array+4*i,node)
        self.ret()
    def is_host(self):
        house=self.world_houses.index(self.arg(0));self.events.append(['host?',house]);self.ret(house==self.case.get('host_house',1),cleanup=4)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('exe',type=Path);parser.add_argument('dll',type=Path);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    original,dll=pefile.PE(str(args.exe)),pefile.PE(str(args.dll))
    base=dict(mode=0,failure=0,briefing=False,movie=0,udp=False)
    cases=[(dict(base,mode=mode,armageddon=armageddon,random=random,shroud=shroud),'ScenarioReadWorld')
           for mode in (0,3,4,5) for armageddon in (False,True) for random in (False,True) for shroud in (False,True)]
    cases += [(dict(base,**change),'ScenarioReadWorld') for change in (
        {'ini_success':False},{'failure':1},{'movie':3},{'random':True,'seed_success':False},
        {'armageddon':True,'ini_success':False},{'campaign':0},{'mode':5,'country':-3},
        {'mode':4,'tournament':1,'delay':True},{'mode':4,'fractions':[0.9996]},
        {'init_depth':0x7fffffff},{'mode':4,'clock_step':64000,'fractions':[0.0]})]
    cases += [(dict(base,**change),'ScenarioReadFile') for change in ({},{'failure':1},{'ini_success':False},{'movie':3})]
    cases += [(dict(base,**change),'ScenarioWaitForPlayers') for change in (
        {'mode':0},{'mode':5},{'mode':3},{'mode':1},
        {'mode':4,'fractions':[0.0,0.1,0.1,0.9995],'clock_step':1600},
        {'mode':4,'fractions':[0.0],'clock_step':64000,'player_progress':{1:0.998,2:0.999}},
        {'mode':4,'fractions':[float('nan')],'clock_step':64000},
        {'mode':4,'fractions':[0.0],'clock_step':64000,'player_progress':{1:0.0,2:0.0}},
        {'mode':3,'clock':0xfffffff0,'fractions':[0.0,0.9995]})]
    cases += [(dict(base,mode=1,modem_status=signals),'ScenarioWaitForPlayers')
              for signals in range(0,0x100,0x10)]
    cases += [(dict(base,mode=mode,random=random,briefing=True),'ScenarioStart') for mode in (0,3,4,5) for random in (False,True)]
    cases += [(dict(base,mode=mode,ini_success=False),'ScenarioStart') for mode in (0,3,4,5)]
    # A real campaign can have no multiplayer NodeName entries while Houses
    # still exist. The old guard incorrectly rejected it before loading began.
    cases += [(dict(base,players=0,houses=2,campaign=0,armageddon=armageddon,random=random),entry)
              for entry in ('ScenarioReadWorld','ScenarioStart')
              for armageddon in (False,True) for random in (False,True)]
    cases += [(dict(base,players=0,houses=2,campaign=0,ini_success=False),entry)
              for entry in ('ScenarioReadWorld','ScenarioStart')]
    results=[]
    for i,(case,entry) in enumerate(cases):
        a,b=(WorldMachine(original,dll,replacement,case).run(entry) for replacement in (False,True))
        assert a==b,(i,case,entry,a,b)
        report_case=json.loads(json.dumps(case),parse_constant=str)
        results.append(dict(entry=entry,case=report_case,passed=True));print('PASS',i,entry,case)
    for random in (False,True):
        case=dict(base,mode=4,random=random)
        a=WorldMachine(original,dll,False,case).run()
        candidate=WorldMachine(original,dll,True,dict(case,udp=True))
        # An invalid non-null pointer makes any accidental transport access
        # fail, rather than masking it with harmless callback stubs.
        candidate.write32(0x887628,0xDEADBEEF)
        b=candidate.run()
        assert a==b,('unbound WinSock slot',random,a,b)
        results.append(dict(entry='ScenarioReadWorld',case=dict(case,original_transport='invalid non-null'),passed=True))
        print('PASS unbound WinSock slot',random)
    for mode in (0,1,3,4,5):
        for present in (False,True):
            case=dict(base,mode=mode)
            if present:
                case['modem_status']=0
            reference=WorldMachine(original,dll,False,case).run('ScenarioWaitForPlayers')
            candidate=WorldMachine(original,dll,True,dict(case,no_device_modem=True))
            candidate.write32(0xB72D20,0xDEADBEEF)
            actual=candidate.run('ScenarioWaitForPlayers')
            assert reference==actual,('core no-device modem',mode,present,reference,actual)
            candidate.call('ScenarioDestroyModemProbe')
            assert candidate.call('ScenarioModemInstanceProbe')==0, 'destroyed modem remains published'
            results.append(dict(entry='ScenarioWaitForPlayers',case=dict(case,
                core_modem='no-device' if present else 'absent',original_modem='invalid non-null'),passed=True))
            print('PASS core modem with poisoned EXE slot',mode,present)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(exe_sha256=SHA,probe_sha256=hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        scope=__doc__,passed=len(results),cases=results),indent=2,allow_nan=False)+'\n')


if __name__=='__main__':main()
