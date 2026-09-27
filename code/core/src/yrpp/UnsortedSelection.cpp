// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9, init.cpp Best_Selected_Object.
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md. YR 0x005353D0 adds Berzerk and distance.
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <bit>
#include <climits>
#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif

ObjectClass* YRPP_FASTCALL Unsorted::BestSelectedObject(const CellStruct* cell,ObjectClass* target) noexcept {
    try {
        ObjectClass* best=nullptr;
        int best_priority=-1,best_distance=INT_MAX;
        // Do not snapshot the count: the original re-reads the live selection
        // after its virtual calls. Lower-priority entries never query distance.
        for(int i=0;i<ObjectClass::CurrentObjects.Count;++i) {
            auto* object=ObjectClass::CurrentObjects[i];
            int priority=0;
            if(object && (object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None) {
                auto* techno=static_cast<TechnoClass*>(object);
                priority=2;
                if(techno->Berzerk)priority=0;
                else if(techno->IsUnderEMP())priority=1;
                else if(techno->IsArmed()) {
                    priority=3;
                    if(techno->WhatAmI()!=AbstractType::Building) {
                        priority=4;
                        if(techno->CombatDamage(-1)>0)priority=5;
                    }
                }
            }
            if(priority<best_priority)continue;
            int distance=INT_MAX;
            if(cell || target) {
                CoordStruct from_buffer,to_buffer;
                const auto from=*object->GetCoords(&from_buffer);
                const auto to=cell ? CoordStruct{int(cell->X)*256+128,int(cell->Y)*256+128,0} : *target->GetCoords(&to_buffer);
                const auto delta=[](int a,int b) noexcept {return std::bit_cast<int>(unsigned(a)-unsigned(b));};
                const double x=delta(from.X,to.X),y=delta(from.Y,to.Y),z=delta(from.Z,to.Z);
                distance=game::x87_integer(Math::sqrt(x*x+z*z+y*y));
            }
            if(priority>best_priority || distance<best_distance) {
                best=object;best_priority=priority;best_distance=distance;
            }
        }
        return best;
    } catch(...) {
        return nullptr;
    }
}
