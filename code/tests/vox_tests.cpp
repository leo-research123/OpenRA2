#include "support/test_support.hpp"
#include "yrpp/VoxClass.h"
#include "yrpp/Audio.h"
#include "yrpp/Memory.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/MixFileClass.h"
#include "api/audio_backend.hpp"
#include "api/clock.hpp"
#include "api/filesystem.hpp"
#include <cstring>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
namespace {
struct Device:game::AudioBackend {
    AudioStream identity;
    bool available=true,playing=false,accept=true,resources=false;
    unsigned long long ended=0;
    int creations=0,stops=0,pauses=0,resumes=0,destroys=0,bytes=0;
    std::string name,failure;std::vector<std::string> files;
    bool stream_device_available() noexcept override{return available;}
    bool create_stream(int buffer,int flags,AudioStream*& out) noexcept override {
        EXPECT_EQ(buffer,3000);EXPECT_EQ(flags,0);++creations;if(!available)return false;out=&identity;return true;
    }
    void name_stream(AudioStream&,const char* text)noexcept override{name=text;}
    void destroy_stream(AudioStream& stream)noexcept override{EXPECT_EQ(&stream,&identity);++destroys;}
    void bind_stream_volume(AudioStream&,AudioVolumeControl*,bool)noexcept override{}
    void stop_stream(AudioStream&)noexcept override{playing=false;++stops;ended=Audio::GetTime();}
    void pause_stream(AudioStream&)noexcept override{++pauses;}
    void resume_stream(AudioStream&)noexcept override{++resumes;}
    bool stream_is_playing(const AudioStream&,bool& out)noexcept override{out=playing;return true;}
    bool stream_end_time(const AudioStream&,unsigned long long& out)noexcept override{out=ended;return true;}
    bool play_wav(AudioStream& stream,const char* file,bool resource)noexcept override {
        EXPECT_EQ(&stream,&identity);EXPECT_TRUE(resource);files.emplace_back(file);bytes=0;
        if(!accept)return false;
        if(resources) {
            CCFileClass input(file);if(!input.Open(FileAccessMode::Read)){failure="open";return false;}
            AudioSampleData sample;int size=0;if(!Audio::ReadWAVFile(&input,&sample,&size)){failure="WAV header; size="+std::to_string(size);return false;}
            if(!sample.SampleRate || !sample.NumChannels || size<=0){failure="WAV metadata";return false;}
            std::vector<unsigned char> payload(size);bytes=input.ReadBytes(payload.data(),size);
            if(bytes!=size){failure="short data";return false;}
        }
        playing=true;return true;
    }
    void finish(){playing=false;ended=Audio::GetTime();}
    void destroy_controller(AudioController&)noexcept override{}
    void stop(AudioController&)noexcept override{}
    void end(AudioController&)noexcept override{}
    void stop_looping(AudioController&)noexcept override{}
    void end_looping(AudioController&)noexcept override{}
    void set_event(AudioController&,game::AudioEventHandle,VocClass*)noexcept override{}
    bool get_event(AudioController&,game::AudioEventHandle&)noexcept override{return false;}
    bool get_controller_type(AudioController&,VocClass*&)noexcept override{return false;}
    void set_volume(game::AudioEventHandle,unsigned)noexcept override{}
    void set_panning(game::AudioEventHandle,unsigned)noexcept override{}
    bool get_event_type(game::AudioEventHandle,VocClass*&)noexcept override{return false;}
    void stop_playback(game::AudioPlaybackHandle)noexcept override{}
    void release_buffer(game::AudioBufferHandle)noexcept override{}
};
struct Scope {
    Device& device;game::AudioBackend* previous=nullptr;
    unsigned long long time=Audio::LastTime,gap=VoxClass::GapMilliseconds;
    unsigned read=Audio::TimeReadSerial;
    int eva=VoxClass::EVAIndex,serial=VoxClass::NextSerial,initialized=VoxClass::Initialized;
    int pause=VoxClass::PauseCount,suppress=VoxClass::SuppressCount;
    int control=VoxClass::CurrentControl,priority=VoxClass::CurrentPriority;
    bool quiet=Audio::Quiet;
    Scope(Device& d):device(d){game::get_audio_backend(previous);game::set_audio_backend(d);Audio::LastTime=0x100000000ULL;Audio::TimeReadSerial=0;Audio::Quiet=false;}
    ~Scope(){
        VoxClass::DeleteAll();VoxClass::Shutdown();VoxClass::Current=nullptr;
        if(previous)game::set_audio_backend(*previous);else game::reset_audio_backend();
        Audio::LastTime=time;Audio::TimeReadSerial=read;Audio::Quiet=quiet;VoxClass::EVAIndex=eva;
        VoxClass::NextSerial=serial;VoxClass::Initialized=initialized;VoxClass::GapMilliseconds=gap;
        VoxClass::PauseCount=pause;VoxClass::SuppressCount=suppress;VoxClass::CurrentControl=control;VoxClass::CurrentPriority=priority;
    }
};
struct Clock {unsigned now=0;};
game::ClockServices services(Clock& clock){return {&clock,[](void* p)noexcept{return static_cast<Clock*>(p)->now;},nullptr};}
VoxClass* voice(const char* name,VoxType control,VoxPriority priority) {
    auto* v=GameCreate<VoxClass>(name);v->Type=control;v->Priority=priority;
    std::strcpy(v->Allied,"ALLIED");std::strcpy(v->Russian,"RUSSIAN");std::strcpy(v->Yuri,"YURI");return v;
}
}
TEST(Vox, QueuePriorityInterruptionPauseAndAuthoritativeClock){
    ASSERT_EQ(VoxClass::Array.Count,0);ASSERT_EQ(VoxClass::Stream,nullptr);
    Clock clock;const auto source=services(clock);
    ASSERT_TRUE(game::with_clock(source,[](void* ptr){
        auto& clock=*static_cast<Clock*>(ptr);Device device;Scope scope(device);ASSERT_TRUE(VoxClass::Initialize());
        EXPECT_EQ(device.name,"EVA Stream");EXPECT_NE(VoxClass::Stream,AudioStream::Instance);
        auto* low=voice("LOW",VoxType::Queue,VoxPriority::Low);
        auto* high=voice("HIGH",VoxType::Queue,VoxPriority::Critical);
        auto* interrupt=voice("NOW",VoxType::Interrupt,VoxPriority::Normal);
        auto* queued=voice("NEXT",VoxType::QueuedInterrupt,VoxPriority::Low);
        VoxClass::EVAIndex=1;device.playing=true;
        VoxClass::Play("low");VoxClass::Play("high");VoxClass::Play("HIGH");
        EXPECT_EQ(VoxClass::NextSerial,scope.serial+2);EXPECT_EQ(high->unknown_int_50,1);
        device.playing=false;VoxClass::Update();EXPECT_EQ(VoxClass::Current,high);
        EXPECT_EQ(device.files.back(),"RUSSIAN.WAV");EXPECT_EQ(high->unknown_int_50,0);
        device.finish();const auto before=device.files.size();clock.now=500;VoxClass::Update();
        EXPECT_EQ(device.files.size(),before)<<"strict comparison excludes exact end+500";
        clock.now=501;VoxClass::Update();EXPECT_EQ(VoxClass::Current,low);
        VoxClass::Play("NEXT");VoxClass::Play("NOW");EXPECT_EQ(device.stops,1);
        EXPECT_EQ(low->unknown_int_50,2);EXPECT_EQ(queued->unknown_int_50,2);EXPECT_EQ(VoxClass::GapMilliseconds,0);
        EXPECT_EQ(VoxClass::Current,nullptr)<<"stop timestamp equals current time";
        ++clock.now;VoxClass::Update();EXPECT_EQ(VoxClass::Current,interrupt);
        VoxClass::Pause();VoxClass::Pause();EXPECT_EQ(VoxClass::PauseCount,2);EXPECT_EQ(device.pauses,2);
        VoxClass::Resume();EXPECT_EQ(VoxClass::PauseCount,1);VoxClass::Resume();EXPECT_EQ(VoxClass::PauseCount,0);
        VoxClass::Suppress();const int serial=VoxClass::NextSerial;VoxClass::Play("LOW");EXPECT_EQ(VoxClass::NextSerial,serial);
        VoxClass::Unsuppress();EXPECT_EQ(VoxClass::SuppressCount,0);
        const auto time=Audio::GetTime();VoxClass::Update();EXPECT_EQ(Audio::GetTime(),time)<<"reads never advance time";
        clock.now=0xFFFFFFF0;const auto last=Audio::GetTime();clock.now=5;
        EXPECT_EQ(Audio::GetTime()-last,21u);EXPECT_EQ(Audio::GetTime(),0x200000005ULL);
    },&clock));
}
TEST(Vox, DefinitionsReloadAndOriginalStandardSlotState){
    ASSERT_EQ(VoxClass::Array.Count,0);Device device;Scope scope(device);ASSERT_TRUE(VoxClass::Initialize());
    CCINIClass ini;ini.WriteString("DialogList","0","TEST");ini.WriteString("DialogList","1","test");
    ini.WriteString("TEST","Volume","0.25");ini.WriteString("TEST","Priority","important");
    ini.WriteString("TEST","Type","queued_interrupt");ini.WriteString("TEST","Allied","123456789ABC");
    ASSERT_TRUE(VoxClass::LoadAllFromINI(&ini));ASSERT_EQ(VoxClass::Array.Count,1);
    auto* v=VoxClass::Find("TeSt");ASSERT_NE(v,nullptr);EXPECT_FLOAT_EQ(v->Volume,0.25f);
    EXPECT_EQ(v->Type,VoxType::QueuedInterrupt);EXPECT_EQ(v->Priority,VoxPriority::Important);
    EXPECT_STREQ(v->Allied,"12345678");EXPECT_STREQ(v->Russian,"");
    ini.Clear("TEST","Type");ini.WriteString("TEST","Priority","unknown");ini.Clear("TEST","Volume");
    EXPECT_TRUE(v->LoadFromINI(&ini));EXPECT_FLOAT_EQ(v->Volume,1.0f);EXPECT_EQ(v->Priority,VoxPriority::Important);
    EXPECT_EQ(v->Type,VoxType::QueuedInterrupt);ini.Clear("TEST");v->Volume=0.3f;
    EXPECT_FALSE(v->LoadFromINI(&ini));EXPECT_FLOAT_EQ(v->Volume,1.0f);EXPECT_STREQ(v->Allied,"12345678");
    auto* first=voice("FIRST",VoxType::Standard,VoxPriority::Low);
    auto* second=voice("SECOND",VoxType::Standard,VoxPriority::Important);
    VoxClass::Enqueue(first,0,0);auto* orphan=VoxClass::StandardSlot;
    VoxClass::Enqueue(second,0,2);EXPECT_EQ(first->unknown_int_50,1);EXPECT_EQ(VoxClass::FindQueued(first),nullptr);
    EXPECT_EQ(VoxClass::StandardSlot->Voice,second);
    // Exact YR replacement leaks the displaced detached node. The test releases
    // its saved pointer explicitly; it does not hide a production ownership fix.
    YRMemory::Deallocate(orphan);
    VoxClass::SilenceIndex(VoxClass::FindIndex("SECOND"));EXPECT_EQ(second->unknown_int_50,2);EXPECT_EQ(VoxClass::StandardSlot,nullptr);
}
TEST(Vox, OriginalIniWavAndFailedPlacementFeedback){
    const char* data=std::getenv("RA2_GAME_DATA");if(!data||!*data)GTEST_SKIP()<<"RA2_GAME_DATA enables EVA resources";
    game::ResourceHandle* raw=nullptr;std::string error;ASSERT_TRUE(game::create_resources(data,raw,error))<<error;
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> resources(raw,game::destroy_resources);
    ASSERT_EQ(game::load_resources(*resources,{},error),game::ResourceLoadResult::complete)<<error;
    ASSERT_TRUE(game::with_resources(*resources,[](void*){
        ASSERT_EQ(VoxClass::Array.Count,0);Device device;device.resources=true;Scope scope(device);
        // Original 0x00406C43..0x00406CCF mounts one audio archive after device
        // availability, before EVA definition/playback use. Generic Bootstrap
        // alone does not mount this nested package.
        CCFileClass audio_file("AUDIOMD.MIX");
        MixFileClass audio_mix(audio_file.Exists()?"AUDIOMD.MIX":"AUDIO.MIX");
        ASSERT_GT(audio_mix.CountFiles,0);
        ASSERT_TRUE(VoxClass::Initialize());CCINIClass ini;ASSERT_NE(ini.LoadFromFile("EVAMD.INI"),0);
        VoxClass::LoadAllFromINI(&ini);ASSERT_GT(VoxClass::Array.Count,0);
        auto* failure=VoxClass::Find("EVA_CannotDeployHere");ASSERT_NE(failure,nullptr);
        for(int side=0;side<3;++side) {
            VoxClass::EVAIndex=side;VoxClass::GapMilliseconds=0;device.ended=0;device.playing=false;VoxClass::Current=nullptr;
            VoxClass::Play("EVA_CannotDeployHere");EXPECT_GT(device.bytes,0)<<device.files.back()<<": "<<device.failure;
            EXPECT_EQ(VoxClass::Current,failure);EXPECT_EQ(device.files.back(),std::string(failure->GetFilename())+".WAV");
        }
        HouseTypeClass country("EVA_TEST");HouseClass house(&country);UnitTypeClass type("EVA_PENDING");UnitClass pending(&type,&house);
        auto& d=DisplayClass::Instance;auto* old=d.CurrentBuilding;auto* old_type=d.CurrentBuildingType;
        const bool prox=d.unknown_1180,shroud=d.unknown_1181,tentative=d.unknown_bool_11D0,attack=Game::AttackMoveMode;
        const auto restore=ra2::test::scope_exit([&]{d.CurrentBuilding=old;d.CurrentBuildingType=old_type;d.unknown_1180=prox;
            d.unknown_1181=shroud;d.unknown_bool_11D0=tentative;Game::AttackMoveMode=attack;});
        device.playing=false;device.ended=0;VoxClass::GapMilliseconds=0;VoxClass::Current=nullptr;
        d.CurrentBuilding=&pending;d.CurrentBuildingType=&type;d.unknown_1180=false;d.unknown_1181=true;
        d.unknown_bool_11D0=true;Game::AttackMoveMode=true;const auto count=device.files.size();
        d.LeftMouseButtonUp({},CellStruct{0,0},nullptr,Action::None,0);
        EXPECT_EQ(device.files.size(),count+1);EXPECT_GT(device.bytes,0);EXPECT_EQ(VoxClass::Current,failure);
        EXPECT_FALSE(Game::AttackMoveMode);EXPECT_TRUE(d.unknown_bool_11D0);EXPECT_EQ(d.CurrentBuilding,&pending);
    },nullptr,error))<<error;
}

TEST(Vox, MissingDeviceAndFailedStartDoNotPretendToSpeak){
    ASSERT_EQ(VoxClass::Array.Count,0);ASSERT_EQ(VoxClass::Stream,nullptr);
    Device device;device.available=false;Scope scope(device);
    EXPECT_FALSE(VoxClass::Initialize());EXPECT_EQ(VoxClass::Initialized,1);EXPECT_EQ(VoxClass::Stream,nullptr);
    auto* v=voice("DEVICE_TEST",VoxType::Queue,VoxPriority::Normal);const int before=VoxClass::NextSerial;
    VoxClass::Play("DEVICE_TEST");EXPECT_EQ(VoxClass::NextSerial,before);EXPECT_FALSE(VoxClass::IsSpeaking());
    device.available=true;ASSERT_TRUE(VoxClass::Initialize());device.accept=false;
    VoxClass::Play("DEVICE_TEST");EXPECT_EQ(v->unknown_int_50,2);EXPECT_EQ(VoxClass::Current,nullptr);
    EXPECT_EQ(VoxClass::FindQueued(v),nullptr);EXPECT_FALSE(VoxClass::IsSpeaking());
    device.accept=true;VoxClass::Play("DEVICE_TEST");EXPECT_TRUE(VoxClass::IsSpeaking());
    const int stops=device.stops;VoxClass::SilenceIndex(VoxClass::FindIndex("DEVICE_TEST"));
    EXPECT_EQ(device.stops,stops);EXPECT_EQ(VoxClass::Current,v);EXPECT_EQ(v->unknown_int_50,0);
}
