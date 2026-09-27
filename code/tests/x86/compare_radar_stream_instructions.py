#!/usr/bin/env python3
"""Compare complete Radar/Display/Layer stream entries with fixed original x86.

Compare emitted bytes, stream-call order, HRESULTs, partial object writes,
vector growth and deferred swizzle calls. COM transport, heap storage and
swizzle registration are controlled boundaries; native transport is tested
separately. Original code is the oracle, never generated candidate output.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from probe_build import build_probe

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from compare_radar_instructions import Machine, SHA, ROOT, BASE, OBJ, STACK, STOP, REGS, digest

LAYERS=0x8A0360
STREAM=BASE+0x3000
READ,WRITE=BASE+0x3100,BASE+0x3110
VTABLE=BASE+0x3200
HEAP=BASE+0x50000
SOURCES=['code/core/src/yrpp/RadarClassStream.cpp','code/core/src/yrpp/DisplayClassStream.cpp',
         'code/core/src/yrpp/LayerClassStream.cpp','code/core/src/yrpp/LayerClass.cpp',
         'code/tests/x86/radar_stream_instruction_probe.cpp']


def build(database,directory):
    return build_probe(database, directory, SOURCES, 'radar_stream_instruction_probe.dll',
        object_suffix='.obj',
        commands_file='stream-build-commands.json',
        require_x86=True)


def words(values):
    return struct.pack('<'+'I'*len(values),*(v&0xFFFFFFFF for v in values))


class StreamMachine(Machine):
    def __init__(self,exe,dll,candidate):
        self.cpu=Uc(UC_ARCH_X86,UC_MODE_32); self.candidate=candidate
        for pe in (exe,dll):
            self.cpu.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
            self.cpu.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
        self.cpu.mem_map(BASE,0x200000); self.cpu.mem_map(0,4096)
        self.exports={s.name.decode().strip('@').split('@')[0]:dll.OPTIONAL_HEADER.ImageBase+s.address
                      for s in dll.DIRECTORY_ENTRY_EXPORT.symbols}
        self.cpu.hook_add(UC_HOOK_CODE,self.hook)
        self.put(STREAM,VTABLE); self.put(VTABLE+12,READ,WRITE)
        self.put(BASE+0x3300+8,0x42F860)

    def invoke(self,entry,receiver,args):
        self.put(0,0xFFFFFFFF); self.put(STACK,STOP,*args)
        self.cpu.reg_write(UC_X86_REG_ESP,STACK); self.cpu.reg_write(UC_X86_REG_ECX,receiver)
        self.cpu.reg_write(UC_X86_REG_EDX,0)
        for i,reg in enumerate(REGS): self.cpu.reg_write(reg,0x12345000+i)
        self.cpu.emu_start(entry,STOP,count=2000000)
        assert self.cpu.reg_read(UC_X86_REG_EIP)==STOP
        assert self.cpu.reg_read(UC_X86_REG_ESP)==STACK+4*(1+len(args))
        assert all(self.cpu.reg_read(reg)==0x12345000+i for i,reg in enumerate(REGS))
        return self.cpu.reg_read(UC_X86_REG_EAX)

    def reset(self,case):
        self.case=case; self.calls=[]; self.output=bytearray(); self.position=0
        self.allocations=[]; self.frees=[]; self.swizzles=[]; self.heap=HEAP
        self.resizers={0x42F860,0x5D09C0}; self.growth=0
        self.cpu.mem_write(OBJ,bytes([0xAB])*0x150C)
        self.put(OBJ,0x7F0344)
        for i,(offset,size) in enumerate([(0x1224,4),(0x125C,8)]+[(0x1278+i*24,8) for i in range(22)]):
            data=BASE+0x10000+i*0x100
            self.cpu.mem_write(data,bytes([0xCE])*0x100)
            self.put(OBJ+offset,BASE+0x3300,data,3,1,2,57)
        for offset,value in ((0x14AC,91),(0x14B0,92),(0x14B4,93),(0x14B8,94),(0x14FC,25)):
            self.put(OBJ+offset,value)
        self.put(OBJ+0x14C0,0,123,0,0x87E294,0x11223344)
        self.put(OBJ+0x1500,0x12345678,0xABABABAB,77)
        points=case.get('points',[]); cells=case.get('cells',[])
        if case['operation'].endswith('Save'):
            for offset,values in ((0x125C,points),(0x1224,cells)):
                pointer=self.read(OBJ+offset+4)[0]
                flat=[v for point in values for v in point] if offset==0x125C else values
                self.cpu.mem_write(pointer,words(flat)); self.put(OBJ+offset+8,max(len(values),3))
                self.put(OBJ+offset+16,len(values))
        for i in range(5):
            target=LAYERS+i*24
            if self.candidate: self.invoke(self.exports['LayerStreamConstruct'],target,())
            else: self.put(target,BASE+0x3300,0,0,1,0,10)
            initial=case.get('initial_layers',[[],[],[],[],[]])[i]
            if initial:
                pointer=BASE+0x20000+i*0x100
                self.cpu.mem_write(pointer,words(initial+[0]*32))
                self.put(target+4,pointer,len(initial))
                self.cpu.mem_write(target+12,b'\x01\x00')
                self.put(target+16,len(initial))
            self.resizers.add(self.read(self.read(target)[0]+8)[0])

    def hook(self,cpu,address,size,_):
        sp=cpu.reg_read(UC_X86_REG_ESP)
        if address in (READ,WRITE):
            stream,data,count,done=self.read(sp+4,4)
            assert stream==STREAM and done==0
            for offset in (0x1224,0x125C):
                self.resizers.add(self.read(self.read(OBJ+offset)[0]+8)[0])
            number=len(self.calls)+1
            failed=number==self.case.get('fail_call',-1)
            transferred=count
            if number>=self.case.get('short_from',0xFFFFFFFF) or number in self.case.get('short_calls',[]):
                transferred=min(count,2)
            hr=self.case.get('success_hr',0)
            if failed:
                transferred=0; hr=0x80001234
            elif address==READ:
                incoming=bytes.fromhex(self.case.get('input',''))
                assert self.position+transferred<=len(incoming),(self.case,self.position,count)
                cpu.mem_write(data,incoming[self.position:self.position+transferred]); self.position+=transferred
            else:
                self.output.extend(cpu.mem_read(data,transferred))
            self.calls.append(['read' if address==READ else 'write',count,hr])
            self.ret(hr,16)
        elif address in (0x7C8E17,self.exports['RadarStreamAllocate']):
            count=self.read(sp+4)[0]; assert count<0x10000,count
            output=self.heap; self.heap+=(count+15)&~15
            assert self.heap<STACK
            cpu.mem_write(output,bytes([0xCD])*count)
            self.allocations.append(count); self.ret(output)
        elif address in (0x7C8B3D,self.exports['RadarStreamFree']):
            self.frees.append(self.read(sp+4)[0]); self.ret()
        elif address==0x6CF240:
            receiver,slot=self.read(sp+4,2); assert receiver==0xB0C110
            token=self.read(slot)[0]
            self.swizzles.append([hex(slot),token])
            if not self.case.get('swizzle_error'): self.put(slot,0)
            self.ret(0x80004005 if self.case.get('swizzle_error') else 0,8)
        elif address in self.resizers:
            self.growth+=1
            if self.growth==self.case.get('fail_growth'):
                self.ret(0,8)

    def run(self,case):
        self.reset(case)
        operation=case['operation']; radar=operation.startswith('Radar')
        entry=self.exports[operation] if self.candidate else {
            'RadarStreamLoad':0x6568A0,'RadarStreamSave':0x656AC0,
            'LayerStreamLoad':0x551B90,'LayerStreamSave':0x551B20}[operation]
        hr=self.invoke(entry,OBJ if radar else LAYERS,(0 if case.get('null') else STREAM,))
        data=bytearray(self.cpu.mem_read(OBJ,0x150C))
        vectors=[]
        for offset,size in [(0x1224,4),(0x125C,8)]+[(0x1278+i*24,8) for i in range(22)]:
            vptr,pointer,capacity=self.read(OBJ+offset,3)
            count=self.read(OBJ+offset+16)[0]; assert count<1000
            data[offset:offset+4]=bytes(4) # Runtime-local vtables are not file data.
            data[offset+14:offset+16]=bytes(2) # C++ alignment padding.
            vectors.append(bytes(self.cpu.mem_read(pointer,count*size)).hex() if count else '')
        layers=[]
        for i in range(5):
            address=LAYERS+i*24
            _,pointer,capacity,flags,count,growth=self.read(address,6)
            layers.append([capacity,flags&0xFFFF,count,growth,
                           list(self.read(pointer,count)) if count else []])
        return dict(hr=hex(hr),calls=self.calls,output=self.output.hex(),position=self.position,
                    object=data.hex(),vectors=vectors,layers=layers,swizzles=self.swizzles,
                    allocations=self.allocations,frees=self.frees,growth=self.growth)


def cases():
    yield dict(operation='RadarStreamLoad',null=True)
    yield dict(operation='RadarStreamSave',null=True)
    yield dict(operation='LayerStreamLoad',null=True)
    yield dict(operation='LayerStreamSave',null=True)
    for count in (0,1,2,10,11,23):
        points=[(i*17,-i*31) for i in range(count)]
        cells=[((i+7)<<16)|(i+2) for i in range(count//2)]
        layers=[[0x12340000,0,0x12340000],[],[0x81234567],[],[]] if count else [[],[],[],[],[]]
        record=[]
        for layer in layers: record += [len(layer),*layer]
        record += [len(points),*[v for point in points for v in point],len(cells),*cells,3,1,4,2]
        calls=5+sum(map(len,layers))+2+len(points)+len(cells)+4
        # Error after each transfer boundary; parent Load continues on errors.
        # Repeated zero count records make every parent error deterministic.
        for success_hr in (0,1,7):
            yield dict(operation='RadarStreamLoad',input=words(record).hex(),success_hr=success_hr)
            yield dict(operation='RadarStreamSave',points=points,cells=cells,initial_layers=layers,success_hr=success_hr)
        for failed in range(6+sum(map(len,layers)),calls+1):
            yield dict(operation='RadarStreamLoad',input=words(record).hex(),fail_call=failed)
        for failed in range(1,calls+1):
            yield dict(operation='RadarStreamSave',points=points,cells=cells,initial_layers=layers,fail_call=failed)
        for fail_growth in (1,2,3):
            # Radar queues may drop an entry on failed growth. Empty display
            # layers keep the swizzle precondition valid in this fixture.
            tail=[0]*5+[len(points),*[v for point in points for v in point],len(cells),*cells,3,1,4,2]
            yield dict(operation='RadarStreamLoad',input=words(tail).hex(),fail_growth=fail_growth)
    for failed in range(1,6):
        yield dict(operation='RadarStreamLoad',input=words([0]*40).hex(),fail_call=failed)
    for count in (-1,-2147483648,0,1,2,11):
        entries=[0x12340000+i*4 for i in range(max(count,0))]
        for initial in ([],[0x11223344]):
            # Borrowed initial storage cannot grow; only test cases whose
            # first count swizzle destinations are valid storage.
            if initial and count>1: continue
            layers=[initial,[],[],[],[]]
            for error in (False,True):
                yield dict(operation='LayerStreamLoad',input=words([count,*entries]).hex(),
                           initial_layers=layers,swizzle_error=error)
            for failed in range(1,max(count,0)+2):
                yield dict(operation='LayerStreamLoad',input=words([count,*entries]).hex(),
                           initial_layers=layers,fail_call=failed)
        yield dict(operation='LayerStreamSave',initial_layers=[entries,[],[],[],[]])
    yield dict(operation='RadarStreamSave',short_from=1,success_hr=1)
    yield dict(operation='RadarStreamLoad',input=(words([0]*7)+bytes(range(1,9))).hex(),short_from=8,success_hr=1)
    # Reused temporary buffers are defined after the first complete element.
    prefix=words([0]*5+[2,0x11223344,0x55667788])+b'\xAA\xBB'
    yield dict(operation='RadarStreamLoad',input=(prefix+words([0,3,1,4,2])).hex(),short_calls=[8],success_hr=1)
    record=words([0]*5+[0,2,0x11223344])+b'\xAA\xBB'+words([3,1,4,2])
    yield dict(operation='RadarStreamLoad',input=record.hex(),short_calls=[9],success_hr=1)
    yield dict(operation='LayerStreamLoad',input=(words([2,0x11223344])+b'\xAA\xBB').hex(),short_calls=[3],success_hr=1)
    for point_count,cell_count in ((-1,0),(0,-1),(-2147483648,-2147483648)):
        yield dict(operation='RadarStreamLoad',input=words([0]*5+[point_count,cell_count,3,1,4,2]).hex())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--compile-db',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(); assert digest(args.exe)==SHA
    directory=args.output.resolve(); directory.mkdir(parents=True,exist_ok=True)
    path=build(args.compile_db,directory)
    exe,dll=pefile.PE(str(args.exe)),pefile.PE(str(path))
    original,candidate=StreamMachine(exe,dll,False),StreamMachine(exe,dll,True)
    groups={}; differences=[]
    for index,case in enumerate(cases()):
        try: left,right=original.run(case),candidate.run(case)
        except Exception:
            print(json.dumps(dict(index=index,case=case))); raise
        group=groups.setdefault(case['operation'],dict(cases=0,mismatches=0)); group['cases']+=1
        changed={key:dict(original=left[key],candidate=right[key]) for key in left if left[key]!=right[key]}
        if changed:
            group['mismatches']+=1; differences.append(dict(index=index,case=case,differences=changed))
    tracked=SOURCES+['code/core/include/yrpp/RadarClass.h','code/core/include/yrpp/MapClass.h',
                     'code/core/include/yrpp/Audio.h','code/tests/x86/compare_radar_stream_instructions.py']
    report=dict(scope=__doc__,exe_sha256=SHA,dll_sha256=digest(path),groups=groups,
                source_sha256={p:digest(ROOT/p) for p in tracked},all_compared_contracts_match=not differences,
                controlled_boundaries=['IStream Read/Write with null byte-count pointer','heap allocate/free',
                    'SwizzleManager registration','optional failed vector capacity call',
                    'native pointer identities mapped to original 32-bit tokens by test transport'],
                abi_checks=['x86 class/field sizes','thiscall/fastcall receiver and stack cleanup',
                            'callee-saved registers','all five Display layers execute original/compiled bodies'],
                exclusions=['C++ exceptions','undefined/truncated uninitialized local reads',
                            'out-of-bounds swizzle after layer allocation failure',
                            'unused capacity bytes and vptr identity','whole-game save orchestration'])
    (directory/'stream-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    (directory/'stream-mismatches.json').write_text(json.dumps(differences,indent=2)+'\n')
    print(json.dumps(dict(groups=groups,all_compared_contracts_match=not differences),indent=2))
    return int(bool(differences))


if __name__=='__main__': raise SystemExit(main())
