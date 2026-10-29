// YR 4F4320/4F43F0/4F4410/4F4450. The unfortunately named SetButtons is a membership
// query in the target binary, not a setter.
#include "yrpp/GScreenClass.h"
#include "yrpp/GadgetClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/WWMouseClass.h"
#if !defined(RA2_YRPP_GAME)
void GScreenClass::GetInputAndUpdate(DWORD& outKeyCode, int& outMouseX, int& outMouseY) {
    // The pointer is read first: Update receives the position sampled before the key.
    // A missing device (no UI session) is a pointer outside every region.
    outMouseX = WWMouseClass::Instance ? WWMouseClass::Instance->GetX() : -1;
    outMouseY = WWMouseClass::Instance ? WWMouseClass::Instance->GetY() : -1;
    if (Buttons) {
        if (Buttons->IsListToRedraw()) MarkNeedsRedraw(0);
        // 0x4F436E swaps DSurface::Temp with Hidden around the list input. Gadget
        // input here is surface-free, so the swap has no observable effect.
        outKeyCode = Buttons->Input();
    } else {
        auto* keyboard = InputManagerClass::Instance;
        outKeyCode = keyboard ? keyboard->Check() : 0;
        if (outKeyCode) outKeyCode = keyboard->Get();
    }
    const int key = static_cast<int>(outKeyCode);
    Update(key, Point2D{outMouseX, outMouseY});
}
#endif
bool GScreenClass::SetButtons(GadgetClass* gadget) { return gadget && Buttons == gadget->HeadOfList(); }
bool GScreenClass::AddButton(GadgetClass* gadget) {
    if (!gadget || SetButtons(gadget)) return false;
    if (Buttons)
        gadget->AddTail(*Buttons);
    else
        Buttons = gadget;
    return true;
}
bool GScreenClass::RemoveButton(GadgetClass* gadget) {
    if (!gadget || !SetButtons(gadget)) return false;
    Buttons = gadget->Remove();
    return true;
}
