// Supplied 006B56D0. Native owned images use the original CCFile raw allocator.
#include "yrpp/SmudgeTypeClass.h"
#include "type_resources.hpp"
bool SmudgeTypeClass::LoadFromINI(CCINIClass* ini) {
    if (!ini) return false;
    try {
        // The old image is owned even when legacy ImageAllocated is false.
        // The original loader overwrote that pointer; native rereads release it.
        YRMemory::Deallocate(Image); Image = nullptr; ImageAllocated = false;
        if (!ObjectTypeClass::LoadFromINI(ini)) return false;
        Crater = ini->ReadBool(ID, "Crater", Crater);
        Burn = ini->ReadBool(ID, "Burn", Burn);
        Width = ini->ReadInteger(ID, "Width", Width);
        Height = ini->ReadInteger(ID, "Height", Height);
        char filename[260];
        if (!game::type_image_filename(*this, filename, sizeof(filename), false)) return false;
        return game::load_owned_type_shape(filename, Image);
    } catch (...) { game::type_resource_result(game::TypeResourceStatus::failure); return false; }
}
