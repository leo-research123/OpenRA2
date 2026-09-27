// Locomotor = {4A582741-9839-11d1-B709-00A024DDAFD1}

#pragma once

#include "yrpp/Helpers/CompileTime.h"
#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("4A582741-9839-11D1-B709-00A024DDAFD1")) DriveLocomotionClass : public LocomotionClass, public IPiggyback
{
public:

    /// Global VA: 0x007E7B28.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(const TurnTrackType, [72], TurnTrack, 0x7E7B28)
#else
    static const TurnTrackType (&TurnTrack)[72];
#endif
    /// Global VA: 0x007E7A28.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(const RawTrackType, [16], RawTrack, 0x7E7A28)
#else
    static const RawTrackType (&RawTrack)[16];
#endif

    /// VA: 0x4AF720
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** output) override { JMP_STD(0x4AF720); }
#else
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** output) override;
#endif
    /// VA: 0x4B4CB0
#if defined(RA2_YRPP_GAME)
    ULONG YRPP_STDCALL AddRef() override { JMP_STD(0x4B4CB0); }
#else
    ULONG YRPP_STDCALL AddRef() override;
#endif
    /// VA: 0x4B4CC0
#if defined(RA2_YRPP_GAME)
    ULONG YRPP_STDCALL Release() override { JMP_STD(0x4B4CC0); }
#else
    ULONG YRPP_STDCALL Release() override;
#endif
    /// VA: 0x4B4830
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* output) override { JMP_STD(0x4B4830); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* output) override;
#endif
    /// VA: 0x4AFB80
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving() override { JMP_STD(0x4AFB80); }
#else
    bool YRPP_STDCALL Is_Moving() override;
#endif
    /// VA: 0x4AFC90
#if defined(RA2_YRPP_GAME)
    CoordStruct YRPP_STDCALL Destination() override { JMP_STD(0x4AFC90); }
#else
    CoordStruct YRPP_STDCALL Destination() override;
#endif
    /// VA: 0x4AFCC0
#if defined(RA2_YRPP_GAME)
    CoordStruct YRPP_STDCALL Head_To_Coord() override { JMP_STD(0x4AFCC0); }
#else
    CoordStruct YRPP_STDCALL Head_To_Coord() override;
#endif
    /// VA: 0x4AFF60
#if defined(RA2_YRPP_GAME)
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override { JMP_STD(0x4AFF60); }
#else
    Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override;
#endif
    /// VA: 0x4B0410
#if defined(RA2_YRPP_GAME)
    Matrix3D YRPP_STDCALL Shadow_Matrix(VoxelIndexKey* key) override { JMP_STD(0x4B0410); }
#else
    Matrix3D YRPP_STDCALL Shadow_Matrix(VoxelIndexKey* key) override;
#endif
    /// VA: 0x4B4870
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Z_Adjust() override { JMP_STD(0x4B4870); }
#else
    int YRPP_STDCALL Z_Adjust() override;
#endif
    /// VA: 0x4B4880
#if defined(RA2_YRPP_GAME)
    ZGradient YRPP_STDCALL Z_Gradient() override { JMP_STD(0x4B4880); }
#else
    ZGradient YRPP_STDCALL Z_Gradient() override;
#endif
    /// VA: 0x4B0500
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Process() override { JMP_STD(0x4B0500); }
#else
    bool YRPP_STDCALL Process() override;
#endif
    /// VA: 0x4AFD40
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Move_To(CoordStruct to) override { JMP_STD(0x4AFD40); }
#else
    void YRPP_STDCALL Move_To(CoordStruct to) override;
#endif
    /// VA: 0x4AFE00
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Stop_Moving() override { JMP_STD(0x4AFE00); }
#else
    void YRPP_STDCALL Stop_Moving() override;
#endif
    /// VA: 0x4B0EF0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Do_Turn(DirStruct dir) override { JMP_STD(0x4B0EF0); }
#else
    void YRPP_STDCALL Do_Turn(DirStruct dir) override;
#endif
    /// VA: 0x4B04D0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Unlimbo() override { JMP_STD(0x4B04D0); }
#else
    void YRPP_STDCALL Unlimbo() override;
#endif
    /// VA: 0x4B0C40
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Force_Track(int track, CoordStruct coord) override { JMP_STD(0x4B0C40); }
#else
    void YRPP_STDCALL Force_Track(int track, CoordStruct coord) override;
#endif
    /// VA: 0x4B4820
#if defined(RA2_YRPP_GAME)
    Layer YRPP_STDCALL In_Which_Layer() override { JMP_STD(0x4B4820); }
#else
    Layer YRPP_STDCALL In_Which_Layer() override;
#endif
    /// VA: 0x4AFB40
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Force_New_Slope(int ramp) override { JMP_STD(0x4AFB40); }
#else
    void YRPP_STDCALL Force_New_Slope(int ramp) override;
#endif
    /// VA: 0x4AFC20
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Now() override { JMP_STD(0x4AFC20); }
#else
    bool YRPP_STDCALL Is_Moving_Now() override;
#endif
    /// VA: 0x4B48D0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override { JMP_STD(0x4B48D0); }
#else
    void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) override;
#endif
    /// VA: 0x4B4920
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) override { JMP_STD(0x4B4920); }
#else
    bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) override;
#endif
    /// VA: 0x4B4B00
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Will_Jump_Tracks() override { JMP_STD(0x4B4B00); }
#else
    bool YRPP_STDCALL Will_Jump_Tracks() override;
#endif
    /// VA: 0x4B4C50
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Really_Moving_Now() override { JMP_STD(0x4B4C50); }
#else
    bool YRPP_STDCALL Is_Really_Moving_Now() override;
#endif
    /// VA: 0x4B4BE0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Lock() override { JMP_STD(0x4B4BE0); }
#else
    void YRPP_STDCALL Lock() override;
#endif
    /// VA: 0x4B4BF0
#if defined(RA2_YRPP_GAME)
    void YRPP_STDCALL Unlock() override { JMP_STD(0x4B4BF0); }
#else
    void YRPP_STDCALL Unlock() override;
#endif
    /// VA: 0x4B4C00
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Get_Track_Number() override { JMP_STD(0x4B4C00); }
#else
    int YRPP_STDCALL Get_Track_Number() override;
#endif
    /// VA: 0x4B4C10
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Get_Track_Index() override { JMP_STD(0x4B4C10); }
#else
    int YRPP_STDCALL Get_Track_Index() override;
#endif
    /// VA: 0x4B4C20
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Get_Speed_Accum() override { JMP_STD(0x4B4C20); }
#else
    int YRPP_STDCALL Get_Speed_Accum() override;
#endif
    /// VA: 0x4AF8E0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) override { JMP_STD(0x4AF8E0); }
#else
    HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) override;
#endif
    /// VA: 0x4AF930
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) override { JMP_STD(0x4AF930); }
#else
    HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) override;
#endif
    /// VA: 0x4AF970
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Ok_To_End() override { JMP_STD(0x4AF970); }
#else
    bool YRPP_STDCALL Is_Ok_To_End() override;
#endif
    /// VA: 0x4AF610
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* id) override { JMP_STD(0x4AF610); }
#else
    HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* id) override;
#endif
    /// VA: 0x4B4CD0
#if defined(RA2_YRPP_GAME)
    bool YRPP_STDCALL Is_Piggybacking() override { JMP_STD(0x4B4CD0); }
#else
    bool YRPP_STDCALL Is_Piggybacking() override;
#endif

    /// VA: 0x4B4CF0
    int Size() override {return 0x70;}
    /// VA: 0x4AF780
    HRESULT YRPP_STDCALL Load(IStream* stream) override {JMP_STD(0x4AF780);}
    /// VA: 0x4AF800
    HRESULT YRPP_STDCALL Save(IStream* stream,BOOL clear) override {JMP_STD(0x4AF800);}

    /// VA: 0x4B2630
    bool Start_Of_Move(bool& stopProcessing,bool retry=true,bool forceStraight=false);
    /// VA: 0x4B0F20
    bool While_Moving(bool justStarted=false);
    /// VA: 0x4B4780
    Point2D Smooth_Turn(const Point2D& offset,int& direction);
    /// VA: 0x4B0AD0
    void Mark_Track(const CoordStruct& head,MarkType mark);
    bool Start_Driver(const CoordStruct& head);
    bool Stop_Driver();
    bool Abandon_Navigation();
    void Set_Slope(int ramp);

    // Destructor
    /// VA: 0x4B4D00
#if defined(RA2_YRPP_GAME)
    virtual ~DriveLocomotionClass() RX;
#else
    ~DriveLocomotionClass() override;
#endif

    // Constructor
    /// VA: 0x004AF540.
#if defined(RA2_YRPP_GAME)
    DriveLocomotionClass()
        : DriveLocomotionClass(noinit_t())
    { JMP_THIS(0x4AF540); }
#else
    DriveLocomotionClass() noexcept;
#endif

protected:
    explicit __forceinline DriveLocomotionClass(noinit_t)
        : LocomotionClass(noinit_t())
    { }

    // Properties

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
static_assert(sizeof(DriveLocomotionClass) == 0x70);
static_assert(offsetof(DriveLocomotionClass,DestinationCoord)==0x34);
static_assert(offsetof(DriveLocomotionClass,TrackNumber)==0x58);
#endif
