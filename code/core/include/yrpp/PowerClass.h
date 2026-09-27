#pragma once

#include "yrpp/RadarClass.h"

class NOVTABLE PowerClass : public RadarClass
{
public:
    // Static
    /// Global VA: 0x0087F7E8.
    static PowerClass& Instance;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~PowerClass();
    /// VA: 0x0063FB20
#if defined(RA2_YRPP_GAME)
    void Draw(DWORD force) override { JMP_THIS(0x63FB20); }
#else
    void Draw(DWORD force) override;
#endif
    /// VA: 0x0063FEA0
#if defined(RA2_YRPP_GAME)
    void Update(const int& keyCode, const Point2D& mouseCoords) override { JMP_THIS(0x63FEA0); }
#else
    void Update(const int& keyCode, const Point2D& mouseCoords) override;
#endif
    /// VA: 0x0063F730
#if defined(RA2_YRPP_GAME)
    void Init_Clear() override { JMP_THIS(0x63F730); }
#else
    void Init_Clear() override;
#endif

    // Non-virtual
    /// VA: 0x0063F810
#if defined(RA2_YRPP_GAME)
    void FlashPower() { JMP_THIS(0x63F810); }
#else
    void FlashPower();
#endif
    /// VA: 0x0063F850
#if defined(RA2_YRPP_GAME)
    int DesiredPowerHeight() const { JMP_THIS(0x63F850); }
#else
    int DesiredPowerHeight() const;
#endif
    /// VA: 0x0063F8F0
#if defined(RA2_YRPP_GAME)
    int PowerUpdateDelay() const { JMP_THIS(0x63F8F0); }
#else
    int PowerUpdateDelay() const;
#endif
    /// VA: 0x0063F960
#if defined(RA2_YRPP_GAME)
    int DesiredPowerLevels(int& green, int& yellow, int& red) const { JMP_THIS(0x63F960); }
#else
    int DesiredPowerLevels(int& green, int& yellow, int& red) const;
#endif
    /// VA: 0x0063FDC0
#if defined(RA2_YRPP_GAME)
    void RemovePowerPip() { JMP_THIS(0x63FDC0); }
#else
    void RemovePowerPip();
#endif
    /// VA: 0x0063FE30
#if defined(RA2_YRPP_GAME)
    void AddPowerPip() { JMP_THIS(0x63FE30); }
#else
    void AddPowerPip();
#endif

protected:
    // Constructor
    PowerClass();

    // Properties

public:
    bool PowerNeedRedraw;
    PROTECTED_PROPERTY(BYTE, align_150D[3])
    SysTimerClass unknown_timer_1510;
    DWORD unknown_151C;
    SysTimerClass unknown_timer_1520;
    DWORD unknown_152C;
    DWORD unknown_1530;
    DWORD unknown_1534;
    bool unknown_bool_1538;
    PROTECTED_PROPERTY(BYTE, align_1539[3])
    int PowerOutput; // Original 0x153C records House Power_Drain (legacy YRpp name).
    int PowerDrain;  // Original 0x1540 records House Power_Output (legacy YRpp name).
};
