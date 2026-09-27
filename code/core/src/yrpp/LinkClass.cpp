// LinkClass list operations, calibrated to YR 5565A0..55677E (7b8a0685).
#include "yrpp/LinkClass.h"
LinkClass::LinkClass() noexcept : Next(nullptr), Previous(nullptr) {}
// The old header mislabeled 556550's no-argument constructor as a copy.
// Native convenience copies are detached, like Gadget's actual copy ctor.
LinkClass::LinkClass(LinkClass&) : LinkClass() {}
LinkClass& LinkClass::operator=(LinkClass& other) {
    if (&other!=this) { Remove(); Add(other); }
    return *this;
}
LinkClass::~LinkClass() { LinkClass::Remove(); }
LinkClass* LinkClass::GetNext() { return Next; }
LinkClass* LinkClass::GetPrev() { return Previous; }
LinkClass* LinkClass::HeadOfList() {
    auto* node=this;
    while (node->Previous && node->Previous!=this) node=node->Previous;
    return node;
}
LinkClass* LinkClass::TailOfList() {
    auto* node=this;
    while (node->Next && node->Next!=this) node=node->Next;
    return node;
}
void LinkClass::Zap() { Next=Previous=nullptr; }
LinkClass* LinkClass::Remove() {
    auto* head=HeadOfList(); auto* next=Next;
    if (Previous) Previous->Next=Next;
    if (Next) Next->Previous=Previous;
    Next=Previous=nullptr;
    return head!=this ? head : next;
}
LinkClass* LinkClass::Add(LinkClass& another) {
    if (&another==this) return HeadOfList();
    Remove();
    Next=another.Next; Previous=&another; another.Next=this;
    if (Next) Next->Previous=this;
    return HeadOfList();
}
LinkClass* LinkClass::AddHead(LinkClass& another) {
    if (&another==this) return HeadOfList();
    Remove(); Next=another.HeadOfList(); Next->Previous=this;
    return this;
}
LinkClass* LinkClass::AddTail(LinkClass& another) { return Add(*another.TailOfList()); }
