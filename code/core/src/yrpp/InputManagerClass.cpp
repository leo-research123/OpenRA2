// Original normalized virtual-key state (54F5C0 -> 53EC90). OS left/right
// swapping belongs to the device adapter before it submits normalized keys.
#include "yrpp/InputManagerClass.h"
#include "yrpp/GeneralStructures.h"
#include <algorithm>
InputManagerClass::InputManagerClass() noexcept
    : field_0{}, field_4{}, field_8{}, field_C{}, field_10{}, Keycodes_b{}, Keycodes_w{}, field_314{}, field_318{} {}
bool InputManagerClass::IsKeyPressed(int key) const { return key >= 0 && key < 256 && Keycodes_b[key] != byte{}; }
void InputManagerClass::SetKeyState(int key, bool pressed) noexcept {
    if (key >= 0 && key < 256) Keycodes_b[key] = pressed ? byte{1} : byte{0};
}
void InputManagerClass::SetClickPosition(const Point2D& point) noexcept {
    field_0 = point.X;
    field_4 = point.Y;
}
void InputManagerClass::Reset() noexcept {
    std::fill_n(Keycodes_b, 256, byte{});
    std::fill_n(Keycodes_w, 256, 0);
    field_0 = field_4 = field_8 = field_C = field_10 = field_314 = field_318 = 0;
}
namespace {
constexpr int key_queue_size = 256;
}
WORD InputManagerClass::Check() noexcept { return field_314 == field_318 ? WORD{0} : Keycodes_w[field_314]; }
WORD InputManagerClass::Get() noexcept {
    // 0x54F050 spins on the message pump until a non-zero key is at the head;
    // an empty queue returns no key instead.
    if (field_314 == field_318 || !Keycodes_w[field_314]) return 0;
    const int tail = field_318;
    const WORD key = Keycodes_w[field_314];
    field_314 = (field_314 + 1) % key_queue_size;
    const auto pop = [&]() -> WORD {
        if (field_314 == tail) return 0;
        const WORD word = Keycodes_w[field_314];
        field_314 = (field_314 + 1) % key_queue_size;
        return word;
    };
    switch (static_cast<BYTE>(key)) {
        case 1:
        case 2:
        case 4: {
            const WORD x = pop();
            const WORD y = pop();
            field_0 = field_C = x;
            field_4 = field_10 = y;
            break;
        }
        case 10:
            field_8 = static_cast<int>((static_cast<unsigned>(field_8) & 0xFFFF0000u) | pop());
            break;
        default:
            break;
    }
    return key;
}
bool InputManagerClass::Put(WORD key) noexcept {
    const int next = (field_318 + 1) % key_queue_size;
    if (next == field_314) return false;
    Keycodes_w[field_318] = key;
    field_318 = next;
    return true;
}
void InputManagerClass::Clear() noexcept { field_314 = field_318; }
