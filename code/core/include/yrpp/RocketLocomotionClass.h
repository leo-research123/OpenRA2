// Locomotor = {B7B49766-E576-11D3-9BD9-00104B972FE8}
#pragma once
#include "yrpp/platform/ABI.h"
#include "yrpp/LocomotionClass.h"
struct RocketStruct;

class NOVTABLE __declspec(uuid("B7B49766-E576-11D3-9BD9-00104B972FE8")) RocketLocomotionClass : public LocomotionClass {
public:
    /// VA: 0x661F50
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving() override {
        using Entry = bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x661F50)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Is_Moving() override;
#endif
    /// VA: 0x661FB0
#if defined(RA2_YRPP_GAME)
    CoordStruct YRPP_STDCALL Destination() override {
        CoordStruct result;
        using Entry = CoordStruct*(YRPP_STDCALL*)(ILocomotion*, CoordStruct*);
        reinterpret_cast<Entry>(0x661FB0)(static_cast<ILocomotion*>(this), &result);
        return result;
    }
#else
    CoordStruct YRPP_STDCALL Destination() override;
#endif
    // Source compatibility for the historical explicit hidden-result argument.
    CoordStruct* Destination(CoordStruct* result) {
        *result = Destination();
        return result;
    }
    /// VA: 0x661F90
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Now() override {
        using Entry = bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x661F90)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Is_Moving_Now() override;
#endif
    /// VA: 0x6622C0
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Process() override {
        using Entry = bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x6622C0)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Process() override;
#endif
    /// VA: 0x6632E0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Move_To(CoordStruct to) override {
        using Entry = void(YRPP_STDCALL*)(ILocomotion*, CoordStruct);
        reinterpret_cast<Entry>(0x6632E0)(static_cast<ILocomotion*>(this), to);
    }
#else
    void YRPP_STDCALL Move_To(CoordStruct to) override;
#endif
    /// VA: 0x6633C0
    void YRPP_STDCALL Stop_Moving() override {}
    /// VA: 0x663460
    Layer YRPP_STDCALL In_Which_Layer() override { return Layer::Air; }
    /// VA: 0x663470
#if defined(RA2_YRPP_GAME)
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override {
        Matrix3D result;
        using Entry = Matrix3D*(YRPP_STDCALL*)(ILocomotion*, Matrix3D*, VoxelIndexKey*);
        reinterpret_cast<Entry>(0x663470)(static_cast<ILocomotion*>(this), &result, key);
        return result;
    }
#else
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override;
#endif
    /// VA: 0x6633D0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override { JMP_STD(0x6633D0); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override;
#endif
    /// VA: 0x663410
    HRESULT YRPP_STDCALL Load(IStream* stream) override { JMP_STD(0x663410); }
    /// VA: 0x6635B0
    int Size() override { return 0x60; }
    /// VA: 0x6635C0
    ~RocketLocomotionClass() override = default;
    /// VA: 0x661EC0
#if defined(RA2_YRPP_GAME)
    RocketLocomotionClass() : RocketLocomotionClass(noinit_t()) { JMP_THIS(0x661EC0); }
#else
    RocketLocomotionClass() noexcept;
#endif
    /// VA: 0x6620F0
#if defined(RA2_YRPP_GAME)
    bool ProcessImpact(RocketStruct* rocket) { JMP_THIS(0x6620F0); }
#else
    bool ProcessImpact(RocketStruct* rocket);
#endif
    /// VA: 0x663030
#if defined(RA2_YRPP_GAME)
    void Explode() { JMP_THIS(0x663030); }
#else
    void Explode();
#endif
protected:
    explicit __forceinline RocketLocomotionClass(noinit_t) : LocomotionClass(noinit_t()) {}

public:
    CoordStruct MovingDestination;
    RateTimer MissionTimer;
    CDTimerClass TrailerTimer;
    int MissionState;
    DWORD unknown_44;
    double CurrentSpeed;
    bool unknown_bool_4C;  // Original complete-object offset 0x50: submitted flight layer.
    bool SpawnerIsElite;
    float CurrentPitch;
    DWORD unknown_58;  // Horizontal distance at the end of the climb.
    DWORD unknown_5C;
};
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(RocketLocomotionClass) == 0x60);
static_assert(offsetof(RocketLocomotionClass, MovingDestination) == 0x18);
static_assert(offsetof(RocketLocomotionClass, MissionState) == 0x40);
static_assert(offsetof(RocketLocomotionClass, CurrentSpeed) == 0x48);
static_assert(offsetof(RocketLocomotionClass, CurrentPitch) == 0x54);
#endif
