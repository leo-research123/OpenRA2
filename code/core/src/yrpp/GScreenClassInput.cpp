// YR 4F43F0/4F4410/4F4450. The unfortunately named SetButtons is a membership
// query in the target binary, not a setter.
#include "yrpp/GScreenClass.h"
#include "yrpp/GadgetClass.h"
bool GScreenClass::SetButtons(GadgetClass* gadget) {
    return gadget && Buttons==gadget->HeadOfList();
}
bool GScreenClass::AddButton(GadgetClass* gadget) {
    if (!gadget || SetButtons(gadget)) return false;
    if (Buttons) gadget->AddTail(*Buttons); else Buttons=gadget;
    return true;
}
bool GScreenClass::RemoveButton(GadgetClass* gadget) {
    if (!gadget || !SetButtons(gadget)) return false;
    Buttons=gadget->Remove(); return true;
}
