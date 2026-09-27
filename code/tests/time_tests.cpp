#include "support/test_support.hpp"
#include "game_loop.hpp"
#include "api/clock.hpp"
#include <cmath>
#include <cstring>
#include "yrpp/AnimClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/QueueClass.h"
#include <memory>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <limits>
#include <cfenv>

#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif

void configure(AnimTypeClass& t,int rate,int start,int end,int first,int last,int loops,
 bool reverse,bool pingpong,bool shadow,bool normalized,AnimTypeClass* next,int rmin,int rmax,int dmin,int dmax){
 t.Rate=rate;t.Start=start;t.End=end;t.LoopStart=first;t.LoopEnd=last;t.LoopCount=loops;
 t.Reverse=reverse;t.PingPong=pingpong;t.Shadow=shadow;t.Normalized=normalized;t.Next=next;
 t.RandomRate={rmin,rmax};t.RandomLoopDelay={dmin,dmax};
}

namespace {
struct TestClock {
    double milliseconds = 0;
    static std::uint32_t read(void* p) noexcept { return static_cast<std::uint32_t>(static_cast<TestClock*>(p)->milliseconds); }
    static bool advance(void* p, double seconds) noexcept {
        auto& value = static_cast<TestClock*>(p)->milliseconds;
        value = std::fmod(value + std::fmod(seconds,4294967.296)*1000.0,4294967296.0);
        return true;
    }
};
void time_contracts(void* clock_context) {
    auto& clock = *static_cast<TestClock*>(clock_context);
    const int old_rounding=std::fegetround();
    const auto restore_rounding=ra2::test::scope_exit([old_rounding] { std::fesetround(old_rounding); });
    ScenarioClass scenario;auto* old=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
    const auto restore_scenario = ra2::test::scope_exit([old] { ScenarioClass::Instance=old; });
    std::ifstream fixture(RA2_TIME_FIXTURE);EXPECT_TRUE((bool(fixture))) << "time reference file";char kind;int checks=0;
    std::unique_ptr<TiberiumClass> ore;
    std::unique_ptr<OverlayTypeClass> overlay;
    while(fixture>>kind){
     if(kind=='L'||kind=='M'){
      int speed,start,work,end,allow=1,effective=0;
      if(kind=='M')fixture>>allow;
      fixture>>speed>>start>>work>>end;
      if(kind=='M')fixture>>effective;else effective=speed;
      GameOptionsClass::Instance.GameSpeed=speed;game::GameLoopState state;clock.milliseconds=start;
      state.mode=kind=='M'?GameMode::Campaign:GameMode::Skirmish;state.campaign_speed_setting=allow;
      struct Iteration {TestClock& clock;int work;std::vector<int> phases;} iteration{clock,work,{}};
      Unsorted::CurrentFrame=0;
      game::GameLoopContext c{state,false,&iteration,
       [](void* p){static_cast<Iteration*>(p)->phases.push_back(10+Unsorted::CurrentFrame);return true;},
       [](void* p){auto& t=*static_cast<Iteration*>(p);t.phases.push_back(20+Unsorted::CurrentFrame);t.clock.milliseconds+=t.work;return true;},
       [](void* p){static_cast<Iteration*>(p)->phases.push_back(30+Unsorted::CurrentFrame);return true;}};
      EXPECT_TRUE((game::advance_game_loop(c,0))) << "original main loop entry";
      EXPECT_TRUE((GameOptionsClass::Instance.GameSpeed==effective)) << "original campaign speed lock";
      EXPECT_TRUE(((iteration.phases==std::vector<int>{10,20,30}))) << "original input/render/logic frame trace";
      if(end>clock.milliseconds){
       const double advance=end-clock.milliseconds-1;
       EXPECT_TRUE((game::advance_game_loop(c,advance/1000.0)&&Unsorted::CurrentFrame==1)) << "original wait is still active before deadline";
       EXPECT_TRUE((game::advance_game_loop(c,0.001)&&Unsorted::CurrentFrame==2)) << "original wait ends at captured millisecond";
      }else EXPECT_TRUE((game::advance_game_loop(c,0)&&Unsorted::CurrentFrame==2)) << "CPU overrun and unlimited speed add no wait";
      ++checks;continue;
     }
     if(kind=='Q'){
      int counter,timed,duration,expected_counter,expected_flag,expected_frame,end;
      fixture>>counter>>timed>>duration>>expected_counter>>expected_flag>>expected_frame>>end;
      scenario.unknown_62C=counter;scenario.IsGamePaused=timed;
      scenario.PauseTimer.StartTime=6;scenario.PauseTimer.TimeLeft=duration;
      game::GameLoopState state;clock.milliseconds=100;GameOptionsClass::Instance.GameSpeed=2;Unsorted::CurrentFrame=0;
      struct Pause{TestClock& clock;std::vector<int> trace;} pause{clock,{}};
      game::GameLoopContext c{state,false,&pause,
       [](void* p){static_cast<Pause*>(p)->trace.push_back(10);return true;},
       [](void* p){auto& t=*static_cast<Pause*>(p);t.trace.push_back(20);t.clock.milliseconds+=5;return true;},
       [](void* p){static_cast<Pause*>(p)->trace.push_back(30);return true;}};
      c.scenario=&scenario;
      EXPECT_TRUE((game::advance_game_loop(c,0))) << "original nested/timed pause";
      EXPECT_TRUE((scenario.unknown_62C==unsigned(expected_counter)&&scenario.IsGamePaused==bool(expected_flag)&&Unsorted::CurrentFrame==expected_frame)) << "original pause fields and frame";
      EXPECT_TRUE((pause.trace==(expected_frame?std::vector<int>{10,20,30}:std::vector<int>{20}))) << "paused branch only renders";
      scenario.unknown_62C=0;scenario.IsGamePaused=false;++checks;continue;
     }
     if(kind=='G'){
      std::array<int,21> expected;for(auto& n:expected)fixture>>n;
      if(expected[0]==0){
       Unsorted::CurrentFrame=0;scenario.Random=Randomizer(12345);scenario.TiberiumGrowthEnabled=true;
       scenario.SpecialFlags.TiberiumGrows=true;scenario.SpecialFlags.TiberiumSpreads=false;
       EXPECT_TRUE((MapClass::Instance.CreateEmptyCells({0,0,64,64},0))) << "growth fixture map";
       overlay=std::make_unique<OverlayTypeClass>("TIMEORE");ore=std::make_unique<TiberiumClass>("TIMEORE");
       // FindIndex requires the original overlay Tiberium flag: this growth
       // fixture represents a resource overlay.
       overlay->Tiberium=true;
       ore->Image=overlay.get();ore->NumImages=12;ore->NumFrames=12;ore->GrowthPercentage=0.1;
       auto& l=ore->GrowthLogic;l.Construct();l.Count=16;l.Timer.Start(0);
       for(int i=0;i<16;++i){auto coords=CellStruct{short(40+i),40};auto* cell=MapClass::Instance.TryGetCellAt(coords);EXPECT_TRUE((cell!=nullptr)) << "growth cell";cell->OverlayTypeIndex=overlay->ArrayIndex;cell->OverlayData=1;
        l.Nodes[i]={coords,0};EXPECT_TRUE((l.Queue->Push(&l.Nodes[i]))) << "seed original queue";}
      }
      Unsorted::CurrentFrame=expected[0];TiberiumClass::UpdateGrowth();auto& l=ore->GrowthLogic;
      std::array<int,21> actual{expected[0],l.Count,l.Queue->Count,scenario.Random.Next1,scenario.Random.Next2};
      for(int i=0;i<16;++i)actual[i+5]=MapClass::Instance.TryGetCellAt(CellStruct{short(40+i),40})->OverlayData;
      ASSERT_EQ(actual, expected) << "growth frame " << expected[0];
      if(expected[0]==79){ore.reset();overlay.reset();MapClass::Instance.ReleaseCellStorage();}
      ++checks;continue;
     }
     if(kind=='T'||kind=='U'){
      int cw,growth,fast,enabled,frame,start,left,expected_start,expected_left,expected_cw;
      fixture>>std::hex>>cw>>std::dec>>growth>>fast>>enabled>>frame>>start>>left>>expected_start>>expected_left>>std::hex>>expected_cw>>std::dec;
      TiberiumClass resource("TIMETIMER");resource.Growth=resource.Spread=growth;
      auto& timer=kind=='T'?resource.GrowthLogic.Timer:resource.SpreadLogic.Timer;
      timer.StartTime=start;timer.TimeLeft=left;
      scenario.TiberiumGrowthEnabled=enabled;scenario.SpecialFlags.TiberiumGrows=fast;
      Unsorted::CurrentFrame=frame;
      std::fesetround(cw==0x0E7F?FE_TOWARDZERO:FE_TONEAREST);
      if(kind=='T')TiberiumClass::UpdateGrowth();else TiberiumClass::UpdateSpread();
      const std::array<int,3> actual{timer.StartTime,timer.TimeLeft,std::fegetround()};
      const std::array<int,3> expected{expected_start,expected_left,expected_cw==0x0E7F?FE_TOWARDZERO:FE_TONEAREST};
      ASSERT_EQ(actual,expected) << "timer=" << kind << " delay=" << growth << " fast=" << fast << " enabled=" << enabled << " frame=" << frame << " start=" << start << " left=" << left << " cw=0x" << std::hex << cw;
      ++checks;continue;
     }
     if(kind=='S'){int speed,rate,result;fixture>>speed>>rate>>result;GameOptionsClass::Instance.GameSpeed=speed;EXPECT_TRUE((GameOptionsClass::Instance.GetAnimSpeed(rate)==result)) << "original speed table";++checks;continue;}
     EXPECT_TRUE((kind=='C')) << "case marker";int speed,variant,count;fixture>>speed>>variant>>count;
     GameOptionsClass::Instance.GameSpeed=speed;scenario.Random=Randomizer(12345);Unsorted::CurrentFrame=0;
     AnimTypeClass type("TIME"),next("NEXT");int start=variant==8||variant==9?2:0;
     int loops=variant==1||variant==3||variant==4||variant==7||variant==8?1:3;
     bool hasnext=variant==7||variant==9||variant==11;
     configure(type,variant==0?0:2,start,7,start+1,6,loops,variant==3,variant==4||variant==5,variant==6,
      variant>=9,hasnext?&next:nullptr,variant==11?1:0,variant==11?4:0,variant==2||variant==10?1:0,variant==2||variant==10?3:0);
     configure(next,1,1,5,1,4,2,false,false,false,true,nullptr,2,5,0,0);
     AnimClass anim(&type,{},variant==5?2:0,1);
     for(int row=0;row<count;++row){
      fixture>>kind;EXPECT_TRUE((kind=='A')) << "animation marker";std::array<int,12> expected;for(auto& n:expected)fixture>>n;
      int frame=expected[0];Unsorted::CurrentFrame=frame;anim.Paused=variant==10&&frame>=4&&frame<9;anim.PowerOff=variant==11&&frame>=6&&frame<10;
      anim.Update();
      std::array<int,12> actual{frame,anim.Animation.Value,anim.Animation.Rate,anim.Animation.Timer.StartTime,anim.Animation.Timer.TimeLeft,
       anim.Animation.Step,anim.RemainingIterations,anim.LoopDelay,int(anim.TimeToDie),int(anim.Type==&next),scenario.Random.Next1,scenario.Random.Next2};
      ASSERT_EQ(actual, expected) << "speed=" << speed << " variant=" << variant << " frame=" << frame;++checks;
     }
    }
    EXPECT_TRUE((LogicClass::Instance.Count==0)) << "animation deletion unregisters original Logic list";
    // Host coroutine: each original iteration renders the old frame, updates
    // logic using that frame, then increments and waits in quantized system ticks.
    const bool aborted=Unsorted::DragSelectAborted;
    const auto restore_aborted=ra2::test::scope_exit([&]{Unsorted::DragSelectAborted=aborted;});
    Unsorted::DragSelectAborted=true;
    struct Trace{std::vector<int> phases;};Trace trace;
    for(int speed=0;speed<=6;++speed){
     GameOptionsClass::Instance.GameSpeed=speed;game::GameLoopState state;Unsorted::CurrentFrame=0;
     game::GameLoopContext c{state,false,&trace,
      [](void* p){EXPECT_FALSE(Unsorted::DragSelectAborted);Unsorted::DragSelectAborted=true;static_cast<Trace*>(p)->phases.push_back(10+Unsorted::CurrentFrame);return true;},
      [](void* p){EXPECT_TRUE(Unsorted::DragSelectAborted);static_cast<Trace*>(p)->phases.push_back(20+Unsorted::CurrentFrame);return true;},
      [](void* p){static_cast<Trace*>(p)->phases.push_back(30+Unsorted::CurrentFrame);return true;}};
     trace.phases.clear();EXPECT_TRUE((game::advance_game_loop(c,0))) << "first loop";EXPECT_TRUE(((trace.phases==std::vector<int>{10,20,30}))) << "input render logic ordering";EXPECT_TRUE((Unsorted::CurrentFrame==1)) << "frame increments after consumers";
     for(double stall:{0.05,0.1,0.5,10.0}){int before=Unsorted::CurrentFrame;EXPECT_TRUE((game::advance_game_loop(c,stall))) << "stall accepted";EXPECT_TRUE((Unsorted::CurrentFrame<=before+1)) << "stall never fabricates catch-up loops";}
     EXPECT_TRUE((state.elapsed_seconds>10.64)) << "elapsed wall time not clamped";
     c.paused=true;int frame=Unsorted::CurrentFrame;EXPECT_TRUE((game::advance_game_loop(c,1.0)&&Unsorted::CurrentFrame==frame)) << "pause freezes logic";
     state.focused=false;c.paused=false;auto phases=trace.phases.size();EXPECT_TRUE((game::advance_game_loop(c,5.0)&&trace.phases.size()==phases)) << "focus loss suspends single player";
     state.focused=true;EXPECT_TRUE((game::advance_game_loop(c,0)&&Unsorted::CurrentFrame==frame+1)) << "focus resume no catch-up";
     EXPECT_TRUE((!game::advance_game_loop(c,-1)&&!game::advance_game_loop(c,std::numeric_limits<double>::infinity()))) << "invalid host elapsed rejected";
     state.mode=GameMode::LAN;EXPECT_TRUE((!game::advance_game_loop(c,0))) << "network mode has no guessed scheduler";
    }
    // Presentation callbacks do not advance the clock or animation frame.
    for(int hz:{30,60,144})for(int speed:{0,1,2,6}){
     unsigned long long iterations[2]{};
     for(int present=0;present<2;++present){
      GameOptionsClass::Instance.GameSpeed=speed;Unsorted::CurrentFrame=0;
      game::GameLoopState state;game::GameLoopContext c{state,false,nullptr,nullptr,nullptr,nullptr};
      if(present)c.render=[](void*){return true;};
      for(int i=0;i<hz*2;++i)EXPECT_TRUE((game::advance_game_loop(c,1.0/hz))) << "host refresh sample";
      iterations[present]=state.iterations;
     }
     EXPECT_TRUE((iterations[0]==iterations[1])) << "same calibrated scheduling with/without presentation";
    }
    ScenarioClass::Instance=old;
}

} // namespace
TEST(Time, Contracts) {
    TestClock clock;
    ASSERT_TRUE(game::with_clock({&clock,TestClock::read,TestClock::advance},time_contracts,&clock));
}
TEST(Time, ProductionClockLifetime) {
    const auto start=game::clock_milliseconds();
    SysTimerClass timer(4);
    EXPECT_EQ(game::clock_milliseconds(), start);
    ASSERT_TRUE(game::advance_clock(0.032));
    EXPECT_EQ(game::clock_milliseconds(), std::uint32_t(start+32));
    EXPECT_EQ(timer.GetTimeLeft(), 2);
    game::GameLoopState replacement_session;
    EXPECT_EQ(game::clock_milliseconds(), std::uint32_t(start+32));
    ASSERT_TRUE(game::advance_clock(0));
    EXPECT_EQ(timer.GetTimeLeft(), 2);
    ASSERT_TRUE(game::advance_clock(0.032));
    EXPECT_TRUE(timer.Completed());
    const auto after=game::clock_milliseconds();
    EXPECT_FALSE(game::advance_clock(-1));
    EXPECT_FALSE(game::advance_clock(std::numeric_limits<double>::infinity()));
    EXPECT_EQ(game::clock_milliseconds(), after);
    game::GameLoopContext context{replacement_session,false,nullptr,
        [](void*) {
            const auto before=game::clock_milliseconds();
            EXPECT_FALSE(game::advance_clock(1));
            EXPECT_EQ(game::clock_milliseconds(),before);
            return true;
        },nullptr,nullptr};
    context.state.mode=GameMode::Skirmish;
    GameOptionsClass::Instance.GameSpeed=2;
    ASSERT_TRUE(game::advance_game_loop(context,0));
}
TEST(Time, QueueTimestampsShareMillisecondAuthority) {
    struct Clock { std::uint32_t now=0xFFFFFFF7u;int reads=0; } clock;
    game::ClockServices services{&clock,[](void* context) noexcept {
        auto& clock=*static_cast<Clock*>(context);++clock.reads;return clock.now;
    }};
    ASSERT_TRUE(game::with_clock(services,[](void* context){
        auto& clock=*static_cast<Clock*>(context);
        QueueClass<int,4> queue;
        EXPECT_EQ(SystemTimer::GetMilliseconds(),clock.now);
        EXPECT_EQ(SystemTimer::GetTime(),clock.now>>4);
        const int reads=clock.reads;
        ASSERT_TRUE(queue.Add(10));EXPECT_EQ(clock.reads,reads+1);
        clock.now=5;ASSERT_TRUE(queue.Add(20));
        // Original Queue layout: Count, Head, Tail, four values, four DWORD
        // timestamps. Check raw milliseconds and wrap, not shifted timer ticks.
        static_assert(sizeof(queue)==11*sizeof(int));
        std::uint32_t times[4]{};
        std::memcpy(times,reinterpret_cast<const unsigned char*>(&queue)+7*sizeof(int),sizeof(times));
        EXPECT_EQ(times[0],0xFFFFFFF7u);EXPECT_EQ(times[1],5u);
        ASSERT_TRUE(queue.Add(30));ASSERT_TRUE(queue.Add(40));
        const int full_reads=clock.reads;
        EXPECT_FALSE(queue.Add(50));EXPECT_EQ(clock.reads,full_reads);
    },&clock));
}
