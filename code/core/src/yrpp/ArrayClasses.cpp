// CounterClass implementation moved from YRpp ArrayClasses.h; see yrpp/SOURCE.json.
#include "yrpp/ArrayClasses.h"

CounterClass::CounterClass(const CounterClass &other) : VectorClass(other), Total(other.Total) {}

CounterClass::CounterClass(CounterClass &&other) noexcept
    : VectorClass(std::move(other)), Total(other.Total) {}

CounterClass &CounterClass::operator=(const CounterClass &other) {
    CounterClass(other).Swap(*this);
    return *this;
}

CounterClass &CounterClass::operator=(CounterClass &&other) noexcept {
    CounterClass(std::move(other)).Swap(*this);
    return *this;
}

void CounterClass::Clear() {
    for (int i = 0; i < this->Capacity; ++i) {
        this->Items[i] = 0;
    }

    this->Total = 0;
}

int CounterClass::GetTotal() const { return this->Total; }

bool CounterClass::EnsureItem(int index) {
    if (index < this->Capacity) {
        return true;
    }

    int count = this->Capacity;
    if (this->SetCapacity(index + 10, nullptr)) {
        for (auto i = count; i < this->Capacity; ++i) {
            this->Items[i] = 0;
        }
        return true;
    }

    return false;
}

int CounterClass::operator[](int index) const { return this->GetItemCount(index); }

int CounterClass::GetItemCount(int index) { return this->EnsureItem(index) ? this->Items[index] : 0; }

int CounterClass::GetItemCount(int index) const { return (index < this->Capacity) ? this->Items[index] : 0; }

int CounterClass::Increment(int index) {
    if (this->EnsureItem(index)) {
        ++this->Total;
        return ++this->Items[index];
    }
    return 0;
}

int CounterClass::Decrement(int index) {
    if (this->EnsureItem(index)) {
        --this->Total;
        return --this->Items[index];
    }
    return 0;
}

void CounterClass::Swap(CounterClass &other) noexcept {
    VectorClass::Swap(other);
    using std::swap;
    swap(this->Total, other.Total);
}
