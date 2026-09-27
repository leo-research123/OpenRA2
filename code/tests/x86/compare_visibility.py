#!/usr/bin/env python3
"""Compare production x86 visibility/rectangle bodies with the fixed EXE.

Visibility virtual queries, projection and final drawing use identical boundary
doubles. Compares ordered calls, object bytes, caller clip, return, stack and
nonvolatile registers. The replacement cannot enter the original four bodies.
"""
import argparse, hashlib, itertools, json, random, struct
from pathlib import Path
import pefile
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX
from compare_image_resources import Resources as ResourceMachine, SHA

BASE=0x53000000
OBJ,VT,RADIO,RVT,LINKS,TACTICAL,CLIP,A,B,OUT,DELTAS=[BASE+x for x in (0,0x2000,0x3000,0x5000,0x6000,0x7000,0x9000,0x9100,0x9200,0x9300,0x9400)]
ENTRIES={'object':(0x5F4B10,'ObjectVisible',0x5F4D06),'building':(0x43CEA0,'BuildingVisible',0x43D017),'unit':(0x73B0B0,'UnitVisible',0x73B13C),'rect':(0x421B60,'RectangleIntersection',0x421CC0)}
DEFAULT=dict(kind='object',window=1,debug=0,forced=1,redraw=1,limbo=0,onmap=1,fog=0,extra=0,
             visible=1,rtti=15,tether=0,radio_rtti=6,mission=0,queued=0,door=0,
             point=(91,73),clip=(0,0,320,200),tactical=(20,30,280,160),dimensions=(-50,-50,400,300))

class Machine(ResourceMachine):
    def __init__(self,exe,dll=None):
        super().__init__(exe,dll,{},False)
        self.uc.mem_map(BASE,0x20000);self.events=[]
        self.exports={name.lstrip('@_').split('@')[0]:address for name,address in self.exports.items()}
        self.replacement=dll is not None
        self.uc.hook_add(UC_HOOK_CODE,self.guard)
        for table in (VT,RVT):
            for slot in (0x2C,0xAC,0x12C,0x114,0x4E4,0x184):
                address=table+0x800+slot;self.uc.mem_write(address,b'\xC3');self.write32(table+slot,address)
                self.hooks[address]=lambda slot=slot:self.virtual(slot)
        self.hooks[0x6D2140]=self.project
        self.write32(0x887324,TACTICAL)
    def guard(self,uc,address,size,user):
        if self.replacement:
            assert not any(a<=address<end for a,_,end in ENTRIES.values()),('entered original',hex(address))
    def rectangle(self,p,r):self.uc.mem_write(p,struct.pack('<4i',*r))
    def ints(self,p,n):return list(struct.unpack('<'+'i'*n,self.uc.mem_read(p,4*n)))
    def byte(self,p,v):self.uc.mem_write(p,bytes([v]))
    def virtual(self,slot):
        radio=self.uc.reg_read(UC_X86_REG_ECX)==RADIO
        self.events.append(['radio' if radio else 'object',hex(slot)])
        if slot==0x2C:self.ret(self.c['radio_rtti'] if radio else self.c['rtti'])
        elif slot==0x184:self.ret(self.c['mission'])
        elif slot==0xAC:
            p=self.arg(0);self.uc.mem_write(p,struct.pack('<3i',2048,1792,104));self.ret(p,4)
        elif slot==0x12C:
            p=self.arg(0);self.rectangle(p,self.c['dimensions']);self.ret(p,4)
        else:
            self.events.append(['draw',hex(slot),self.ints(self.arg(0),2),self.ints(self.arg(1),4)])
            self.ret(0,8)
    def project(self):
        self.events.append(['project',self.ints(self.arg(0),3)])
        self.uc.mem_write(self.arg(1),struct.pack('<2i',*self.c['point']));self.ret(self.c['visible'],8)
    def visibility(self,c):
        self.c=c;self.events=[]
        self.uc.mem_write(OBJ,bytes(0x2000));self.uc.mem_write(RADIO,bytes(0x2000))
        self.write32(OBJ,VT);self.write32(RADIO,RVT);self.write32(OBJ+0xE4,LINKS);self.write32(LINKS,RADIO)
        for offset,key in ((0x74,'onmap'),(0x80,'redraw'),(0x81,'limbo'),(0x6E7,'fog'),(0x418,'tether')):self.byte(OBJ+offset,c[key])
        self.write32(RADIO+0xB4,c['queued']);self.byte(RADIO+0x368,c['door']&1);self.byte(RADIO+0x369,c['door']>>1)
        self.write32(0xB73550,c['window']);self.byte(0xA8ED6B,c['debug'])
        self.rectangle(CLIP,c['clip']);self.rectangle(0x886FA0,c['tactical'])
        entry,name,_=ENTRIES[c['kind']]
        result=self.call(self.exports[name] if self.replacement else entry,OBJ,args=(CLIP,c['forced'],c['extra']),kind='bool')
        return dict(result=result,clip=self.ints(CLIP,4),events=self.events,state=bytes(self.uc.mem_read(OBJ,0x2000)).hex())
    def intersect(self,a,b,alias,delta):
        self.rectangle(A,a);self.rectangle(B,b);self.rectangle(OUT,(0,0,0,0));self.uc.mem_write(DELTAS,struct.pack('<2i',17,-23))
        out=(OUT,A,B)[alias];entry,name,_=ENTRIES['rect']
        result=self.call(self.exports[name] if self.replacement else entry,out,A,args=(B,DELTAS if delta&1 else 0,DELTAS+4 if delta&2 else 0),kind='value')
        return dict(result=result,output=self.ints(out,4),delta=self.ints(DELTAS,2))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('exe','dll','report'):p.add_argument('--'+key,type=Path,required=True)
    args=p.parse_args();assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==SHA
    exe=pefile.PE(str(args.exe));dll=pefile.PE(str(args.dll))
    if not hasattr(dll,'DIRECTORY_ENTRY_IMPORT'):dll.DIRECTORY_ENTRY_IMPORT=[]
    original,replacement=Machine(exe),Machine(exe,dll);cases=[]
    for kind in ('object','building','unit'):
        for values in itertools.product((0,1),repeat=5):
            cases.append(DEFAULT|dict(kind=kind,**dict(zip(('window','debug','forced','redraw','limbo'),values))))
    for kind,visible,onmap,fog,extra in itertools.product(('object','building','unit'),(0,1),(0,1),(0,1),(0,1,0x100)):
        cases.append(DEFAULT|dict(kind=kind,visible=visible,onmap=onmap,fog=fog,extra=extra))
    for rect in ((50,60,70,80),(-20,-30,400,300),(20,30,0,10),(0,0,-1,100),(300,30,1,1),(0,200,100,100)):
        for kind in ('object','building'):cases.append(DEFAULT|dict(kind=kind,clip=rect))
    cases.append(DEFAULT|dict(rtti=24,visible=0))
    for rtti,mission,queued,door in itertools.product((1,6),(0,16),(0,16),range(4)):
        cases.append(DEFAULT|dict(kind='unit',tether=1,radio_rtti=rtti,mission=mission,queued=queued,door=door))
    passed=0;failures=[]
    for i,c in enumerate(cases):
        a,b=original.visibility(c),replacement.visibility(c)
        if a!=b:failures.append(dict(case=i,input=c,original=a,replacement=b));break
        passed+=1
    rng=random.Random(0x421B60);rectangles=0
    for i in range(144):
        a=tuple(rng.randrange(-150,151) for _ in range(4));b=tuple(rng.randrange(-150,151) for _ in range(4))
        if i%3==0:a=(0,0,100,100);b=(i-72,i-80,100,100)
        x,y=original.intersect(a,b,i%3,i%4),replacement.intersect(a,b,i%3,i%4)
        if x!=y:failures.append(dict(rect=i,a=a,b=b,original=x,replacement=y));break
        rectangles+=1
    args.report.write_text(json.dumps(dict(scope=__doc__,exe_sha256=SHA,visibility=passed,rectangles=rectangles,failures=failures),indent=2)+'\n')
    print(f'{passed} visibility + {rectangles} rectangle cases; {len(failures)} differences')
    if failures:print(str(failures[0])[:3000]);raise SystemExit(1)

if __name__=='__main__':main()
