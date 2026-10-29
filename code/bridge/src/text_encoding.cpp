#include "text_encoding.hpp"
#include <unicode/ucsdet.h>
#include <unicode/ucnv.h>
#include <unicode/ucnv_err.h>
#include <unicode/ustring.h>
#include <cstring>
#include <memory>

namespace game {
namespace {
void save_encoding(TextEncoding& output, const char* name, int confidence) noexcept {
    const auto size = std::strlen(name);
    if (size >= sizeof(output.name)) return;
    std::memcpy(output.name, name, size + 1);
    output.confidence = confidence;
}
}

bool is_utf8_text(const char* text, int size) noexcept {
    if (size < 0 || (!text && size)) return false;
    if (!size) return true;
    // NUL can indicate UTF-16/32; ESC can introduce ISO-2022 sequences.
    // Neither should bypass the charset detector merely as valid UTF-8 bytes.
    if (std::memchr(text, 0, size) || std::memchr(text, 0x1B, size)) return false;
    UErrorCode status = U_ZERO_ERROR;
    int32_t length = 0;
    u_strFromUTF8(nullptr, 0, &length, text, size, &status);
    return U_SUCCESS(status) || status == U_BUFFER_OVERFLOW_ERROR;
}

TextDecodeStatus detect_text_encoding(const char* sample, int size, TextEncoding& output) noexcept {
    output = {};
    if (size < 0 || (!sample && size)) return TextDecodeStatus::invalid_argument;
    if (!size) {
        save_encoding(output, "UTF-8", 100);
        return TextDecodeStatus::success;
    }
    UErrorCode status = U_ZERO_ERROR;
    int32_t signature_size = 0;
    const auto* signature = ucnv_detectUnicodeSignature(sample, size, &signature_size, &status);
    if (U_FAILURE(status)) return TextDecodeStatus::invalid_text;
    if (signature) {
        save_encoding(output, signature, 100);
        return TextDecodeStatus::success;
    }
    if (is_utf8_text(sample, size)) {
        save_encoding(output, "UTF-8", 100);
        return TextDecodeStatus::success;
    }
    std::unique_ptr<UCharsetDetector, decltype(&ucsdet_close)> detector(ucsdet_open(&status), ucsdet_close);
    if (U_FAILURE(status) || !detector) return TextDecodeStatus::unknown_encoding;
    ucsdet_setText(detector.get(), sample, size, &status);
    const auto* match = ucsdet_detect(detector.get(), &status);
    if (U_FAILURE(status) || !match) return TextDecodeStatus::unknown_encoding;
    const int confidence = ucsdet_getConfidence(match, &status);
    const auto* name = ucsdet_getName(match, &status);
    // ICU's low-confidence guesses include unrelated UTF-16 interpretations
    // of tiny legacy strings. Do not silently publish those as valid text.
    if (U_FAILURE(status) || !name || confidence < 30) return TextDecodeStatus::unknown_encoding;
    save_encoding(output, name, confidence);
    return *output.name ? TextDecodeStatus::success : TextDecodeStatus::unknown_encoding;
}

TextDecodeStatus decode_text_utf8(const char* text, int size, const TextEncoding& encoding, char* output, int capacity,
                                  int& written) noexcept {
    written = 0;
    if (output && capacity > 0) output[0] = 0;
    if (size < 0 || (!text && size) || !output || capacity <= 0) return TextDecodeStatus::invalid_argument;
    const bool fixed_width_unicode =
        std::strncmp(encoding.name, "UTF-16", 6) == 0 || std::strncmp(encoding.name, "UTF-32", 6) == 0;
    // ASCII and valid UTF-8 fields stay readable in mixed legacy documents.
    // A known UTF-16/32 document can contain fields made entirely of ASCII
    // bytes; those still need its fixed-width converter.
    if (!size || (!fixed_width_unicode && is_utf8_text(text, size))) {
        if (size >= 3 && std::memcmp(text, "\xEF\xBB\xBF", 3) == 0) {
            text += 3;
            size -= 3;
        }
        if (size >= capacity) return TextDecodeStatus::buffer_too_small;
        if (size) std::memcpy(output, text, size);
        output[size] = 0;
        written = size;
        return TextDecodeStatus::success;
    }
    if (!*encoding.name) return TextDecodeStatus::unknown_encoding;
    if (!std::memchr(encoding.name, 0, sizeof(encoding.name))) return TextDecodeStatus::invalid_argument;
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<UConverter, decltype(&ucnv_close)> source(ucnv_open(encoding.name, &status), ucnv_close);
    std::unique_ptr<UConverter, decltype(&ucnv_close)> target(ucnv_open("UTF-8", &status), ucnv_close);
    if (U_FAILURE(status) || !source || !target) return TextDecodeStatus::unknown_encoding;
    ucnv_setToUCallBack(source.get(), UCNV_TO_U_CALLBACK_STOP, nullptr, nullptr, nullptr, &status);
    ucnv_setFromUCallBack(target.get(), UCNV_FROM_U_CALLBACK_STOP, nullptr, nullptr, nullptr, &status);
    auto* next = output;
    const auto* input = text;
    ucnv_convertEx(target.get(), source.get(), &next, output + capacity - 1, &input, text + size, nullptr, nullptr,
                   nullptr, nullptr, true, true, &status);
    if (U_FAILURE(status)) {
        output[0] = 0;
        return status == U_BUFFER_OVERFLOW_ERROR ? TextDecodeStatus::buffer_too_small : TextDecodeStatus::invalid_text;
    }
    written = static_cast<int>(next - output);
    if (written >= 3 && std::memcmp(output, "\xEF\xBB\xBF", 3) == 0) {
        written -= 3;
        std::memmove(output, output + 3, written);
    }
    output[written] = 0;
    return TextDecodeStatus::success;
}
}
