// Native visual subset of 0x00427D00, 0x00427B50 and 0x00428C30.
#include "yrpp/AnimTypeClass.h"
#include "type_resources.hpp"
#include "RulesClassReaders.hpp"
#include "yrpp/WarheadTypeClass.h"
#include <algorithm>
SHPStruct* AnimTypeClass::GetImage() const{return Image;}
void AnimTypeClass::Load2DArt(TheaterType){
 if(!Image)LoadTypeImage();auto* shape=Image?Image->GetData():nullptr;if(!shape)return;
 if(!End)End=shape->Frames/(Shadow?2:1);
 if(!LoopEnd)LoopEnd=End;
 MiddleFrameIndex=shape->Frames/2;MiddleFrameWidth=MiddleFrameHeight=-1;
}
bool AnimTypeClass::LoadFromINI(CCINIClass*ini){if(!ini)return false;auto&art=game::type_art_ini();const char*section=ID;
 const bool old_shadow=Shadow;
 Layer=static_cast<::Layer>(art.ReadLayer(section,"Layer",static_cast<int>(Layer)));
 art.ReadString(section,"Image",ImageFile,ImageFile,sizeof(ImageFile));art.ReadString(section,"Name",Name,Name,sizeof(Name));
#define B(f) f=art.ReadBool(section,#f,f)
 B(Theater);B(NewTheater);B(Shadow);B(Reverse);B(PingPong);B(Flat);B(AltPalette);B(ShouldUseCellDrawer);B(UseNormalLight);B(Translucent);B(HideIfNoOre);B(Normalized);B(IsTiberium);B(DemandLoad);B(FreeLoad);B(DoubleThick);B(IsAnimatedTiberium);
 B(Bouncer);B(IsMeteor);B(IsFlamingGuy);B(IsVeins);B(Tiled);B(ShouldFogRemove);
#undef B
 if(Shadow!=old_shadow){End=Shadow?End/2:End*2;LoopEnd=std::min(LoopEnd,End);}
 if(!LoadTypeImage())return false;Load2DArt(TheaterType{});
#define I(f) f=art.ReadInteger(section,#f,f)
 I(Start);I(LoopStart);I(LoopEnd);I(End);I(LoopCount);I(ZAdjust);I(YSortAdjust);I(YDrawOffset);I(Translucency);
 I(TrailerSeperation);I(DamageRadius);I(SpawnCount);I(RunningFrames);I(DetailLevel);I(TranslucencyDetailLevel);
#undef I
 // OpenTS animtype.cpp Read_INI; YR 0x00427D00. The EXE does not
 // read MaxZVel: +0x320 retains its constructor value even if MinZVel rises.
 Elasticity=art.ReadDouble(section,"Elasticity",Elasticity);
 MaxXYVel=art.ReadDouble(section,"MaxXYVel",MaxXYVel);
 MinZVel=art.ReadDouble(section,"MinZVel",MinZVel);
 Damage=art.ReadDouble(section,"Damage",Damage);
 read_rule_type(art,section,"BounceAnim",AbstractType::AnimType,BounceAnim);
 read_rule_type(art,section,"ExpireAnim",AbstractType::AnimType,ExpireAnim);
 read_rule_type(art,section,"TrailerAnim",AbstractType::AnimType,TrailerAnim);
 read_rule_type(art,section,"Spawns",AbstractType::AnimType,Spawns);
 read_rule_type(art,section,"Warhead",AbstractType::WarheadType,Warhead);
 const int rate=art.ReadInteger(section,"Rate",-1);
 if(rate!=-1)Rate=rate>0?900/rate:0;
 int pair[2]{},defaults[2]{-1,-1};
 art.Read2Integers(pair,section,"RandomRate",defaults);
 if(pair[0]!=-1)RandomRate.Min=pair[0]>0?900/pair[0]:0;
 if(pair[1]!=-1)RandomRate.Max=pair[1]>0?900/pair[1]:0;
 RandomRate.Max=std::max(RandomRate.Max,0);RandomRate.Min=std::min(RandomRate.Min,RandomRate.Max);
 defaults[0]=RandomLoopDelay.Min;defaults[1]=RandomLoopDelay.Max;
 art.Read2Integers(pair,section,"RandomLoopDelay",defaults);RandomLoopDelay={pair[0],pair[1]};
 char next[64]{};art.ReadString(section,"Next","",next,sizeof(next));
 if(*next&&_strcmpi(next,"none")&&_strcmpi(next,"<none>"))Next=FindOrAllocate(next);
 // Zero is a stopped timer in 0x004244D4; do not silently turn it into Rate=1.
 if(Rate<0||Rate>INT_MAX/8||RandomRate.Min<0||RandomRate.Max>INT_MAX/8||
    RandomLoopDelay.Min<0||RandomLoopDelay.Max<0)return false;
 return true;
}
