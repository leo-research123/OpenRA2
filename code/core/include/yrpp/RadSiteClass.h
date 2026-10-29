/*
    RadSites
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Timer.h"

class LightSourceClass;
class WeaponTypeClass;

class NOVTABLE RadSiteClass : public AbstractClass {
public:
    static const AbstractType AbsID = AbstractType::RadSite;

    // Static
    /// Global VA: 0x00B04BD0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<RadSiteClass*>, Array, 0xB04BD0u)
#else
    static DynamicVectorClass<RadSiteClass*>& Array;
#endif

    /// VA: 0x0065B470
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override { JMP_STD(0x65B470); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override;
#endif
    /// VA: 0x0065B3D0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Load(IStream* stream) override { JMP_STD(0x65B3D0); }
#else
    HRESULT YRPP_STDCALL Load(IStream*) override { return static_cast<HRESULT>(0x80004001u); }
#endif
    /// VA: 0x0065B450
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clear) override { JMP_STD(0x65B450); }
#else
    HRESULT YRPP_STDCALL Save(IStream*, BOOL) override { return static_cast<HRESULT>(0x80004001u); }
#endif
    /// VA: 0x0065B2F0
#if defined(RA2_YRPP_GAME)
    ~RadSiteClass() override { JMP_THIS(0x65B2F0); }
#else
    ~RadSiteClass() override;
#endif
    /// VA: 0x0065B3C0
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x0065B3A0
    int Size() const override { return sizeof(*this); }
    /// VA: 0x0065B800
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x65B800); }
#else
    void Update() override;
#endif

    // non-virtual
public:
    // Start irradiating an area. Be sure to set the BaseCell, Spread and RadLevel first!
    /// VA: 0x0065B580
#if defined(RA2_YRPP_GAME)
    void Activate() { JMP_THIS(0x65B580); }
#else
    void Activate();
#endif

    // Remove the radiation effect of this site completely
    /// VA: 0x0065BB50
#if defined(RA2_YRPP_GAME)
    void Deactivate() { JMP_THIS(0x65BB50); }
#else
    void Deactivate();
#endif

    // Add the radiation of this RadSite to the cells.
    /// VA: 0x0065B9C0
#if defined(RA2_YRPP_GAME)
    void Radiate() { JMP_THIS(0x65B9C0); }
#else
    void Radiate();
#endif

    // Decrease the radiation of this RadSite by one step.
    /// VA: 0x0065BD00
#if defined(RA2_YRPP_GAME)
    void DecreaseRadiation() { JMP_THIS(0x65BD00); }
#else
    void DecreaseRadiation();
#endif

    // Decrease the cell tint and intensity of this RadSite.
    /// VA: 0x0065BE90
#if defined(RA2_YRPP_GAME)
    void DecreaseLight() { JMP_THIS(0x65BE90); }
#else
    void DecreaseLight();
#endif

    // Add this RadLevel to the current radiation.
    /// VA: 0x0065B530
#if defined(RA2_YRPP_GAME)
    void Add(int nRadLevel) { JMP_THIS(0x65B530); }
#else
    void Add(int nRadLevel);
#endif

    // Gets the current rad level (corresponds to the RadLevel at BaseCell).
    /// VA: 0x0065B510
#if defined(RA2_YRPP_GAME)
    int GetRadLevel() { JMP_THIS(0x65B510); }
#else
    int GetRadLevel();
#endif

    // Gets the rad level applied by this RadSite to a certain cell.
    /// VA: 0x0065B8F0
#if defined(RA2_YRPP_GAME)
    double GetRadLevelAt(CellStruct* pCell) { JMP_THIS(0x65B8F0); }
#else
    double GetRadLevelAt(CellStruct* pCell);
#endif

    // Sets the rad level and the appropriate duration values.
    /// VA: 0x0065B4F0
#if defined(RA2_YRPP_GAME)
    void SetRadLevel(int nRadLevel) { JMP_THIS(0x65B4F0); }
#else
    void SetRadLevel(int nRadLevel);
#endif

    // Sets the center cell. Do not change when RadSite is activated.
    /// VA: 0x0065B4C0
#if defined(RA2_YRPP_GAME)
    void SetBaseCell(CellStruct* pCell) { JMP_THIS(0x65B4C0); }
#else
    void SetBaseCell(CellStruct* pCell);
#endif

    // Gets the spread in cells.
    /// VA: 0x0065B4B0
#if defined(RA2_YRPP_GAME)
    int GetSpread() { JMP_THIS(0x65B4B0); }
#else
    int GetSpread();
#endif

    // Sets the spread in cells. Also updates the SpreadInLeptons.
    /// VA: 0x0065B4D0
#if defined(RA2_YRPP_GAME)
    void SetSpread(int nCells) { JMP_THIS(0x65B4D0); }
#else
    void SetSpread(int nCells);
#endif

    // helper methods

    // Gets the current strenght of the effect, the ratio between time left and initial duration.
    double GetEffectPercentage() {
        return (this->RadDuration <= 0)
                   ? 0.0
                   : static_cast<double>(this->RadTimeLeft) / static_cast<double>(this->RadDuration);
    }

    // Constructor
    /// VA: 0x0065B1E0
#if defined(RA2_YRPP_GAME)
    RadSiteClass() noexcept : RadSiteClass(noinit_t()) { JMP_THIS(0x65B1E0); }
#else
    RadSiteClass() noexcept;
#endif

    RadSiteClass(CellStruct nBaseCoords, int nSpread, int nRadLevel) noexcept : RadSiteClass() {
        SetBaseCell(&nBaseCoords);
        SetSpread(nSpread);
        SetRadLevel(nRadLevel);
    }

protected:
    explicit __forceinline RadSiteClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

    // Properties

public:
    LightSourceClass* LightSource;                  // the light source attached to this instance
    DECLARE_PROPERTY(CDTimerClass, RadLevelTimer);  // used to count down RadLevelDelay
    DECLARE_PROPERTY(CDTimerClass, RadLightTimer);  // used to count down RadLightDelay
    CellStruct BaseCell;                            // center cell
    int Spread;                                     // range in cells
    int SpreadInLeptons;                            // range in leptons
    int RadLevel;                                   // the radiation level,
    int LevelSteps;          // cell's rad level reduced by (RadLevel/LevelSteps) every time RadLevelTimer elapses
    int Intensity;           // the intensity at the beginning
    TintStruct Tint;         // RadColor at the current level
    int IntensitySteps;      // the number of intensity decreases during the duration
    int IntensityDecrement;  // Intensity decremented by this every time RadLightDelay elapses
    int RadDuration;         // as currently set up, the rad site will stay for so many frames
    int RadTimeLeft;         // the remaining frames. divided by RadDuration gives the factor
};
