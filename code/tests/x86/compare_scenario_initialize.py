#!/usr/bin/env python3
"""686B20 initialization controller and 527CC0 rectangle differential.

Real INI nodes, file parsing, metadata, globals, waypoints and cell iteration
execute on both sides. Rules passes, AssignHouses and ReadINI (covered by their
own differential suites), world clearing, object readers and rendering are
controlled boundaries. This does not certify complete world construction.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn.x86_const import *
from compare_scenario_ini import IniMachine, INI, STRAW
from compare_scenario_world import WorldMachine, MANAGER, OBJECT, SUPPORT, ENTRIES, SHA
from compare_scenario_start import FILE_VTABLE

ENTRIES.update(ScenarioInitializeWorldINI=0x686B20, ScenarioReadRectangle=0x527CC0)
MAP = 0x87F7E8


class InitializeMachine(WorldMachine):
    def __init__(self, original, dll, replacement, case, mock_clear=True):
        super().__init__(original, dll, replacement, case, mock_initialize=False)
        self.files, self.side_calls = {}, 0
        if replacement:
            # Match the world suite's declared transport boundary. The core
            # file chain now owns its objects instead of calling EXE ctors.
            self.hooks[self.call('ScenarioAcquireFileBoundary')] = self.acquire_file
            self.hooks[self.call('ScenarioReleaseFileBoundary')] = self.release_file
            self.hooks[self.call('ScenarioCreateTacticalBoundary')] = self.create_tactical
        self.write32(FILE_VTABLE+8,0x473FC0)
        self.hooks.update({0x473A80: self.default_file, 0x473FC0: self.file_name,
            0x473C50: self.file_exists, 0x473CE0: self.file_close,
            0x7CB673: self.splitpath, 0x7CA422: lambda: self.strncpy(wide=True),
            0x7CB4CC: self.wcsncmp, 0x6851F0: lambda: self.event('clear world'),
            0x653F50: self.map_size, 0x654490: self.local_size,
            0x5EE9D0: lambda: self.mode_event('special', special=True),
            0x552D60: lambda: self.manager_event('draw loading'),
            0x6D1C20: self.tactical_new, 0x6D5F60: self.tactical_rect,
            0x5349C0: self.theater, 0x534FA0: self.load_side,
            0x50C170: self.find_house, 0x4ACE70: self.read_map,
            0x47D2B0: self.refresh, 0x568BB0: self.total,
            0x6CF230: self.clear_swizzler, 0x74DE90: self.voxels,
            0x4309D0: self.beacon, 0x4F42F0: self.sidebar,
            SUPPORT+0x9800: self.read_country,
            SUPPORT+0x9820: lambda: self.mode_event('prepare'),
            SUPPORT+0x9840: lambda: self.mode_event('start'),
            SUPPORT+0x9860: lambda: self.mode_event('ready', ready=True),
            SUPPORT+0x9880: self.tactical_destroy,
            SUPPORT+0x98A0: self.clear_radar_cells})
        for desc in dll.DIRECTORY_ENTRY_IMPORT:
            for imported in desc.imports:
                if imported.name == b'wcsncmp':
                    self.hooks[self.read32(imported.address)] = self.wcsncmp
                if imported.name == b'strrchr':
                    self.hooks[self.read32(imported.address)] = self.strrchr
                if imported.name == b'wcslen':
                    self.hooks[self.read32(imported.address)] = self.wcslen
        for address, name in ((0x6F2040,'finish teams'), (0x465CC0,'building tiles'),
                (0x5FDDF0,'overlay bridges'), (0x722D00,'tile back'),
                (0x722240,'tile front'), (0x654650,'radar'),
                (0x725C70,'expired objects'), (0x452D40,'operational buildings'),
                (0x5866C0,'fog'), (0x660B00,'release helpers'), (0x657CE0,'redraw')):
            self.hooks[address] = lambda name=name: self.step_event(name)
        for address, name, global_arg in ((0x5009B0,'houses',False),
                (0x6F19B0,'teams',True), (0x691970,'scripts',True),
                (0x6E8220,'task forces',True), (0x7275D0,'trigger types',False),
                (0x6E5ED0,'tags',False), (0x41F2E0,'ai triggers',True),
                (0x7283C0,'triggers',False), (0x5FD2E0,'overlays',False),
                (0x71CA70,'terrain',False), (0x743270,'units',False),
                (0x41B110,'aircraft',False), (0x51FB00,'infantry',False),
                (0x44F820,'buildings',False), (0x6B4C80,'smudges',False)):
            self.hooks[address] = lambda name=name, global_arg=global_arg: self.objects(name, global_arg)
        for export, address, name, cleanup in (
                ('ScenarioRulesCountries',0x6722F0,'countries',4),
                ('ScenarioRulesGeneral',0x66D530,'general',4),
                ('ScenarioRulesInit',0x6686C0,'initialize',4),
                ('ScenarioRulesOverlay',0x668BF0,'overlay',4),
                ('ScenarioRulesCommands',0x674650,'commands',8)):
            self.boundary(export,address,lambda name=name,cleanup=cleanup: self.rules_pass(name,cleanup))
        self.boundary('ScenarioAssignHouses',0x687F10,lambda: self.event('assign houses'))
        self.boundary('ScenarioReadINI',0x689E90,self.read_scenario)
        if mock_clear:
            self.boundary('ScenarioClearWorld',0x6851F0,lambda: self.event('clear world'))
        else:
            del self.hooks[0x6851F0]
        self.main_ini = INI
        self.rules_ini = self.malloc(0x58)
        self.ini_labels = {INI:'map', self.rules_ini:'rules', 0x887128:'ai', 0x887208:'ui'}
        main = case.get('ini','[Basic]\nOfficial=yes\nInitTime=456\n[Map]\nTheater=SNOW\nSize=1,2,30,40\nLocalSize=3,4,20,25\n')
        for address, data in ((INI,main),(self.rules_ini,'[VariableNames]\n2=Ready\n4=Start\n'),(0x887128,''),(0x887208,'')):
            self.load_into(address,data)
        self.write32(0x887048,self.rules_ini)
        self.write32(0xA8EB64,case.get('difficulty',1)); self.write32(0xA8B278,case.get('difficulty',2))
        self.write32(0xA8E960,0x8F009ABC); self.write32(OBJECT,0xABCD9BCD)
        self.write32(0xA8E7A8,12345)
        self.uc.mem_write(0x829AE4,b'\x01')
        self.uc.mem_write(0xA8ED91,bytes([case.get('custom_ai',False)]))
        self.uc.mem_write(0xA8B31F,bytes([case.get('fog',False)]))
        self.uc.mem_write(0xA8B260,bytes([case.get('bridges',True)]))
        self.write32(0xA8B244,2 if case.get('special_mode') else 0)
        self.uc.mem_write(OBJECT+0x125C,case.get('filename','Mixed.map').encode()+b'\0')
        self.uc.mem_write(OBJECT+0x34BD,bytes([case.get('random',False)]))
        self.uc.mem_write(OBJECT+0x1180,b'\x37'*64)
        self.uc.mem_write(OBJECT+0x11C0,struct.pack('<16h',*range(1,17)))
        self.uc.mem_write(OBJECT+0x1360,b'Z\0'*45)
        self.uc.mem_write(OBJECT+0x13DA,b'Y\0'*45)
        self.uc.mem_write(OBJECT+0x1450,b'B\0'*1024)
        self.uc.mem_write(OBJECT+0x1C50,b'old briefing\0')
        self.uc.mem_write(OBJECT+0x362C,b'L'*272)
        country=self.read32(self.read32(0xA83C9C)); self.country=country
        self.write32(country,SUPPORT+0x9A00); self.write32(SUPPORT+0x9A00+100,SUPPORT+0x9800)
        for house in self.world_houses: self.write32(house+52,country)
        self.mode_object=SUPPORT+0x9B00
        self.write32(0xA8B23C,self.mode_object); self.write32(self.mode_object,SUPPORT+0x9C00)
        for offset, address in ((124,0x9860),(128,0x9820),(132,0x9840)):
            self.write32(SUPPORT+0x9C00+offset,SUPPORT+address)
        self.write32(0x880A0C,SUPPORT+0x9D00);self.write32(SUPPORT+0x9D00+12,SUPPORT+0x98A0)
        self.old_tactical=SUPPORT+0xA000
        self.write32(self.old_tactical,SUPPORT+0xA100);self.write32(SUPPORT+0xA100+32,SUPPORT+0x9880)
        self.write32(0x887324,self.old_tactical if case.get('old_tactical',True) else 0)
        self.uc.mem_map(0x4000000,0x100000)
        self.write32(MAP+0x13C,0x4000000)
        self.cell=self.malloc(0x200); self.write32(0x4000000+1025*4,self.cell)
        self.write32(MAP+0xF4,2)
        self.uc.mem_write(MAP+0xFC,struct.pack('<4i',9,8,7,6))
        self.events.clear()

    def boundary(self, export, original, callback):
        address = original
        if self.replacement:
            entry = self.exports[export]; code = bytes(self.uc.mem_read(entry,5))
            assert code[0] == 0xE9, (export,code.hex())
            address = entry+5+struct.unpack('<i',code[1:])[0]
        self.hooks[address]=callback

    def load_into(self, address, data):
        self.execute(0x535AA0,address)
        self.input,self.position=data.encode(),0
        self.hooks[0x4A2780]=self.get
        result=self.execute(0x525A60,address,STRAW,0)
        assert result==bool(data),(hex(address),data,result)
        del self.hooks[0x4A2780]

    def default_file(self):
        address=self.uc.reg_read(UC_X86_REG_ECX)
        self.files[address]=dict(name='',data=b'',position=0,opened=False)
        self.uc.mem_write(address,bytes(0x6C));self.write32(address,FILE_VTABLE)
        self.events.append(['file default']);self.ret(address)

    def acquire_file(self):
        assert self.arg(1) == 0
        address = self.malloc(0x6C)
        self.files[address] = dict(name='', data=b'', position=0, opened=False)
        self.uc.mem_write(address, bytes(0x6C)); self.write32(address, FILE_VTABLE)
        self.write32(self.arg(2), address)
        self.events.append(['file default']); self.ret(1)

    def release_file(self):
        f = self.files[self.arg(1)]
        assert not f['opened']
        self.events.append(['release', f['name'], self.read32(0xA8E7AC)])
        self.ret()

    def fd(self): return self.files[self.uc.reg_read(UC_X86_REG_ECX)]

    def file_name(self):
        name=self.string(self.arg(0)).decode(); f=self.fd();f.update(name=name,position=0,opened=False)
        available=self.case.get('files',{})
        f['data']=available.get(name,b'')
        if isinstance(f['data'],str):f['data']=f['data'].encode()
        self.events.append(['file name',name]);self.ret(self.arg(0),cleanup=4)

    def file_exists(self):
        f=self.fd();assert self.arg(0)==0
        self.events.append(['exists',f['name']]);self.ret(f['name'] in self.case.get('files',{}),cleanup=4)
    def file_has_handle(self): self.ret(self.fd()['opened'])
    def file_open_read(self):
        f=self.fd();assert self.arg(0)==1
        f.update(opened=True,position=0);self.events.append(['open',f['name']]);self.ret(1,cleanup=4)
    def file_read(self):
        f=self.fd();assert f['opened']
        count=min(self.arg(1),len(f['data'])-f['position'])
        if count:self.uc.mem_write(self.arg(0),f['data'][f['position']:f['position']+count])
        f['position']+=count;self.ret(count,cleanup=8)
    def file_close(self):
        f=self.fd();f['opened']=False;self.events.append(['close',f['name']]);self.ret()
    def file_destroy(self,deleting=False):
        f=self.fd();assert not f['opened']
        self.events.append(['release',f['name'],self.read32(0xA8E7AC)])
        self.ret(cleanup=4 if deleting else 0)
    def splitpath(self):
        source=self.string(self.arg(0)).replace(b'\\',b'/').split(b'/')[-1].split(b':')[-1]
        stem=source.rsplit(b'.',1)[0]
        assert self.arg(1)==self.arg(2)==self.arg(4)==0
        self.uc.mem_write(self.arg(3),stem+b'\0');self.ret()
    def wcsncmp(self):
        for i in range(self.arg(2)):
            a=struct.unpack('<H',self.uc.mem_read(self.arg(0)+i*2,2))[0]
            b=struct.unpack('<H',self.uc.mem_read(self.arg(1)+i*2,2))[0]
            if a!=b:self.ret((a>b)-(a<b));return
            if not a:break
        self.ret()
    def wcslen(self):
        count=0
        while bytes(self.uc.mem_read(self.arg(0)+count*2,2))!=b'\0\0':count+=1
        self.ret(count)
    def strrchr(self):
        offset=(self.string(self.arg(0))+b'\0').rfind(bytes([self.arg(1)&255]))
        self.ret(self.arg(0)+offset if offset>=0 else 0)
    def unused_brief_name(self):
        self.format(self.arg(0),self.string(self.arg(1)),self.uc.reg_read(UC_X86_REG_ESP)+12,512)
    def text(self):
        if not hasattr(self,'case'):return super().text()
        label=self.string(self.uc.reg_read(UC_X86_REG_ECX)).decode()
        if label=='GUI:SkirmishGame':return super().text()
        self.events.append(['string',label,self.arg(1)])
        value=self.case.get('strings',{}).get(label,'MISSING:'+label)
        data=value.encode('utf-16le')+b'\0\0';address=self.malloc(len(data));self.uc.mem_write(address,data)
        self.ret(address,cleanup=8)
    def ini_label(self,address): return self.ini_labels.get(address,'auxiliary')
    def rules_pass(self,name,cleanup):
        self.events.append(['rules',name,self.ini_label(self.arg(0))]+([self.arg(1)&255] if cleanup==8 else []))
        self.ret(1,cleanup=cleanup)
    def read_country(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==self.country
        self.events.append(['country',self.ini_label(self.arg(0))]);self.ret(1,cleanup=4)
    def map_size(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==MAP
        rect=list(struct.unpack('<4i',self.uc.mem_read(self.arg(0),16)))
        self.events.append(['map size',rect]+[self.arg(i)&255 for i in (1,2,3)])
        # The downstream map module supplies a small valid traversal grid.
        self.write32(MAP+0xF4,2);self.ret(cleanup=16)
    def local_size(self):
        rect=bytes(self.uc.mem_read(self.arg(0),16));self.uc.mem_write(MAP+0xFC,rect)
        self.events.append(['local size',list(struct.unpack('<4i',rect))]);self.ret(cleanup=4)
    def mode_event(self,name,ready=False,special=False):
        value=self.uc.reg_read(UC_X86_REG_ECX)&255 if special else None if ready else self.arg(0)&255
        self.events.append(['mode',name,value]);self.ret(cleanup=0 if ready or special else 4)
    def tactical_destroy(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==self.old_tactical and self.arg(0)==1
        self.events.append(['destroy tactical']);self.ret(cleanup=4)
    def tactical_new(self):
        self.events.append(['create tactical']);self.ret(self.uc.reg_read(UC_X86_REG_ECX))
    def create_tactical(self):
        # Rendering construction is controlled on both sides of this suite;
        # compare_map_root/Tactical constructor fixtures cover real objects.
        address = self.malloc(0xE18)
        self.write32(address, SUPPORT + 0xA100)
        self.write32(self.arg(1), address)
        self.events.append(['create tactical']); self.ret(1)
    def clear_radar_cells(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==MAP+0x1224
        self.event('clear radar cells')
    def tactical_rect(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==self.read32(0x887324)
        self.events.append(['tactical rect',list(struct.unpack('<4i',self.uc.mem_read(self.arg(0),16)))]);self.ret(cleanup=4)
    def theater(self):
        self.events.append(['theater',self.uc.reg_read(UC_X86_REG_ECX)]);self.ret()
    def find_house(self):
        name=self.string(self.uc.reg_read(UC_X86_REG_ECX)).decode()
        self.events.append(['find house',name]);self.ret(-1 if self.case.get('unknown_house') else 0)
    def load_side(self):
        self.side_calls+=1
        self.events.append(['load side',self.uc.reg_read(UC_X86_REG_ECX),self.read32(OBJECT+0x34B8)])
        self.ret(self.side_calls!=self.case.get('side_failure'))
    def read_scenario(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==OBJECT and self.arg(0)==INI
        self.events.append(['read scenario'])
        if self.case.get('change_side'):self.write32(self.country+188,1)
        if self.case.get('drop_mode'):self.write32(0xA8B23C,0)
        self.ret(not self.case.get('read_failure'),cleanup=4)
    def objects(self,name,global_arg):
        ini=self.uc.reg_read(UC_X86_REG_ECX)
        self.events.append(['objects',name,self.ini_label(ini)]+([self.uc.reg_read(UC_X86_REG_EDX)&255] if global_arg else [])
            +([self.uc.mem_read(0x829AE4,1)[0]] if name=='buildings' else []));self.ret()
    def read_map(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==MAP and self.arg(0)==INI
        self.events.append(['objects','map']);self.ret(cleanup=4)
    def refresh(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==self.cell and self.arg(0)==0xFFFFFFFF
        self.events.append(['refresh cell']);self.ret(cleanup=4)
    def total(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==MAP and self.arg(0)==0
        self.events.append(['map total']);self.ret(1234,cleanup=4)
    def clear_swizzler(self):
        assert self.arg(0)==0xB0C110
        self.events.append(['clear swizzler']);self.ret(cleanup=4)
    def voxels(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)&255==1;self.event('voxel cache')
    def beacon(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==0x89C3B0;self.event('beacon')
    def sidebar(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==MAP and self.arg(0)==2
        self.events.append(['sidebar',2]);self.ret(cleanup=4)
    def step_event(self,name):
        if name in ('fog','radar','redraw'):assert self.uc.reg_read(UC_X86_REG_ECX)==MAP
        self.events.append([name]+([self.read32(0xA8E7AC)] if name=='operational buildings' else []));self.ret()
    def run(self):
        result=self.call('ScenarioInitializeWorldINI',ecx=INI,edx=self.case.get('skip_units',False))&255
        return dict(result=result,events=self.events,scenario=self.state().hex(),
            flags=self.read32(0xA8E960),depth=self.read32(0xA8E7AC),crc=self.read32(0xA8E7A8),
            building=self.uc.mem_read(0x829AE4,1)[0],total=self.read32(MAP+0x134))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe',type=Path);parser.add_argument('dll',type=Path)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    original,dll=pefile.PE(str(args.exe)),pefile.PE(str(args.dll))
    results=[]
    base=dict(mode=0,failure=0,briefing=False,movie=0)
    cases=[dict(base,mode=mode,fog=fog,skip_units=skip) for mode in (0,3,4,5) for fog in (False,True) for skip in (False,True)]
    cases += [dict(base,**change) for change in (
        {'side_failure':1},{'side_failure':2},{'read_failure':True},{'unknown_house':True},
        {'change_side':True},{'change_side':True,'side_failure':2},{'campaign':0},
        {'old_tactical':False},{'difficulty':-1},{'init_depth':0x7FFFFFFF},
        {'mode':5,'special_mode':True},{'mode':5,'drop_mode':True},
        {'mode':5,'bridges':False,'fog':True},{'mode':5,'random':True},
        {'mode':5,'country':-3},{'mode':5,'ini':'[Basic]\n'},
        {'mode':5,'ini':'[Map]\nSize=7,x\nLocalSize=\n'},
        {'mode':5,'custom_ai':True,'files':{'TMCJ4F.INI':'[General]\nCreditTicks=17\n'}},
        {'mode':5,'custom_ai':True},
        {'files':{'Mixed.INI':'[General]\nCreditTicks=17\n','MISSIONMD.INI':'[Mixed.map]\nName=Mission\n'}},
        {'filename':'C:\\Maps\\Mixed.map','files':{'Mixed.INI':'[General]\nCreditTicks=17\n'}},
        {'files':{'MISSIONMD.INI':'[Mixed.map]\nName=\\x0041\\x0042\nBriefing=TXT:Brief\nUIName=TXT:Title\nLSLoadMessage=Load\nLSLoadBriefing=Brief\nLS640BriefLocX=-7\nLS640BriefLocY=8\nLS800BriefLocX=9\nLS800BriefLocY=10\nLS640BkgdName=Small\nLS800BkgdName=Large\nLS800BkgdPal=Colors\n'},'strings':{'TXT:Brief':'Briefing text','TXT:Title':'Title'}},
        {'files':{'MISSIONMD.INI':'[Mixed.map]\nUIName=TXT:Title\n'},'strings':{'TXT:Title':'T'*60,'TXT:TitleSav':'S'*60}},
        {'files':{'MISSIONMD.INI':'[Mixed.map]\nUIName=TXT:Title\nBriefing=TXT:Brief\n'},'strings':{'TXT:Title':'T'*43+'\U0001F600 tail','TXT:Brief':'B'*1022+'\U0001F600 tail'}},
        {'files':{'MISSIONMD.INI':'[Mixed.map]\nUIName=\nBriefing=\n'}},
        {'files':{'MISSIONMD.INI':'[Other.map]\nName=Other\n'}},
        {'files':{'Mixed.INI':'[Mixed.map]\nUIName=TXT:Title\n','MISSIONMD.INI':'[Other.map]\nName=Other\n'}})]
    for i,case in enumerate(cases):
        a,b=(InitializeMachine(original,dll,replacement,case).run() for replacement in (False,True))
        if a!=b:
            args.report.parent.mkdir(parents=True,exist_ok=True)
            args.report.with_suffix('.failure.json').write_text(json.dumps(dict(case=case,original=a,replacement=b),indent=2))
            raise AssertionError((i,case,[key for key in a if a[key]!=b[key]]))
        results.append(dict(entry='ScenarioInitializeWorldINI',case=case,passed=True));print('PASS initialize',i)
    for value in (None,'','1','-1,2','7,x,9,10','1,2,3,4',' 1, 2, 3, 4','1 ,2,3,4','1,2,3,4,5'):
        outputs=[]
        for replacement in (False,True):
            m=IniMachine(original,dll,replacement);m.load('[Map]\n'+('' if value is None else 'Size='+value+'\n'))
            defaults=m.malloc(16);output=m.malloc(16);m.uc.mem_write(defaults,struct.pack('<4i',1,2,30,40))
            result=m.call('ScenarioReadRectangle',ecx=INI,args=(output,m.cstring(b'Map'),m.cstring(b'Size'),defaults))
            assert result==output;outputs.append(bytes(m.uc.mem_read(output,16)))
        assert outputs[0]==outputs[1],(value,outputs)
        results.append(dict(entry='ScenarioReadRectangle',value=value,passed=True));print('PASS rectangle',repr(value))
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(exe_sha256=SHA,probe_sha256=hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        scope=__doc__,passed=len(results),cases=results),indent=2)+'\n')


if __name__=='__main__':main()
