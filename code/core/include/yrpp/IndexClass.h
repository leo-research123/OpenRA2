#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/Memory.h"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <utility>
#include "yrpp/GenericList.h"

/*
* IndexClass, most impl from CCR
* --secsome
*/

// TKey is always int in WW's code
template<typename TKey, typename TValue>
class IndexClass
{
public:
    IndexClass(void);
    IndexClass(const IndexClass&) = delete;
    IndexClass& operator=(const IndexClass&) = delete;
    // Does not insert on a miss and preserves output. Like IsPresent, this
    // may sort IndexTable and update Archive, even through a const reference.
    // Callers must synchronize access; const does not imply thread safety.
    bool TryGet(TKey id, TValue& output) const {
        if (!IsPresent(id)) return false;
        output = Archive->Data; return true;
    }
    ~IndexClass(void);

    constexpr bool AddIndex(TKey id, const TValue& data);
    bool AddIndex(TKey id, TValue&& data);
    bool RemoveIndex(TKey id);
    bool IsPresent(TKey id) const;
    int Count() const;
    const TValue& FetchIndex(TKey id) const;
    TValue& FetchIndex(TKey id);
    void Clear();
    bool Reverse(int nAmount);
    inline void Sort();
    static int YRPP_CDECL Comparator(void const* ptr, void const* ptr2);

    struct NodeElement
    {
        NodeElement& operator=(const NodeElement& node) { ID = node.ID; Data = node.Data; return *this; }
        bool operator<(const NodeElement& another) const { return ID < another.ID; }
        bool operator==(const NodeElement& another) const { return ID == another.ID; }

        TKey ID;
        TValue Data;
    };

    NodeElement* IndexTable;
    int IndexCount;
    int IndexSize;
    bool IsSorted;
    char padding[3]{};
    NodeElement* Archive;

    // ranged for support
    NodeElement* begin() const { return IndexTable; }
    NodeElement* end() const { return IndexTable ? IndexTable + IndexCount : nullptr; }

private:
    bool IncreaseTableSize(int nAmount);
    bool IsArchiveSame(TKey id) const;
    void InvalidateArchive();
    void SetArchive(NodeElement const* pNode);
    NodeElement const* SearchForNode(TKey id) const;
};

template<typename TKey, typename TValue>
IndexClass<TKey, TValue>::IndexClass() :
    IndexTable(0),
    IndexCount(0),
    IndexSize(0),
    IsSorted(false),
    Archive(0)
{
    InvalidateArchive();
}

template<typename TKey, typename TValue>
IndexClass<TKey, TValue>::~IndexClass()
{
    Clear();
}

template<typename TKey, typename TValue>
void IndexClass<TKey, TValue>::Clear()
{
    // Upstream uses GameCreateArray but scalar GameDelete. Use its existing
    // array deleter so every constructed NodeElement is destroyed.
    GameDeleteArray(IndexTable, static_cast<size_t>(IndexSize));
    IndexTable = 0;
    IndexCount = 0;
    IndexSize = 0;
    IsSorted = false;
    InvalidateArchive();
}

template<typename TKey, typename TValue>
bool IndexClass<TKey, TValue>::IncreaseTableSize(int amount)
{
    if (amount < 0 || amount > std::numeric_limits<int>::max() - IndexSize) return false;
    // Preserve the former array helper's size bound before the upstream
    // GameAllocator multiplies count by sizeof(NodeElement), especially on x86.
    if (static_cast<size_t>(IndexSize + amount) >
        static_cast<size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(NodeElement)) return false;

    NodeElement* table = GameCreateArray<NodeElement>(static_cast<size_t>(IndexSize + amount));
    if (table != nullptr)
    {
        for (int i = 0; i < IndexCount; ++i)
            table[i] = IndexTable[i];

        GameDeleteArray(IndexTable, static_cast<size_t>(IndexSize));
        IndexTable = table;
        IndexSize += amount;
        InvalidateArchive();

        return true;
    }
    return false;
}

template<typename TKey, typename TValue>
bool IndexClass<TKey, TValue>::Reverse(int amount)
{
    Clear();
    return IncreaseTableSize(amount);
}

template<typename TKey, typename TValue>
inline void IndexClass<TKey, TValue>::Sort()
{
    if (!IsSorted)
    {
        // Preserve the calibrated gamemd CRT ordering for equal IDs.
        // This local template body introduces no additional IndexClass member.
        auto sort_range = [](auto&& sort_range, NodeElement* items, int low, int high) -> void {
            while (low < high) {
                if (high - low + 1 <= 8) {
                    for (int end = high; end > low; --end) {
                        int maximum = low;
                        for (int i = low + 1; i <= end; ++i)
                            if (items[maximum].ID < items[i].ID) maximum = i;
                        std::swap(items[maximum], items[end]);
                    }
                    return;
                }
                std::swap(items[low + (high - low + 1) / 2], items[low]);
                int left = low, right = high + 1;
                for (;;) {
                    do { ++left; } while (left <= high && !(items[low].ID < items[left].ID));
                    do { --right; } while (right > low && !(items[right].ID < items[low].ID));
                    if (right < left) break;
                    std::swap(items[left], items[right]);
                }
                std::swap(items[low], items[right]);
                // Recurse into the smaller partition so stack depth remains bounded.
                if (right - low >= high - left + 1) {
                    if (left < high) sort_range(sort_range, items, left, high);
                    high = right - 1;
                } else {
                    if (low + 1 < right) sort_range(sort_range, items, low, right - 1);
                    low = left;
                }
            }
        };
        if (IndexCount > 1) sort_range(sort_range, IndexTable, 0, IndexCount - 1);
        InvalidateArchive();
        IsSorted = true;
    }
}

template<typename TKey, typename TValue>
int IndexClass<TKey, TValue>::Count() const
{
    return IndexCount;
}

template<typename TKey, typename TValue>
bool IndexClass<TKey, TValue>::IsPresent(TKey id) const
{
    if (!IndexCount)
        return false;

    if (IsArchiveSame(id))
        return true;

    NodeElement const* nodeptr = SearchForNode(id);

    if (nodeptr != nullptr)
    {
        const_cast<IndexClass<TKey, TValue>*>(this)->SetArchive(nodeptr);
        return true;
    }

    return false;
}

template<typename TKey, typename TValue>
const TValue& IndexClass<TKey, TValue>::FetchIndex(TKey id) const
{
    static const TValue empty{};
    return IsPresent(id) ? Archive->Data : empty;
}

template<typename TKey, typename TValue>
TValue& IndexClass<TKey, TValue>::FetchIndex(TKey id)
{
    if (!IsPresent(id))
    {
        if (!AddIndex(id, TValue()) || !IsPresent(id)) throw std::bad_alloc();
    }

    return Archive->Data;
}

template<typename TKey, typename TValue>
bool IndexClass<TKey, TValue>::IsArchiveSame(TKey id) const
{
    return Archive != 0 && Archive->ID == id;
}

template<typename TKey, typename TValue>
void IndexClass<TKey, TValue>::InvalidateArchive()
{
    Archive = nullptr;
}

template<typename TKey, typename TValue>
void IndexClass<TKey, TValue>::SetArchive(NodeElement const* node)
{
    Archive = const_cast<NodeElement*>(node);
}

template<typename TKey, typename TValue>
constexpr bool IndexClass<TKey, TValue>::AddIndex(TKey id, const TValue& data)
{
    if (IndexCount + 1 > IndexSize)
    {
        if (!IncreaseTableSize(IndexSize == 0 ? 10 : IndexSize))
            return false;
    }

    IndexTable[IndexCount].ID = std::move(id);
    IndexTable[IndexCount].Data = std::move(data);
    ++IndexCount;
    IsSorted = false;

    return true;
}

template<typename TKey, typename TValue>
bool IndexClass<TKey, TValue>::AddIndex(TKey id, TValue&& data)
{
    if (IndexCount + 1 > IndexSize)
    {
        if (!IncreaseTableSize(IndexSize == 0 ? 10 : IndexSize))
            return false;
    }

    IndexTable[IndexCount].ID = std::move(id);
    IndexTable[IndexCount].Data = std::move(data);
    ++IndexCount;
    IsSorted = false;

    return true;
}

template<typename TKey, typename TValue>
bool IndexClass<TKey, TValue>::RemoveIndex(TKey id)
{
    // Host safety difference: honor the same cached duplicate as TryGet.
    // Re-sorting here can remove a different node from the one the caller frees.
    const auto* found = IsPresent(id) ? Archive : nullptr;
    const int found_index = found ? static_cast<int>(found - IndexTable) : -1;

    if (found_index != -1)
    {
        for (int i = found_index + 1; i < IndexCount; ++i)
            IndexTable[i - 1] = IndexTable[i];
        --IndexCount;

        IndexTable[IndexCount] = std::move(NodeElement(TKey(), TValue())); // zap last (now unused) element

        InvalidateArchive();
        return true;
    }

    return false;
}

// See RA1 code
template<typename TKey, typename TValue>
int YRPP_CDECL IndexClass<TKey, TValue>::Comparator(void const* ptr1, void const* ptr2)
{
    const NodeElement* n1 = static_cast<const NodeElement*>(ptr1);
    const NodeElement* n2 = static_cast<const NodeElement*>(ptr2);

    if (n1->ID == n2->ID)
        return 0;
    if (n1->ID < n2->ID)
        return -1;

    return 1;
}

template<typename TKey, typename TValue>
typename IndexClass<TKey, TValue>::NodeElement const* IndexClass<TKey, TValue>::SearchForNode(TKey id) const
{
    if (!IndexCount)
        return 0;

    const_cast<IndexClass<TKey, TValue>*>(this)->Sort();

    int start = 0, count = IndexCount;
    while (count > 0) {
        const int half = count / 2;
        const auto& candidate = IndexTable[start + half];
        if (candidate.ID == id) return &candidate;
        if (candidate.ID > id) count = half;
        else { start += half + 1; count -= half + 1; }
    }
    return nullptr;
}
