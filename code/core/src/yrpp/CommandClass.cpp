// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 _command.cpp; YR command list 0x87F658 / hotkeys 0x87F680.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/CommandClass.h"
namespace {
DynamicVectorClass<CommandClass*> commands;
IndexClass<int,CommandClass*> hotkeys;
}
DynamicVectorClass<CommandClass*>& CommandClass::Array=commands;
IndexClass<int,CommandClass*>& CommandClass::Hotkeys=hotkeys;
