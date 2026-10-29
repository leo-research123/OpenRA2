#include "text_encoding.hpp"
#include <gtest/gtest.h>
#include <cstring>
#include <string>

using game::TextDecodeStatus;

TEST(TextEncoding, Utf8AndBoundaries) {
    game::TextEncoding encoding;
    ASSERT_EQ(game::detect_text_encoding("中文 UTF-8", 12, encoding), TextDecodeStatus::success);
    EXPECT_STREQ(encoding.name, "UTF-8");
    char output[64]{};
    int written = -1;
    ASSERT_EQ(game::decode_text_utf8("中文 UTF-8", 12, encoding, output, sizeof(output), written),
              TextDecodeStatus::success);
    EXPECT_STREQ(output, "中文 UTF-8");
    EXPECT_EQ(written, 12);
    EXPECT_EQ(game::decode_text_utf8("中文", 6, encoding, output, 3, written), TextDecodeStatus::buffer_too_small);
    EXPECT_EQ(written, 0);
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(game::detect_text_encoding(nullptr, 1, encoding), TextDecodeStatus::invalid_argument);
    EXPECT_EQ(encoding.name[0], 0);
    EXPECT_EQ(game::decode_text_utf8(nullptr, -1, encoding, output, sizeof(output), written),
              TextDecodeStatus::invalid_argument);
    EXPECT_EQ(game::detect_text_encoding(nullptr, 0, encoding), TextDecodeStatus::success);
    EXPECT_EQ(game::decode_text_utf8(nullptr, 0, encoding, output, 1, written), TextDecodeStatus::success);
}

TEST(TextEncoding, SimplifiedDocument) {
    const unsigned char bytes[] = {0xCE, 0xD2, 0xC3, 0xC7, 0xD0, 0xE8, 0xD2, 0xAA, 0xCD, 0xEA, 0xB3, 0xC9, 0xD5,
                                   0xE2, 0xB8, 0xF6, 0xC8, 0xCE, 0xCE, 0xF1, 0xA3, 0xAC, 0xB1, 0xA3, 0xBB, 0xA4,
                                   0xBB, 0xF9, 0xB5, 0xD8, 0xA3, 0xAC, 0xD6, 0xB8, 0xBB, 0xD3, 0xB2, 0xBF, 0xB6,
                                   0xD3, 0xBB, 0xF7, 0xB0, 0xDC, 0xB5, 0xD0, 0xC8, 0xCB, 0xA1, 0xA3};
    const auto* field = reinterpret_cast<const char*>(bytes);
    std::string sample;
    for (int i = 0; i < 16; ++i) {
        sample.append(field, sizeof(bytes));
        sample.push_back('\n');
    }
    game::TextEncoding encoding;
    ASSERT_EQ(game::detect_text_encoding(sample.data(), static_cast<int>(sample.size()), encoding),
              TextDecodeStatus::success);
    char output[512]{};
    int written = 0;
    ASSERT_EQ(game::decode_text_utf8(field, sizeof(bytes), encoding, output, sizeof(output), written),
              TextDecodeStatus::success)
        << encoding.name;
    EXPECT_STREQ(output, "我们需要完成这个任务，保护基地，指挥部队击败敌人。") << encoding.name;
    ASSERT_EQ(game::decode_text_utf8("UTF-8 编辑", 12, encoding, output, sizeof(output), written),
              TextDecodeStatus::success);
    EXPECT_STREQ(output, "UTF-8 编辑");
}

TEST(TextEncoding, TraditionalDocument) {
    const unsigned char bytes[] = {0xA7, 0xDA, 0xAD, 0xCC, 0xBB, 0xDD, 0xAD, 0x6E, 0xA7, 0xB9, 0xA6, 0xA8, 0xB3,
                                   0x6F, 0xAD, 0xD3, 0xA5, 0xF4, 0xB0, 0xC8, 0xA1, 0x41, 0xAB, 0x4F, 0xC5, 0x40,
                                   0xB0, 0xF2, 0xA6, 0x61, 0xA1, 0x41, 0xAB, 0xFC, 0xB4, 0xA7, 0xB3, 0xA1, 0xB6,
                                   0xA4, 0xC0, 0xBB, 0xB1, 0xD1, 0xBC, 0xC4, 0xA4, 0x48, 0xA1, 0x43};
    const auto* field = reinterpret_cast<const char*>(bytes);
    std::string sample;
    for (int i = 0; i < 16; ++i) {
        sample.append(field, sizeof(bytes));
        sample.push_back('\n');
    }
    game::TextEncoding encoding;
    ASSERT_EQ(game::detect_text_encoding(sample.data(), static_cast<int>(sample.size()), encoding),
              TextDecodeStatus::success);
    char output[512]{};
    int written = 0;
    ASSERT_EQ(game::decode_text_utf8(field, sizeof(bytes), encoding, output, sizeof(output), written),
              TextDecodeStatus::success)
        << encoding.name;
    EXPECT_STREQ(output, "我們需要完成這個任務，保護基地，指揮部隊擊敗敵人。") << encoding.name;
    ASSERT_EQ(game::decode_text_utf8("UTF-8 编辑", 12, encoding, output, sizeof(output), written),
              TextDecodeStatus::success);
    EXPECT_STREQ(output, "UTF-8 编辑");
}

TEST(TextEncoding, JapaneseDocument) {
    const unsigned char bytes[] = {0x82, 0xB1, 0x82, 0xCC, 0x94, 0x43, 0x96, 0xB1, 0x82, 0xF0, 0x8A, 0xAE, 0x97,
                                   0xB9, 0x82, 0xB5, 0x82, 0xC4, 0x8A, 0xEE, 0x92, 0x6E, 0x82, 0xF0, 0x8E, 0xE7,
                                   0x82, 0xE8, 0x81, 0x41, 0x95, 0x94, 0x91, 0xE0, 0x82, 0xF0, 0x8E, 0x77, 0x8A,
                                   0xF6, 0x82, 0xB5, 0x82, 0xC4, 0x93, 0x47, 0x82, 0xF0, 0x93, 0x7C, 0x82, 0xB5,
                                   0x82, 0xC4, 0x82, 0xAD, 0x82, 0xBE, 0x82, 0xB3, 0x82, 0xA2, 0x81, 0x42};
    const auto* field = reinterpret_cast<const char*>(bytes);
    std::string sample;
    for (int i = 0; i < 16; ++i) {
        sample.append(field, sizeof(bytes));
        sample.push_back('\n');
    }
    game::TextEncoding encoding;
    ASSERT_EQ(game::detect_text_encoding(sample.data(), static_cast<int>(sample.size()), encoding),
              TextDecodeStatus::success);
    char output[512]{};
    int written = 0;
    ASSERT_EQ(game::decode_text_utf8(field, sizeof(bytes), encoding, output, sizeof(output), written),
              TextDecodeStatus::success)
        << encoding.name;
    EXPECT_STREQ(output, "この任務を完了して基地を守り、部隊を指揮して敵を倒してください。") << encoding.name;
    ASSERT_EQ(game::decode_text_utf8("UTF-8 编辑", 12, encoding, output, sizeof(output), written),
              TextDecodeStatus::success);
    EXPECT_STREQ(output, "UTF-8 编辑");
}

TEST(TextEncoding, RussianDocument) {
    const unsigned char bytes[] = {0xCD, 0xE5, 0xEE, 0xE1, 0xF5, 0xEE, 0xE4, 0xE8, 0xEC, 0xEE, 0x20, 0xE2, 0xFB,
                                   0xEF, 0xEE, 0xEB, 0xED, 0xE8, 0xF2, 0xFC, 0x20, 0xE7, 0xE0, 0xE4, 0xE0, 0xED,
                                   0xE8, 0xE5, 0x2C, 0x20, 0xE7, 0xE0, 0xF9, 0xE8, 0xF2, 0xE8, 0xF2, 0xFC, 0x20,
                                   0xE1, 0xE0, 0xE7, 0xF3, 0x20, 0xE8, 0x20, 0xF3, 0xED, 0xE8, 0xF7, 0xF2, 0xEE,
                                   0xE6, 0xE8, 0xF2, 0xFC, 0x20, 0xE2, 0xF0, 0xE0, 0xE6, 0xE5, 0xF1, 0xEA, 0xE8,
                                   0xE5, 0x20, 0xE2, 0xEE, 0xE9, 0xF1, 0xEA, 0xE0, 0x2E};
    const auto* field = reinterpret_cast<const char*>(bytes);
    std::string sample;
    for (int i = 0; i < 16; ++i) {
        sample.append(field, sizeof(bytes));
        sample.push_back('\n');
    }
    game::TextEncoding encoding;
    ASSERT_EQ(game::detect_text_encoding(sample.data(), static_cast<int>(sample.size()), encoding),
              TextDecodeStatus::success);
    char output[512]{};
    int written = 0;
    ASSERT_EQ(game::decode_text_utf8(field, sizeof(bytes), encoding, output, sizeof(output), written),
              TextDecodeStatus::success)
        << encoding.name;
    EXPECT_STREQ(output, "Необходимо выполнить задание, защитить базу и уничтожить вражеские войска.") << encoding.name;
    ASSERT_EQ(game::decode_text_utf8("UTF-8 编辑", 12, encoding, output, sizeof(output), written),
              TextDecodeStatus::success);
    EXPECT_STREQ(output, "UTF-8 编辑");
}

TEST(TextEncoding, Iso2022JapaneseDocument) {
    const unsigned char bytes[] = {0x1B, 0x24, 0x42, 0x24, 0x33, 0x24, 0x4E, 0x47, 0x24, 0x4C, 0x33, 0x24, 0x72, 0x34,
                                   0x30, 0x4E, 0x3B, 0x24, 0x37, 0x24, 0x46, 0x34, 0x70, 0x43, 0x4F, 0x24, 0x72, 0x3C,
                                   0x69, 0x24, 0x6A, 0x21, 0x22, 0x49, 0x74, 0x42, 0x62, 0x24, 0x72, 0x3B, 0x58, 0x34,
                                   0x78, 0x24, 0x37, 0x24, 0x46, 0x45, 0x28, 0x24, 0x72, 0x45, 0x5D, 0x24, 0x37, 0x24,
                                   0x46, 0x24, 0x2F, 0x24, 0x40, 0x24, 0x35, 0x24, 0x24, 0x21, 0x23, 0x1B, 0x28, 0x42};
    const auto* field = reinterpret_cast<const char*>(bytes);
    std::string sample;
    for (int i = 0; i < 16; ++i) {
        sample.append(field, sizeof(bytes));
        sample.push_back('\n');
    }
    game::TextEncoding encoding;
    ASSERT_EQ(game::detect_text_encoding(sample.data(), static_cast<int>(sample.size()), encoding),
              TextDecodeStatus::success);
    char output[512]{};
    int written = 0;
    ASSERT_EQ(game::decode_text_utf8(field, sizeof(bytes), encoding, output, sizeof(output), written),
              TextDecodeStatus::success)
        << encoding.name;
    EXPECT_STREQ(output, "この任務を完了して基地を守り、部隊を指揮して敵を倒してください。") << encoding.name;
    ASSERT_EQ(game::decode_text_utf8("UTF-8 编辑", 12, encoding, output, sizeof(output), written),
              TextDecodeStatus::success);
    EXPECT_STREQ(output, "UTF-8 编辑");
}

TEST(TextEncoding, InvalidTextAndAmbiguousSample) {
    game::TextEncoding encoding;
    const char short_text[] = {static_cast<char>(0xD6), static_cast<char>(0xD0)};
    EXPECT_EQ(game::detect_text_encoding(short_text, sizeof(short_text), encoding), TextDecodeStatus::unknown_encoding);
    EXPECT_EQ(encoding.name[0], 0);
    char output[32] = "previous";
    int written = -1;
    EXPECT_EQ(game::decode_text_utf8(short_text, sizeof(short_text), encoding, output, sizeof(output), written),
              TextDecodeStatus::unknown_encoding);
    std::strcpy(encoding.name, "GB18030");
    ASSERT_EQ(game::decode_text_utf8(short_text, sizeof(short_text), encoding, output, sizeof(output), written),
              TextDecodeStatus::success);
    EXPECT_STREQ(output, "中");
    const char invalid[] = {static_cast<char>(0xFF)};
    EXPECT_EQ(game::decode_text_utf8(invalid, 1, encoding, output, sizeof(output), written),
              TextDecodeStatus::invalid_text);
    EXPECT_EQ(written, 0);
    EXPECT_EQ(output[0], 0);
}

TEST(TextEncoding, UnicodeSignatures) {
    const char utf16[] = {static_cast<char>(0xFF), static_cast<char>(0xFE), 0x2D, 0x4E, static_cast<char>(0x87), 0x65};
    EXPECT_FALSE(game::is_utf8_text(utf16, sizeof(utf16)));
    game::TextEncoding encoding;
    ASSERT_EQ(game::detect_text_encoding(utf16, sizeof(utf16), encoding), TextDecodeStatus::success);
    char output[32]{};
    int written = 0;
    ASSERT_EQ(game::decode_text_utf8(utf16, sizeof(utf16), encoding, output, sizeof(output), written),
              TextDecodeStatus::success);
    EXPECT_STREQ(output, "中文");
    const char utf16_field[] = {0x2D, 0x4E, 0x32, 0x4E};
    ASSERT_EQ(game::decode_text_utf8(utf16_field, sizeof(utf16_field), encoding, output, sizeof(output), written),
              TextDecodeStatus::success);
    EXPECT_STREQ(output, "中串");
    const char utf8[] = {static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF), 'A'};
    ASSERT_EQ(game::detect_text_encoding(utf8, sizeof(utf8), encoding), TextDecodeStatus::success);
    ASSERT_EQ(game::decode_text_utf8(utf8, sizeof(utf8), encoding, output, sizeof(output), written),
              TextDecodeStatus::success);
    EXPECT_STREQ(output, "A");
}
