// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 storage.cpp; YR 0x6C9600..0x6C96B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/AbstractClass.h"
#include "yrpp/TiberiumClass.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
namespace {
float* slot(StorageClass& store,int index){
 switch(index){case 0:return &store.Tiberium1;case 1:return &store.Tiberium2;case 2:return &store.Tiberium3;case 3:return &store.Tiberium4;default:return nullptr;}
}
}
float StorageClass::GetAmount(int index) const {auto* value=slot(const_cast<StorageClass&>(*this),index);return value?*value:0.0f;}
float StorageClass::GetTotalAmount() const {int amount=0;for(int i=0;i<4;++i)amount=rule_integer(double(amount)+GetAmount(i));return float(amount);}
float StorageClass::AddAmount(float amount,int index){auto* value=slot(*this,index);if(!value)return 0;*value+=amount;return *value;}
float StorageClass::RemoveAmount(float amount,int index){auto* value=slot(*this,index);if(!value)return 0;const float taken=std::min(amount,*value);*value-=taken;return taken;}
int StorageClass::GetTotalValue() const {int value=0;for(int i=0;i<4&&i<TiberiumClass::Array.Count;++i)if(GetAmount(i)>0)value=rule_integer(double(value)+double(GetAmount(i))*TiberiumClass::Array[i]->Value);return value;}
