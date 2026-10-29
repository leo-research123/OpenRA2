// Placement/removal base geometry, YR 0x00455F10 and 0x004561F0.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
namespace {
RectangleStruct spacing_area(const BuildingClass& building) {
    const int spacing = RulesClass::Instance->AIBaseSpacing;
    const auto& location = building.Location;
    return {short(location.X / 256 - spacing), short(location.Y / 256 - spacing),
            2 * spacing + building.Type->GetFoundationWidth(), 2 * spacing + building.Type->GetFoundationHeight(false)};
}
void update_perimeter(HouseClass& owner, const RectangleStruct& area, bool removing) {
    auto& map = MapClass::Instance;
    auto& cells = owner.Base.Cells_24;
    const unsigned mask = 1u << (unsigned(owner.ArrayIndex) & 31u);
    for (int x = area.X - 1; x < area.X + area.Width + 2; ++x)
        for (int y = area.Y - 1; y < area.Y + area.Height + 2; ++y) {
            const CellStruct at{short(x), short(y)};
            auto* cell = map.GetCellAt(at);
            const int neighbours = cell->GetBaseSpacingMask(owner.ArrayIndex);
            if (neighbours == 0xFF) {
                cells.Remove(at);
            } else if (neighbours > 0) {
                cells.AddUnique(at);
            } else if (removing) {
                cells.Remove(at);
                // The original queries the cell again after vector removal.
                map.GetCellAt(at)->BaseSpacerOfHouses &= ~mask;
            }
        }
}
}
void BuildingClass::UpdateBaseSpacing(bool rebuildOnly) {
    const auto area = spacing_area(*this);
    const unsigned mask = 1u << (unsigned(Owner->ArrayIndex) & 31u);
    for (int x = area.X; x < area.X + area.Width; ++x)
        for (int y = area.Y; y < area.Y + area.Height; ++y)
            MapClass::Instance.GetCellAt(CellStruct{short(x), short(y)})->BaseSpacerOfHouses |= mask;
    auto& bounds = Owner->Base.SpacingBounds;
    if (!bounds.X) bounds.X = area.X;
    if (!bounds.Y) bounds.Y = area.Y;
    if (area.X < bounds.X) {
        bounds.Width += bounds.X - area.X;
        bounds.X = area.X;
    }
    if (area.X + area.Width > bounds.X + bounds.Width) bounds.Width = area.X + area.Width - bounds.X;
    if (area.Y < bounds.Y) {
        bounds.Height += bounds.Y - area.Y;
        bounds.Y = area.Y;
    }
    if (area.Y + area.Height > bounds.Y + bounds.Height) bounds.Height = area.Y + area.Height - bounds.Y;
    if (!rebuildOnly) update_perimeter(*Owner, area, false);
}
void BuildingClass::RemoveBaseSpacing() {
    const auto area = spacing_area(*this);
    const int spacing = RulesClass::Instance->AIBaseSpacing;
    const unsigned mask = 1u << (unsigned(Owner->ArrayIndex) & 31u);
    auto& map = MapClass::Instance;
    for (int x = area.X; x < area.X + area.Width; ++x)
        for (int y = area.Y; y < area.Y + area.Height; ++y)
            map.GetCellAt(CellStruct{short(x), short(y)})->BaseSpacerOfHouses &= ~mask;
    // A neighbouring building can occur in several foundation cells. The
    // original repeats the mark for each occurrence and includes other owners.
    for (int x = area.X - spacing; x < area.X + area.Width + 2 * spacing; ++x)
        for (int y = area.Y - spacing; y < area.Y + area.Height + 2 * spacing; ++y) {
            auto* building = map.GetCellAt(CellStruct{short(x), short(y)})->GetBuilding();
            if (building && building != this && (!building->Type->UndeploysInto || !building->Type->ResourceGatherer) &&
                !building->IsStrange())
                building->UpdateBaseSpacing(true);
        }
    update_perimeter(*Owner, area, true);
}
