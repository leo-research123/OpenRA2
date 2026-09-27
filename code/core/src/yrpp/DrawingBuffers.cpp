// Fixed YRpp Drawing.h ABuffer/ZBuffer layout, calibrated at 4114B0/7BD130.
#include "yrpp/DrawingBuffers.h"
#include "yrpp/Surface.h"
#include "yrpp/Memory.h"
#ifndef RA2_IMAGE_GAME
namespace { ABuffer* alpha_buffer = nullptr; ZBuffer* depth_buffer = nullptr; }
ABuffer*& ABuffer::Instance = alpha_buffer;
ZBuffer*& ZBuffer::Instance = depth_buffer;
#endif

namespace {
template<class T> void initialize(T& buffer, RectangleStruct bounds, WORD fill) {
    buffer.Bounds = bounds; buffer.Width = bounds.Width; buffer.Height = bounds.Height;
    buffer.Surface = GameCreate<BSurface>(bounds.Width, bounds.Height, 2);
    buffer.BufferSize = buffer.Surface->GetPitch() * bounds.Height;
    buffer.Surface->Fill(fill);
    buffer.BufferHead = buffer.Surface->Lock(0, 0);
    buffer.BufferTail = static_cast<byte*>(buffer.BufferHead) + buffer.BufferSize;
    buffer.MaxValue = 0x8000;
    buffer.Surface->Unlock();
}
void* ring_location(BSurface* surface, int offset, void* tail, int size, int x, int y) {
    auto address=reinterpret_cast<std::uintptr_t>(surface->Lock(x,y));
    surface->Unlock();
    address+=std::uint32_t(offset);
    if (address>=reinterpret_cast<std::uintptr_t>(tail)) address-=std::uint32_t(size);
    return reinterpret_cast<void*>(address);
}
}
// 410CE0 / 7BC970. The original types require explicit ReleaseSurface.
ABuffer::ABuffer(RectangleStruct bounds) : BufferPosition(0) { initialize(*this, bounds, 127); }
ZBuffer::ZBuffer(RectangleStruct bounds) : BufferOffset(0) { initialize(*this, bounds, 0xffff); }
void ABuffer::ReleaseSurface() { GameDelete(Surface); Surface = nullptr; }
void ZBuffer::ReleaseSurface() {
    GameDelete(Surface); Surface = nullptr; // make native repeated release safe
}
bool ABuffer::Fill(WORD value) { return Surface && Surface->Fill(value); }
bool ZBuffer::Fill(WORD value) { return Surface && Surface->Fill(value); }
void* ABuffer::GetBuffer(int x, int y) { return ring_location(Surface,BufferPosition,BufferTail,BufferSize,x,y); }
void* ZBuffer::GetBuffer(int x, int y) { return ring_location(Surface,BufferOffset,BufferTail,BufferSize,x,y); }
