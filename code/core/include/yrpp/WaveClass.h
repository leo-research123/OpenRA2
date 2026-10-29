#pragma once
#include "yrpp/ObjectClass.h"
#include "yrpp/Facing.h"

class TechnoClass;
class NOVTABLE WaveClass : public ObjectClass {
public:
    static const AbstractType AbsID = AbstractType::Wave;
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<WaveClass*>, Array, 0xA8EC38u)
#else
    static DynamicVectorClass<WaveClass*>& Array;
#endif
    // Lookup tables shared by every wave, filled by InitializeTables.
    // Truncated distance sqrt(x * x + y * y), indexed by 300 * y + x.
    /// Global VA: 0x00B4669C.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(unsigned short, [90000], DistanceTable, 0xB4669Cu)
#else
    static unsigned short (&DistanceTable)[90000];
#endif
    // truncate(sin(i * 0.125) * 12 + 0.49), used by sonic waves.
    /// Global VA: 0x00B46254.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(short, [500], SonicAmplitudeTable, 0xB46254u)
#else
    static short (&SonicAmplitudeTable)[500];
#endif
    // truncate(sin(i * 0.1) * 12 + 0.49), used by magnetron waves.
    /// Global VA: 0x00B45E68.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(short, [496], MagnetronAmplitudeTable, 0xB45E68u)
#else
    static short (&MagnetronAmplitudeTable)[496];
#endif
    // Pixel strength per amplitude: 110, 118, ... 206.
    /// Global VA: 0x00B46648.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(int, [13], StrengthTable, 0xB46648u)
#else
    static int (&StrengthTable)[13];
#endif
    // Set by InitializeTables; the constructors initialize the tables on first use.
    /// Global VA: 0x00B725CC.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, TablesInitialized, 0xB725CCu)
#else
    static bool& TablesInitialized;
#endif
    /// VA: 0x75F020
#if defined(RA2_YRPP_GAME)
    static void InitializeTables() { JMP_STD(0x75F020); }
#else
    static void InitializeTables();
#endif
    /// VA: 0x75F650
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Load(IStream* stream) override { JMP_STD(0x75F650); }
#else
    HRESULT YRPP_STDCALL Load(IStream* stream) override;
#endif
    /// VA: 0x75F7D0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clearDirty) override { JMP_STD(0x75F7D0); }
#else
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clearDirty) override;
#endif
    /// VA: 0x75F840
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override { JMP_STD(0x75F840); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override;
#endif
    /// VA: 0x763200
#if defined(RA2_YRPP_GAME)
    ~WaveClass() override {
#if defined(_MSC_VER) && defined(_M_IX86)
        // Only the scalar deleting entry exists. Supply flags=0 when a
        // qualified destructor call reaches this non-deleting wrapper.
        EPILOG_THISCALL;
        _asm { pop edx }
        _asm {push 0} _asm {push edx} JMP(0x763200);
#else
        JMP_THIS(0x763200);
#endif
    }
#else
    ~WaveClass() override;
#endif
    /// VA: 0x75F610
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x75F610); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    /// VA: 0x7631F0
#if defined(RA2_YRPP_GAME)
    AbstractType WhatAmI() const override { JMP_THIS(0x7631F0); }
#else
    AbstractType WhatAmI() const override;
#endif
    /// VA: 0x75F880
#if defined(RA2_YRPP_GAME)
    int Size() const override { JMP_THIS(0x75F880); }
#else
    int Size() const override;
#endif
    /// VA: 0x760F50
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x760F50); }
#else
    void Update() override;
#endif
    /// VA: 0x75F890
#if defined(RA2_YRPP_GAME)
    Layer InWhichLayer() const override { JMP_THIS(0x75F890); }
#else
    Layer InWhichLayer() const override;
#endif
    /// VA: 0x75F8A0
#if defined(RA2_YRPP_GAME)
    ObjectTypeClass* GetType() const override { JMP_THIS(0x75F8A0); }
#else
    ObjectTypeClass* GetType() const override;
#endif
    /// VA: 0x75F8B0
#if defined(RA2_YRPP_GAME)
    bool Unlimbo(const CoordStruct& location, DirType direction) override { JMP_THIS(0x75F8B0); }
#else
    bool Unlimbo(const CoordStruct& location, DirType direction) override;
#endif
    /// VA: 0x75F980
#if defined(RA2_YRPP_GAME)
    bool Limbo() override { JMP_THIS(0x75F980); }
#else
    bool Limbo() override;
#endif
    /// VA: 0x75F9F0
#if defined(RA2_YRPP_GAME)
    void DrawIt(Point2D* location, RectangleStruct* bounds) const override { JMP_THIS(0x75F9F0); }
#else
    void DrawIt(Point2D* location, RectangleStruct* bounds) const override;
#endif
    /// VA: 0x762070
#if defined(RA2_YRPP_GAME)
    void Draw_Magnetic(const CoordStruct& from, const CoordStruct& to) { JMP_THIS(0x762070); }
#else
    void Draw_Magnetic(const CoordStruct& from, const CoordStruct& to);
#endif
    /// VA: 0x761640
#if defined(RA2_YRPP_GAME)
    void Draw_NonMagnetic(const CoordStruct& from, const CoordStruct& to) { JMP_THIS(0x761640); }
#else
    void Draw_NonMagnetic(const CoordStruct& from, const CoordStruct& to);
#endif
    /// VA: 0x762AF0
#if defined(RA2_YRPP_GAME)
    void Update_Wave() { JMP_THIS(0x762AF0); }
#else
    void Update_Wave();
#endif
    /// VA: 0x75F330
#if defined(RA2_YRPP_GAME)
    void DamageArea(const CoordStruct& location) const { JMP_THIS(0x75F330); }
#else
    void DamageArea(const CoordStruct& location) const;
#endif
    /// VA: 0x7610F0
#if defined(RA2_YRPP_GAME)
    void UpdateCells() { JMP_THIS(0x7610F0); }
#else
    void UpdateCells();
#endif
    /// VA: 0x75E950
#if defined(RA2_YRPP_GAME)
    WaveClass(const CoordStruct& from, const CoordStruct& to, TechnoClass* owner, WaveType mode, AbstractClass* target)
        : WaveClass(noinit_t()) {
        JMP_THIS(0x75E950);
    }
#else
    // Allocation failures terminate on this side of the game/host boundary.
    WaveClass(const CoordStruct& from, const CoordStruct& to, TechnoClass* owner, WaveType mode,
              AbstractClass* target) noexcept;
#endif
protected:
    explicit WaveClass(noinit_t) : ObjectClass(noinit_t()) {}

private:
    void PrepareGeometry(const CoordStruct& from, const CoordStruct& to, bool magnetic);

public:
    AbstractClass* Target;
    WaveType Type;
    CoordStruct LimboCoords;
    CoordStruct Pos0;
    Point2D WaveStartMiddle;
    Point2D WaveEndMiddle;
    Point2D WaveEndSide1;
    Point2D WaveEndSide2;
    Point2D WaveStartSide1;
    Point2D WaveStartSide2;
    CoordStruct WaveEndSide1Coord;
    CoordStruct WaveEndSide2Coord;
    CoordStruct WaveStartSide1Coord;
    CoordStruct WaveStartSide2Coord;
    bool IsTraveling;
    bool ShouldEnd;
    BYTE field_12E;
    BYTE field_12F;
    int WaveEC;  // for sonic/magna only
    int WaveCount;
    double MatrixScale1;
    double MatrixScale2;
    int PointData_Counter;
    Point2D* PointData_Pointer;
    Point2D SonicPoints[6];
    Point2D MagPoints[4];
    int PointData2_X;
    int PointData2_Y;
    Point2D* PointData2_Pointer;
    int PitchData[8];
    int FacingIndex;
    int LaserEC;  // for lasers only, ctor = 160, per frame -= 6, 32 == dtor
    TechnoClass* Owner;
    FacingClass Facing;
    DynamicVectorClass<CellClass*> Cells;
    int ColorData[14];
};
