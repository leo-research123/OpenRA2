// Original YR Jumpjet locomotor; three COM subobjects and original field layout.
#pragma once
#include "yrpp/platform/ABI.h"
#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("92612C46-F71F-11D1-AC9F-006008055BB5")) JumpjetLocomotionClass : public LocomotionClass,
                                                                                                 public IPiggyback {
public:
    enum State { Grounded, Ascending, Hovering, Cruising, Descending, Crashing, Unknown };
    /// VA: 0x54DC60
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** output) override { JMP_STD(0x54DC60); }
#else
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** output) override;
#endif
    /// VA: 0x54B710
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override { JMP_STD(0x54B710); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override;
#endif
    /// VA: 0x54AD30
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Link_To_Object(void* object) override { JMP_STD(0x54AD30); }
#else
    HRESULT YRPP_STDCALL Link_To_Object(void* object) override;
#endif
    /// VA: 0x54AE50
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving() override { JMP_STD(0x54AE50); }
#else
    bool YRPP_STDCALL Is_Moving() override;
#endif
    /// VA: 0x54AE60
#if defined(RA2_YRPP_GAME)
    CoordStruct YRPP_STDCALL Destination() override { JMP_STD(0x54AE60); }
#else
    CoordStruct YRPP_STDCALL Destination() override;
#endif
    /// VA: 0x54D9B0
#if defined(RA2_YRPP_GAME)
    CoordStruct YRPP_STDCALL Head_To_Coord() override { JMP_STD(0x54D9B0); }
#else
    CoordStruct YRPP_STDCALL Head_To_Coord() override;
#endif
    /// VA: 0x54AEC0
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Process() override { JMP_STD(0x54AEC0); }
#else
    bool YRPP_STDCALL Process() override;
#endif
    /// VA: 0x54B1C0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Move_To(CoordStruct to) override { JMP_STD(0x54B1C0); }
#else
    void YRPP_STDCALL Move_To(CoordStruct to) override;
#endif
    /// VA: 0x54B4D0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Stop_Moving() override { JMP_STD(0x54B4D0); }
#else
    void YRPP_STDCALL Stop_Moving() override;
#endif
    /// VA: 0x54B6E0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Do_Turn(DirStruct dir) override { JMP_STD(0x54B6E0); }
#else
    void YRPP_STDCALL Do_Turn(DirStruct dir) override;
#endif
    /// VA: 0x54B8D0
#if defined(RA2_YRPP_GAME)
    Layer YRPP_STDCALL In_Which_Layer() override { JMP_STD(0x54B8D0); }
#else
    Layer YRPP_STDCALL In_Which_Layer() override;
#endif
    /// VA: 0x54D0D0
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Now() override { JMP_STD(0x54D0D0); }
#else
    bool YRPP_STDCALL Is_Moving_Now() override;
#endif
    /// VA: 0x54D930
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override { JMP_STD(0x54D930); }
#else
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override;
#endif
    /// VA: 0x54DCC0
#if defined(RA2_YRPP_GAME)
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override { JMP_STD(0x54DCC0); }
#else
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override;
#endif
    /// VA: 0x54DA00
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) override { JMP_STD(0x54DA00); }
#else
    HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) override;
#endif
    /// VA: 0x54DA50
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) override { JMP_STD(0x54DA50); }
#else
    HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) override;
#endif
    /// VA: 0x54DB00
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Ok_To_End() override { JMP_STD(0x54DB00); }
#else
    bool YRPP_STDCALL Is_Ok_To_End() override;
#endif
    /// VA: 0x54DB50
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* id) override { JMP_STD(0x54DB50); }
#else
    HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* id) override;
#endif
    /// VA: 0x54DF50
    ULONG YRPP_STDCALL AddRef() override { return LocomotionClass::AddRef(); }
    /// VA: 0x54DF60
    ULONG YRPP_STDCALL Release() override { return LocomotionClass::Release(); }
    /// VA: 0x54DF70
    bool YRPP_STDCALL Is_Piggybacking() override { return Piggybackee != nullptr; }
    /// VA: 0x55AC00
    void YRPP_STDCALL Limbo() override {}
    /// VA: 0x54B750
    HRESULT YRPP_STDCALL Load(IStream* stream) override { JMP_STD(0x54B750); }
    /// VA: 0x54B7E0
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clear) override { JMP_STD(0x54B7E0); }
    /// VA: 0x54DF90
    int Size() override { return 0x98; }
    /// VA: 0x54DFA0
#if defined(RA2_YRPP_GAME)
    ~JumpjetLocomotionClass() override {
        if (Piggybackee) Piggybackee->Release();
    }
#else
    ~JumpjetLocomotionClass() override;
#endif
    /// VA: 0x54AC40
#if defined(RA2_YRPP_GAME)
    JumpjetLocomotionClass() : LocomotionClass(noinit_t()) { JMP_THIS(0x54AC40); }
#else
    JumpjetLocomotionClass() noexcept;
#endif
    // Source compatibility for the old explicit hidden-result declaration.
    CoordStruct* Destination(CoordStruct* output) {
        *output = Destination();
        return output;
    }
    /// VA: 0x54B980
#if defined(RA2_YRPP_GAME)
    void ProcessGrounded() { JMP_THIS(0x54B980); }
#else
    void ProcessGrounded();
#endif
    /// VA: 0x54BA30
#if defined(RA2_YRPP_GAME)
    void ProcessAscending() { JMP_THIS(0x54BA30); }
#else
    void ProcessAscending();
#endif
    /// VA: 0x54BD30
#if defined(RA2_YRPP_GAME)
    void ProcessHovering() { JMP_THIS(0x54BD30); }
#else
    void ProcessHovering();
#endif
    /// VA: 0x54BFF0
#if defined(RA2_YRPP_GAME)
    void ProcessCruising() { JMP_THIS(0x54BFF0); }
#else
    void ProcessCruising();
#endif
    /// VA: 0x54C550
#if defined(RA2_YRPP_GAME)
    void ProcessDescending() { JMP_THIS(0x54C550); }
#else
    void ProcessDescending();
#endif
    /// VA: 0x54CA90
#if defined(RA2_YRPP_GAME)
    void ProcessCrashing() { JMP_THIS(0x54CA90); }
#else
    void ProcessCrashing();
#endif
    /// VA: 0x54D0F0
#if defined(RA2_YRPP_GAME)
    void MovementAI() { JMP_THIS(0x54D0F0); }
#else
    void MovementAI();
#endif
    /// VA: 0x54D6D0
#if defined(RA2_YRPP_GAME)
    CoordStruct LandingCoord(const CoordStruct& at) { JMP_THIS(0x54D6D0); }
#else
    CoordStruct LandingCoord(const CoordStruct& at);
#endif
    /// VA: 0x54D820
#if defined(RA2_YRPP_GAME)
    int FlightFloor() { JMP_THIS(0x54D820); }
#else
    int FlightFloor();
#endif
protected:
    explicit __forceinline JumpjetLocomotionClass(noinit_t) : LocomotionClass(noinit_t()) {}
    // Properties

public:
    int TurnRate;
    int Speed;
    float Climb;
    float Crash;
    int Height;
    float Accel;
    float Wobbles;
    int Deviation;
    bool NoWobbles;
    BYTE unknown_3D;
    BYTE unknown_3E;
    BYTE unknown_3F;
    CoordStruct DestinationCoords;
    bool IsMoving;
    BYTE unknown_4D;
    BYTE unknown_4E;
    BYTE unknown_4F;
    JumpjetLocomotionClass::State State;
    FacingClass LocomotionFacing;
    BYTE unknown_6C;
    BYTE unknown_6D;
    BYTE unknown_6E;
    BYTE unknown_6F;
    double CurrentSpeed;
    double MaxSpeed;
    int CurrentHeight;
    BYTE unknown_84;
    BYTE unknown_85;
    BYTE unknown_86;
    BYTE unknown_87;
    double CurrentWobbles;
    bool DestinationReached;
    BYTE unknown_91;
    BYTE unknown_92;
    BYTE unknown_93;
    ILocomotion* Piggybackee;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(JumpjetLocomotionClass) == 0x98);
static_assert(offsetof(JumpjetLocomotionClass, TurnRate) == 0x1C);
static_assert(offsetof(JumpjetLocomotionClass, DestinationCoords) == 0x40);
static_assert(offsetof(JumpjetLocomotionClass, State) == 0x50);
static_assert(offsetof(JumpjetLocomotionClass, CurrentSpeed) == 0x70);
static_assert(offsetof(JumpjetLocomotionClass, Piggybackee) == 0x94);
#endif
