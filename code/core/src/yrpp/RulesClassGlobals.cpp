#include "yrpp/RulesClass.h"

namespace {
RulesClass* instance = nullptr;
}
RulesClass*& RulesClass::Instance = instance;
