// Original missing-member enumeration, YR 0x006EF4D0.
#include "yrpp/TeamClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/TaskForceClass.h"

void TeamClass::GetTaskForceMissingMemberTypes(DynamicVectorClass<TechnoTypeClass*>& dest) const {
    // Append first, then remove one occurrence for each linked member. The
    // destination need not be empty and duplicate task-force entries matter.
    for (int i = 0; i < Type->TaskForce->CountEntries; ++i) {
        const auto entry = Type->TaskForce->Entries[i];
        for (int remaining = entry.Amount; remaining > 0; --remaining)
            dest.AddItem(entry.Type);
    }
    for (auto* member = FirstUnit; member; member = member->NextTeamMember)
        dest.Remove(member->GetTechnoType());
}
