// Copyright 2020 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from REDALERT/LISTNODE.H; see third_party/ea/README.md and LICENSE.TXT
// for the original notice, additional permissions and local modifications.
// YRpp GenericList.h API; migrated EA-derived link/unlink implementation.
#include "yrpp/GenericList.h"
#include "yrpp/Memory.h"

GenericNode::GenericNode(GenericNode& node) { node.Link(this); }
GenericNode& GenericNode::operator=(GenericNode& node) {
    if (&node != this) node.Link(this);
    return *this;
}

GenericList* GenericNode::MainList() const {
    if (!NextNode && !PrevNode) return nullptr;
    const GenericNode* head = this;
    // Fixed YRpp/EA sources accidentally advance from this->PrevNode and
    // return this. Correct those defects without an owner field/side table.
    while (head->PrevNode) head = head->PrevNode;
    // Supported compilers implement offsetof for this polymorphic layout;
    // compat asserts the original x86 FirstNode offset (4) independently.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
    auto* owner = reinterpret_cast<const char*>(head) - offsetof(GenericList, FirstNode);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    return const_cast<GenericList*>(reinterpret_cast<const GenericList*>(owner));
}

GenericNode::~GenericNode() { Unlink(); }

void GenericNode::Unlink() {
    if (IsValid()) {
        PrevNode->NextNode = NextNode;
        NextNode->PrevNode = PrevNode;
        PrevNode = nullptr;
        NextNode = nullptr;
    }
}

void GenericNode::Link(GenericNode *node) {
    node->Unlink();
    node->NextNode = NextNode;
    node->PrevNode = this;
    if (NextNode)
        NextNode->PrevNode = node;
    NextNode = node;
}

GenericNode *GenericNode::Next() const { return NextNode; }

GenericNode *GenericNode::Prev() const { return PrevNode; }

bool GenericNode::IsValid() const { return NextNode && PrevNode; }

GenericList::GenericList() { FirstNode.Link(&LastNode); }

GenericList::~GenericList() {
    while (!IsEmpty())
        First()->Unlink();
}

GenericNode *GenericList::First() const { return FirstNode.Next(); }

GenericNode *GenericList::Last() const { return LastNode.Prev(); }

void GenericList::AddHead(GenericNode *node) { FirstNode.Link(node); }

void GenericList::AddTail(GenericNode *node) { LastNode.Prev()->Link(node); }

bool GenericList::IsEmpty() const { return !FirstNode.Next()->IsValid(); }

void GenericList::Delete() {
    while (!IsEmpty()) GameDelete(FirstNode.Next());
}
