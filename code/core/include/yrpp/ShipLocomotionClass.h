// Locomotor = {2BEA74E1-7CCA-11d3-BE14-00104B62A16C}
#pragma once
#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("2BEA74E1-7CCA-11D3-BE14-00104B62A16C")) ShipLocomotionClass : public LocomotionClass,
                                                                                              public IPiggyback {
public:
    /// Global VA: 0x7F2960
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(const RawTrackType, [14], RawTrack, 0x7F2960)
#else
    static const RawTrackType (&RawTrack)[14];
#endif
    /// Global VA: 0x7F2A40
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(const TurnTrackType, [67], TurnTrack, 0x7F2A40)
#else
    static const TurnTrackType (&TurnTrack)[67];
#endif
    /// VA: 0x69EE30
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** output) override {
        using Entry = HRESULT(YRPP_STDCALL*)(IPersistStream*, REFIID, void**);
        return reinterpret_cast<Entry>(0x69EE30)(static_cast<IPersistStream*>(this), iid, output);
    }
#else
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** output) override;
#endif
    /// VA: 0x6A4260
#if defined(RA2_YRPP_GAME)
    ULONG YRPP_STDCALL AddRef() override {
        using Entry = ULONG(YRPP_STDCALL*)(IPersistStream*);
        return reinterpret_cast<Entry>(0x6A4260)(static_cast<IPersistStream*>(this));
    }
#else
    ULONG YRPP_STDCALL AddRef() override;
#endif
    /// VA: 0x6A4270
#if defined(RA2_YRPP_GAME)
    ULONG YRPP_STDCALL Release() override {
        using Entry = ULONG(YRPP_STDCALL*)(IPersistStream*);
        return reinterpret_cast<Entry>(0x6A4270)(static_cast<IPersistStream*>(this));
    }
#else
    ULONG YRPP_STDCALL Release() override;
#endif
    /// VA: 0x6A3E60
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* output) override {
        using Entry = HRESULT(YRPP_STDCALL*)(IPersistStream*, CLSID*);
        return reinterpret_cast<Entry>(0x6A3E60)(static_cast<IPersistStream*>(this), output);
    }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* output) override;
#endif
    /// VA: 0x69EFF0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) override {
        using Entry = HRESULT(YRPP_STDCALL*)(IPiggyback*, ILocomotion*);
        return reinterpret_cast<Entry>(0x69EFF0)(static_cast<IPiggyback*>(this), pointer);
    }
#else
    HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) override;
#endif
    /// VA: 0x69F040
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) override {
        using Entry = HRESULT(YRPP_STDCALL*)(IPiggyback*, ILocomotion**);
        return reinterpret_cast<Entry>(0x69F040)(static_cast<IPiggyback*>(this), pointer);
    }
#else
    HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) override;
#endif
    /// VA: 0x69F080
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Ok_To_End() override {
        using Entry = bool(YRPP_STDCALL*)(IPiggyback*);
        return reinterpret_cast<Entry>(0x69F080)(static_cast<IPiggyback*>(this));
    }
#else
    bool YRPP_STDCALL Is_Ok_To_End() override;
#endif
    /// VA: 0x69ED20
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* id) override {
        using Entry = HRESULT(YRPP_STDCALL*)(IPiggyback*, GUID*);
        return reinterpret_cast<Entry>(0x69ED20)(static_cast<IPiggyback*>(this), id);
    }
#else
    HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* id) override;
#endif
    /// VA: 0x6A4280
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Piggybacking() override {
        using Entry = bool(YRPP_STDCALL*)(IPiggyback*);
        return reinterpret_cast<Entry>(0x6A4280)(static_cast<IPiggyback*>(this));
    }
#else
    bool YRPP_STDCALL Is_Piggybacking() override;
#endif
    /// VA: 0x69F290
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving() override {
        using Entry = bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x69F290)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Is_Moving() override;
#endif
    /// VA: 0x69F330
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Now() override {
        using Entry = bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x69F330)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Is_Moving_Now() override;
#endif
    /// VA: 0x4B4C50
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Really_Moving_Now() override {
        using Entry = bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x4B4C50)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Is_Really_Moving_Now() override;
#endif
    /// VA: 0x69F3A0
#if defined(RA2_YRPP_GAME)
    CoordStruct YRPP_STDCALL Destination() override {
        CoordStruct result;
        using Entry = CoordStruct*(YRPP_STDCALL*)(ILocomotion*, CoordStruct*);
        reinterpret_cast<Entry>(0x69F3A0)(static_cast<ILocomotion*>(this), &result);
        return result;
    }
#else
    CoordStruct YRPP_STDCALL Destination() override;
#endif
    /// VA: 0x69F3D0
#if defined(RA2_YRPP_GAME)
    CoordStruct YRPP_STDCALL Head_To_Coord() override {
        CoordStruct result;
        using Entry = CoordStruct*(YRPP_STDCALL*)(ILocomotion*, CoordStruct*);
        reinterpret_cast<Entry>(0x69F3D0)(static_cast<ILocomotion*>(this), &result);
        return result;
    }
#else
    CoordStruct YRPP_STDCALL Head_To_Coord() override;
#endif
    /// VA: 0x69F450
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Move_To(CoordStruct to) override {
        using Entry = void(YRPP_STDCALL*)(ILocomotion*, CoordStruct);
        reinterpret_cast<Entry>(0x69F450)(static_cast<ILocomotion*>(this), to);
    }
#else
    void YRPP_STDCALL Move_To(CoordStruct to) override;
#endif
    /// VA: 0x69F510
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Stop_Moving() override {
        using Entry = void(YRPP_STDCALL*)(ILocomotion*);
        reinterpret_cast<Entry>(0x69F510)(static_cast<ILocomotion*>(this));
    }
#else
    void YRPP_STDCALL Stop_Moving() override;
#endif
    /// VA: 0x6A05C0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Do_Turn(DirStruct dir) override {
        using Entry = void(YRPP_STDCALL*)(ILocomotion*, DirStruct);
        reinterpret_cast<Entry>(0x6A05C0)(static_cast<ILocomotion*>(this), dir);
    }
#else
    void YRPP_STDCALL Do_Turn(DirStruct dir) override;
#endif
    /// VA: 0x69FBE0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Unlimbo() override {
        using Entry = void(YRPP_STDCALL*)(ILocomotion*);
        reinterpret_cast<Entry>(0x69FBE0)(static_cast<ILocomotion*>(this));
    }
#else
    void YRPP_STDCALL Unlimbo() override;
#endif
    /// VA: 0x69F250
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Force_New_Slope(int ramp) override {
        using Entry = void(YRPP_STDCALL*)(ILocomotion*, int);
        reinterpret_cast<Entry>(0x69F250)(static_cast<ILocomotion*>(this), ramp);
    }
#else
    void YRPP_STDCALL Force_New_Slope(int ramp) override;
#endif
    /// VA: 0x6A3E50
#if defined(RA2_YRPP_GAME)
    Layer YRPP_STDCALL In_Which_Layer() override {
        using Entry = Layer(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x6A3E50)(static_cast<ILocomotion*>(this));
    }
#else
    Layer YRPP_STDCALL In_Which_Layer() override;
#endif
    /// VA: 0x6A3EA0
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Z_Adjust() override {
        using Entry = int(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x6A3EA0)(static_cast<ILocomotion*>(this));
    }
#else
    int YRPP_STDCALL Z_Adjust() override;
#endif
    /// VA: 0x6A3EB0
#if defined(RA2_YRPP_GAME)
    ZGradient YRPP_STDCALL Z_Gradient() override {
        using Entry = ZGradient(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x6A3EB0)(static_cast<ILocomotion*>(this));
    }
#else
    ZGradient YRPP_STDCALL Z_Gradient() override;
#endif
    /// VA: 0x6A4210
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Lock() override {
        using Entry = void(YRPP_STDCALL*)(ILocomotion*);
        reinterpret_cast<Entry>(0x6A4210)(static_cast<ILocomotion*>(this));
    }
#else
    void YRPP_STDCALL Lock() override;
#endif
    /// VA: 0x6A4220
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Unlock() override {
        using Entry = void(YRPP_STDCALL*)(ILocomotion*);
        reinterpret_cast<Entry>(0x6A4220)(static_cast<ILocomotion*>(this));
    }
#else
    void YRPP_STDCALL Unlock() override;
#endif
    /// VA: 0x6A4230
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Get_Track_Number() override {
        using Entry = int(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x6A4230)(static_cast<ILocomotion*>(this));
    }
#else
    int YRPP_STDCALL Get_Track_Number() override;
#endif
    /// VA: 0x6A4240
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Get_Track_Index() override {
        using Entry = int(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x6A4240)(static_cast<ILocomotion*>(this));
    }
#else
    int YRPP_STDCALL Get_Track_Index() override;
#endif
    /// VA: 0x6A4250
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Get_Speed_Accum() override {
        using Entry = int(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x6A4250)(static_cast<ILocomotion*>(this));
    }
#else
    int YRPP_STDCALL Get_Speed_Accum() override;
#endif
    /// VA: 0x6A0310
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Force_Track(int track, CoordStruct coord) override {
        using Entry = void(YRPP_STDCALL*)(ILocomotion*, int, CoordStruct);
        reinterpret_cast<Entry>(0x6A0310)(static_cast<ILocomotion*>(this), track, coord);
    }
#else
    void YRPP_STDCALL Force_Track(int track, CoordStruct coord) override;
#endif
    /// VA: 0x6A3F00
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override {
        using Entry = void(YRPP_STDCALL*)(ILocomotion*, MarkType);
        reinterpret_cast<Entry>(0x6A3F00)(static_cast<ILocomotion*>(this), mark);
    }
#else
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override;
#endif
    /// VA: 0x6A3F50
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) override {
        using Entry = bool(YRPP_STDCALL*)(ILocomotion*, CoordStruct);
        return reinterpret_cast<Entry>(0x6A3F50)(static_cast<ILocomotion*>(this), to);
    }
#else
    bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) override;
#endif
    /// VA: 0x6A4130
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Will_Jump_Tracks() override {
        using Entry = bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x6A4130)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Will_Jump_Tracks() override;
#endif
    /// VA: 0x6A3DB0
#if defined(RA2_YRPP_GAME)
    Point2D Smooth_Turn(const Point2D& offset, int& direction) {
        Point2D result;
        using Entry = Point2D*(YRPP_THISCALL*)(ShipLocomotionClass*, Point2D*, const Point2D*, int*);
        reinterpret_cast<Entry>(0x6A3DB0)(this, &result, &offset, &direction);
        return result;
    }
#else
    Point2D Smooth_Turn(const Point2D& offset, int& direction);
#endif
    /// VA: 0x6A01A0
#if defined(RA2_YRPP_GAME)
    void Mark_Track(const CoordStruct& head, MarkType mark) {
        using Entry = void(YRPP_THISCALL*)(ShipLocomotionClass*, const CoordStruct*, MarkType);
        reinterpret_cast<Entry>(0x6A01A0)(this, &head, mark);
    }
#else
    void Mark_Track(const CoordStruct& head, MarkType mark);
#endif
    /// VA: 0x6A04C0
#if defined(RA2_YRPP_GAME)
    bool Start_Driver(const CoordStruct& head) {
        using Entry = bool(YRPP_THISCALL*)(ShipLocomotionClass*, const CoordStruct*);
        return reinterpret_cast<Entry>(0x6A04C0)(this, &head);
    }
#else
    bool Start_Driver(const CoordStruct& head);
#endif
    /// VA: 0x69F0C0
#if defined(RA2_YRPP_GAME)
    Matrix3D Slope_Matrix() {
        Matrix3D result;
        using Entry = Matrix3D*(YRPP_THISCALL*)(ShipLocomotionClass*, Matrix3D*);
        reinterpret_cast<Entry>(0x69F0C0)(this, &result);
        return result;
    }
#else
    Matrix3D Slope_Matrix();
#endif
    /// VA: 0x69F1B0
#if defined(RA2_YRPP_GAME)
    double Slope_Ratio() {
        using Entry = double(YRPP_THISCALL*)(ShipLocomotionClass*);
        return reinterpret_cast<Entry>(0x69F1B0)(this);
    }
#else
    double Slope_Ratio();
#endif
    /// VA: 0x69F5E0
#if defined(RA2_YRPP_GAME)
    BOOL Is_Stable() {
        using Entry = BOOL(YRPP_THISCALL*)(ShipLocomotionClass*);
        return reinterpret_cast<Entry>(0x69F5E0)(this);
    }
#else
    BOOL Is_Stable();
#endif
    /// VA: 0x69F670
#if defined(RA2_YRPP_GAME)
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override {
        Matrix3D result;
        using Entry = Matrix3D*(YRPP_STDCALL*)(ILocomotion*, Matrix3D*, VoxelIndexKey*);
        reinterpret_cast<Entry>(0x69F670)(static_cast<ILocomotion*>(this), &result, key);
        return result;
    }
#else
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override;
#endif
    /// VA: 0x69FB20
#if defined(RA2_YRPP_GAME)
    Matrix3D YRPP_STDCALL Shadow_Matrix(VoxelIndexKey* key) override {
        Matrix3D result;
        using Entry = Matrix3D*(YRPP_STDCALL*)(ILocomotion*, Matrix3D*, VoxelIndexKey*);
        reinterpret_cast<Entry>(0x69FB20)(static_cast<ILocomotion*>(this), &result, key);
        return result;
    }
#else
    Matrix3D YRPP_STDCALL Shadow_Matrix(VoxelIndexKey* key) override;
#endif
    /// VA: 0x69FC10
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Process() override {
        using Entry = bool(YRPP_STDCALL*)(ILocomotion*);
        return reinterpret_cast<Entry>(0x69FC10)(static_cast<ILocomotion*>(this));
    }
#else
    bool YRPP_STDCALL Process() override;
#endif
    /// VA: 0x6A05F0
#if defined(RA2_YRPP_GAME)
    bool While_Moving(bool justStarted = false) {
        using Entry = bool(YRPP_THISCALL*)(ShipLocomotionClass*, bool);
        return reinterpret_cast<Entry>(0x6A05F0)(this, justStarted);
    }
#else
    bool While_Moving(bool justStarted = false);
#endif
    /// VA: 0x6A1C80
#if defined(RA2_YRPP_GAME)
    bool Start_Of_Move(bool& stopProcessing, bool retry = true, bool forceStraight = false) {
        using Entry = bool(YRPP_THISCALL*)(ShipLocomotionClass*, bool*, bool, bool);
        return reinterpret_cast<Entry>(0x6A1C80)(this, &stopProcessing, retry, forceStraight);
    }
#else
    bool Start_Of_Move(bool& stopProcessing, bool retry = true, bool forceStraight = false);
#endif
    // Inlined original navigation operations used by the movement implementation.
    bool Stop_Driver();
    bool Abandon_Navigation();
    /// VA: 0x69F210
#if defined(RA2_YRPP_GAME)
    void Set_Slope(int ramp) {
        using Entry = void(YRPP_THISCALL*)(ShipLocomotionClass*, int);
        reinterpret_cast<Entry>(0x69F210)(this, ramp);
    }
#else
    void Set_Slope(int ramp);
#endif
    /// VA: 0x6A42A0
    int Size() override { return 0x70; }
    // The reference address is the scalar deleting destructor.
    /// VA: 0x6A42B0
#if defined(RA2_YRPP_GAME)
    ~ShipLocomotionClass() override RX;
#else
    ~ShipLocomotionClass() override;
#endif
    /// VA: 0x69EC50
#if defined(RA2_YRPP_GAME)
    ShipLocomotionClass() : ShipLocomotionClass(noinit_t()) { JMP_THIS(0x69EC50); }
#else
    ShipLocomotionClass() noexcept;
#endif
protected:
    explicit __forceinline ShipLocomotionClass(noinit_t) : LocomotionClass(noinit_t()) {}

public:
    int CurrentRamp;
    int PreviousRamp;
    RateTimer SlopeTimer;
    CoordStruct DestinationCoord;
    CoordStruct HeadToCoord;
    int SpeedAccum;
    double movementspeed_50;
    int TrackNumber;
    int TrackIndex;
    bool IsOnShortTrack;
    BYTE IsTurretLockedDown;
    bool IsRotating;
    bool IsDriving;
    bool IsRocking;
    bool UnLocked;
    ILocomotion* Piggybackee;
};
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(ShipLocomotionClass) == 0x70);
static_assert(offsetof(ShipLocomotionClass, DestinationCoord) == 0x34);
static_assert(offsetof(ShipLocomotionClass, Piggybackee) == 0x68);
#endif
