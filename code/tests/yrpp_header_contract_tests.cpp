#include "support/test_support.hpp"
// F02: this must remain the first include, on both host and Microsoft x86.
#include "yrpp/TechnoClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/CRC.h"
#include "yrpp/IndexClass.h"
#include "yrpp/PCX.h"
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {

static_assert(!std::is_copy_constructible_v<PCX> && !std::is_copy_assignable_v<PCX>);
static_assert(!std::is_move_constructible_v<PCX> && !std::is_move_assignable_v<PCX>);
static_assert(!std::is_copy_constructible_v<GenericList>);
static_assert(std::is_constructible_v<GenericNode, GenericNode&>);
static_assert(!std::is_constructible_v<GenericNode, const GenericNode&>);
static_assert(std::is_assignable_v<GenericNode&, GenericNode&>);
struct Item : Node<Item> {
    static inline int destroyed = 0;
    ~Item() override { ++destroyed; }
};
struct Prefix {
    virtual ~Prefix() = default;
    int value = 0;
};
struct OffsetItem : Prefix, Node<OffsetItem> {};
static_assert(std::is_same_v<decltype(std::declval<Item>().Next()), Item*>);
static_assert(std::is_same_v<decltype(std::declval<Item>().Prev()), Item*>);
static_assert(std::is_same_v<decltype(std::declval<Item>().MainList()), List<Item>*>);
static_assert(std::is_same_v<decltype(std::declval<OffsetItem>().Next()), OffsetItem*>);
static_assert(std::is_same_v<decltype(std::declval<OffsetItem>().Prev()), OffsetItem*>);
static_assert(std::is_same_v<decltype(std::declval<List<OffsetItem>>().First()), OffsetItem*>);
static_assert(std::is_same_v<decltype(std::declval<List<OffsetItem>>().Last()), OffsetItem*>);
static_assert(std::is_same_v<decltype(FileAccessMode::Read | FileAccessMode::Write),
    EnumFlagHelper<FileAccessMode>>);
constexpr bool file_flags() {
    FileAccessMode mode = FileAccessMode::None;
    mode |= FileAccessMode::Read;
    if (!(mode & FileAccessMode::Read) || (mode & FileAccessMode::Write)) return false;
    mode |= FileAccessMode::Write;
    mode &= ~FileAccessMode::Read;
    return mode == FileAccessMode::Write &&
        static_cast<FileAccessMode>((mode | FileAccessMode::Read) & FileAccessMode::ReadWrite)
            == FileAccessMode::ReadWrite;
}
static_assert(file_flags());
static_assert(std::is_convertible_v<const char*, RawFileClass>);
// Old subclass constructor forms remain available after the header split.
struct FileWrapper : FileClass {
    explicit FileWrapper(noinit_t tag) : FileClass(tag) {}
};
struct RawWrapper : RawFileClass {
    explicit RawWrapper(noinit_t tag) : RawFileClass(tag) {}
};

void list_contracts() {
    List<Item> first, second;
    Item a, b, c;
    EXPECT_TRUE((first.IsEmpty() && !a.MainList())) << "empty list/detached owner";
    first.AddHead(&a); first.AddTail(&b);
    EXPECT_TRUE((a.Next() == &b && b.Prev() == &a && a.MainList() == &first && b.MainList() == &first)) << "typed traversal and owner from every node";
    GenericNode copy(a);
    EXPECT_TRUE((a.GenericNode::Next() == &copy && copy.Next() == &b && copy.MainList() == &first)) << "node copy inserts after source";
    copy = copy;
    EXPECT_TRUE((copy.Prev() == &a && copy.Next() == &b)) << "node self assignment preserves links";
    second.AddHead(&c);
    copy = c;
    EXPECT_TRUE((a.Next() == &b && c.GenericNode::Next() == &copy && copy.MainList() == &second)) << "node assignment relinks between lists";
    copy.Unlink();
    EXPECT_TRUE((!copy.MainList() && !copy.Next() && !copy.Prev())) << "unlink clears links";
    first.AddHead(&b);
    EXPECT_TRUE((first.front() == &b && b.Next() == &a && first.size() == 2)) << "head transfer";
    a.Unlink(); b.Unlink(); c.Unlink();
    EXPECT_TRUE((first.empty() && second.empty() && !(first.begin() != first.end()))) << "empty iteration";
    const int before = Item::destroyed;
    first.AddHead(GameCreate<Item>()); first.AddTail(GameCreate<Item>());
    first.Delete();
    EXPECT_TRUE((first.IsEmpty() && Item::destroyed == before + 2)) << "Delete invokes virtual node destructors";
    first.Delete();
    {
        GenericList temporary;
        temporary.AddTail(&a);
    }
    EXPECT_TRUE((!a.IsValid() && !a.MainList())) << "list destruction unlinks borrowed node";
}

template<class T>
void typed_traversal_contracts() {
    List<T> list;
    T a, b, c;
    const auto& links = static_cast<const GenericList&>(list);
    auto* head = links.Last();
    auto* tail = links.First();
    EXPECT_TRUE((head && tail && head != tail && !head->IsValid() && !tail->IsValid() &&
        head->Next() == tail && tail->Prev() == head)) << "raw empty-list sentinels remain accessible";
    EXPECT_TRUE((!list.First() && !list.Last() && !list.front() && !(list.begin() != list.end()))) << "typed empty-list access never downcasts a sentinel";
    EXPECT_TRUE((!a.Next() && !a.Prev())) << "detached typed links are null";
    list.AddTail(&a); list.AddTail(&b); list.AddTail(&c);
    EXPECT_TRUE((list.First() == &a && list.Last() == &c && list.front() == &a)) << "typed endpoints adjust the base address";
    EXPECT_TRUE((a.Next() == &b && b.Next() == &c && c.Prev() == &b && b.Prev() == &a)) << "typed links adjust the base address in both directions";
    EXPECT_TRUE((!a.Prev() && !c.Next() && a.GenericNode::Prev() == head && c.GenericNode::Next() == tail)) << "typed traversal ends at null while raw links keep the sentinels";
    T* expected[] = {&a, &b, &c};
    unsigned count = 0;
    for (auto* node = list.First(); node; node = node->Next()) {
        EXPECT_TRUE((count < 3 && node == expected[count])) << "forward traversal visits each complete object";
        ++count;
    }
    EXPECT_TRUE((count == 3)) << "forward traversal reaches the tail";
    count = 0;
    for (auto* node = list.Last(); node; node = node->Prev()) {
        EXPECT_TRUE((count < 3 && node == expected[2 - count])) << "reverse traversal visits each complete object";
        ++count;
    }
    EXPECT_TRUE((count == 3)) << "reverse traversal reaches the head";
    count = 0;
    for (auto* node : list) {
        EXPECT_TRUE((count < 3 && node == expected[count])) << "iterator keeps correct derived addresses";
        ++count;
    }
    EXPECT_TRUE((count == 3)) << "iterator visits every element";
    b.Unlink();
    EXPECT_TRUE((!b.Next() && !b.Prev() && a.Next() == &c && c.Prev() == &a)) << "removal clears typed links and reconnects neighbors";
    a.Unlink();
    EXPECT_TRUE((list.First() == &c && list.Last() == &c && c.IsValid() && !c.Next() && !c.Prev())) << "single element has null typed neighbors and valid raw links";
    c.Unlink();
    EXPECT_TRUE((!list.First() && !list.Last() && !list.front() && list.size() == 0 &&
        links.First() == tail && links.Last() == head)) << "last removal restores both boundary contracts";
}

void typed_list_contracts() {
    Item zero;
    OffsetItem offset;
    const auto displacement = reinterpret_cast<std::uintptr_t>(static_cast<GenericNode*>(&offset)) -
        reinterpret_cast<std::uintptr_t>(&offset);
    EXPECT_TRUE((static_cast<void*>(static_cast<GenericNode*>(&zero)) == static_cast<void*>(&zero))) << "single-base fixture has zero base offset";
    EXPECT_TRUE((displacement != 0)) << "multiple-inheritance fixture must exercise a nonzero base offset";
    typed_traversal_contracts<Item>();
    typed_traversal_contracts<OffsetItem>();
    std::cout << "PASS: typed traversal, null boundaries and raw sentinels; nonzero base offset="
        << displacement << '\n';
}

void index_contracts() {
    IndexClass<int, int> values;
    values.AddIndex(9, 90); values.AddIndex(2, 20);
    int output = 1234;
    const auto& query = values;
    EXPECT_TRUE((!values.IsSorted && !query.TryGet(5, output) && output == 1234 &&
        values.IsSorted && values.Count() == 2 && !values.Archive)) << "missing query sorts without inserting or replacing output";
    EXPECT_TRUE((query.TryGet(2, output) && output == 20 && values.Archive)) << "successful query caches";
    const auto* saved = values.Archive;
    EXPECT_TRUE((!query.TryGet(4, output) && output == 20 && values.Archive == saved)) << "miss keeps cached hit";
    values.AddIndex(1, 10);
    values.Sort();
    EXPECT_TRUE((!values.Archive && values.IsSorted && query.TryGet(2, output) && output == 20)) << "sorting invalidates moved cache entries";

    IndexClass<int, Item*> owned;
    auto* a = GameCreate<Item>(); auto* b = GameCreate<Item>();
    owned.AddIndex(7, a);
    Item* selected = nullptr;
    EXPECT_TRUE((owned.TryGet(7, selected) && selected == a)) << "cache first duplicate";
    owned.AddIndex(7, b);
    EXPECT_TRUE((!owned.IsSorted && owned.RemoveIndex(7))) << "remove cached duplicate before sort";
    GameDelete(selected);
    EXPECT_TRUE((owned.TryGet(7, selected) && selected == b && owned.Count() == 1)) << "surviving duplicate owns live value";
    EXPECT_TRUE((owned.RemoveIndex(7))) << "remove final duplicate"; GameDelete(selected);
    EXPECT_TRUE((!owned.Archive && !owned.RemoveIndex(7))) << "removal invalidates cache/miss is safe";
    // Exercise a sorted duplicate run, selecting whichever node the target
    // ordering actually returns and deleting exactly that value each time.
    for (int i = 0; i < 12; ++i) owned.AddIndex(i % 3, GameCreate<Item>());
    const int before = Item::destroyed;
    for (int key = 0; key < 3; ++key) {
        while (owned.TryGet(key, selected)) {
            EXPECT_TRUE((owned.RemoveIndex(key))) << "remove sorted duplicate";
            GameDelete(selected);
        }
    }
    EXPECT_TRUE((owned.Count() == 0 && Item::destroyed == before + 12)) << "all duplicate values released once";
}

// Independent bitwise CRC oracle, not the implementation's table/helpers.
uint32_t crc32(const unsigned char* data, int length, uint32_t value = 0) {
    value = ~value;
    for (int i = 0; i < length; ++i) {
        value ^= data[i];
        for (int bit = 0; bit < 8; ++bit) value = (value >> 1) ^ (0xedb88320u & (0u - (value & 1u)));
    }
    return ~value;
}
template<class T, std::size_t N>
void scalar(T value, const std::array<unsigned char, N>& encoding) {
    for (unsigned prefix = 0; prefix < 4; ++prefix) {
        CRCEngine actual;
        unsigned char stream[12]{0x61, 0x62, 0x63};
        for (unsigned i = 0; i < prefix; ++i) actual(char(stream[i]));
        actual(value); // Each public overload must link and use target bytes.
        std::memcpy(stream + prefix, encoding.data(), N);
        const unsigned length = prefix + N, complete = length & ~3u, tail = length % 4;
        const uint32_t running = crc32(stream, complete);
        uint32_t expected = running;
        if (tail) {
            unsigned char padded[4]{};
            std::memcpy(padded, stream + complete, tail);
            padded[tail] = static_cast<unsigned char>(tail);
            for (unsigned i = tail + 1; i < 4; ++i) padded[i] = padded[0];
            expected = crc32(padded, 4, running);
        }
        EXPECT_TRUE((std::bit_cast<uint32_t>(actual.CRC) == running && actual.Index == int(tail))) << "scalar staging state";
        EXPECT_TRUE((std::bit_cast<uint32_t>(actual()) == expected && std::bit_cast<uint32_t>(actual()) == expected)) << "scalar encoding and repeatable padding";
    }
}
void crc_contracts() {
    scalar(false, std::array<unsigned char, 1>{0});
    scalar(true, std::array<unsigned char, 1>{1});
    scalar(short(-12345), std::array<unsigned char, 2>{0xc7, 0xcf});
    scalar(int(0x12345678), std::array<unsigned char, 4>{0x78, 0x56, 0x34, 0x12});
    scalar(int(-1), std::array<unsigned char, 4>{0xff, 0xff, 0xff, 0xff});
    scalar(-0.0f, std::array<unsigned char, 4>{0, 0, 0, 0x80});
    scalar(1.5f, std::array<unsigned char, 4>{0, 0, 0xc0, 0x3f});
    scalar(std::bit_cast<float>(0x7fc12345u), std::array<unsigned char, 4>{0x45, 0x23, 0xc1, 0x7f});
    scalar(-0.0, std::array<unsigned char, 8>{0, 0, 0, 0, 0, 0, 0, 0x80});
    scalar(1.5, std::array<unsigned char, 8>{0, 0, 0, 0, 0, 0, 0xf8, 0x3f});
    CRCEngine direct;
    direct.StagingBuffer.Composite = 0x12345678;
    direct(42);
    EXPECT_TRUE((direct.Index == 0 && direct.StagingBuffer.Composite == 0x12345678)) << "direct complete words leave unused staging intact";
    const auto result = direct();
    EXPECT_TRUE((direct(nullptr, 5) == result && direct("x", 0) == result && direct("x", -1) == result)) << "null and nonpositive byte buffers do not change CRC";
}

void crc_original_reference(const char* path) {
    std::ifstream reference(path);
    EXPECT_TRUE((bool(reference))) << "open original scalar CRC corpus";
    char kind;
    unsigned count = 0;
    while (reference >> kind) {
        uint64_t bits;
        uint32_t initial_crc, initial_staging, expected, final_crc, final_staging;
        int initial_index, final_index;
        reference >> bits >> initial_crc >> initial_index >> initial_staging >> expected
            >> final_crc >> final_index >> final_staging;
        EXPECT_TRUE((bool(reference))) << "complete scalar CRC record";
        struct Guarded { CRCEngine crc; uint32_t canary = 0xa5a5a5a5; } value;
        value.crc.CRC = std::bit_cast<int32_t>(initial_crc);
        value.crc.Index = initial_index;
        value.crc.StagingBuffer.Composite = std::bit_cast<int32_t>(initial_staging);
        switch (kind) {
        case 'b': value.crc(bits != 0); break;
        case 's': value.crc(std::bit_cast<short>(static_cast<uint16_t>(bits))); break;
        case 'i': value.crc(std::bit_cast<int>(static_cast<uint32_t>(bits))); break;
        case 'f': value.crc(std::bit_cast<float>(static_cast<uint32_t>(bits))); break;
        case 'd': value.crc(std::bit_cast<double>(bits)); break;
        default: throw std::runtime_error("unknown scalar CRC overload");
        }
        EXPECT_TRUE((std::bit_cast<uint32_t>(value.crc()) == expected &&
            std::bit_cast<uint32_t>(value.crc.CRC) == final_crc && value.crc.Index == final_index &&
            std::bit_cast<uint32_t>(value.crc.StagingBuffer.Composite) == final_staging)) << "scalar CRC differs from original instructions";
        EXPECT_TRUE((value.canary == 0xa5a5a5a5)) << "do not reproduce original byte +12 overrun";
        ++count;
    }
    EXPECT_TRUE((count == 612)) << "complete native scalar CRC corpus";
}
}

TEST(YrppHeaderContract, Contracts) {
    const auto [argc, argv] = ra2::test::arguments();


            list_contracts(); typed_list_contracts(); index_contracts(); crc_contracts();
            crc_original_reference(argc == 2 ? argv[1] : RA2_CRC_SCALAR_FIXTURE);
}
