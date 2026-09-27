#pragma once

#include "yrpp/Memory.h"
#include <utility>
#include <limits>

#include <algorithm>

//========================================================================
//=== VectorClass ========================================================
//========================================================================

template <typename T>
class VectorClass
{
public:
    // Native host allocation supports native alignment; x86 layout is checked separately.
    static constexpr bool GameCreateDisallowed = true;
    constexpr VectorClass() noexcept = default;

    explicit VectorClass(int capacity, T* pMem = nullptr) {
        if(capacity > 0) {
            this->Capacity = capacity;

            if(pMem) {
                this->Items = pMem;
            } else {
                this->Items = DLLCreateArray<T>(static_cast<size_t>(capacity));
                if(!this->Items) throw std::bad_alloc();
                this->IsAllocated = true;
            }
        }
    }

    VectorClass(const VectorClass &other) {
        if(other.Capacity > 0) {
            this->Items = DLLCreateArray<T>(static_cast<size_t>(other.Capacity));
            if(!this->Items) throw std::bad_alloc();
            this->IsAllocated = true;
            this->Capacity = other.Capacity;
            for(auto i = 0; i < other.Capacity; ++i) {
                this->Items[i] = other.Items[i];
            }
        }
    }

    VectorClass(VectorClass &&other) noexcept :
        Items(other.Items),
        Capacity(other.Capacity),
        IsInitialized(other.IsInitialized),
        IsAllocated(std::exchange(other.IsAllocated, false))
    { }

    virtual ~VectorClass() noexcept {
        if(this->Items && this->IsAllocated) {
            DLLDeleteArray(this->Items, static_cast<size_t>(this->Capacity));
            *static_cast<T* volatile*>(&this->Items) = nullptr;
        }
        // Original Vector destruction keeps borrowed Items, but clears ownership
        // and capacity (including the inlined Scenario list destructors).
        // Explicit observable stores retain the original ABI's destruction state
        // even when the compiler inlines this body into an object's destructor.
        *static_cast<volatile bool*>(&this->IsAllocated) = false;
        *static_cast<volatile int*>(&this->Capacity) = 0;
    }

    VectorClass& operator = (const VectorClass &other) {
        VectorClass(other).Swap(*this);
        return *this;
    }

    VectorClass& operator = (VectorClass &&other) noexcept {
        VectorClass(std::move(other)).Swap(*this);
        return *this;
    }

    virtual bool operator == (const VectorClass &other) const {
        if(this->Capacity != other.Capacity) {
            return false;
        }

        for(auto i = 0; i < this->Capacity; ++i) {
            if(this->Items[i] == other.Items[i]) {
                continue; // kapow! don't rewrite this to != unless you know why you're doing it
            }
            return false;
        }

        return true;
    }

    bool operator != (const VectorClass &other) const {
        return !(*this == other);
    }

    virtual bool SetCapacity(int capacity, T* pMem = nullptr) {
        if(capacity < 0) return false;
        if(capacity != 0) {
            this->IsInitialized = false;

            bool bMustAllocate = (pMem == nullptr);
            if(!pMem) {
                pMem = DLLCreateArray<T>(static_cast<size_t>(capacity));
            }

            this->IsInitialized = true;

            if(!pMem) {
                return false;
            }

            if(this->Items) {
                auto n = (capacity < this->Capacity) ? capacity : this->Capacity;
                for(auto i = 0; i < n; ++i) {
                    pMem[i] = std::move_if_noexcept(this->Items[i]);
                }

                if(this->IsAllocated) {
                    DLLDeleteArray(this->Items, static_cast<size_t>(this->Capacity));
                    this->Items = nullptr;
                }
            }

            this->IsAllocated = bMustAllocate;
            this->Items = pMem;
            this->Capacity = capacity;
        } else {
            Clear();
        }
        return true;
    }

    virtual void Clear() {
        VectorClass(std::move(*this));
        this->Items = nullptr;
        this->Capacity = 0;
    }

    virtual int FindItemIndex(const T& item) const {
        if(!this->IsInitialized) {
            return 0;
        }

        for(auto i = 0; i < this->Capacity; ++i) {
            if(this->Items[i] == item) {
                return i;
            }
        }

        return -1;
    }

    virtual int GetItemIndex(const T* pItem) const final {
        if(!this->IsInitialized) {
            return 0;
        }

        return pItem - this->Items;
    }

    virtual T GetItem(int i) const final {
        return this->Items[i];
    }

    T& operator [] (int i) {
        return this->Items[i];
    }

    const T& operator [] (int i) const {
        return this->Items[i];
    }

    bool Reserve(int capacity) {
        if(!this->IsInitialized) {
            return false;
        }

        if(this->Capacity >= capacity) {
            return true;
        }

        return SetCapacity(capacity, nullptr);
    }

    void Swap(VectorClass& other) noexcept {
        using std::swap;
        swap(this->Items, other.Items);
        swap(this->Capacity, other.Capacity);
        swap(this->IsInitialized, other.IsInitialized);
        swap(this->IsAllocated, other.IsAllocated);
    }

    T* Items{ nullptr };
    int Capacity{ 0 };
    bool IsInitialized{ true };
    bool IsAllocated{ false };
};

//========================================================================
//=== DynamicVectorClass =================================================
//========================================================================

template <typename T>
class DynamicVectorClass : public VectorClass<T>
{
public:
    constexpr DynamicVectorClass() noexcept = default;
    static constexpr bool GameCreateDisallowed = true;
    explicit DynamicVectorClass(int capacity, T* pMem = nullptr)
        : VectorClass<T>(capacity, pMem)
    { }

    DynamicVectorClass(const DynamicVectorClass &other) {
        if(other.Capacity > 0) {
            this->Items = DLLCreateArray<T>(static_cast<size_t>(other.Capacity));
            if(!this->Items) throw std::bad_alloc();
            this->IsAllocated = true;
            this->Capacity = other.Capacity;
            for(auto i = 0; i < other.Count; ++i) {
                this->Items[i] = other.Items[i];
            }
        }
        // Original copy constructors preserve growth policy even for empty lists.
        this->Count = other.Count;
        this->CapacityIncrement = other.CapacityIncrement;
    }

    DynamicVectorClass(DynamicVectorClass &&other) noexcept
        : VectorClass<T>(std::move(other)), Count(other.Count),
        CapacityIncrement(other.CapacityIncrement)
    { }

    DynamicVectorClass& operator = (const DynamicVectorClass &other) {
        DynamicVectorClass(other).Swap(*this);
        return *this;
    }

    DynamicVectorClass& operator = (DynamicVectorClass &&other) noexcept {
        DynamicVectorClass(std::move(other)).Swap(*this);
        return *this;
    }

    virtual bool SetCapacity(int capacity, T* pMem = nullptr) override {
        bool bRet = VectorClass<T>::SetCapacity(capacity, pMem);

        if(this->Capacity < this->Count) {
            this->Count = this->Capacity;
        }

        return bRet;
    }

    virtual void Clear() override {
        VectorClass<T>::Clear();
        this->Count = 0;
    }

    // Original-game arrays may carry an EXE-owned implementation in this slot.
    // Do not mark final: that would let the compiler bypass its virtual table.
    virtual int FindItemIndex(const T& item) const override {
        if(!this->IsInitialized) {
            return 0;
        }

        for(int i = 0; i < this->Count; i++) {
            if(this->Items[i] == item) {
                return i;
            }
        }

        return -1;
    }

    bool ValidIndex(int index) const {
        return static_cast<unsigned int>(index) < static_cast<unsigned int>(this->Count);
    }

    T GetItemOrDefault(int i) const {
        return this->GetItemOrDefault(i, T());
    }

    T GetItemOrDefault(int i, T def) const {
        if(this->ValidIndex(i)) {
            return this->Items[i];
        }
        return def;
    }

    T* begin() const {
        // if(!this->IsInitialized) {
        //	return nullptr;
        //}
        return this->Items;
    }

    T* end() const {
        // if(!this->IsInitialized) {
        //	return nullptr;
        //}
        return this->Items ? this->Items + this->Count : nullptr;
    }

    T* front() const {
        return begin();
    }

    T* back() const {
        return end() - 1;
    }

    bool AddItem(T item) {
        if(this->Count >= this->Capacity) {
            if(!this->IsAllocated && this->Capacity != 0) {
                return false;
            }

            if(this->CapacityIncrement <= 0) {
                return false;
            }

            if(this->Capacity > std::numeric_limits<int>::max() - this->CapacityIncrement
                || !this->SetCapacity(this->Capacity + this->CapacityIncrement, nullptr)) {
                return false;
            }
        }

        this->Items[Count++] = std::move(item);
        return true;
    }

    template <class... _Valty>
    constexpr decltype(auto) emplace_back(_Valty&&... _Val) {
        AddItem(T{ _Val... });
        return *back();
    }

    bool AddUnique(const T &item) {
        int idx = this->FindItemIndex(item);
        return idx < 0 && this->AddItem(item);
    }

    bool RemoveItem(int index) {
        if(!this->ValidIndex(index)) {
            return false;
        }

        --this->Count;
        for(int i = index; i < this->Count; ++i) {
            this->Items[i] = std::move_if_noexcept(this->Items[i + 1]);
        }

        return true;
    }

    bool Remove(const T &item) {
        int idx = this->FindItemIndex(item);
        return idx >= 0 && this->RemoveItem(idx);
    }

    void Swap(DynamicVectorClass& other) noexcept {
        VectorClass<T>::Swap(other);
        using std::swap;
        swap(this->Count, other.Count);
        swap(this->CapacityIncrement, other.CapacityIncrement);
    }

    int Count{ 0 };
    int CapacityIncrement{ 10 };
};

//========================================================================
//=== TypeList ===========================================================
//========================================================================

template <typename T>
class TypeList : public DynamicVectorClass<T>
{
public:
    constexpr TypeList() noexcept = default;
    static constexpr bool GameCreateDisallowed = true;
    explicit TypeList(int capacity, T* pMem = nullptr)
        : DynamicVectorClass<T>(capacity, pMem)
    { }

    TypeList(const TypeList &other)
        : DynamicVectorClass<T>(other), unknown_18(other.unknown_18)
    { }

    TypeList(TypeList &&other) noexcept
        : DynamicVectorClass<T>(std::move(other)), unknown_18(other.unknown_18)
    { }

    TypeList& operator = (const TypeList &other) {
        TypeList(other).Swap(*this);
        return *this;
    }

    TypeList& operator = (TypeList &&other) noexcept {
        TypeList(std::move(other)).Swap(*this);
        return *this;
    }

    void Swap(TypeList& other) noexcept {
        DynamicVectorClass<T>::Swap(other);
        using std::swap;
        swap(this->unknown_18, other.unknown_18);
    }

    int unknown_18{ 0 };
};

//========================================================================
//=== CounterClass =======================================================
//========================================================================

class CounterClass : public VectorClass<int>
{
public:
    constexpr CounterClass() noexcept = default;
    static constexpr bool GameCreateDisallowed = true;
    CounterClass(const CounterClass& other);

    CounterClass(CounterClass &&other) noexcept;

    CounterClass& operator = (const CounterClass &other);

    CounterClass& operator = (CounterClass &&other) noexcept;

    virtual void Clear() override;

    int GetTotal() const;

    bool EnsureItem(int index);

    int operator[] (int index) const;

    int GetItemCount(int index);

    int GetItemCount(int index) const;

    int Increment(int index);

    int Decrement(int index);

    void Swap(CounterClass& other) noexcept;

    int Total{ 0 };
};
