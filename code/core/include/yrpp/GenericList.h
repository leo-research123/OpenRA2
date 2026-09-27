// Copyright 2020 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from REDALERT/LISTNODE.H; see third_party/ea/README.md and LICENSE.TXT
// for the original notice, additional permissions and local modifications.
// YRpp GenericList.h API; migrated EA-derived link/unlink implementation.
#pragma once

#include <cstddef>

class GenericList;
class GenericNode {
public:
    GenericNode() = default;
    virtual ~GenericNode();
    // Original intrusive copy: insert this node immediately after node.
    GenericNode(GenericNode& node);
    GenericNode& operator=(GenericNode& node);

    void Unlink();
    void Link(GenericNode* node);
    GenericNode* Next() const;
    GenericNode* Prev() const;
    bool IsValid() const;
    // Requires a node belonging to a GenericList; detached nodes return null.
    GenericList* MainList() const;

protected:
    GenericNode* NextNode = nullptr;
    GenericNode* PrevNode = nullptr;
};

template<class T> class List;
template<class T> class Node : public GenericNode {
public:
    List<T>* MainList() const { return static_cast<List<T>*>(GenericNode::MainList()); }
    // Typed traversal requires T elements linked in a GenericList. Sentinels
    // and detached links return null; inspect raw links through GenericNode.
    T* Next() const {
        auto* node = GenericNode::Next();
        return node && node->IsValid() ? static_cast<T*>(node) : nullptr;
    }
    T* Prev() const {
        auto* node = GenericNode::Prev();
        return node && node->IsValid() ? static_cast<T*>(node) : nullptr;
    }
};

class GenericList {
public:
    GenericList();
    virtual ~GenericList();
    GenericList(const GenericList&) = delete;
    GenericList& operator=(const GenericList&) = delete;
    GenericNode* First() const;
    GenericNode* Last() const;
    void AddHead(GenericNode* node);
    void AddTail(GenericNode* node);
    bool IsEmpty() const;
    // Deletes all nodes in the Game allocation domain; list destruction only
    // unlinks them. Do not call Delete for stack or otherwise borrowed nodes.
    void Delete();

protected:
    GenericNode FirstNode;
    GenericNode LastNode;
private:
    friend class GenericNode; // MainList recovers the owner from FirstNode.
};

template<class T> class List : public GenericList {
public:
    // All element nodes must be T objects. Empty lists return null, while
    // GenericList::First/Last retain the original raw sentinel access.
    T* First() const {
        auto* node = GenericList::First();
        return node && node->IsValid() ? static_cast<T*>(node) : nullptr;
    }
    T* Last() const {
        auto* node = GenericList::Last();
        return node && node->IsValid() ? static_cast<T*>(node) : nullptr;
    }
    bool empty() const { return IsEmpty(); } // host iteration convenience
    // Never cast either GenericNode sentinel to a derived MIX object.
    class Iterator {
    public:
        explicit Iterator(GenericNode* node) : node_(node) {}
        T* operator*() const { return static_cast<T*>(node_); }
        Iterator& operator++() { node_ = node_->Next(); return *this; }
        bool operator!=(const Iterator& other) const { return node_ != other.node_; }
    private:
        GenericNode* node_;
    };
    Iterator begin() const { return Iterator(FirstNode.Next()); }
    Iterator end() const { return Iterator(const_cast<GenericNode*>(&LastNode)); }
    T* front() const { return First(); }
    size_t size() const {
        size_t count = 0;
        for (auto it = begin(); it != end(); ++it) ++count;
        return count;
    }
};
