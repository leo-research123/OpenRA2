// Locomotor = {4A582746-9839-11D1-B709-00A024DDAFD1}
#pragma once
#include "yrpp/platform/ABI.h"
#include "yrpp/LocomotionClass.h"
class NOVTABLE __declspec(uuid("4A582746-9839-11D1-B709-00A024DDAFD1")) FlyLocomotionClass : public LocomotionClass {
public:
    // Original COM entries receive the ILocomotion subobject (complete object + 0x4).
    // Structure results follow that receiver on the stack. Preserve both when
    // making a qualified call; an ordinary virtual call uses the game vtable.
    /// VA: 0x004CCA20
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Link_To_Object(void* object) override {
        using Entry=HRESULT(YRPP_STDCALL*)(ILocomotion*,void*);
        return reinterpret_cast<Entry>(0x4CCA20)(static_cast<ILocomotion*>(this),object);
    }
#else
    HRESULT YRPP_STDCALL Link_To_Object(void* object) override;
#endif
    /// VA: 0x004CCA90
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving() override {
        using Entry=bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x4CCA90)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Is_Moving() override;
#endif
    /// VA: 0x004CCAE0
#if defined(RA2_YRPP_GAME)
    CoordStruct YRPP_STDCALL Destination() override {
        CoordStruct result;
        using Entry=CoordStruct*(YRPP_STDCALL*)(ILocomotion*,CoordStruct*);
        reinterpret_cast<Entry>(0x4CCAE0)(static_cast<ILocomotion*>(this),&result);return result;
    }
#else
    CoordStruct YRPP_STDCALL Destination() override;
#endif
    /// VA: 0x004CCAC0
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Now() override {
        using Entry=bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x4CCAC0)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Is_Moving_Now() override;
#endif
    /// VA: 0x004CCB40
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Process() override {
        using Entry=bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x4CCB40)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Process() override;
#endif
    /// VA: 0x004CCC80
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Move_To(CoordStruct to) override {
        using Entry=void(YRPP_STDCALL*)(ILocomotion*,CoordStruct);
        return reinterpret_cast<Entry>(0x4CCC80)(static_cast<ILocomotion*>(this),to);
    }
#else
    void YRPP_STDCALL Move_To(CoordStruct to) override;
#endif
    /// VA: 0x004CCFD0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Stop_Moving() override {
        using Entry=void(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x4CCFD0)(static_cast<ILocomotion*>(this));
    }
#else
    void YRPP_STDCALL Stop_Moving() override;
#endif
    /// VA: 0x004CFC10
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Do_Turn(DirStruct dir) override {
        using Entry=void(YRPP_STDCALL*)(ILocomotion*,DirStruct);
        return reinterpret_cast<Entry>(0x4CFC10)(static_cast<ILocomotion*>(this),dir);
    }
#else
    void YRPP_STDCALL Do_Turn(DirStruct dir) override;
#endif
    /// VA: 0x004CFCF0
#if defined(RA2_YRPP_GAME)
    Layer YRPP_STDCALL In_Which_Layer() override {
        using Entry=Layer(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x4CFCF0)(static_cast<ILocomotion*>(this));
    }
#else
    Layer YRPP_STDCALL In_Which_Layer() override;
#endif
    /// VA: 0x004B6620
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override {
        using Entry=void(YRPP_STDCALL*)(ILocomotion*,MarkType);
        return reinterpret_cast<Entry>(0x4B6620)(static_cast<ILocomotion*>(this),mark);
    }
#else
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override;
#endif
    /// VA: 0x004B4CA0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Limbo() override {
        using Entry=void(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x4B4CA0)(static_cast<ILocomotion*>(this));
    }
#else
    void YRPP_STDCALL Limbo() override;
#endif
    /// VA: 0x004CF610
#if defined(RA2_YRPP_GAME)
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override {
        Matrix3D result;
        using Entry=Matrix3D*(YRPP_STDCALL*)(ILocomotion*,Matrix3D*,VoxelIndexKey*);
        reinterpret_cast<Entry>(0x4CF610)(static_cast<ILocomotion*>(this),&result,key);return result;
    }
#else
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override;
#endif
    /// VA: 0x004CFB00
#if defined(RA2_YRPP_GAME)
    Matrix3D YRPP_STDCALL Shadow_Matrix(VoxelIndexKey* key) override {
        Matrix3D result;
        using Entry=Matrix3D*(YRPP_STDCALL*)(ILocomotion*,Matrix3D*,VoxelIndexKey*);
        reinterpret_cast<Entry>(0x4CFB00)(static_cast<ILocomotion*>(this),&result,key);return result;
    }
#else
    Matrix3D YRPP_STDCALL Shadow_Matrix(VoxelIndexKey* key) override;
#endif
    /// VA: 0x004CF830
#if defined(RA2_YRPP_GAME)
    Point2D YRPP_STDCALL Draw_Point() override {
        Point2D result;
        using Entry=Point2D*(YRPP_STDCALL*)(ILocomotion*,Point2D*);
        reinterpret_cast<Entry>(0x4CF830)(static_cast<ILocomotion*>(this),&result);return result;
    }
#else
    Point2D YRPP_STDCALL Draw_Point() override;
#endif
    /// VA: 0x004CF940
#if defined(RA2_YRPP_GAME)
    Point2D YRPP_STDCALL Shadow_Point() override {
        Point2D result;
        using Entry=Point2D*(YRPP_STDCALL*)(ILocomotion*,Point2D*);
        reinterpret_cast<Entry>(0x4CF940)(static_cast<ILocomotion*>(this),&result);return result;
    }
#else
    Point2D YRPP_STDCALL Shadow_Point() override;
#endif
    /// VA: 0x004CFC80
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* output) override { JMP_STD(0x4CFC80); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* output) override;
#endif
    /// VA: 0x004D0390
#if defined(RA2_YRPP_GAME)
    int Size() override { JMP_THIS(0x4D0390); }
#else
    int Size() override;
#endif
    /// VA: 0x004CFE20
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Apparent_Speed() override {
        using Entry=int(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x4CFE20)(static_cast<ILocomotion*>(this));
    }
#else
    int YRPP_STDCALL Apparent_Speed() override;
#endif
    /// VA: 0x004CFCC0
    HRESULT YRPP_STDCALL Load(IStream* stream) override { JMP_STD(0x4CFCC0); }
    /// VA: 0x0055AA60
    HRESULT YRPP_STDCALL Save(IStream* stream,BOOL clearDirty) override { JMP_STD(0x55AA60); }
    /// VA: 0x004D03A0
    ~FlyLocomotionClass() override = default;
    /// VA: 0x004CC9A0
#if defined(RA2_YRPP_GAME)
    FlyLocomotionClass():LocomotionClass(noinit_t{}) { JMP_THIS(0x4CC9A0); }
#else
    FlyLocomotionClass();
#endif

    bool AirportBound;
    CoordStruct MovingDestination;
    CoordStruct XYZ2;
    bool HasMoveOrder;
    int FlightLevel;
    double TargetSpeed;
    double CurrentSpeed;
    char IsTakingOff;
    bool IsLanding;
    bool WasLanding;
    bool unknown_bool_53;
    DWORD unknown_54;
    DWORD unknown_58;
    bool IsElevating;
    bool unknown_bool_5D;
    bool unknown_bool_5E;
    bool unknown_bool_5F;
};
#if UINTPTR_MAX == UINT32_MAX
static_assert(sizeof(FlyLocomotionClass)==0x60);
#endif
