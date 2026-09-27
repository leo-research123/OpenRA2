#!/usr/bin/env python3
"""534450 object destruction differential, including ClearWorld integration.

Real target-index sorting, collection iteration, notices ownership, virtual
dispatch and initialization depth execute on both sides. Owning destructors,
COM Release and external resource/expiration operations are controlled. No
534450/6851F0/686B20 controller is mocked in its integrated execution path.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import pefile
from unicorn.x86_const import *
from compare_scenario_clear import ClearMachine, MAP, OBJECT, SUPPORT, ENTRIES, SHA, TYPES

ENTRIES['ScenarioDestroyWorldObjects']=0x534450
COLLECTIONS=[('bullets',0xA8ED40),('objects',0xA8E360),('tags',0xB0E720),
    ('triggers',0xA8EAE8),('tubes',0x8B4138),('building lights',0x8B4190),
    ('overlays',0xA8EC50),('particle systems',0xA80208),('waves',0xA8EC38),
    ('factories',0xA83E30),('sides',0x8B4120),('teams',0x8B40E8),
    ('houses',0xA80228),('animations',0xA8E9A8),('scripts',0x8872B0),
    ('radiation sites',0xB04BD0),('light sources',0xABCA10),('empulses',0x8A3870),
    ('capture managers',0x89E0F0),('disk lasers',0x8A0208),('parasites',0xAC4910),
    ('temporals',0xB0EC60),('airstrikes',0x889FB8),('spawn managers',0xB0B880),
    ('bombs',0x89C668),('spotlights',0xAC1678),('fogged objects',0x8B3D10),
    ('alpha shapes',0x88A0F0),('terrain',0xA8E988),('types',TYPES)]
TARGETS,NOTICES=0xB0E840,0xB0F698


class DestructionMachine(ClearMachine):
    def __init__(self, original, dll, replacement, case):
        super().__init__(original,dll,replacement,case,mock_destroy=False)
        self.flushes=0;self.deferred=[];self.spawned=False;self.dead=set()
        self.group_addresses=dict(COLLECTIONS)
        self.hooks.update({0x69E210:lambda:self.module('unload shapes'),
            0x725C70:self.expired,0x4C29E0:lambda:self.module('electric bolts'),
            0x556DF0:lambda:self.module('line trails'),0x550000:lambda:self.module('lasers'),
            0x430980:lambda:self.module('beacons'),0x565B00:lambda:self.module('map objects'),
            0x5FF2D0:self.spotlight_destruct})
        self.array(NOTICES,[self.particle,self.other_particle],'notices',owned=case.get('owned',True))
        amount=case.get('amount',2)
        for name,address in COLLECTIONS:
            if name=='types':continue
            items=[self.object(name+':'+str(i),8 if name=='bullets' else 6,case.get('references',3)) for i in range(amount)]
            self.array(address,items,name,capacity=max(amount+8,16))
        rows=[]
        ids=case.get('ids',[9,-1,0,2147483647,-2147483648,7])
        for i,key in enumerate(ids):
            if i==0 and amount and case.get('indexed_collection',True):
                obj=self.read32(self.read32(self.group_addresses['objects']+4))
            else:obj=self.object('target:'+str(i),6,0)
            rows.append((key,obj))
        if case.get('presorted'):rows.sort(key=lambda row:row[0])
        table=self.malloc(max(8*len(rows),16))
        self.write32(TARGETS,table);self.write32(TARGETS+4,len(rows));self.write32(TARGETS+8,len(rows))
        self.uc.mem_write(TARGETS+12,bytes([case.get('presorted',False)]))
        self.write32(TARGETS+16,table+8 if len(rows)>1 or case.get('stale_archive') else 0)
        for i,(key,obj) in enumerate(rows):self.write32(table+i*8,key);self.write32(table+i*8+4,obj)
        self.uc.mem_write(0x829AE4,b'\x01')
        if case.get('no_scenario'):self.write32(0xA8B230,0)
        self.events=[];self.freed=[]

    def members(self,address):
        return [self.read32(self.read32(address+4)+4*i) for i in range(self.read32(address+16))]

    def names(self,address):
        return [self.clear_objects[item]['name'] if item in self.clear_objects else
            'loaded house '+str(self.world_houses.index(item)) for item in self.members(address)]

    def objects(self,name,global_arg):
        if name=='houses':
            # The existing House reader recreates the scene's House collection.
            # Supply its output at this controlled module boundary, after all
            # old registered objects have passed through real teardown.
            self.write32(0xA8022C,self.house_array);self.write32(0xA80238,len(self.world_houses))
        super().objects(name,global_arg)

    def depth(self):return self.read32(0xA8E7AC)

    def remove_target(self,address):
        table=self.read32(TARGETS);count=self.read32(TARGETS+4)
        rows=[bytes(self.uc.mem_read(table+8*i,8)) for i in range(count)]
        updated=[row for row in rows if struct.unpack('<II',row)[1]!=address]
        if len(rows)!=len(updated):
            if updated:self.uc.mem_write(table,b''.join(updated))
            self.write32(TARGETS+4,len(updated));self.write32(TARGETS+16,0)

    def delete_state(self,address,action='delete'):
        assert address not in self.dead,('double delete',self.clear_objects[address])
        obj=self.clear_objects[address];self.dead.add(address)
        self.events.append([action,obj['name'],self.depth(),self.uc.mem_read(TARGETS+12,1)[0],bool(self.read32(TARGETS+16))])
        self.remove_target(address)
        for _,array in COLLECTIONS+[('tag types',0xB0E780)]:
            if address in self.members(array):self.remove(array,address)
        if self.case.get('deferred') and obj['name']=='objects:0':
            pending=self.members(self.group_addresses['tags'])
            if pending:self.deferred.append(pending[0])
        if self.case.get('spawn') and not self.spawned and obj['name'].startswith('objects:'):
            self.spawned=True
            array=self.group_addresses['objects'];count=self.read32(array+16)
            item=self.object('objects:spawned',6,0)
            self.write32(self.read32(array+4)+count*4,item);self.write32(array+16,count+1)

    def destroy_object(self):
        assert self.arg(0)==1
        address=self.uc.reg_read(UC_X86_REG_ECX)
        self.delete_state(address);self.ret(address,cleanup=4)

    def release_object(self):
        address=self.arg(0);obj=self.clear_objects[address]
        obj['references']-=1
        self.events.append(['release',obj['name'],obj['references'],self.depth()])
        if obj['references']==0:self.delete_state(address,'released')
        self.ret(obj['references'],cleanup=4)

    def spotlight_destruct(self):
        address=self.uc.reg_read(UC_X86_REG_ECX)
        assert address in self.members(self.group_addresses['spotlights'])
        self.delete_state(address,'spotlight destructor');self.ret()

    def module(self,name):
        this=self.uc.reg_read(UC_X86_REG_ECX)
        if name=='map objects':assert this==MAP
        if name=='beacons':assert this==0x89C3B0
        self.events.append([name,self.depth()]);self.ret()

    def expired(self):
        self.flushes+=1
        self.events.append(['expired',self.flushes,self.depth()])
        while self.deferred:
            address=self.deferred.pop(0)
            if address not in self.dead:self.delete_state(address,'deferred delete')
        if self.flushes==2 and self.case.get('depth_change'):self.write32(0xA8E7AC,self.depth()+5)
        self.ret()

    def run(self):
        if self.case.get('initialize'):
            result=self.call('ScenarioInitializeWorldINI',ecx=self.main_ini,edx=0)&255
        else:
            entry='ScenarioClearWorld' if self.case.get('clear') else 'ScenarioDestroyWorldObjects'
            result=self.call(entry)
        notice_pointer=self.read32(NOTICES+4)
        return dict(result=result,events=self.events,depth=self.depth(),flushes=self.flushes,
            collections={name:dict(items=self.names(address),capacity=self.read32(address+8),
                flags=bytes(self.uc.mem_read(address+12,2)).hex()) for name,address in COLLECTIONS},
            index=dict(count=self.read32(TARGETS+4),capacity=self.read32(TARGETS+8),
                sorted=self.uc.mem_read(TARGETS+12,1)[0],archive=bool(self.read32(TARGETS+16))),
            notices=dict(count=self.read32(NOTICES+16),capacity=self.read32(NOTICES+8),
                flags=bytes(self.uc.mem_read(NOTICES+12,2)).hex(),pointer=bool(notice_pointer)),
            # CCINI's private temporary allocations differ across modules;
            # compare the world slots and owned spotlights in this suite.
            freed=[self.allocations[p] if p in self.allocations else self.clear_objects[p]['name']
                for p in self.freed if p in self.allocations or p in self.clear_objects],
            tactical=bool(self.read32(0x887324)),scenario=self.state().hex())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe',type=Path);parser.add_argument('dll',type=Path)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    original,dll=pefile.PE(str(args.exe)),pefile.PE(str(args.dll))
    base=dict(mode=0,failure=0,briefing=False,movie=0)
    cases=[dict(base,amount=count,owned=owned,presorted=sorted,old_tactical=tactical)
        for count in (0,1,3) for owned in (False,True) for sorted in (False,True) for tactical in (False,True)]
    cases += [dict(base,**change) for change in (
        {'ids':[],'types':[],'amount':0},{'ids':[],'stale_archive':True},
        {'ids':[5]},{'ids':[7,7,7,7,7,7,7,7,7,7]},
        {'ids':list(range(19,-1,-1))},{'ids':[3,-1,8,0,9,7,3,6,2,5,4,-2147483648,2147483647]},
        {'deferred':True},{'spawn':True},{'deferred':True,'spawn':True},
        {'depth_change':True},{'init_depth':0x7FFFFFFF},{'no_scenario':True},
        {'clear':True},{'clear':True,'owned':False},{'clear':True,'deferred':True},
        {'clear':True,'spawn':True},{'clear':True,'old_tactical':False},
        {'initialize':True},{'initialize':True,'owned':False},
        {'initialize':True,'side_failure':1},{'initialize':True,'read_failure':True},
        {'initialize':True,'files':{'MISSIONMD.INI':'[Mixed.map]\nUIName=TXT:Title\n'}})]
    results=[]
    for i,case in enumerate(cases):
        a,b=(DestructionMachine(original,dll,replacement,case).run() for replacement in (False,True))
        if a!=b:
            args.report.parent.mkdir(parents=True,exist_ok=True)
            args.report.with_suffix('.failure.json').write_text(json.dumps(dict(case=case,original=a,replacement=b),indent=2))
            raise AssertionError((i,case,[key for key in a if a[key]!=b[key]]))
        results.append(dict(case=case,passed=True));print('PASS destruction',i)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(exe_sha256=SHA,probe_sha256=hashlib.sha256(args.dll.read_bytes()).hexdigest(),
        scope=__doc__,passed=len(results),cases=results),indent=2)+'\n')


if __name__=='__main__':main()
