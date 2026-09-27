/*
 * Adapts EA REDALERT/MISSION.CPP, fixed revision
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts
 * Inc. GPL-3.0-or-later with terms in third_party/ea/LICENSE.TXT.
 * YR 5B3700/5B3740/5B3760/5B3910: double rates, 32 names, INI cache reset.
 */
#include "yrpp/MissionClass.h"
#include "yrpp/CCINIClass.h"

MissionControlClass::MissionControlClass() noexcept : ArrayIndex(-1), NoThreat(false),
    Zombie(false), Recruitable(true), Paralyzed(false), Retaliate(true), Scatter(true),
    Rate(.016), AARate(.016) { }

const char* YRPP_FASTCALL MissionControlClass::FindName(const Mission& mission) {
    const int index = static_cast<int>(mission);
    // Bound invalid indices locally; the target indexes outside Names for
    // values other than -1 and 0..31.
    return index >= 0 && index < 32 ? Names[index] : "<none>";
}
const char* MissionControlClass::GetName() {
    const auto mission = static_cast<Mission>(ArrayIndex);
    return FindName(mission);
}
Mission YRPP_FASTCALL MissionControlClass::FindIndex(const char* name) {
    if (name)
        for (int index = 0; index < 32; ++index)
            if (!_strcmpi(Names[index], name)) return static_cast<Mission>(index);
    return static_cast<Mission>(-1);
}
MissionControlClass* YRPP_FASTCALL MissionControlClass::Find(const char* name) {
    const int index = static_cast<int>(FindIndex(name));
    return index < 0 ? nullptr : &Array[index];
}
bool MissionControlClass::LoadFromINI(CCINIClass* ini) {
    if (!ini) return false;
    ini->Reset();
    const char* section = GetName();
    if (!ini->GetSection(section)) return false;
    NoThreat = ini->ReadBool(section, "NoThreat", NoThreat);
    Zombie = ini->ReadBool(section, "Zombie", Zombie);
    Recruitable = ini->ReadBool(section, "Recruitable", Recruitable);
    Paralyzed = ini->ReadBool(section, "Paralyzed", Paralyzed);
    Retaliate = ini->ReadBool(section, "Retaliate", Retaliate);
    Scatter = ini->ReadBool(section, "Scatter", Scatter);
    Rate = ini->ReadDouble(section, "Rate", Rate);
    AARate = ini->ReadDouble(section, "AARate", 0.0);
    if (AARate == 0.0) AARate = Rate;
    return true;
}
