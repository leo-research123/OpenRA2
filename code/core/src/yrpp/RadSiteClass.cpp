// Original YR RadSiteClass, 0x0065B1E0..0x0065BED0.
#include "yrpp/RadSiteClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "RulesClassReaders.hpp"
#include <bit>
#include <cstdlib>
#include <cstring>

namespace {
DynamicVectorClass<RadSiteClass*> sites;
int wrap(unsigned value) { return std::bit_cast<int>(value); }
int product(int a, int b) { return wrap(unsigned(a) * unsigned(b)); }
template <class Action>
void visit(RadSiteClass& site, Action action) {
    const int left = wrap(unsigned(int(site.BaseCell.X)) - unsigned(site.Spread));
    const int top = wrap(unsigned(int(site.BaseCell.Y)) - unsigned(site.Spread));
    const int right = wrap(unsigned(left) + 2u * unsigned(site.Spread) + 1u);
    const int bottom = wrap(unsigned(top) + 2u * unsigned(site.Spread) + 1u);
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            CellStruct at{static_cast<short>(x), static_cast<short>(y)};
            const double level = site.GetRadLevelAt(&at);
            action(*MapClass::Instance.GetCellAt(at), level);
        }
    }
}
}

DynamicVectorClass<RadSiteClass*>& RadSiteClass::Array = sites;

RadSiteClass::RadSiteClass() noexcept
    : AbstractClass(), LightSource(nullptr), RadLevelTimer(0), RadLightTimer(0), BaseCell{0, 0}, Spread(0),
      SpreadInLeptons(0), RadLevel(0), LevelSteps(0), Intensity(0), Tint{}, IntensitySteps(0), IntensityDecrement(0),
      RadDuration(0), RadTimeLeft(0) {
    if (!Array.AddItem(this)) std::abort();
}

RadSiteClass::~RadSiteClass() {
    MapClass::Instance.GetCellAt(BaseCell)->SetRadSite(nullptr);
    NotifyObjectExpired(true);
    if (LightSource) {
        GameDelete(LightSource);
        LightSource = nullptr;
    }
    Array.Remove(this);
}

HRESULT YRPP_STDCALL RadSiteClass::GetClassID(CLSID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    const DWORD words[]{0x4104D740u, 0x11D3D507u, 0xA000388Cu, 0x44BE33C9u};
    std::memcpy(id, words, sizeof(words));
    return 0;
}

void RadSiteClass::SetBaseCell(CellStruct* cell) { BaseCell = *cell; }
void RadSiteClass::SetSpread(int cells) {
    Spread = cells;
    SpreadInLeptons = wrap((unsigned(cells) << 8) + 128u);
}
int RadSiteClass::GetSpread() { return Spread; }
void RadSiteClass::SetRadLevel(int level) {
    RadLevel = level;
    RadDuration = product(level, RulesClass::Instance->RadDurationMultiple);
    RadTimeLeft = RadDuration;
}
int RadSiteClass::GetRadLevel() { return RadDuration > 0 ? product(RadLevel, RadTimeLeft) / RadDuration : 0; }

double RadSiteClass::GetRadLevelAt(CellStruct* cell) {
    auto* base = MapClass::Instance.GetCellAt(BaseCell);
    auto* target = MapClass::Instance.GetCellAt(*cell);
    const auto origin = base->GetCoords();
    const auto position = target->GetCoords();
    const int x = wrap(unsigned(origin.X) - unsigned(position.X));
    const int y = wrap(unsigned(origin.Y) - unsigned(position.Y));
    const int z = wrap(unsigned(origin.Z) - unsigned(position.Z));
    const int distance = rule_integer(Math::sqrt(double(x) * x + double(z) * z + double(y) * y));
    return distance <= SpreadInLeptons ? double(wrap(unsigned(SpreadInLeptons) - unsigned(distance))) /
                                             double(SpreadInLeptons) * double(RadLevel)
                                       : 0.0;
}

void RadSiteClass::Activate() {
    const auto& rules = *RulesClass::Instance;
    RadLevelTimer.Start(rules.RadLevelDelay);
    RadLightTimer.Start(rules.RadLightDelay);
    int red = rules.RadColor.R, green = rules.RadColor.G, blue = rules.RadColor.B;
    // The original snow path uses signed -128 for red and blue.
    if (ScenarioClass::Instance->Theater == TheaterType::Snow) red = blue = -128;
    const auto scale = [](double value) { return rule_integer(value > 2000.0 ? 2000.0 : value); };
    Intensity = scale(double(RadLevel) * rules.RadLightFactor);
    Tint.Red = scale(double(1000 * red / 255) * rules.RadTintFactor);
    Tint.Green = scale(double(1000 * green / 255) * rules.RadTintFactor);
    Tint.Blue = scale(double(1000 * blue / 255) * rules.RadTintFactor);
    LevelSteps = RadDuration / rules.RadLevelDelay;
    IntensitySteps = RadDuration / rules.RadLightDelay;
    IntensityDecrement = Intensity / IntensitySteps;
    const auto at = MapClass::Instance.GetCellAt(BaseCell)->GetCoords();
    if (LightSource) {
        LightSource->ChangeLevels(Intensity, Tint, 0);
    } else {
        LightSource = GameCreate<LightSourceClass>(at, SpreadInLeptons, Intensity, Tint);
        LightSource->DetailLevel = 0;
        LightSource->Activate();
    }
    Radiate();
}

void RadSiteClass::Radiate() {
    visit(*this, [](CellClass& cell, double level) { cell.RadLevel_Increase(level); });
}
void RadSiteClass::DecreaseRadiation() {
    visit(*this, [this](CellClass& cell, double level) { cell.RadLevel_Decrease(level / double(LevelSteps)); });
}
void RadSiteClass::Deactivate() {
    const int steps = wrap(unsigned(RadTimeLeft / RulesClass::Instance->RadLevelDelay) + 1u);
    visit(*this, [this, steps](CellClass& cell, double level) {
        cell.RadLevel_Decrease(level / double(LevelSteps) * double(steps));
    });
}
void RadSiteClass::Add(int amount) {
    const int remaining = rule_integer(double(RadLevel) * double(RadTimeLeft) / double(RadDuration));
    Deactivate();
    SetRadLevel(wrap(unsigned(remaining) + unsigned(amount)));
    Activate();
}
void RadSiteClass::DecreaseLight() {
    const TintStruct tint{product(RadTimeLeft, Tint.Red) / RadDuration, product(RadTimeLeft, Tint.Green) / RadDuration,
                          product(RadTimeLeft, Tint.Blue) / RadDuration};
    LightSource->ChangeLevels(wrap(unsigned(LightSource->LightIntensity) - unsigned(IntensityDecrement)), tint, 0);
}
void RadSiteClass::Update() {
    RadTimeLeft = wrap(unsigned(RadTimeLeft) - 1u);
    if (!RadLevelTimer.GetTimeLeft()) {
        DecreaseRadiation();
        RadLevelTimer.Start(RulesClass::Instance->RadLevelDelay);
    }
    if (!RadLightTimer.GetTimeLeft()) {
        DecreaseLight();
        RadLightTimer.Start(RulesClass::Instance->RadLightDelay);
    }
    if (RadTimeLeft <= 0) GameDelete(this);
}
