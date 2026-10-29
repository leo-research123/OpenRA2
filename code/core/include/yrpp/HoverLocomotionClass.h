// Locomotor = {4A582742-9839-11D1-B709-00A024DDAFD1}
#pragma once
#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("4A582742-9839-11D1-B709-00A024DDAFD1")) HoverLocomotionClass : public LocomotionClass {
public:
    /// VA: 0x517070
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* output) override { JMP_STD(0x517070); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* output) override;
#endif
    /// VA: 0x513CB0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Link_To_Object(void* object) override { JMP_STD(0x513CB0); }
#else
    HRESULT YRPP_STDCALL Link_To_Object(void* object) override;
#endif
    /// VA: 0x514C30
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving() override { JMP_STD(0x514C30); }
#else
    bool YRPP_STDCALL Is_Moving() override;
#endif
    /// VA: 0x514CB0
#if defined(RA2_YRPP_GAME)
    CoordStruct YRPP_STDCALL Destination() override { JMP_STD(0x514CB0); }
#else
    CoordStruct YRPP_STDCALL Destination() override;
#endif
    /// VA: 0x514D10
#if defined(RA2_YRPP_GAME)
    CoordStruct YRPP_STDCALL Head_To_Coord() override { JMP_STD(0x514D10); }
#else
    CoordStruct YRPP_STDCALL Head_To_Coord() override;
#endif
    /// VA: 0x513F40
#if defined(RA2_YRPP_GAME)
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override { JMP_STD(0x513F40); }
#else
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override;
#endif
    /// VA: 0x5142A0
#if defined(RA2_YRPP_GAME)
    Matrix3D YRPP_STDCALL Shadow_Matrix(VoxelIndexKey* key) override { JMP_STD(0x5142A0); }
#else
    Matrix3D YRPP_STDCALL Shadow_Matrix(VoxelIndexKey* key) override;
#endif
    /// VA: 0x514310
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Process() override { JMP_STD(0x514310); }
#else
    bool YRPP_STDCALL Process() override;
#endif
    /// VA: 0x514D90
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Move_To(CoordStruct to) override { JMP_STD(0x514D90); }
#else
    void YRPP_STDCALL Move_To(CoordStruct to) override;
#endif
    /// VA: 0x516320
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Stop_Moving() override { JMP_STD(0x516320); }
#else
    void YRPP_STDCALL Stop_Moving() override;
#endif
    /// VA: 0x516370
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Do_Turn(DirStruct direction) override { JMP_STD(0x516370); }
#else
    void YRPP_STDCALL Do_Turn(DirStruct direction) override;
#endif
    /// VA: 0x516BF0
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Power_Off() override { JMP_STD(0x516BF0); }
#else
    bool YRPP_STDCALL Power_Off() override;
#endif
    /// VA: 0x516C70
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Powered() override { JMP_STD(0x516C70); }
#else
    bool YRPP_STDCALL Is_Powered() override;
#endif
    /// VA: 0x516CA0
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Ion_Sensitive() override { JMP_STD(0x516CA0); }
#else
    bool YRPP_STDCALL Is_Ion_Sensitive() override;
#endif
    /// VA: 0x516E10
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Push(DirStruct direction) override { JMP_STD(0x516E10); }
#else
    bool YRPP_STDCALL Push(DirStruct direction) override;
#endif
    /// VA: 0x516FC0
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Shove(DirStruct direction) override { JMP_STD(0x516FC0); }
#else
    bool YRPP_STDCALL Shove(DirStruct direction) override;
#endif
    /// VA: 0x517100
#if defined(RA2_YRPP_GAME)
    Layer YRPP_STDCALL In_Which_Layer() override { JMP_STD(0x517100); }
#else
    Layer YRPP_STDCALL In_Which_Layer() override;
#endif
    /// VA: 0x514C80
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Now() override { JMP_STD(0x514C80); }
#else
    bool YRPP_STDCALL Is_Moving_Now() override;
#endif
    /// VA: 0x5171C0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override { JMP_STD(0x5171C0); }
#else
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override;
#endif
    /// VA: 0x517210
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) override { JMP_STD(0x517210); }
#else
    bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) override;
#endif
    /// VA: 0x513D20
#if defined(RA2_YRPP_GAME)
    void Gravity_AI() { JMP_THIS(0x513D20); }
#else
    void Gravity_AI();
#endif
    /// VA: 0x515ED0
#if defined(RA2_YRPP_GAME)
    void Motion_AI() { JMP_THIS(0x515ED0); }
#else
    void Motion_AI();
#endif
    /// VA: 0x514F70
#if defined(RA2_YRPP_GAME)
    Move While_Moving(bool firstPass) { JMP_THIS(0x514F70); }
#else
    Move While_Moving(bool firstPass);
#endif
    /// VA: 0x5163B0
#if defined(RA2_YRPP_GAME)
    bool Reduce_Path_Length() { JMP_THIS(0x5163B0); }
#else
    bool Reduce_Path_Length();
#endif
    /// VA: 0x5164D0
#if defined(RA2_YRPP_GAME)
    void Start_Of_Move(int mode) { JMP_THIS(0x5164D0); }
#else
    void Start_Of_Move(int mode);
#endif
    /// VA: 0x517110
#if defined(RA2_YRPP_GAME)
    void Start() { JMP_THIS(0x517110); }
#else
    void Start();
#endif
    // Retain the historical entry names; native logic uses the typed methods.
    /// VA: 0x514F70
    void sub_514F70(bool arg) {
#if defined(RA2_YRPP_GAME)
        JMP_THIS(0x514F70);
#else
        (void)While_Moving(arg);
#endif
    }
    /// VA: 0x5164D0
    void sub_5164D0(bool arg) {
#if defined(RA2_YRPP_GAME)
        JMP_THIS(0x5164D0);
#else
        Start_Of_Move(int(arg));
#endif
    }
    /// VA: 0x5170B0
    HRESULT YRPP_STDCALL Load(IStream* stream) override { JMP_STD(0x5170B0); }
    /// VA: 0x5172B0
    int Size() override { return 0x78; }
    /// VA: 0x513C20
#if defined(RA2_YRPP_GAME)
    HoverLocomotionClass() : LocomotionClass(noinit_t()) { JMP_THIS(0x513C20); }
#else
    HoverLocomotionClass() noexcept;
#endif
    /// VA: 0x513D00
#if defined(RA2_YRPP_GAME)
    ~HoverLocomotionClass() override { JMP_THIS(0x513D00); }
#else
    ~HoverLocomotionClass() override;
#endif
protected:
    explicit __forceinline HoverLocomotionClass(noinit_t) : LocomotionClass(noinit_t()) {}

private:
    // Inlined original Hover operations; these introduce no object state.
    void Stop();
    void Stop_Driver();
    void Do_Shove();

public:
    // Renamed only to allow the original Destination() virtual override.
    CoordStruct DestinationCoord;
    CoordStruct HeadToCoord;
    FacingClass LocomotionFacing;
    double MaxSpeed;
    double CurrentSpeed;
    double BoostSpeed;
    double CurrentWobbles;
    bool unknown_bool_68;
    int unknown_int_6C;
    bool unknown_bool_70;
};
