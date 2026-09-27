#!/usr/bin/env python3
"""Audit original Radar x86 instructions against the current compiled methods.

This is a diagnostic, not a golden-output generator. Reports actual mismatches;
returns 1 when equivalence fails. No candidate results become expected values.
State fixtures are original-layout method inputs, not constructed game worlds.
"""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
from probe_build import build_probe

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

SHA = '7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6'
ROOT = Path(__file__).resolve().parents[3]
BASE, OBJ, STACK, STOP = 0x1000000, 0x1000000, 0x1180000, 0x1190000
OUT, ARG, RAW, PIXELS = BASE+0x2000, BASE+0x2100, BASE+0x10000, BASE+0x100000
FIELDS = {
    'state': (0x14AC, 'I'), 'mode': (0x14B0, 'I'), 'pending_mode': (0x14B4, 'I'),
    'flag_14BC': (0x14BC, 'B'), 'flag_14BD': (0x14BD, 'B'),
    'available': (0x14D8, 'B'), 'redraw': (0x14D9, 'B'), 'full_redraw': (0x14DA, 'B'),
    'frame': (0x14FC, 'I'), 'timer_start': (0x1500, 'I'), 'timer_left': (0x1508, 'I'),
    'cell_count': (0x1234, 'I'), 'point_count': (0x126C, 'I'),
}
TRANSITION = {'state','mode','pending_mode','available','frame','flag_14BC','flag_14BD'}
REGS = (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


SOURCES = ('code/core/src/yrpp/RadarClassState.cpp',
           'code/core/src/yrpp/RadarClassGeometry.cpp',
           'code/core/src/yrpp/RadarClassRefresh.cpp',
           'code/tests/x86/radar_instruction_probe.cpp')

def build(compile_db, directory):
    return build_probe(compile_db, directory, SOURCES, 'radar_instruction_probe.dll',
        object_suffix='.obj',
        commands_file='build-commands.json',
        require_x86=True)


class Machine:
    def __init__(self, exe, dll, candidate):
        self.cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.candidate = candidate
        for pe in (exe,dll):
            base = pe.OPTIONAL_HEADER.ImageBase
            self.cpu.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
            self.cpu.mem_write(base,pe.get_memory_mapped_image())
        self.cpu.mem_map(BASE,0x200000)
        self.cpu.mem_map(0,4096) # SEH chain storage for compiler-generated noexcept wrapper.
        self.put(0,0xffffffff)
        self.exports = {s.name.decode().strip('@').split('@')[0]:
                        dll.OPTIONAL_HEADER.ImageBase+s.address
                        for s in dll.DIRECTORY_ENTRY_EXPORT.symbols}
        self.cpu.hook_add(UC_HOOK_CODE,self.hook)
        self.tick = 0; self.events = []; self.fragment = False
        self.put(0x8871E0,BASE+0x5000)
        self.put(BASE+0x5000+0x6F0,101,102)
        self.put(0x87E2A0,0) # Audio device absent; original controller stop still executes.
        self.put(0xA83D4C,BASE+0x6000)
        self.cpu.mem_write(BASE+0x7000,b'\xc7\x41\x10\x00\x00\x00\x00\xc3')
        self.put(BASE+0x7100+12,BASE+0x7000)

    def put(self,address,*values):
        self.cpu.mem_write(address,struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values)))

    def read(self,address,count=1):
        return struct.unpack('<'+'I'*count,self.cpu.mem_read(address,4*count))

    def ret(self,value=0,pop=0):
        sp = self.cpu.reg_read(UC_X86_REG_ESP)
        self.cpu.reg_write(UC_X86_REG_EIP,self.read(sp)[0])
        self.cpu.reg_write(UC_X86_REG_ESP,sp+4+pop)
        self.cpu.reg_write(UC_X86_REG_EAX,value)

    def hook(self,cpu,address,size,_):
        if self.fragment and address in (0x6532AA,0x65336D):
            cpu.emu_stop(); return
        if address==0x6C8C40:
            self.ret(self.tick)
        elif address==0x4068E0:
            self.ret() # Debug logging only.
        elif address==0x406060:
            self.events.append(['stop_audio',hex(cpu.reg_read(UC_X86_REG_ECX)-OBJ)])
        elif address==0x750920:
            sp=cpu.reg_read(UC_X86_REG_ESP)
            self.events.append(['play_audio',cpu.reg_read(UC_X86_REG_ECX),
                                cpu.reg_read(UC_X86_REG_EDX),*self.read(sp+4,2)])
            self.ret(0,8) # Device boundary: fastcall, two stack parameters.
        elif address==0x4A88C0:
            self.ret() # Display::Init_Clear is outside the Radar-specific write set.

    def reset(self,values,tick=0,observer=False,sound=False):
        self.cpu.mem_write(OBJ,bytes(0x150C))
        initial = dict(state=0,mode=0,pending_mode=4,frame=0,timer_start=0xffffffff,
                       timer_left=0,available=0,redraw=0,full_redraw=0,
                       flag_14BC=0,flag_14BD=0,cell_count=0,point_count=0)
        initial.update(values)
        for name,value in initial.items():
            offset,fmt = FIELDS[name]
            self.cpu.mem_write(OBJ+offset,struct.pack('<'+fmt,value & (255 if fmt=='B' else 0xffffffff)))
        for offset in (0x1224,0x125C):
            self.put(OBJ+offset,BASE+0x7100)
        self.tick=tick&0xffffffff; self.put(self.exports['RadarProbeTick'],self.tick)
        self.sound=bool(sound)
        self.cpu.mem_write(self.exports['RadarProbeSound'],bytes([self.sound]))
        self.put(0xA8B238,5 if observer else 0)
        self.cpu.mem_write(BASE+0x6000+501,bytes([bool(observer)]))
        self.put(0xA8B538,1 if observer else 0)
        self.events=[]
        self.put(0,0xffffffff)

    def snapshot(self):
        return {name:struct.unpack('<'+fmt,self.cpu.mem_read(OBJ+offset,struct.calcsize(fmt)))[0]
                for name,(offset,fmt) in FIELDS.items()}

    def call(self,operation,value=0,args=None):
        self.fragment = operation=='Advance' and not self.candidate
        if self.candidate:
            entry = self.exports['RadarProbe'+operation]
            arguments = args if args is not None else (() if operation in ('Advance','Clear','Foundation') else (value,))
        else:
            entry = {'Activate':0x656BE0,'Mode':0x656CB0,'Available':0x656DF0,
                     'Advance':0x6531DB,'Clear':0x652DE0,'Foundation':0x6563B0,'Refresh':0x6562D0,
                     'QueueNextMovie':0x652CD0,'Existing':0x656DE0,'Names':0x656E50,'Movie':0x656E70,
                     'Redraw':0x656E90,'Background':0x6551C0,'CellRect':0x655050,'CellPixel':0x6550C0}[operation]
            arguments = args if args is not None else (value,int(self.sound)) if operation in ('Activate','Mode') else (value,) if operation=='Available' else ()
        self.put(STACK,STOP,*arguments)
        self.cpu.reg_write(UC_X86_REG_ESP,STACK)
        self.cpu.reg_write(UC_X86_REG_ECX,OBJ)
        self.cpu.reg_write(UC_X86_REG_EDX,0)
        self.cpu.reg_write(UC_X86_REG_EAX,0)
        self.cpu.reg_write(UC_X86_REG_FPCW,0x0E7F)
        self.cpu.reg_write(UC_X86_REG_MXCSR,0x1F80)
        for i,reg in enumerate(REGS):self.cpu.reg_write(reg,0x12345000+i)
        if self.fragment:
            self.cpu.reg_write(UC_X86_REG_ESI,OBJ)
            self.cpu.reg_write(UC_X86_REG_EBP,1)
            self.cpu.reg_write(UC_X86_REG_EBX,0)
        self.cpu.emu_start(entry,STOP,count=20000000)
        end = self.cpu.reg_read(UC_X86_REG_EIP)
        if self.fragment:
            assert end in (0x6532AA,0x65336D),hex(end)
            assert self.cpu.reg_read(UC_X86_REG_ESP)==STACK
        else:
            assert end==STOP,hex(end)
            assert self.cpu.reg_read(UC_X86_REG_ESP)==STACK+4*(1+len(arguments))
            assert all(self.cpu.reg_read(reg)==0x12345000+i for i,reg in enumerate(REGS))
        self.fragment=False
        return self.cpu.reg_read(UC_X86_REG_EAX)


def base_state_cases():
    for state,value,flags in itertools.product(range(6),range(2),range(4)):
        yield 'Activate',dict(state=state,mode=1,frame=25,redraw=flags&1,full_redraw=flags>>1),value,0,False
    for state,mode,wanted,available,flags in itertools.product(range(6),range(5),range(5),range(2),range(4)):
        yield 'Mode',dict(state=state,mode=mode,available=available,frame=25,redraw=flags&1,full_redraw=flags>>1),wanted,0,False
    for state,mode,available,wanted,flags in itertools.product(range(6),range(5),range(2),range(2),range(4)):
        yield 'Available',dict(state=state,mode=mode,available=available,frame=25,redraw=flags&1,full_redraw=flags>>1),wanted,0,False
    for state,mode,wanted,available in itertools.product(range(6),range(5),range(5),range(2)):
        yield 'Mode',dict(state=state,mode=mode,available=available,frame=25),wanted,0,True
    timers = [(0xffffffff,0,0),(0xffffffff,4,0),(0,4,0),(0,4,3),(0,4,4),
              (0,4,5),(100,4,103),(100,4,104),(70002416,0,0),(70002416,0,600),
              (70002416,0,70002416),(0,0,0)]
    for state,frame,flags,(start,left,now) in itertools.product(range(6),(0,1,24,25,31,32),range(4),timers):
        yield 'Advance',dict(state=state,mode=1,frame=frame,timer_start=start,timer_left=left,
                             redraw=flags&1,full_redraw=flags>>1),0,now,False
    for state in range(6):
        yield 'Clear',dict(state=state,mode=3,frame=32,available=1,flag_14BC=1,
                           flag_14BD=1,cell_count=3,point_count=5),0,0,False


def state_cases():
    for case in base_state_cases():
        yield (*case,False)
        if case[0] in ('Activate','Mode'):
            yield (*case,True)


def audit_states(original,candidate):
    groups = {}; mismatches = []
    for op,initial,value,tick,observer,sound in state_cases():
        key = op+('_observer' if observer else '')+('_sound' if sound else '')
        group = groups.setdefault(key,dict(cases=0,full_state_mismatches=0,transition_mismatches=0,
                                          redraw_mismatches=0,timer_mismatches=0,queue_mismatches=0,audio_call_mismatches=0))
        group['cases']+=1
        for m in (original,candidate):m.reset(initial,tick,observer,sound);m.call(op,value)
        left,right=original.snapshot(),candidate.snapshot()
        different={name:{'original':left[name],'candidate':right[name]} for name in FIELDS if left[name]!=right[name]}
        group['full_state_mismatches']+=bool(different)
        group['transition_mismatches']+=bool(set(different)&TRANSITION)
        group['redraw_mismatches']+=bool(set(different)&{'redraw','full_redraw'})
        group['timer_mismatches']+=bool(set(different)&{'timer_start','timer_left'})
        group['queue_mismatches']+=bool(set(different)&{'cell_count','point_count'})
        audio=original.events!=candidate.events
        group['audio_call_mismatches']+=audio
        if different or audio:
            mismatches.append(dict(operation=key,initial=initial,value=value,tick=tick,sound=sound,fields=different,
                                   original_calls=original.events,candidate_calls=candidate.events))
    return groups,mismatches


def audit_geometry(machine,path,pixels_path):
    groups={};examples=[]
    for line in path.read_text().splitlines():
        kind,*v=line.split();v=list(map(int,v));machine.reset({})
        if kind=='S':
            width,height,x,y,scale=v
            ok=machine.call('Fit',args=(width,height,OUT,OUT+8))&255
            expected=(x,y,scale);actual=machine.read(OUT,3)
        else:
            bits,offset,origin,x,y,w,h,*rest=v
            machine.put(OBJ+0x1488,bits);machine.put(OBJ+0x1490,offset,0,origin,x,y,w,h)
            if kind=='P':
                wx,wy,wz,restrict,px,py=rest;machine.put(ARG,wx,wy,wz)
                ok=machine.call('Project',args=(OUT,ARG,restrict))==OUT
                expected=(px&0xffffffff,py&0xffffffff);actual=machine.read(OUT,2)
            elif kind=='I':
                px,py,cx,cy=rest;machine.put(ARG,px,py)
                ok=machine.call('Inverse',args=(ARG,OUT))&255
                expected=(cx,cy);actual=struct.unpack('<hh',machine.cpu.mem_read(OUT,4))
            elif kind=='F':
                cx,cy,vx,vy,*expected=rest
                machine.cpu.mem_write(ARG,struct.pack('<hh',cx,cy));machine.put(ARG+8,vx,vy)
                ok=machine.call('Frame',args=(ARG,ARG+8))&255
                expected=tuple(n&0xffffffff for n in expected);actual=machine.read(OBJ+0x14DC,4)
            else:raise AssertionError(kind)
        group=groups.setdefault(kind,dict(cases=0,mismatches=0));group['cases']+=1
        if not ok or actual!=expected:
            group['mismatches']+=1
            if len(examples)<20:examples.append(dict(kind=kind,input=v,expected=expected,actual=actual,accepted=bool(ok)))
    data=pixels_path.read_bytes();magic,n=struct.unpack_from('<4sI',data);assert magic==b'RDR1';pos=8
    group=groups['RGB565']=dict(cases=n,pixels=0,pixel_mismatches=0)
    for i in range(n):
        width,height,w,h=struct.unpack_from('<4I',data,pos);pos+=16
        source=data[pos:pos+width*height*3];pos+=len(source)
        expected=data[pos:pos+w*h*2];pos+=len(expected)
        machine.cpu.mem_write(RAW,source);machine.cpu.mem_write(PIXELS,bytes(w*h*2))
        ok=machine.call('Resample',args=(RAW,width*height,width,height,PIXELS,w*h))&255
        assert ok
        actual=bytes(machine.cpu.mem_read(PIXELS,w*h*2))
        group['pixels']+=w*h
        group['pixel_mismatches']+=sum(a!=b for a,b in zip(struct.unpack('<'+'H'*(w*h),expected),struct.unpack('<'+'H'*(w*h),actual)))
    assert pos==len(data)
    return groups,examples


def audit_vectors(original,candidate):
    foundations=dict(cases=0,mismatches=0)
    refresh=dict(cases=0,normal_mismatches=0,exhausted_mismatches=0)
    examples=[]
    for scale in (.01,.13,.3,.675,.9,1,1.8,3.5):
        snapshots=[]
        for m in (original,candidate):
            m.reset({})
            m.cpu.mem_write(OBJ+0x1488,struct.pack('<f',scale))
            for i in range(22):
                m.put(OBJ+0x1278+24*i,BASE+0x7100,BASE+0x20000+i*0x8000,4096,1,0,0)
            m.call('Foundation')
            state=[]
            for i in range(22):
                count=m.read(OBJ+0x1278+24*i+16)[0]
                assert count<=4096
                state.append((count,bytes(m.cpu.mem_read(BASE+0x20000+i*0x8000,count*8))))
            snapshots.append(state)
        for i,(left,right) in enumerate(zip(*snapshots)):
            foundations['cases']+=1
            if left!=right:
                foundations['mismatches']+=1
                examples.append(dict(operation='Foundation',scale=scale,foundation=i,
                                     original_count=left[0],candidate_count=right[0]))
    for width,height in ((1,1),(9,7),(140,108)):
        points=[(-1,0),(0,-1),(width,0),(0,height),(0,0),(width-1,height-1),(width//2,height//2)]
        for point,flags,exhausted in itertools.product(points,range(4),(False,True)):
            states=[]
            for m in (original,candidate):
                m.reset(dict(redraw=flags&1,full_redraw=flags>>1))
                surface,table=BASE+0x9000,BASE+0xA000
                m.put(surface,table,width,height)
                m.cpu.mem_write(BASE+0x7020,b'\x8b\x41\x04\xc3')
                m.cpu.mem_write(BASE+0x7040,b'\x8b\x41\x08\xc3')
                m.cpu.mem_write(BASE+0x7060,b'\x31\xc0\xc2\x08\x00')
                m.put(table+0x7C,BASE+0x7020,BASE+0x7040)
                m.put(BASE+0x7100+8,BASE+0x7060)
                m.put(OBJ+0x121C,surface)
                m.put(OBJ+0x149C,0,0,width,height)
                bits=(width*height+7)//8
                m.put(OBJ+0x1274,BASE+0x10000)
                m.cpu.mem_write(BASE+0x10000,bytes(bits))
                m.cpu.mem_write(BASE+0x20000,bytes(64*8))
                m.put(OBJ+0x125C,BASE+0x7100,BASE+0x20000,1 if exhausted else 64,1,1 if exhausted else 0,10)
                m.put(ARG,*point)
                m.call('Refresh',args=(ARG,))
                m.call('Refresh',args=(ARG,)) # Check repeated-point deduplication.
                count=m.read(OBJ+0x126C)[0]
                assert count<=64
                states.append(dict(count=count,points=bytes(m.cpu.mem_read(BASE+0x20000,count*8)).hex(),
                                   bits=bytes(m.cpu.mem_read(BASE+0x10000,bits)).hex(),
                                   redraw=m.snapshot()['redraw'],full_redraw=m.snapshot()['full_redraw']))
            refresh['cases']+=1
            if states[0]!=states[1]:
                refresh['exhausted_mismatches' if exhausted else 'normal_mismatches']+=1
                if len(examples)<8:
                    examples.append(dict(operation='Refresh',width=width,height=height,point=point,
                                         exhausted=exhausted,original=states[0],candidate=states[1]))
    return dict(foundations=foundations,refresh=refresh),examples


def audit_helpers(original,candidate):
    result={}
    def compare(operation,values,args=(),setup=None,read=None):
        states=[]
        for m in (original,candidate):
            m.reset(values)
            if setup: setup(m)
            returned=m.call(operation,args=args)
            states.append((m.snapshot(),read(m,returned) if read else None))
        row=result.setdefault(operation,dict(cases=0,mismatches=0))
        row['cases']+=1
        row['mismatches']+=states[0]!=states[1]
    for state,mode,available in itertools.product(range(6),range(5),range(2)):
        values=dict(state=state,mode=mode,available=available,frame=13)
        for operation in ('Existing','Names','Movie'):
            compare(operation,values,read=lambda m,r:r&255)
        compare('QueueNextMovie',values)
    for flags,complete in itertools.product(range(4),range(2)):
        compare('Redraw',dict(redraw=flags&1,full_redraw=flags>>1),args=(complete,))
    for x,y,width,scale in itertools.product((-3,-1,0,1,9,19,20,31),(-2,0,1,17),(10,20),(.3,1,1.8)):
        def setup(m):
            m.put(OBJ+0x104,width)
            m.put(OBJ+0x1490,0)
            m.put(OBJ+0x1498,8)
            m.put(OBJ+0x149C,16,49,140,108)
            m.cpu.mem_write(OBJ+0x1488,struct.pack('<f',scale))
            m.cpu.mem_write(ARG,struct.pack('<hh',x,y))
        for operation in ('CellRect','CellPixel'):
            compare(operation,{},args=(OUT,ARG),setup=setup,
                    read=lambda m,r:(r,bytes(m.cpu.mem_read(OUT,16)).hex()))
    for flags,duplicate,exhausted in itertools.product(range(4),range(2),range(2)):
        def setup(m):
            m.cpu.mem_write(BASE+0x7060,b'\x31\xc0\xc2\x08\x00')
            m.put(BASE+0x7100+8,BASE+0x7060)
            m.put(OBJ+0x1224,BASE+0x7100,BASE+0x20000,2 if exhausted else 16,1,2,10)
            m.cpu.mem_write(BASE+0x20000,struct.pack('<hhhh',4,5,6,7)+bytes(56))
            m.cpu.mem_write(ARG,struct.pack('<hh',4 if duplicate else 8,5 if duplicate else 9))
        compare('Background',dict(redraw=flags&1,full_redraw=flags>>1),args=(ARG,),setup=setup,
                read=lambda m,r:bytes(m.cpu.mem_read(BASE+0x20000,m.read(OBJ+0x1234)[0]*4)).hex())
    return result


def audit_cross_epoch(original,candidate):
    result=[]
    initial=dict(state=2,mode=1,frame=32,available=0,timer_start=70002416,timer_left=0)
    for m in (original,candidate):
        m.reset(initial)
        for tick in range(601):
            m.tick=tick;m.put(m.exports['RadarProbeTick'],tick);m.call('Advance')
        stalled=m.snapshot()
        m.put(OBJ+0x1500,600);m.put(OBJ+0x1508,0)
        for tick in range(600,729):
            m.tick=tick;m.put(m.exports['RadarProbeTick'],tick);m.call('Advance')
        result.append(dict(implementation='candidate' if m.candidate else 'original',
                           after_9600_ms=stalled,after_rebase_and_2048_ms=m.snapshot()))
    assert all(r['after_9600_ms']['state']==2 and r['after_9600_ms']['frame']==32 for r in result)
    assert all(r['after_rebase_and_2048_ms']['state']==0 and r['after_rebase_and_2048_ms']['frame']==0 for r in result)
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--compile-db',type=Path,required=True)
    parser.add_argument('--geometry',type=Path,required=True)
    parser.add_argument('--pixels',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();assert digest(args.exe)==SHA
    directory=args.output.resolve();directory.mkdir(parents=True,exist_ok=True)
    dll_path=build(args.compile_db,directory)
    assert dll_path.exists()
    exe,dll=pefile.PE(str(args.exe)),pefile.PE(str(dll_path))
    original,candidate=Machine(exe,dll,False),Machine(exe,dll,True)
    states,mismatches=audit_states(original,candidate)
    geometry,geometry_examples=audit_geometry(candidate,args.geometry,args.pixels)
    vectors,vector_examples=audit_vectors(original,candidate)
    helpers=audit_helpers(original,candidate)
    cross_epoch=audit_cross_epoch(original,candidate)
    geometry_failures=sum(g.get('mismatches',0)+g.get('pixel_mismatches',0) for g in geometry.values())
    sources=['code/core/src/yrpp/RadarClassState.cpp','code/core/src/yrpp/RadarClassGeometry.cpp',
             'code/core/src/yrpp/RadarClassRefresh.cpp',
             'code/core/include/yrpp/RadarClass.h','code/core/include/yrpp/Timer.h',
             'code/tests/x86/radar_instruction_probe.cpp','code/tests/x86/compare_radar_instructions.py']
    report=dict(scope=__doc__,exe_sha256=SHA,dll_sha256=digest(dll_path),
                geometry_fixture_sha256=digest(args.geometry),pixels_fixture_sha256=digest(args.pixels),
                source_sha256={p:digest(ROOT/p) for p in sources},states=states,geometry=geometry,vectors=vectors,helpers=helpers,
                all_compared_contracts_match=not mismatches and not geometry_failures
                    and not vectors['foundations']['mismatches'] and not vectors['refresh']['normal_mismatches']
                    and not vectors['refresh']['exhausted_mismatches'] and not any(r['mismatches'] for r in helpers.values()),
                geometry_examples=geometry_examples,vector_examples=vector_examples,cross_epoch=cross_epoch,
                controlled_boundaries=['SystemTimer tick source','debug logging','sound device Play callback',
                  'Display::Init_Clear parent call','fixed-capacity vector Clear callback',
                  'Surface width/height accessors','vector SetCapacity failure callback'],
                executed_original_audio='0x00406060 with audio device unavailable; stop calls are still recorded',
                abi_checks=['x86 size/field offsets compile-time asserted','stack cleanup and callee-saved registers checked for complete calls'],
                exclusions=['full Draw and device surfaces','object tracking/render/click paths','complete lifecycle/heap/COM',
                  'whole original Init_Clear parent chain','exception propagation','Timer CurrentTime empty-member padding',
                  'machine return registers for original void-like mutators',
                  'unused world dependencies linked as traps; reaching any trap fails the audit'])
    (directory/'comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    (directory/'mismatches.json').write_text(json.dumps(mismatches,indent=2)+'\n')
    print(json.dumps({'states':states,'geometry':geometry,'vectors':vectors,'helpers':helpers,
                     'cross_epoch_reproduced_by_both':True,
                     'all_compared_contracts_match':report['all_compared_contracts_match']},indent=2))
    return 0 if report['all_compared_contracts_match'] else 1


if __name__=='__main__':
    raise SystemExit(main())
