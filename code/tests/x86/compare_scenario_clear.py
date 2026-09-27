#!/usr/bin/env python3
"""6851F0 clear-world differential using original objects and collection layouts.

Scenario Reset/SetGlobal, volume math, type filtering, vector edits and cleanup
ordering execute real code. 534450 destruction, owning object destructors/COM
Release, capacity growth and downstream map/render/audio modules are controlled.
No invocation of 6851F0 or its compiled replacement is mocked.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn.x86_const import *
from compare_scenario_initialize import InitializeMachine, MAP, OBJECT, SUPPORT, ENTRIES, SHA

ENTRIES['ScenarioClearWorld']=0x6851F0
TYPES, TILES, OBJECTS, TAG_TYPES = 0xA8E968,0xA8ED28,0xA8E360,0xB0E780


class ClearMachine(InitializeMachine):
    def __init__(self, original, dll, replacement, case, mock_destroy=True):
        super().__init__(original,dll,replacement,case,mock_clear=False)
        self.clear_objects,self.allocations={},{}
        self.hooks.update({0x534450:self.destroy_world,
            0x62E650:self.drain_particles,0x660C50:self.reset_positions,
            0x6E57F0:self.global_changed,
            SUPPORT+0xA600:self.rtti,SUPPORT+0xA620:self.what_am_i,
            SUPPORT+0xA640:self.release_object,SUPPORT+0xA660:self.destroy_object,
            SUPPORT+0xA680:self.grow_types,SUPPORT+0xA6A0:lambda:self.clear_step('logic')})
        if mock_destroy:
            self.boundary('ScenarioDestroyWorldObjects',0x534450,self.destroy_world)
        else:
            del self.hooks[0x534450]
        for address,name in ((0x750FA0,'audio'),(0x53A090,'stop lightning'),
                (0x565B00,'map objects'),(0x6E5570,'tags'),(0x6DA980,'tactical records'),
                (0x5549A0,'lights'),(0x539760,'lightning'),(0x4C5470,'empulses'),
                (0x74D760,'veinholes'),(0x722390,'tile front'),(0x722E50,'tile back'),
                (0x439110,'bombs'),(0x54E6F0,'kamikazes'),(0x413800,'aircraft tracker'),
                (0x6370B0,'planning'),(0x5BDF50,'map initialize'),(0x52CB90,'campaigns'),
                (0x734210,'sidebar timers'),(0x734270,'sidebar objects')):
            self.hooks[address]=lambda name=name:self.clear_step(name)
        self.write32(0x87F778,SUPPORT+0xA700);self.write32(SUPPORT+0xA700+12,SUPPORT+0xA6A0)
        self.write32(SUPPORT+0xA400+8,SUPPORT+0xA680)
        self.write32(SUPPORT+0xA500+12,SUPPORT+0xA600)
        for slot,address in ((11,0xA620),(2,0xA640),(8,0xA660)):
            self.write32(SUPPORT+0xA480+slot*4,SUPPORT+address)
        types=[];tiles=[]
        for i,kind in enumerate(case.get('types',[14,18,7,18,18,35])):
            obj=self.object('type '+str(i),kind,0)
            types.append(obj)
            if kind==18:tiles.append(obj)
        self.array(TYPES,types,'types',capacity=max(len(types),case.get('capacity',6)))
        self.array(TILES,tiles,'tiles')
        self.array(OBJECTS,[self.object('object '+str(i),kind,case.get('references',1))
            for i,kind in enumerate(case.get('objects',[8,6,8]))],'objects')
        self.array(TAG_TYPES,[self.object('tag type '+str(i),39,0)
            for i in range(case.get('tags',2))],'tag types')
        self.collections={'zones':MAP+0x50,'map tags':0x8B41A8,'logic tags':0x8B40C8,'current objects':0xA8ECB8}
        for name,address in self.collections.items():
            self.array(address,[123,456],name,owned=case.get('owned',True),capacity=3)
        self.particle=self.object('particles',24,0)
        self.other_particle=self.object('replacement particles',24,0)
        self.write32(0xA8ED78,self.particle if case.get('particles',True) else 0)
        self.uc.mem_write(OBJECT+0x1CB0,b'\0')
        for i in range(50):self.uc.mem_write(OBJECT+0x1CB0+i*41,bytes([case.get('globals',[0,1,255,2])[i%len(case.get('globals',[0,1,255,2]))]]))
        self.write32(0xA83D4C,self.world_houses[0])
        self.write32(0x83D834,1234)
        self.uc.mem_write(SUPPORT+0x800,struct.pack('<8I',17,18,19,20,21,22,23,24))
        self.uc.mem_write(MAP+0xEC,struct.pack('<4i',1,2,3,4))
        self.write32(0xB05458,case.get('empty_cell',0))
        self.events=[];self.freed=[]

    def array(self,address,items,name,owned=True,capacity=None):
        capacity=len(items) if capacity is None else capacity
        storage=self.malloc(max(capacity*16,16))
        self.allocations[storage]=name
        self.write32(address,SUPPORT+0xA400);self.write32(address+4,storage)
        self.write32(address+8,capacity);self.uc.mem_write(address+12,bytes([1,owned]))
        self.write32(address+16,len(items));self.write32(address+20,3)
        for i,item in enumerate(items):self.write32(storage+i*4,item)

    def object(self,name,kind,references):
        address=self.malloc(0x200)
        self.clear_objects[address]=dict(name=name,kind=kind,references=references)
        self.write32(address,SUPPORT+0xA480);self.write32(address+4,SUPPORT+0xA500)
        return address

    def names(self,address):
        return [self.clear_objects[self.read32(self.read32(address+4)+i*4)]['name'] for i in range(self.read32(address+16))]

    def rtti(self):
        obj=self.clear_objects[self.arg(0)-4]
        self.events.append(['rtti',obj['name']]);self.ret(obj['kind'],cleanup=4)

    def what_am_i(self):
        obj=self.clear_objects[self.uc.reg_read(UC_X86_REG_ECX)]
        self.events.append(['what am i',obj['name']]);self.ret(obj['kind'])

    def remove(self,array,address):
        items=self.read32(array+4);count=self.read32(array+16)
        values=[self.read32(items+i*4) for i in range(count)]
        values.remove(address);self.write32(array+16,len(values))
        for i,value in enumerate(values):self.write32(items+i*4,value)

    def release_object(self):
        address=self.arg(0);obj=self.clear_objects[address]
        obj['references']-=1
        self.events.append(['release object',obj['name'],obj['references'],self.uc.mem_read(0x829AE4,1)[0]])
        if obj['references']==0:self.remove(OBJECTS,address)
        self.ret(obj['references'],cleanup=4)

    def destroy_object(self):
        address=self.uc.reg_read(UC_X86_REG_ECX);obj=self.clear_objects[address]
        assert self.arg(0)==1
        self.events.append(['delete',obj['name'],self.uc.mem_read(0x829AE4,1)[0]])
        if obj['name'].startswith('tag type'):self.remove(TAG_TYPES,address)
        elif obj['name'].startswith('object'):self.remove(OBJECTS,address)
        self.ret(address,cleanup=4)

    def destroy_world(self):
        self.events.append(['destroy world',self.names(TYPES)])
        assert all(self.clear_objects[self.read32(self.read32(TYPES+4)+4*i)]['kind']!=18 for i in range(self.read32(TYPES+16)))
        self.write32(TYPES+16,0)
        if self.case.get('grow'):
            self.write32(TYPES+8,0);self.uc.mem_write(TYPES+13,b'\0')
        if self.case.get('destroy_tactical_in_world'):self.write32(0x887324,0)
        self.ret()

    def grow_types(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==TYPES and self.arg(1)==0
        capacity=self.arg(0);self.events.append(['grow types',capacity])
        if not self.case.get('growth_failure'):
            storage=self.malloc(capacity*4);old=self.read32(TYPES+4)
            count=self.read32(TYPES+16)
            if count:self.uc.mem_write(storage,bytes(self.uc.mem_read(old,count*4)))
            self.write32(TYPES+4,storage);self.write32(TYPES+8,capacity);self.uc.mem_write(TYPES+13,b'\x01')
        self.ret(not self.case.get('growth_failure'),cleanup=8)

    def clear_step(self,name):
        this=self.uc.reg_read(UC_X86_REG_ECX)
        expected={'map objects':MAP,'map initialize':MAP,'bombs':0x87F5D8,
            'kamikazes':0xABC5F8,'aircraft tracker':0x887888,'logic':0x87F778,'sidebar objects':0}
        if name in expected:assert this==expected[name],(name,hex(this))
        self.events.append([name,self.uc.mem_read(OBJECT+0x218,1)[0],bool(self.read32(0xA83D4C)),
            self.uc.mem_read(0x829AE4,1)[0]])
        self.ret()

    def global_changed(self):
        index=self.uc.reg_read(UC_X86_REG_ECX)
        assert self.uc.mem_read(OBJECT+0x1CB0+index*41,1)[0]==0
        assert self.uc.mem_read(OBJECT+0x34AA,1)[0]==1
        self.events.append(['global changed',index]);self.ret()

    def drain_particles(self):
        assert self.uc.reg_read(UC_X86_REG_ECX)==self.particle
        self.events.append(['drain particles'])
        if self.case.get('drain_clears'):self.write32(0xA8ED78,0)
        if self.case.get('drain_replaces'):self.write32(0xA8ED78,self.other_particle)
        self.ret()

    def tactical_new(self):
        address=self.uc.reg_read(UC_X86_REG_ECX)
        self.write32(address,SUPPORT+0xA100)
        self.events.append(['create tactical']);self.ret(address)

    def tactical_destroy(self):
        address=self.uc.reg_read(UC_X86_REG_ECX)
        assert address==self.read32(0x887324) and self.arg(0)==1
        self.events.append(['destroy tactical','old' if address==self.old_tactical else 'new']);self.ret(cleanup=4)

    def reset_positions(self):
        self.events.append(['reset positions',self.uc.mem_read(OBJECT+0x218,1)[0]])
        self.write32(OBJECT+0x214,7654321)
        self.ret(self.case.get('reset_result',12345))

    def run(self):
        if self.case.get('initialize'):
            result=self.call('ScenarioInitializeWorldINI',ecx=self.main_ini,edx=0)&255
        else:result=self.call('ScenarioClearWorld')
        arrays={name:dict(count=self.read32(address+16),capacity=self.read32(address+8),
            flags=bytes(self.uc.mem_read(address+12,2)).hex(),
            pointer=self.allocations.get(self.read32(address+4),'null' if not self.read32(address+4) else 'unknown'))
            for name,address in self.collections.items()}
        return dict(result=result,events=self.events,scenario=self.state().hex(),types=self.names(TYPES),
            # Compare world slot ownership, not auxiliary CCINI's different
            # private index allocations in the integrated initializer cases.
            type_capacity=self.read32(TYPES+8),arrays=arrays,freed=[self.allocations[p] for p in self.freed if p in self.allocations],
            current_player=self.read32(0xA83D4C),particle=self.read32(0xA8ED78),
            map_rect=bytes(self.uc.mem_read(MAP+0xEC,16)).hex(),
            volume=bytes(self.uc.mem_read(SUPPORT+0x800,32)).hex(),paused_volume=self.read32(0x83D834),
            objects=self.names(OBJECTS),tags=self.names(TAG_TYPES))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe',type=Path);parser.add_argument('dll',type=Path)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    original,dll=pefile.PE(str(args.exe)),pefile.PE(str(args.dll))
    base=dict(mode=0,failure=0,briefing=False,movie=0)
    cases=[dict(base,mode=mode,owned=owned,particles=particles,references=refs)
        for mode in (0,3,5) for owned in (False,True) for particles in (False,True) for refs in (1,3)]
    cases += [dict(base,**change) for change in (
        {'types':[],'objects':[],'tags':0,'globals':[0]},
        {'types':[18,18,18]},{'types':[14,7,35]},{'grow':True},
        {'grow':True,'growth_failure':True},{'drain_clears':True},
        {'drain_replaces':True},{'destroy_tactical_in_world':True},
        {'old_tactical':False},{'empty_cell':0x000A000B},{'reset_result':-1},
        {'initialize':True},{'initialize':True,'owned':False},
        {'initialize':True,'side_failure':1},{'initialize':True,'read_failure':True},
        {'initialize':True,'files':{'MISSIONMD.INI':'[Mixed.map]\nUIName=TXT:Title\n'}})]
    results=[]
    for i,case in enumerate(cases):
        a,b=(ClearMachine(original,dll,replacement,case).run() for replacement in (False,True))
        if a!=b:
            args.report.parent.mkdir(parents=True,exist_ok=True)
            args.report.with_suffix('.failure.json').write_text(json.dumps(dict(case=case,original=a,replacement=b),indent=2))
            raise AssertionError((i,case,[key for key in a if a[key]!=b[key]]))
        results.append(dict(case=case,passed=True));print('PASS clear',i)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(exe_sha256=SHA,probe_sha256=hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        scope=__doc__,passed=len(results),cases=results),indent=2)+'\n')


if __name__=='__main__':main()
