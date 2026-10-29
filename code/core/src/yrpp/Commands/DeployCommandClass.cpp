// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/DeployCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"

#if !defined(RA2_YRPP_GAME)
const char* DeployCommandClass::GetName() const { return "DeployObject"; }

const wchar_t* DeployCommandClass::GetUIName() const { return StringTable::LoadString("TXT_DEPLOY_OBJECT"); }

const wchar_t* DeployCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_CONTROL"); }

const wchar_t* DeployCommandClass::GetUIDescription() const {
    return StringTable::LoadString("TXT_DEPLOY_OBJECT_DESC");
}

void DeployCommandClass::Execute(WWKey) const { Game::DeploySelected(); }
#endif
