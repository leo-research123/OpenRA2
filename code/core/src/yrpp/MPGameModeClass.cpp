// MPGameModeClass rules INI, YR 0x005D67B0.
#include "yrpp/MPGameModeClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/CCINIClass.h"

CCINIClass* MPGameModeClass::GetINI() {
    if (!INI) {
        INI = new CCINIClass;
        CCFileClass file(INIFilename.Buffer);
        if (file.Exists(false)) INI->ReadCCFile(&file, false, false);
    }
    return INI;
}

MPGameModeClass::MPGameModeClass(wchar_t** title, wchar_t** tooltip, char** file, char** filter, bool ai, int index)
    : unknown_4(false), MPTeams{}, UIName(title && *title ? *title : L""),
      CSFTooltip(tooltip && *tooltip ? *tooltip : L""), MPModeIndex(index),
      INIFilename(file && *file ? *file : ""), MapFilter(filter && *filter ? *filter : ""),
      AIAllowed(ai), INI(nullptr), AlliesAllowed(true), wolTourney(true), wolClanTourney(true), MustAlly(false) {
    auto* ini = GetINI();
    const char* section = "MultiplayerDialogSettings";
    wolTourney = ini->ReadBool(section, "WonlineTournamentAllowed", wolTourney);
    wolClanTourney = ini->ReadBool(section, "WonlineClanTournamentAllowed", wolClanTourney);
    AlliesAllowed = ini->ReadBool(section, "AlliesAllowed", AlliesAllowed);
    MustAlly = ini->ReadBool(section, "MustAlly", MustAlly) && AlliesAllowed;
}
MPGameModeClass::~MPGameModeClass() {
    for (auto* team : MPTeams) delete team;
    MPTeams.Clear();
    delete INI;
}
