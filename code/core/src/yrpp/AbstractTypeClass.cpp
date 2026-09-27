// OpenTS 44fac744 code/abstype.cpp confirms constructor/registry/INI/CRC roles.
// YR 0x410800 / 0x4109C0 adds fixed ID/name storage and a localized UIName.
#include "yrpp/AbstractTypeClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/StringTable.h"
#include <cstdio>
#include <cstring>

AbstractTypeClass::AbstractTypeClass(const char* id) noexcept : AbstractClass() {
    if (id) std::strncpy(ID, id, sizeof(ID));
    else std::snprintf(ID, sizeof(ID), "%08X", static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(this)));
    zero_3C = 0; // The original ID terminator is the next byte, outside ID[24].
    UINameLabel[0] = 0;
    UIName = L"";
    std::memset(Name, 0, sizeof(Name));
    for (std::size_t i = 0; i < sizeof(ID) && ID[i]; ++i) Name[i] = ID[i];
    Array.AddItem(this); // Target does not turn a failed vector insertion into an exception.
}
AbstractTypeClass::~AbstractTypeClass() { Array.Remove(this); }
const char* AbstractTypeClass::get_ID() const { return ID; }

// RA1 Read_INI has a different type model; YR 410A60/410B90 use these two keys.
bool AbstractTypeClass::LoadFromINI(CCINIClass* ini) {
    try {
    if (!ini || !ini->GetSection(ID)) return false;
    char name[sizeof(Name)], label[sizeof(UINameLabel)];
    ini->ReadString(ID, "Name", Name, name, sizeof(name));
    ini->ReadString(ID, "UIName", UINameLabel, label, sizeof(label));
    std::strncpy(Name, name, sizeof(Name) - 1); Name[sizeof(Name) - 1] = 0;
    std::strncpy(UINameLabel, label, sizeof(UINameLabel) - 1); UINameLabel[sizeof(UINameLabel) - 1] = 0;
    UIName = *UINameLabel ? StringTable::LoadString(UINameLabel) : L"";
    return true;
    } catch (...) { return false; }
}
bool AbstractTypeClass::SaveToINI(CCINIClass* ini) {
    try {
    if (!ini) return false;
    ini->Clear(ID, nullptr);
    ini->WriteString(ID, "Name", Name);
    if (*UINameLabel) ini->WriteString(ID, "UIName", UINameLabel);
    return true;
    } catch (...) { return false; }
}

#include "yrpp/CRC.h"
#include "yrpp/RulesClass.h"
void (*AbstractTypeClass::PointerExpirationObserver)(AbstractTypeClass*, bool) noexcept = nullptr;
void AbstractTypeClass::NotifyTypeExpired(bool removed) {
    if (PointerExpirationObserver) PointerExpirationObserver(this, removed);
    for (int i = 0; i < TypeExpirationListeners.Count; ++i)
        if (auto* receiver = TypeExpirationListeners[i]) receiver->PointerExpired(this, removed);
    if (RulesClass::Instance) RulesClass::Instance->RulesClass::PointerGotInvalid(this, removed);
}
void AbstractTypeClass::LoadTheaterSpecificArt(TheaterType) {} // 410C20: nullsub
void AbstractTypeClass::ComputeCRC(CRCEngine& crc) const { // 410BE0, calls 4A1DB0
    AbstractClass::ComputeCRC(crc);
    crc(ID, static_cast<int>(std::strlen(ID)));
    crc(Name, static_cast<int>(std::strlen(Name)));
    crc(UINameLabel, static_cast<int>(std::strlen(UINameLabel)));
}
