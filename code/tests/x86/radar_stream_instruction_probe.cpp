// Production Radar/Display/Layer stream bodies, compiled for original x86 ABI.
// Stream and native reference transport are replaced only at their boundaries.
#include "yrpp/RadarClass.h"
#include "yrpp/ObjectClass.h"
#include "type_stream.hpp"
#include <cstddef>

static_assert(sizeof(RadarClass)==0x150C);
static_assert(sizeof(LayerClass)==0x18);
static_assert(offsetof(RadarClass,unknown_points_125C)==0x125C);
static_assert(offsetof(RadarClass,unknown_cells_1124)==0x1224);
static_assert(offsetof(RadarClass,RadarAudio)==0x14C0);

extern "C" {
int _fltused=0;
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void __cdecl __std_exception_copy(const __std_exception_data*,__std_exception_data*){__debugbreak();}
void __cdecl __std_exception_destroy(__std_exception_data*){__debugbreak();}
void __stdcall _CxxThrowException(void*,void*){__debugbreak();}
void __cdecl ProbeUnexpectedRuntime(){__debugbreak();}
__declspec(noreturn) void __cdecl abort(){__debugbreak();for(;;){}}
void (*ProbeWatson)()=ProbeUnexpectedRuntime;
void* ProbeTypeInfo[2]={nullptr,nullptr};
void* __cdecl memset(void* target,int value,std::size_t count){
    auto* bytes=static_cast<volatile unsigned char*>(target);
    for(std::size_t i=0;i<count;++i)bytes[i]=static_cast<unsigned char>(value);
    return target;
}
// Allocation functions are named/exported so Unicorn can supply an identical
// heap to both binaries; reaching these trap bodies is an audit failure.
volatile std::size_t RadarStreamRequest;
void* volatile RadarStreamFreed;
__declspec(dllexport) __declspec(noinline) void* __cdecl RadarStreamAllocate(std::size_t count){
    RadarStreamRequest=count;__debugbreak();return reinterpret_cast<void*>(count);
}
__declspec(dllexport) __declspec(noinline) void __cdecl RadarStreamFree(void* address){
    RadarStreamFreed=address;__debugbreak();
}
__declspec(dllexport) HRESULT __fastcall RadarStreamLoad(RadarClass* self,void*,IStream* stream){return self->RadarClass::Load(stream);}
__declspec(dllexport) HRESULT __fastcall RadarStreamSave(RadarClass* self,void*,IStream* stream){return self->RadarClass::Save(stream);}
__declspec(dllexport) HRESULT __fastcall LayerStreamLoad(LayerClass* self,void*,IStream* stream){return self->LayerClass::Load(stream);}
__declspec(dllexport) HRESULT __fastcall LayerStreamSave(LayerClass* self,void*,IStream* stream){return self->LayerClass::Save(stream);}
__declspec(dllexport) void __fastcall LayerStreamConstruct(LayerClass* self,void*){::new(self) LayerClass;}
}
void* __cdecl operator new(std::size_t count){return RadarStreamAllocate(count);}
void __cdecl operator delete(void* address,std::size_t) noexcept{RadarStreamFree(address);}
#pragma comment(linker,"/alternatename:__imp___invoke_watson=_ProbeWatson")
#pragma comment(linker,"/alternatename:??_7type_info@@6B@=_ProbeTypeInfo")
AudioIDXData*& AudioIDXData::Instance=*reinterpret_cast<AudioIDXData**>(0x87E294);
LayerClass (&MapClass::ObjectsInLayers)[5]=*reinterpret_cast<LayerClass(*)[5]>(0x8A0360);
namespace game {
HRESULT read_stream_bytes(IStream* stream,void* data,std::uint32_t size) noexcept {
    return stream->Read(data,size,nullptr);
}
HRESULT write_stream_bytes(IStream* stream,const void* data,std::uint32_t size) noexcept {
    return stream->Write(data,size,nullptr);
}
HRESULT type_stream_save_token(const AbstractClass* object,std::uint32_t& token) noexcept {
    token=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(object)); return 0;
}
HRESULT swizzle_object_reference(ObjectClass*& object) noexcept {
    return reinterpret_cast<HRESULT(__stdcall*)(void*,void**)>(0x6CF240)(
        reinterpret_cast<void*>(0xB0C110),reinterpret_cast<void**>(&object));
}
}
