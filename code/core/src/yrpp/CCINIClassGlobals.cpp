// Standalone storage for the fixed YRpp CCINI global references.
// The original-game target supplies bindings in compat instead of this file.
#include "yrpp/CCINIClass.h"

namespace {
DWORD rules_hash = 0, art_hash = 0, ai_hash = 0;
CCINIClass* rules = nullptr;
CCINIClass ai, art, ui, settings;
}
DWORD& CCINIClass::RulesHash = rules_hash;
DWORD& CCINIClass::ArtHash = art_hash;
DWORD& CCINIClass::AIHash = ai_hash;
CCINIClass*& CCINIClass::INI_Rules = rules;
CCINIClass& CCINIClass::INI_AI = ai;
CCINIClass& CCINIClass::INI_Art = art;
CCINIClass& CCINIClass::INI_UIMD = ui;
CCINIClass& CCINIClass::INI_RA2MD = settings;
