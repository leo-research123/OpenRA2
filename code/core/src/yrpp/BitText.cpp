// The native BitText instance (global 0x0089C4B8). Its DrawText (YR 0x00434CD0) is the presentation
// builder build_bit_text (presentation/ui/bit_text.cpp).
#include "yrpp/BitText.h"

namespace {
BitText text_instance;
BitText* text_pointer = &text_instance;
}
BitText*& BitText::Instance = text_pointer;
