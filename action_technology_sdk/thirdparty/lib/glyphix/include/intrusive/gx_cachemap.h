/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_sharedref.h"
#include "gx_utils.h"
#include "intrusive/gx_hashmap.h"

namespace gx {
namespace intrusive {
namespace internal {
class CacheMapList {
public:
    CacheMapList() { clear(); }

    struct Node {
        Node *next, *prev;
    };
    GX_NODISCARD Node *front() const { return m_body.next; }
    GX_NODISCARD Node *back() const { return m_body.prev; }
    GX_NODISCARD const Node *end() const { return &m_body; }
    void push_front(Node *node); // NOLINT(readability-*)
    void move_front(Node *node); // NOLINT(readability-*)
    Node *erase(Node *node);
    void clear();

private:
    Node m_body;
};
} // namespace internal

template<typename T, typename Ref = SharedRef<T>, typename Helper = utils::cache_helper<T>,
         typename Alloc = std::allocator<T> >
class CacheMap : public NonCopyable<CacheMap<T, Ref, Alloc> > {
    struct Node : internal::CacheMapList::Node {
        Ref ref;
#ifdef GX_COPMATIBLE_CXX_98
        explicit Node(const Ref &ref) : ref(ref) {}
#else
        explicit Node(Ref ref) noexcept : ref(std::move(ref)) {}
#endif
        GX_NODISCARD std::size_t hash() const {
            typedef typename utils::remove_reference<typename Helper::key_type>::type KT;
            return utils::hash<KT>()(Helper().key(*ref));
        }
        template<typename KT1> bool operator==(const KT1 &key) const {
            typedef typename utils::remove_const<
                typename utils::remove_reference<typename Helper::key_type>::type>::type KT2;
            return utils::equal<KT2>()(Helper().key(*ref), key);
        }
        bool operator==(const Node &rhs) const { return *this == Helper().key(*rhs.ref); }
    };
    struct Hash {
        std::size_t operator()(const Node &node) {
            typedef typename utils::remove_const<
                typename utils::remove_reference<typename Helper::key_type>::type>::type KT;
            return utils::hash<KT>()(Helper().key(*node.ref));
        }
        template<typename KT> std::size_t operator()(const KT &key) {
            return utils::hash<KT>()(key);
        }
    };
    typedef intrusive::HashMap<
        Node, intrusive::HashNode<Node, Hash>,
        typename Alloc::template rebind<intrusive::HashNode<Node, Hash> >::other>
        HashMap;

public:
    typedef typename Helper::key_type key_type;
    typedef T value_type;
    typedef Ref refer_type;
    typedef typename Helper::size_type size_type;
    typedef Helper helper_type;
    typedef typename HashMap::allocator_type allocator_type;

    class iterator {
    public:
        iterator() : m_node() {}
        GX_NODISCARD bool empty() const { return !m_node; }
        GX_NODISCARD const refer_type &ref() const { return m_node->ref; }
        explicit operator refer_type() { return m_node->ref; }
        T &operator*() const { return m_node->ref.operator*(); }
        T *operator->() const { return m_node->ref.operator->(); }
        bool operator==(iterator rhs) const { return m_node == rhs.m_node; }
        bool operator!=(iterator rhs) const { return m_node != rhs.m_node; }

        iterator &operator++() {
            m_node = static_cast<Node *>(m_node->next);
            return *this;
        }
        iterator operator++(int) {
            iterator temp = this;
            return *(++temp);
        }
        iterator &operator--() {
            m_node = static_cast<Node *>(m_node->prev);
            return *this;
        }
        iterator operator--(int) {
            iterator temp = this;
            return *(++temp);
        }

    private:
        explicit iterator(Node *node) : m_node(node) {}
        Node *m_node;
        friend class CacheMap;
    };
    typedef std::reverse_iterator<iterator> reverse_iterator;

    explicit CacheMap(size_type capacity = 32) : m_size(), m_capacity(capacity) {}
    ~CacheMap() GX_DEFAULT;
    GX_NODISCARD bool empty() const GX_NOEXCEPT { return m_map.empty(); }
    GX_NODISCARD size_type count() const GX_NOEXCEPT { return size_type(m_map.size()); }
    GX_NODISCARD size_type size() const GX_NOEXCEPT { return m_size; }
    GX_NODISCARD size_type capacity() const GX_NOEXCEPT { return m_capacity; }
    GX_NODISCARD size_type elementCount() const GX_NOEXCEPT { return m_map.size(); }
    GX_NODISCARD size_type elementCapacity() const GX_NOEXCEPT { return m_map.capacity(); }
    GX_NODISCARD iterator begin() const GX_NOEXCEPT { return iterator((Node *)m_list.front()); }
    GX_NODISCARD iterator end() const GX_NOEXCEPT { return iterator((Node *)m_list.end()); }
    GX_NODISCARD reverse_iterator rbegin() const GX_NOEXCEPT { return reverse_iterator(end()); }
    GX_NODISCARD reverse_iterator rend() const GX_NOEXCEPT { return reverse_iterator(begin()); }

    void setCapacity(size_type capacity) { m_capacity = capacity; }

    template<class KT> GX_NODISCARD bool contains(const KT &key) const {
        return !m_map.find(key).end();
    }
    template<class KT> GX_NODISCARD iterator find(const KT &key) {
        typename HashMap::iterator res = m_map.find(key);
        return res.end() ? end() : iterator(&*res);
    }
    template<class KT> GX_NODISCARD iterator get(const KT &key) {
        iterator position = find(key);
        if (position != end())
            use(position);
        return position;
    }
    std::pair<iterator, bool> put(const refer_type &ref, bool evict = true) {
        std::pair<typename HashMap::iterator, bool> res = m_map.insert(Node(ref));
        if (res.second) {
            m_list.push_front(&*res.first);
            m_size += Helper().size(*ref);
            if (evict)
                this->evict();
            return std::pair<iterator, bool>(iterator(static_cast<Node *>(m_list.front())), true);
        }
        return std::pair<iterator, bool>(iterator(static_cast<Node *>(&*res.first)), false);
    }
    void use(iterator position) { m_list.move_front(position.m_node); }
    size_type evict() { return evict(size() - min(size(), capacity())); }
    size_type evict(size_type baseline) {
        if (baseline == 0)
            return 0;
        size_type released = 0;
        Node *node = static_cast<Node *>(m_list.back());
        const Node *end = static_cast<const Node *>(m_list.end());
        while (node != end && released < baseline) { // the first node will not be flushed.
            Node *prev = static_cast<Node *>(node->prev);
            // first, try releasing the node.
            size_type delta = Helper().release(*node->ref, baseline - released);
            if (delta > 0) {             // the successfully released node.
                m_list.move_front(node); // move the node to the front.
                released += delta;
            } else if (node->ref.use_count() == 1) { // erase unused unreleased node.
                released += Helper().size(*node->ref);
                m_list.erase(node);
                m_map.erase(Helper().key(*node->ref));
            }
            node = prev;
        }
        released = min(released, m_size);
        m_size -= released;
        // Auto shrink when the HashMap buckets-size is too large.
        if (max<size_type>(m_map.size(), 4) << 2 <= m_map.capacity())
            shrink();
        return released;
    }
    refer_type evictLast() {
        Node *node = static_cast<Node *>(m_list.back());
        const Node *end = static_cast<const Node *>(m_list.end());
        while (m_size > m_capacity && node != end) {
            Node *prev = static_cast<Node *>(node->prev);
            value_type &value = *node->ref;
            // first, try releasing the node.
            size_type released = Helper().release(value, Helper().size(value));
            if (released > 0) {          // the successfully released node.
                m_list.move_front(node); // move the node to the front.
                m_size -= min(released, m_size);
            } else if (node->ref.use_count() == 1) { // erase unused unreleased node.
                refer_type ref = node->ref;
                m_list.erase(node);
                m_map.erase(Helper().key(value));
                m_size -= min(Helper().size(value), m_size);
                return ref;
            }
            node = prev;
        }
        return refer_type();
    }
    iterator erase(iterator position) {
        GX_ASSERT(!position.empty());
        if (position == end())
            return position;
        m_size -= min(Helper().size(*position), m_size);
        iterator next((Node *)m_list.erase(position.m_node));
        m_map.erase(Helper().key(*position));
        return next;
    }
    template<class KT> iterator erase(const KT &key) { return erase(find(key)); }
    void clear() {
        HashMap temp;
        m_map.swap(temp);
        m_list.clear();
        m_size = 0;
    }
    void shrink() { m_map.rehash(count() * 6 / 5); }
    void adjust(int size, bool evict = true) {
        m_size += size;
        if (evict)
            this->evict();
    }

private:
    HashMap m_map;
    internal::CacheMapList m_list;
    size_type m_size, m_capacity;
};
} // namespace intrusive
} // namespace gx
