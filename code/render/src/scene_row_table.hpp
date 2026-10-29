#pragma once
#include <cstdint>
#include <memory>
#include <utility>

namespace render {
// Immutable ordered row directory. A publication copies only paths to changed
// rows; older queued frames retain their original tree. Lighting-only updates
// retain the whole root. Priorities depend only on the row key, not history.
template <class Row>
class SceneRowTable {
public:
    using Key = std::pair<int, int>;
    using Value = std::shared_ptr<const Row>;

private:
    struct Node;
    using Link = std::shared_ptr<const Node>;
    struct Node {
        Key key;
        Value value;
        Link left, right;
        std::uint64_t priority;
    };
    Link root_;
    static std::uint64_t priority(Key key) noexcept {
        std::uint64_t v = (std::uint64_t(std::uint32_t(key.first)) << 32) | std::uint32_t(key.second);
        v += 0x9E3779B97F4A7C15ull;
        v = (v ^ (v >> 30)) * 0xBF58476D1CE4E5B9ull;
        v = (v ^ (v >> 27)) * 0x94D049BB133111EBull;
        return v ^ (v >> 31);
    }
    static Link node(Key key, const Value& value, Link left, Link right) {
        return std::make_shared<Node>(Node{key, value, std::move(left), std::move(right), priority(key)});
    }
    static void split(const Link& tree, Key key, Link& left, Link& right) {
        if (!tree) {
            left.reset();
            right.reset();
            return;
        }
        if (tree->key < key) {
            Link tail;
            split(tree->right, key, tail, right);
            left = node(tree->key, tree->value, tree->left, std::move(tail));
        } else {
            Link head;
            split(tree->left, key, left, head);
            right = node(tree->key, tree->value, std::move(head), tree->right);
        }
    }
    static Link set(const Link& tree, Key key, const Value& value) {
        if (!tree) return node(key, value, {}, {});
        if (tree->key == key) return tree->value == value ? tree : node(key, value, tree->left, tree->right);
        if (priority(key) < tree->priority) {
            Link left, right;
            split(tree, key, left, right);
            return node(key, value, std::move(left), std::move(right));
        }
        if (key < tree->key) return node(tree->key, tree->value, set(tree->left, key, value), tree->right);
        return node(tree->key, tree->value, tree->left, set(tree->right, key, value));
    }
    static Link merge(const Link& left, const Link& right) {
        if (!left) return right;
        if (!right) return left;
        if (left->priority < right->priority)
            return node(left->key, left->value, left->left, merge(left->right, right));
        return node(right->key, right->value, merge(left, right->left), right->right);
    }
    static Link erase(const Link& tree, Key key) {
        if (!tree) return tree;
        if (tree->key == key) return merge(tree->left, tree->right);
        if (key < tree->key) {
            auto left = erase(tree->left, key);
            return left == tree->left ? tree : node(tree->key, tree->value, std::move(left), tree->right);
        }
        auto right = erase(tree->right, key);
        return right == tree->right ? tree : node(tree->key, tree->value, tree->left, std::move(right));
    }
    static const Value* get(const Node* tree, Key key) noexcept {
        while (tree) {
            if (key == tree->key) return &tree->value;
            tree = (key < tree->key ? tree->left : tree->right).get();
        }
        return nullptr;
    }
    template <class F>
    static void visit(const Node* tree, F& call) {
        if (!tree) return;
        visit(tree->left.get(), call);
        call(tree->key, tree->value);
        visit(tree->right.get(), call);
    }
    template <class F>
    static void changes(const Node* old, const Node* next, F& call) {
        if (old == next) return;
        if (old && next && old->key == next->key) {
            changes(old->left.get(), next->left.get(), call);
            if (old->value != next->value) call(next->key, old->value, next->value);
            changes(old->right.get(), next->right.get(), call);
            return;
        }
        // A new or removed row can rotate the tree. This uncommon structural
        // case compares the affected subtrees without allocating a key set.
        auto removed = [&](Key key, const Value& value) {
            const auto* replacement = get(next, key);
            if (!replacement || *replacement != value) call(key, value, replacement ? *replacement : Value{});
        };
        visit(old, removed);
        auto added = [&](Key key, const Value& value) {
            if (!get(old, key)) call(key, Value{}, value);
        };
        visit(next, added);
    }

public:
    const Value* get(Key key) const noexcept { return get(root_.get(), key); }
    void set(Key key, const Value& value) { root_ = value ? set(root_, key, value) : erase(root_, key); }
    void erase(Key key) { root_ = erase(root_, key); }
    template <class F>
    void visit(F&& call) const {
        visit(root_.get(), call);
    }
    template <class F>
    void changes(const SceneRowTable* previous, F&& call) const {
        changes(previous ? previous->root_.get() : nullptr, root_.get(), call);
    }
};
}
