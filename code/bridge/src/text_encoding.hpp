#pragma once

namespace game {
struct TextEncoding {
    char name[64]{};
    int confidence = 0;
};
enum class TextDecodeStatus { success, invalid_argument, unknown_encoding, invalid_text, buffer_too_small };

// ICU detection uses a document sample, not an isolated short label. Failure
// leaves output empty. These internal boundaries never propagate exceptions.
bool is_utf8_text(const char* text, int size) noexcept;
TextDecodeStatus detect_text_encoding(const char* sample, int size, TextEncoding& output) noexcept;
TextDecodeStatus decode_text_utf8(const char* text, int size, const TextEncoding& encoding, char* output, int capacity,
                                  int& written) noexcept;
}
