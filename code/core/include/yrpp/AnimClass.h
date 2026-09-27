/*
    Animations
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/BounceClass.h"
#include "yrpp/StageClass.h"

// forward declarations
class AnimTypeClass;
class BulletClass;
class HouseClass;
class LightConvertClass;

class NOVTABLE AnimClass : public ObjectClass
{
public:
    /// VA: 0x00424CB0
#if defined(RA2_YRPP_GAME)
    Layer InWhichLayer() const override { JMP_THIS(0x00424CB0); }
#else
    Layer InWhichLayer() const override;
#endif
    // Original inherited overrides; references only, no new ABI slots.
    using ObjectClass::GetCoords;
    /// VA: 0x00422BE0
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetCoords(CoordStruct* output) const override { JMP_THIS(0x422BE0); }
#else
    CoordStruct* GetCoords(CoordStruct* output) const override;
#endif

    // Update.
    /// VA: 0x00423AC0.
    /// VA: 0x00422CA0
    void DrawIt(Point2D* location, RectangleStruct* bounds) const override;
    /// VA: 0x00425630
    int GetZ() const override;

    static const AbstractType AbsID = AbstractType::Anim;

    // Static
    /// Global VA: 0x00A8E9A8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<AnimClass*>, Array, 0xA8E9A8u)
#else
    static DynamicVectorClass<AnimClass*>& Array;
#endif

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual ~AnimClass() RX;
#else
    virtual ~AnimClass();
#endif

    // AbstractClass
    /// VA: 0x00425150.
#if defined(RA2_YRPP_GAME)
    virtual void PointerExpired(AbstractClass* pAbstract, bool detachFromAll) override JMP_THIS(0x425150);
#else
    virtual void PointerExpired(AbstractClass* pAbstract, bool detachFromAll) override;
#endif
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual AbstractType WhatAmI() const RT(AbstractType);
#else
    virtual AbstractType WhatAmI() const override;
#endif
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual int	Size() const R0;
#else
    virtual int Size() const override;
#endif

#if !defined(RA2_YRPP_GAME)
    ObjectTypeClass* GetType() const override;
    void Update() override;
#endif
    // ObjectClass
    /// VA: 0x00422BC0
#if defined(RA2_YRPP_GAME)
    int GetYSort() const override { JMP_THIS(0x00422BC0); }
#else
    int GetYSort() const override;
#endif
    // AnimClass
    /// VA: 0x00425670
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void FlamingGuyAI() noexcept {
        reinterpret_cast<void (YRPP_THISCALL*)(AnimClass*)>(0x425670)(this);
    }
#else
    void FlamingGuyAI() noexcept;
#endif
    /// VA: 0x00425D10
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) CoordStruct* NextFlamingGuyCoords(CoordStruct* result) noexcept {
        return reinterpret_cast<CoordStruct* (YRPP_THISCALL*)(AnimClass*,CoordStruct*)>(0x425D10)(this,result);
    }
#else
    CoordStruct* NextFlamingGuyCoords(CoordStruct* result) noexcept;
#endif
    /// VA: 0x004260F0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) bool IsValidFlamingGuyCell(const CellStruct& cell) noexcept {
        return reinterpret_cast<bool (YRPP_THISCALL*)(AnimClass*,const CellStruct&)>(0x4260F0)(this,cell);
    }
#else
    bool IsValidFlamingGuyCell(const CellStruct& cell) noexcept;
#endif
    /// VA: 0x004251F0
    static void YRPP_FASTCALL ApplyNukeDamage(AnimClass* unused,const CellStruct* cell) { JMP_STD(0x4251F0); }
    /// VA: 0x00423930
#if defined(RA2_YRPP_GAME)
    virtual int AnimExtras() { JMP_THIS(0x423930); }
#else
    virtual int AnimExtras();
#endif
    /// VA: 0x00425510
    virtual int GetEnd() const;

    /// VA: 0x00424B50.
#if defined(RA2_YRPP_GAME)
    void SetOwnerObject(ObjectClass *pOwner)
        { JMP_THIS(0x424B50); }
#else
    void SetOwnerObject(ObjectClass *pOwner);
#endif

    void Pause() {
        this->Paused = true;
        this->Unpaused = false;
        this->PausedAnimFrame = this->Animation.Value;
    }

    void Unpause() {
        this->Paused = false;
        this->Unpaused = true;
    }

    // Constructor
    // TODO fix
    /// VA: 0x00421EA0.
#if defined(RA2_YRPP_GAME)
    AnimClass(AnimTypeClass* pAnimType, const CoordStruct& Location, int LoopDelay = 0,
        int LoopCount = 1, DWORD flags = 0x600, int ForceZAdjust = 0, bool reverse = false) noexcept
        : AnimClass(noinit_t())
    { JMP_THIS(0x421EA0); }
#else
    AnimClass(AnimTypeClass* pAnimType, const CoordStruct& Location, int LoopDelay = 0,
        int LoopCount = 1, DWORD flags = 0x600, int ForceZAdjust = 0, bool reverse = false) noexcept;
#endif

    // Anim start logic: sound event handling, tiberium chain reaction etc.
    /// VA: 0x00424CE0.
    void Start() const
        { JMP_THIS(0x424CE0); }

    // Anim midpoint logic: particle spawning, smudges etc.
    /// VA: 0x00424F00.
    bool Middle() const
        { JMP_THIS(0x424F00); }

protected:
    explicit __forceinline AnimClass(noinit_t) noexcept
        : ObjectClass(noinit_t())
    { }

    // Properties

public:

    DECLARE_PROPERTY(StageClass, Animation);
    AnimTypeClass* Type; // AnimTypeClass.
    ObjectClass * OwnerObject; // Set by AnimClass::SetOwnerObject (0x424B50)
    DWORD unknown_D0; // Only set in CTOR/DTOR, never used. Used in Phobos for storing AnimExt::ExtData pointer.
    LightConvertClass* LightConvert; // Palette from building for building animations.
    int LightConvertIndex; // assert(ColorScheme::Array[this->LightConvertIndex] == this->LightConvert);
    char PaletteName[0x20]; // Filename set for destroy anims
    int TintColor;
    int ZAdjust;
    int YSortAdjust; // Same as YSortAdjust from Type
    CoordStruct FlamingGuyCoords; // The destination the anim tries to reach
    int FlamingGuyRetries; // Number of failed attemts to reach water. the random destination generator stops if >= 7
    bool IsBuildingAnim; // Whether this anim will invalidate on buildings, and whether it's tintable
    bool UnderTemporal; // Ttemporal'd building's active anims
    bool Paused; // If paused, does not advance anim, does not deliver damage
    bool Unpaused; // Set when unpaused
    int PausedAnimFrame; // The animation value when paused
    bool Reverse; // Anim is forced to be played from end to start
    PROTECTED_PROPERTY(BYTE, padding_121[0x7]);
    DECLARE_PROPERTY(BounceClass, Bounce);
    BYTE TranslucencyLevel; // On a scale of 1 - 100
    bool TimeToDie; // Or something to that effect, set just before UnInit
    BulletClass* AttachedBullet;
    HouseClass* Owner; // Used for remap (AltPalette)
    int LoopDelay; // Randomized value, depending on RandomLoopDelay
    double Accum; // Stores accumulated fractional animation damage and gets added to Type->Damage if at least 1.0 or above. Defaults to 1.0.
    BlitterFlags AnimFlags; // Argument that's 0x600 most of the time
    bool HasExtras; // Enables IsMeteor and Bouncer special behavior (AnimExtras)
    byte RemainingIterations; // Defaulted to deleteAfterIterations, when reaches zero, UnInit() is called
    bool UseCellLightConvert; // If set to true checks cell at render coords and uses its LightConvert if available
    bool DeleteOnMapCleanup;  // Set on tile anims, anims are deleted in MapClass::Overpass() (0x568BB0)
    bool IsInert; // Not official name, only set to true on TActionClass-created animations and prevents sounds, damage and TiberiumChainReaction from working.
    bool IsFogged;
    bool FlamingGuyExpire; // Finish animation and remove
    bool UnableToContinue; // Set when something prevents the anim from going on: cell occupied, veins destoyed or unit gone, ...
    bool SkipProcessOnce; // Set in constructor, cleared during Update. skips damage, veins, tiberium chain reaction and animation progress
    bool Invisible; // Don't draw, but Update state anyway
    bool PowerOff; // Powered animation has no power
    PROTECTED_PROPERTY(BYTE, unused_19F);
    DECLARE_PROPERTY(AudioController, StartSoundAudioController);
    DECLARE_PROPERTY(AudioController, StopSoundAudioController);
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(AnimClass) == 0x1C8);
#endif
