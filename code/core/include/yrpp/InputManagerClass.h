#pragma once

#include "yrpp/GameOptionsClass.h"

#include "yrpp/Helpers/CompileTime.h"

#include "yrpp/GeneralStructures.h"
class InputManagerClass {
public:
    /// Global VA: 0x0087F770.
    static InputManagerClass*& Instance;
    InputManagerClass() noexcept;
    void SetKeyState(int key, bool pressed) noexcept;
    void SetClickPosition(const Point2D& point) noexcept;
    void Reset() noexcept;

    // Keyboard queue (WWKeyboardClass): a 256-entry ring of 16-bit key codes in
    // Keycodes_w, first unread index in field_314, next free index in field_318.
    // The queue is empty when both indexes are equal and full one slot early.
    // The original pumps the Windows message queue inside Check/Get; a native host
    // delivers its events through Put instead, so an empty queue never blocks.
    /// VA: 0x0054F000.
#if defined(RA2_YRPP_GAME)
    WORD Check() { JMP_THIS(0x54F000); }
#else
    WORD Check() noexcept;
#endif
    // Removes the next key. A mouse button code (1, 2, 4) is followed in the
    // queue by its X and Y words, code 10 by one word; they are consumed here and
    // stored in field_0/field_C and field_4/field_10 (button), field_8 (code 10).
    /// VA: 0x0054F050.
#if defined(RA2_YRPP_GAME)
    WORD Get() { JMP_THIS(0x54F050); }
#else
    WORD Get() noexcept;
#endif
    // False when the ring is full.
    /// VA: 0x0054F1C0.
#if defined(RA2_YRPP_GAME)
    bool Put(WORD key) { JMP_THIS(0x54F1C0); }
#else
    bool Put(WORD key) noexcept;
#endif
    // Discards all queued keys.
    /// VA: 0x0054F720.
#if defined(RA2_YRPP_GAME)
    void Clear() { JMP_THIS(0x54F720); }
#else
    void Clear() noexcept;
#endif

    /// VA: 0x0054F720.
    byte DoSomething()
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x54F720);
    }
#else
    {
        Clear();
        return 0;
    }
#endif

    /// VA: 0x0054F5C0.
    bool IsKeyPressed(int key) const;

    bool IsForceFireKeyPressed() const {
        return this->IsKeyPressed(GameOptionsClass::Instance.KeyForceFire1) ||
               this->IsKeyPressed(GameOptionsClass::Instance.KeyForceFire2);
    }

    bool IsForceMoveKeyPressed() const {
        return this->IsKeyPressed(GameOptionsClass::Instance.KeyForceMove1) ||
               this->IsKeyPressed(GameOptionsClass::Instance.KeyForceMove2);
    }

    bool IsForceSelectKeyPressed() const {
        return this->IsKeyPressed(GameOptionsClass::Instance.KeyForceSelect1) ||
               this->IsKeyPressed(GameOptionsClass::Instance.KeyForceSelect2);
    }

protected:
    int field_0;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    byte Keycodes_b[256];
    WORD Keycodes_w[256];
    int field_314;
    int field_318;
};
