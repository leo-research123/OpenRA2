#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/Memory.h"
#include <functional>
#include <cstdint>
#include <cstdlib>

template<typename T, typename Pr = std::less<T>>
class PriorityQueueClass
{
public:
    PriorityQueueClass(int capacity)
    {
        Capacity = capacity;
        Nodes = (T**)YRMemory::Allocate(sizeof(T*) * (Capacity + 1));
        if (!Nodes) std::abort(); // Original pool allocation failure is fatal.
        Count = 0;
        LMost = (T*)nullptr;
        RMost = (T*)0xFFFFFFFF;

        ClearAll();
    }

    ~PriorityQueueClass()
    {
        YRMemory::Deallocate(Nodes);
        Nodes = (T**)nullptr;
    }

    void Clear()
    {
        memset(Nodes, 0, sizeof(T*) * (Count + 1));
        Count = 0;
    }

    T* Top()
    {
        return Count == 0 ? nullptr : Nodes[1];
    }

    bool Pop()
    {
        if (Count == 0)
            return false;

        Nodes[1] = Nodes[Count--];
        int now = 1;
        while (now * 2 <= Count)
        {
            int next = now * 2;
            if (next < Count && Comp(Nodes[next + 1], Nodes[next]))
                ++next;
            if (!Comp(Nodes[next], Nodes[now]))
                break;

            // Westwood did Nodes[now] = Nodes[next] here
            std::swap(Nodes[now], Nodes[next]);

            now = next;
        }

        return true;
    }

    bool Push(T* pValue)
    {
        if (Count >= Capacity)
            return false;

        Nodes[++Count] = pValue;
        int now = Count;
        while (now != 1)
        {
            int next = now / 2;
            if (!Comp(Nodes[now], Nodes[next]))
                break;

            // Westwood did Nodes[now] = Nodes[next] here
            std::swap(Nodes[now], Nodes[next]);

            now = next;
        }

        return true;
    }

    bool WWPop()
    {
        if (Pop())
        {
            for (int i = 1; i <= Count; ++i)
                WWPointerUpdate(Nodes[i]);

            return true;
        }

        return false;
    }

    bool WWPush(T* pValue)
    {
        if (Push(pValue))
        {
            WWPointerUpdate(pValue);

            return true;
        }

        return false;
    }

    // OpenTS 44fac744 priority.h Insert / Extract_Min / Replace_Root,
    // GPL-3.0-or-later, Copyright 2026 OpenTS contributors; see
    // third_party/opents/LICENSE.md for EA Section 7 terms.
    // YR's AStar instantiations store pointers in a one-based fixed heap,
    // reserve Capacity's last slot, and keep equal scores in heap order.
    bool Insert(T* value) noexcept
    {
        if (Count + 1 >= Capacity) return false;
        int index = ++Count;
        while (index > 1 && Comp(value, Nodes[index / 2])) {
            Nodes[index] = Nodes[index / 2];
            index /= 2;
        }
        Nodes[index] = value;
        const auto address = reinterpret_cast<std::uintptr_t>(value);
        if (address > reinterpret_cast<std::uintptr_t>(LMost)) LMost = value;
        if (address < reinterpret_cast<std::uintptr_t>(RMost)) RMost = value;
        return true;
    }

    T* Extract_Min() noexcept
    {
        if (!Count) return nullptr;
        T* result = Nodes[1];
        Nodes[1] = Nodes[Count];
        Nodes[Count--] = nullptr;
        if (Count) Heapify(1);
        return result;
    }

    T* Replace_Root(T* candidate) noexcept
    {
        if (!Count || Comp(candidate, Nodes[1])) return candidate;
        T* result = Nodes[1];
        Nodes[1] = candidate;
        Heapify(1);
        return result;
    }

private:
    // Hierarchical AStar instantiation; regular AStar inlines the same sift.
    /// VA: 0x0042DCA0
    void Heapify(int index) noexcept
    {
        for (;;) {
            int smallest = index, left = index * 2, right = left + 1;
            if (left <= Count && Comp(Nodes[left], Nodes[smallest])) smallest = left;
            if (right <= Count && Comp(Nodes[right], Nodes[smallest])) smallest = right;
            if (smallest == index) return;
            std::swap(Nodes[index], Nodes[smallest]);
            index = smallest;
        }
    }

    void ClearAll()
    {
        memset(Nodes, 0, sizeof(T*) * (Capacity + 1));
    }

    bool Comp(T* p1, T* p2)
    {
        return Pr()(*p1, *p2);
    }

    void WWPointerUpdate(T* pValue)
    {
        if (pValue > RMost)
            RMost = pValue;
        if (pValue < LMost)
            LMost = pValue;
    }

public:
    int Count;
    int Capacity;
    T** Nodes;
    T* LMost;
    T* RMost;
};

struct PriorityQueueClassNode
{
    /// VA: 0x0042B1F0.
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL SurfaceDataCount() JMP_STD(0x42B1F0);
#else
    static int YRPP_FASTCALL SurfaceDataCount();
#endif
    /// VA: 0x0042B1C0.
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL ToSurfaceIndex(const CellStruct& mapCoord) JMP_STD(0x42B1C0);
#else
    static int YRPP_FASTCALL ToSurfaceIndex(const CellStruct& mapCoord);
#endif

    int ToSurfaceIndex()
    {
        return ToSurfaceIndex(MapCoord);
    }

    CellStruct MapCoord;
    float Score;

    bool operator<(const PriorityQueueClassNode& another) const
    {
        return Score < another.Score;
    }
};
