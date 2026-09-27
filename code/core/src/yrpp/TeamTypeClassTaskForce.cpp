// YR 0x6F1FA0 / 0x6F2040. This post-pass is absent from OpenTS 44fac744;
// calibrated against the target instead of substituting TS team movement rules.
#include "yrpp/MapClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TechnoTypeClass.h"
namespace {
// 0x5889F0: choose the first most-permissive zone compatible with both inputs.
int intersect_movement(int first, int second) {
  if (first < 0 || first >= 13 || second < 0 || second >= 13)
    return -1;
  const auto &table = MapClass::MovementAdjustArray;
  int result = -1, best = 0;
  for (int zone = 0; zone < 13; ++zone) {
    bool compatible = true;
    int matches = 0;
    for (int cell = 0; cell < 8; ++cell) {
      if ((table[first][cell] == 2 || table[second][cell] == 2) &&
          table[zone][cell] == 1)
        compatible = false;
      if (table[first][cell] == 1 && table[second][cell] == 1 &&
          table[zone][cell] == 1)
        ++matches;
    }
    if (compatible && matches > best) {
      best = matches;
      result = zone;
    }
  }
  return result;
}
} // namespace
void TeamTypeClass::ProcessTaskForce() {
  field_F0 = true;
  field_F1 = false;
  field_EC = 9;
  // A missing task force is malformed input in the original (null dereference).
  // Leave constructor defaults for that undefined-input case in the native
  // core.
  if (TaskForce)
    for (int i = 0; i < TaskForce->CountEntries; ++i) {
      const auto *type = TaskForce->Entries[i].Type;
      if (type->Naval) {
        if (type->Passengers)
          field_F1 = true;
        else
          field_F0 = false;
      }
      field_EC =
          intersect_movement(static_cast<int>(type->MovementZone), field_EC);
    }
  if (IsBaseDefense)
    field_F0 = false;
}
void TeamTypeClass::ProcessAllTaskforces() {
  for (auto *type : Array)
    type->ProcessTaskForce();
}
