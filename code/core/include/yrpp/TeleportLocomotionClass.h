// Locomotor = {4A582747-9839-11d1-B709-00A024DDAFD1}

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("4A582747-9839-11D1-B709-00A024DDAFD1")) TeleportLocomotionClass
    : public LocomotionClass,
      public IPiggyback {
public:
    // IUnknown
    /// VA: 0x719E30
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject) override;
    /// VA: 0x71A0E0
    ULONG YRPP_STDCALL AddRef() override;
    /// VA: 0x71A0F0
    ULONG YRPP_STDCALL Release() override;

    // IPiggyback
    /// VA: 0x719E90
    HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) override;
    /// VA: 0x719EE0
    HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) override;
    /// VA: 0x719F30
    bool YRPP_STDCALL Is_Ok_To_End() override;
    /// VA: 0x719F80
    HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* classid) override;
    /// VA: 0x71A100
    bool YRPP_STDCALL Is_Piggybacking() override;

    // ILocomotion
    /// VA: 0x718080
    bool YRPP_STDCALL Is_Moving() override;
    /// VA: 0x7180A0
    CoordStruct YRPP_STDCALL Destination() override;
    /// VA: 0x7192F0
    bool YRPP_STDCALL Process() override;
    /// VA: 0x718100
    void YRPP_STDCALL Move_To(CoordStruct to) override;
    /// VA: 0x718230
    void YRPP_STDCALL Stop_Moving() override;
    /// VA: 0x7192C0
    void YRPP_STDCALL Do_Turn(DirStruct coord) override;
    /// VA: 0x719E20
    Layer YRPP_STDCALL In_Which_Layer() override;
    /// VA: 0x71A090
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override;

    // IPersist
    /// VA: 0x719C60
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x719CA0
    HRESULT YRPP_STDCALL Load(IStream* pStm) override;
    /// VA: 0x719D40
    HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override;

    // Destructor
    /// VA: 0x71A130
    ~TeleportLocomotionClass() override;

    // LocomotionClass
    /// VA: 0x71A120
    int Size() override { return sizeof(*this); }

    // TeleportLocomotionClass
    /// VA: 0x719BF0
    virtual void YRPP_STDCALL ProcessChronoWait();
    // Source compatibility for the historical placeholder; not a virtual ABI entry.
    void vt_entry_28(DWORD) { ProcessChronoWait(); }
    /// VA: 0x718090
    virtual bool IsStill();

    // Constructor
    /// VA: 0x718000
    TeleportLocomotionClass() noexcept;

protected:
    explicit __forceinline TeleportLocomotionClass(noinit_t) : LocomotionClass(noinit_t()) {}

private:
    /// VA: 0x718B70
    bool SelectChronoMoveDestination(const CoordStruct& destination);
    /// VA: 0x718260
    bool PrepareChronoDestination(CoordStruct destination, bool force);

public:
    // Original landing consequences, without starting the warp state machine.
    // Native callers contain recoverable exceptions at their operation boundary.
    /// VA: 0x7187A0
    void CheckChronoLanding(CoordStruct destination);

    // Properties

public:
    CoordStruct MovingDestination;  //Current destination
    CoordStruct LastCoords;         //Marked occupation bits there
    bool Moving;                    //Is currently moving
    bool unknown_bool_35;
    bool unknown_bool_36;
    int State;
    CDTimerClass Timer;
    ILocomotion* Piggybackee;
};
