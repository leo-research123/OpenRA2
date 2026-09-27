#!/usr/bin/env python3
"""Full 0x004AB9B0 release body, including real typed Event constructors/ring writes.

Original-layout objects and explicit shared callee/virtual boundaries compare
ordered arguments and caller state. Callee bodies (especially Vox, beacon
placement and planning execution) are NOT proved by this caller test. Dynamic
cast uses shared outcomes after checking the real source/destination descriptor
names. Native initializes unused event stack bytes; only those original undefined
bytes and scratch/buffer addresses are normalized. This is not full Radar input
integration or original-process heap/callback acceptance.
"""
import argparse,hashlib,itertools,json,re,struct
from pathlib import Path
from probe_build import build_probe
import pefile
from compare_radar_commands import Machine as BaseMachine,BASE,MAP,STACK,STOP,SHA
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[3]
SOURCES=['code/core/src/yrpp/DisplayClassLeftRelease.cpp','code/core/src/yrpp/EventClassConstruction.cpp','code/tests/x86/display_left_release_probe.cpp']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(db,out):
    return build_probe(db, out, SOURCES, 'display_left_release_probe.dll',
        object_suffix='-left-release.obj',
        symbols=True)
class Machine(BaseMachine):
    def __init__(self,*args):
        super().__init__(*args);self.special={};self.callback=0
        for line in args[2].read_text().splitlines():
            f=line.split()
            if len(f)>=3:
                if f[1].startswith('?Play@VoxClass@@'):self.special[int(f[2],16)]=0x752700
                if f[1].startswith('?PlaceBeacon@BeaconManagerClass@@'):self.special[int(f[2],16)]=0x430BA0
                if f[1].startswith('?BandboxSelectionCallback@DisplayClass@@'):self.callback=int(f[2],16)
    def cstring(self,p):return bytes(self.c.mem_read(p,120)).split(b'\0')[0].decode('ascii')
    def hook(self,c,pc,size,data):
        pc=self.special.get(pc,pc);sp=c.reg_read(UC_X86_REG_ESP);this=c.reg_read(UC_X86_REG_ECX);edx=c.reg_read(UC_X86_REG_EDX)
        if pc==0x4A8EB0:
            t,h,f,at=self.read(sp+4,4);self.trace.append(['proximity',this,t,h,f,*self.cell(at)])
            self.ret(self.cfg['prox'],16)
        elif pc==0x452670:
            t,h=self.read(sp+4,2);self.trace.append(['upgrade',this,t,h]);self.ret(self.cfg['upgrade'],8)
        elif pc==0x752700:
            self.trace.append(['vox',self.cstring(this),edx,self.read(sp+4)[0]]);self.ret(pop=4)
        elif pc==0x7CAAE4:
            obj,offset,src,dst,reference=self.read(sp+4,5)
            assert self.cstring(src+8)=='.?AVObjectTypeClass@@'
            assert self.cstring(dst+8)=='.?AVTechnoTypeClass@@'
            self.trace.append(['dynamic_cast',obj,offset,reference]);self.ret(obj if self.cfg['cast'] else 0)
        elif pc==BASE+0x30100:
            self.trace.append(['time']);self.ret(self.cfg['time'])
        elif pc==0x734270:
            self.trace.append(['sidebar',this]);self.ret()
        elif pc in (0x4A8BF0,0x4A8D50):
            ptr=self.read(sp+4)[0];self.trace.append(['foundation' if pc==0x4A8BF0 else 'copy_foundation',this,ptr])
            self.put(this+(0x117C if pc==0x4A8BF0 else 0x118C),ptr);self.ret(pop=4)
        elif pc==0x54F5C0:
            self.trace.append(['key',self.read(sp+4)[0]]);self.ret(self.cfg['shift'],4)
        elif pc==0x6DA080:
            self.trace.append(['band_objects',this]);self.ret(self.cfg['band_objects'])
        elif pc==0x48DC90:
            self.trace.append(['unselect']);self.put(0xA8ECC8,0);self.byte(BASE+0x1083,0);self.ret()
        elif pc==0x6D9FF0:
            callback=self.read(sp+4)[0];assert callback==(self.callback if self.candidate else 0x4AC2B0)
            self.trace.append(['band_select',this]);self.ret(pop=4)
        elif pc==0x70D150:self.trace.append(['action_line']);self.ret()
        elif pc==0x50B6F0:self.trace.append(['controlled',this]);self.ret(self.cfg['controlled'])
        elif pc==0x732D00:self.trace.append(['type_selecting']);self.ret(self.cfg['type_select'])
        elif pc in (0x732600,0x7327D0):
            self.trace.append(['type_deselect' if pc==0x732600 else 'type_select',self.cstring(this)]);self.ret()
        elif pc==0x5657A0:
            self.trace.append(['cell',*self.cell(self.read(sp+4)[0])]);self.ret(BASE+0x50000,4)
        elif pc==0x430F70:self.trace.append(['select_beacon',*self.read(sp+4,3)]);self.ret(1,12)
        elif pc in (0x639040,0x639130):
            name='plan_selection' if pc==0x639040 else 'plan_capacity';self.trace.append([name]);self.ret(self.cfg[name])
        elif pc==0x4AE750:
            self.trace.append(['active',this,*self.read(sp+4,3)]);self.ret(pop=12)
        elif pc==0x6E6AB0:
            self.trace.append(['target',self.read(sp+4)[0]]);self.put(this,0x12345678);self.byte(this+4,self.cfg['rtti']);self.ret(this,4)
        elif pc==0x430BA0:self.trace.append(['place_beacon',*self.read(sp+4,5)]);self.ret(pop=20)
        elif pc==0x4AC960:self.trace.append(['beacon_mode',this,self.read(sp+4)[0]]);self.ret(pop=4)
        elif pc==0x6CEEB0:self.trace.append(['super',this]);self.ret(BASE+0x70000 if self.cfg['super'] else 0)
        elif BASE+0x30000<=pc<BASE+0x30100:
            which=pc-BASE-0x30000
            if which==0:
                self.trace.append(['visual',this,self.read(sp+4)[0]&0xffff,self.read(sp+8)[0]]);self.ret(self.cfg['visual'],8)
            elif which==0x10:
                self.trace.append(['rtti',this]);self.ret(self.cfg['type_rtti'] if this==BASE+0x8000 else self.cfg['rtti'])
            elif which==0x20:
                self.trace.append(['type_index',this])
                if self.cfg['mutate_pending']:self.put(self.display+0x11A4,BASE+0x4000)
                self.ret(42)
            elif which==0x30:self.trace.append(['foundation_data',this,self.read(sp+4)[0]&255]);self.ret(BASE+0x7800,4)
            elif which==0x40:self.trace.append(['cursor',this,*self.read(sp+4,2)]);self.ret(1,8)
            elif which==0x50:self.trace.append(['owner',this]);self.ret(BASE+0x10000)
            elif which==0x60:self.trace.append(['type',this]);self.ret(BASE+0x8000)
            elif which==0x70:self.trace.append(['select',this]);self.byte(this+0x83,1);self.ret(1)
            elif which==0x80:self.trace.append(['deselect',this]);self.byte(this+0x83,0);self.ret()
            elif which==0x90:self.trace.append(['selectable',this]);self.ret(self.cfg['selectable'])
            elif which==0xA0:self.trace.append(['sensor',this]);self.ret(self.cfg['sensor'])
            else:raise AssertionError(hex(pc))
    def run(self,cfg):
        self.reset('release');self.cfg=dict(action=1,obj=1,rtti=6,type_rtti=7,prox=1,upgrade=0,cast=1,naval=0,pending=0,shroud=1,
            band=0,shift=0,band_objects=1,controlled=1,type_select=0,selected=0,count=1,selectable=1,sensor=0,translucency=0,power=1,
            plan_selection=1,plan_capacity=1,super=0,mute=0,debug=0,scenario=1,active=1,visual=0,mini=0,cell=(34,56),offset=(2,-1),
            local=0,queue=0,tail=0,time=0xFFFFFFF7,house=3,mutate_pending=0,level=5)
        self.cfg.update(cfg);q=self.cfg
        d=BASE+0xB000 if q['local'] else MAP;self.display=d
        self.c.mem_write(MAP,bytes(0x1200));self.c.mem_write(d,bytes(0x1200))
        self.c.mem_write(BASE+0x20000,bytes(0xE00));self.put(0x887324,BASE+0x20000);self.put(0x87F770,BASE+0x21000)
        self.put(0xA83D4C,BASE+0x10000);self.put(BASE+0x10030,q['house'])
        self.put(0xA8B538,q['mute']);self.put(0xA8ED84,187);self.byte(0xA8ED5C,q['scenario']);self.byte(0xA8E9A0,q['active'])
        self.byte(0xA8ED6B,q['debug']);self.byte(0xA8ED9D,0);self.byte(0xB0FE58,1);self.put(0x8A0740,104)
        self.put(0xA8ECBC,BASE+0x500);self.put(0xA8ECC8,q['count']);self.put(BASE+0x500,BASE+0x1000)
        self.put(BASE+0x1000,BASE+0x22000);self.put(BASE+0x4000,BASE+0x22000);self.put(BASE+0x8000,BASE+0x22000)
        self.put(BASE+0x121C,BASE+0x10000);self.byte(BASE+0x1083,q['selected']);self.byte(BASE+0x16ED,q['translucency']);self.byte(BASE+0x1660,q['power'])
        self.byte(BASE+0x8CCE,q['naval']);self.c.mem_write(BASE+0x8024,b'RELEASE_TYPE\0');self.byte(BASE+0x5011B,q['level'])
        for slot,off in ((0x68,0),(0x2C,0x10),(0x40,0x20),(0x90,0x30),(0x48,0x40),(0x3C,0x50),(0x88,0x60),(0x14C,0x70),(0x150,0x80),(0x138,0x90),(0x328,0xA0)):
            self.put(BASE+0x22000+slot,BASE+0x30000+off)
        self.put(d,BASE+0x22000);self.put(d+0x11A4,BASE+0x1000 if q['pending'] else 0);self.put(d+0x11A8,BASE+0x8000)
        self.put(d+0x11AC,7);self.put(d+0x117C,BASE+0x7600);self.c.mem_write(d+0x1178,struct.pack('<hh',*q['offset']))
        self.byte(d+0x1180,q['prox']);self.byte(d+0x1181,q['shroud']);self.byte(d+0x11CF,q['band']);self.byte(d+0x11D0,1)
        self.c.mem_write(BASE+0x600,struct.pack('<hh',*q['cell']));self.put(BASE+0x70098,19)
        self.c.mem_write(0xA802C8,b'\xA5'*(12+128*115));self.put(0xA802C8,q['queue'],17,q['tail']);self.put(0x7E1530,BASE+0x30100)
        self.call('LeftRelease',0x4AB9B0,d,[BASE+0x700,BASE+0x600,BASE+0x1000 if q['obj'] else 0,q['action'],q['mini']])
        after_count,head,tail=self.read(0xA802C8,3);assert head==17
        added=after_count-q['queue'];events=[]
        for i in range(added):
            slot=(q['tail']+i)%128;ptr=0xA802D4+slot*111;raw=bytearray(self.c.mem_read(ptr,111));kind=raw[0]
            payload={0:0,1:5,2:5,11:16,18:8,21:5,22:5,23:4}[kind]
            events.append([raw[0],raw[2:7].hex(),raw[7:7+payload].hex(),self.read(0xA83A54+slot*4)[0]])
        # A full queue must be wholly unchanged, not just Count/Tail.
        untouched=[]
        slots={(q['tail']+i)%128 for i in range(added)}
        for i in range(128):
            if i not in slots:
                assert bytes(self.c.mem_read(0xA802D4+i*111,111))==b'\xA5'*111
                assert self.read(0xA83A54+i*4)[0]==0xA5A5A5A5
        return dict(trace=self.trace,display=bytes(self.c.mem_read(d,0x1200)).hex(),global_display=bytes(self.c.mem_read(MAP,0x1200)).hex(),
                    band_redraw=cbyte(self.c,BASE+0x20D7D),aborted=cbyte(self.c,0xA8ED9D),attack=cbyte(self.c,0xB0FE58),
                    selected=cbyte(self.c,BASE+0x1083),count=self.read(0xA8ECC8)[0],queue=[after_count,head,tail],events=events)
def cbyte(c,p):return c.mem_read(p,1)[0]
def main():
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--compile-db',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert digest(a.exe)==SHA;a.output.mkdir(parents=True,exist_ok=True);dll,symbols=build(a.compile_db,a.output)
    exe,image=pefile.PE(str(a.exe)),pefile.PE(str(dll));old,new=Machine(exe,image,symbols,False),Machine(exe,image,symbols,True)
    cases={}
    def group(name,keys,values,base={}):cases[name]=[dict(base,**dict(zip(keys,v))) for v in itertools.product(*values)]
    group('actions',['action','obj','plan_selection','plan_capacity','super'],[range(73),(0,1),(0,1),(0,1),(0,1)])
    cases['actions']=[x for x in cases['actions'] if x['obj'] or x['action'] not in (10,33)]
    group('selection',['action','rtti','controlled','selected','selectable','type_select','translucency','sensor'],[(0,7,8),(2,6),(0,1),(0,1),(0,1),(0,1),(0,15),(0,1)])
    group('band',['band_objects','shift','action','mini'],[(0,1),(0,1),(0,1,7,8),(0,1)],{'band':1})
    group('hidden',['scenario','active','debug','visual','action'],[(0,1),(0,1),(0,1),(0,5),(0,1,7,8)])
    group('placement',['prox','shroud','upgrade','cast','naval','debug','local','type_rtti'],[(0,1),(0,1),(0,1),(0,1),(0,1),(0,1),(0,1),(7,41)],{'pending':1})
    group('queue',['action','queue','tail','house','super'],[(12,13,10,33,60),(0,127,128,129),(0,127),(-1,0,127,128,255),(0,1)],{'mute':1,'rtti':2})
    group('coordinates',['cell','offset','level','action'],[((-32768,32767),(0,0),(32767,-32768)),((2,-1),(32767,-32768)),(-128,-1,0,127),(60,61)],{'obj':0})
    group('placement_mutation',['local','mutate_pending','cell','rtti','type_rtti'],[(0,1),(0,1),((-32768,32767),(32767,-32768)),(2,6),(7,41)],{'pending':1,'queue':127,'tail':127})
    group('target_events',['action','rtti','power'],[(10,12,13,33),(-1,0,1,2,6,41),(0,1)])
    group('local_input',['action','count','mini'],[range(73),(0,1),(0,1)],{'local':1})
    group('placement_queue',['queue','tail','obj','debug','time'],[(0,127,128,129),(0,127),(0,1),(0,1),(0,0xFFFFFFFF)],{'pending':1})
    differences=[]
    for group_,inputs in cases.items():
        for cfg in inputs:
            try:expected=old.run(cfg);actual=new.run(cfg)
            except Exception:
                (a.output/'left-release-partial.json').write_text(json.dumps(differences,indent=2)+'\n')
                print('FAILED INPUT',group_,cfg,'OLD TRACE',old.trace,'NEW TRACE',new.trace,flush=True);raise
            if expected!=actual:
                differences.append({'group':group_,'input':cfg,'original':{k:v for k,v in expected.items() if v!=actual[k]},'candidate':{k:v for k,v in actual.items() if v!=expected[k]}})
        print(group_,len(inputs),'cumulative mismatches',len(differences),flush=True)
    report={'exe_sha256':SHA,'cases':{k:len(v) for k,v in cases.items()},'mismatches':len(differences),'differences':differences,'boundary':__doc__,'sources':{s:digest(ROOT/s) for s in SOURCES}}
    (a.output/'left-release-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    if differences:print(json.dumps(differences[:2],indent=2))
    return bool(differences)
if __name__=='__main__':raise SystemExit(main())
