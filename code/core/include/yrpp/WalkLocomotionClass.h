#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("4A582744-9839-11D1-B709-00A024DDAFD1")) WalkLocomotionClass : public LocomotionClass, public IPiggyback
{
public:
    // Three original COM subobjects: IPersistStream, ILocomotion, IPiggyback.
    /// VA: 0x0075C7F0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** output) override { JMP_STD(0x75C7F0); }
#else
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** output) override;
#endif
    /// VA: 0x0075CB80
    ULONG YRPP_STDCALL AddRef() override { return LocomotionClass::AddRef(); }
    /// VA: 0x0075CB90
    ULONG YRPP_STDCALL Release() override { return LocomotionClass::Release(); }
    /// VA: 0x0075C640
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override { JMP_STD(0x75C640); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override;
#endif
    /// VA: 0x0075C680
    HRESULT YRPP_STDCALL Load(IStream* stream) override { JMP_STD(0x75C680); }
    /// VA: 0x0075C700
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clear) override { JMP_STD(0x75C700); }
    /// VA: 0x0075CBE0
#if defined(RA2_YRPP_GAME)
    ~WalkLocomotionClass() override { if (Piggybackee) Piggybackee->Release(); }
#else
    ~WalkLocomotionClass() override;
#endif
    /// VA: 0x0075CBD0
    int Size() override { return 0x3C; }

    /// VA: 0x0075AB30
    bool YRPP_STDCALL Is_Moving() override { return IsMoving; }
    /// VA: 0x0075ABA0
    CoordStruct YRPP_STDCALL Destination() override { return Is_Moving() ? DestinationCoord : CoordStruct::Empty; }
    /// VA: 0x0075AC00
    CoordStruct YRPP_STDCALL Head_To_Coord() override { return HeadToCoord != CoordStruct::Empty ? HeadToCoord : LinkedTo->Location; }
    /// VA: 0x0075AC80
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Process() override { JMP_STD(0x75AC80); }
#else
    bool YRPP_STDCALL Process() override;
#endif
    /// VA: 0x0075ACB0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Move_To(CoordStruct to) override { JMP_STD(0x75ACB0); }
#else
    void YRPP_STDCALL Move_To(CoordStruct to) override;
#endif
    /// VA: 0x0075ADA0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Stop_Moving() override { JMP_STD(0x75ADA0); }
#else
    void YRPP_STDCALL Stop_Moving() override;
#endif
    /// VA: 0x0075AE00
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Do_Turn(DirStruct dir) override { JMP_STD(0x75AE00); }
#else
    void YRPP_STDCALL Do_Turn(DirStruct dir) override;
#endif
    /// VA: 0x0075C7E0
    Layer YRPP_STDCALL In_Which_Layer() override { return Layer::Ground; }
    /// VA: 0x0075AE30
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Force_Immediate_Destination(CoordStruct coord) override { JMP_STD(0x75AE30); }
#else
    void YRPP_STDCALL Force_Immediate_Destination(CoordStruct coord) override;
#endif
    /// VA: 0x0075AB40
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Now() override { JMP_STD(0x75AB40); }
#else
    bool YRPP_STDCALL Is_Moving_Now() override;
#endif
    /// VA: 0x0075CA30
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override { JMP_STD(0x75CA30); }
#else
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override;
#endif
    /// VA: 0x0075CA80
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) override { JMP_STD(0x75CA80); }
#else
    bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) override;
#endif
    /// VA: 0x0075CB20
    bool YRPP_STDCALL Is_Really_Moving_Now() override { return IsReallyMoving; }
    /// VA: 0x0075CBC0
    void YRPP_STDCALL Stop_Movement_Animation() override { IsReallyMoving = false; }
    /// VA: 0x0075CB30
    void YRPP_STDCALL Limbo() override { DestinationCoord = HeadToCoord = CoordStruct::Empty; }

    /// VA: 0x0075C850
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) override { JMP_STD(0x75C850); }
#else
    HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) override;
#endif
    /// VA: 0x0075C8A0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) override { JMP_STD(0x75C8A0); }
#else
    HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) override;
#endif
    /// VA: 0x0075C8E0
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Ok_To_End() override { JMP_STD(0x75C8E0); }
#else
    bool YRPP_STDCALL Is_Ok_To_End() override;
#endif
    /// VA: 0x0075C920
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* classid) override { JMP_STD(0x75C920); }
#else
    HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* classid) override;
#endif
    /// VA: 0x0075CBA0
    bool YRPP_STDCALL Is_Piggybacking() override { return Piggybackee != nullptr; }

    /// VA: 0x0075C240
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) bool Mark_Head_To(const CoordStruct& coord) { JMP_THIS(0x75C240); }
#else
    bool Mark_Head_To(const CoordStruct& coord);
#endif

    /// VA: 0x0075AEC0
#if defined(RA2_YRPP_GAME)
    void Movement_AI(bool firstPass) { JMP_THIS(0x75AEC0); }
#else
    void Movement_AI(bool firstPass);
#endif

    /// VA: 0x0075AA90
#if defined(RA2_YRPP_GAME)
    WalkLocomotionClass() : WalkLocomotionClass(noinit_t()) { JMP_THIS(0x75AA90); }
#else
    WalkLocomotionClass() noexcept;
#endif

protected:
    explicit __forceinline WalkLocomotionClass(noinit_t) : LocomotionClass(noinit_t()) { }

public:
    // Renamed from Destination to unhide the original virtual at ILocomotion + 0x14.
    CoordStruct DestinationCoord;
    CoordStruct HeadToCoord;
    bool IsMoving;
    bool InProcessing;
    bool IsReallyMoving;
    ILocomotion* Piggybackee;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(LocomotionClass) == 0x18);
static_assert(sizeof(WalkLocomotionClass) == 0x3C);
static_assert(offsetof(WalkLocomotionClass, DestinationCoord) == 0x1C);
static_assert(offsetof(WalkLocomotionClass, Piggybackee) == 0x38);
#endif
