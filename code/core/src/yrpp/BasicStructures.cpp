// Color and palette methods moved from YRpp BasicStructures.h; see yrpp/SOURCE.json.
#include "yrpp/BasicStructures.h"
#include <algorithm>
#include <cstring>

ColorStruct::ColorStruct(Color16Struct const color)
    : B(static_cast<BYTE>(color.B << 3u | color.B >> 2u)),
      G(static_cast<BYTE>(color.G << 2u | color.G >> 4u)),
      R(static_cast<BYTE>(color.R << 3u | color.R >> 2u)) {}

ColorStruct::ColorStruct(WORD const color) : ColorStruct(Color16Struct(color)) {}

ColorStruct::operator WORD() const { return static_cast<WORD>(Color16Struct(*this)); }

ColorStruct::ColorStruct(BYTE const r, BYTE const g, BYTE const b) : R(r), G(g), B(b) {}

ColorStruct::ColorStruct(const ColorStruct &c) : R(c.R), G(c.G), B(c.B) {}

ColorStruct::ColorStruct(DWORD const color) { memcpy(static_cast<void*>(this), &color, sizeof(ColorStruct)); }

bool ColorStruct::operator==(ColorStruct const rhs) const { return R == rhs.R && G == rhs.G && B == rhs.B; }

bool ColorStruct::operator!=(ColorStruct const rhs) const { return !(*this == rhs); }

ColorStruct ColorStruct::operator+(ColorStruct const rhs) const {
    return ColorStruct{(BYTE)std::min(255, this->R + rhs.R), (BYTE)std::min(255, this->G + rhs.G),
                       (BYTE)std::min(255, this->B + rhs.B)};
}

void ColorStruct::operator+=(ColorStruct const rhs) {
    this->R = (BYTE)std::min(255, this->R + rhs.R);
    this->G = (BYTE)std::min(255, this->G + rhs.G);
    this->B = (BYTE)std::min(255, this->B + rhs.B);
}

ColorStruct::operator DWORD() const {
    DWORD ret = 0;
    memcpy(&ret, this, sizeof(ColorStruct));
    return ret;
}

ColorStruct &BytePalette::operator[](int const idx) { return this->Entries[idx]; }

ColorStruct const &BytePalette::operator[](int const idx) const { return this->Entries[idx]; }

bool TintStruct::operator==(TintStruct const rhs) const {
    return Red == rhs.Red && Green == rhs.Green && Blue == rhs.Blue;
}

bool TintStruct::operator!=(TintStruct const rhs) const { return !(*this == rhs); }

bool TintStruct::operator<(TintStruct const rhs) const {
    if (Red < rhs.Red)
        return true;
    if (Green < rhs.Green)
        return true;
    if (Blue < rhs.Blue)
        return true;
    return false;
}

Color16Struct::Color16Struct(ColorStruct const color)
    : B(static_cast<unsigned short>(color.B >> 3u)), G(static_cast<unsigned short>(color.G >> 2u)),
      R(static_cast<unsigned short>(color.R >> 3u)) {}

Color16Struct::Color16Struct(WORD const color) { memcpy(this, &color, sizeof(Color16Struct)); }

Color16Struct::Color16Struct(DWORD const color) : Color16Struct(ColorStruct(color)) {}

bool Color16Struct::operator==(Color16Struct const rhs) const {
    return R == rhs.R && G == rhs.G && B == rhs.B;
}

bool Color16Struct::operator!=(Color16Struct const rhs) const { return !(*this == rhs); }

Color16Struct::operator WORD() const {
    WORD ret;
    memcpy(&ret, this, sizeof(Color16Struct));
    return ret;
}

Color16Struct::operator DWORD() const { return static_cast<DWORD>(ColorStruct(*this)); }

TintStruct::TintStruct(int r, int g, int b) : Red{r}, Green{g}, Blue{b} {}
