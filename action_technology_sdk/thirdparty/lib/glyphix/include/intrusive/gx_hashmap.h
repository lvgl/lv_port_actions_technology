/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_utils.h"
#include <memory>
#include <utility>

namespace gx {
namespace intrusive {
namespace internal {
class HashNodeBase {
public:
    HashNodeBase() GX_DEFAULT;

private:
    class HashNodeBase *hash_member; // NOLINT(readability-*)
    friend class HashMapBase;
};

class HashMapBase {
public:
    HashMapBase() GX_NOEXCEPT : m_size() {}
    ~HashMapBase() GX_DEFAULT;
    GX_NODISCARD bool empty() const { return !m_size; }
    GX_NODISCARD std::size_t size() const { return m_size; }
    GX_NODISCARD std::size_t capacity() const { return buckets.size(); }

protected:
    struct iterator_base { // NOLINT(readability-*)
        iterator_base() : bucket(), last(), prev() {}
        iterator_base(HashNodeBase **bucket, HashNodeBase **last, HashNodeBase *prev = nullptr)
            : bucket(bucket), last(last), prev(prev) {}
        GX_NODISCARD bool end() const { return bucket >= last; }
        iterator_base &operator++();
        iterator_base operator++(int);
        bool operator==(const iterator_base &rhs) const {
            return prev == rhs.prev && bucket == rhs.bucket;
        }
        bool operator!=(const iterator_base &rhs) const { return !(*this == rhs); }

    protected:
        GX_NODISCARD HashNodeBase *node() const { return prev ? prev->hash_member : *bucket; }
        HashNodeBase **bucket, **last;
        HashNodeBase *prev;
        friend class HashMapBase;
    };

    class Buckets {
    public:
        typedef HashNodeBase **iterator;

        Buckets() : m_first(), m_last() {}
        Buckets(HashNodeBase **vector, std::size_t size);
        ~Buckets() GX_DEFAULT;
        GX_NODISCARD bool empty() const { return m_first == m_last; }
        GX_NODISCARD std::size_t size() const { return m_last - m_first; }
        GX_NODISCARD iterator begin() const { return m_first; }
        GX_NODISCARD iterator end() const { return m_last; }
        GX_NODISCARD iterator hashBucket(std::size_t hash) const { return begin() + hash % size(); }
        iterator_base insert(std::size_t hash, HashNodeBase *node) const;
        void swap(Buckets &other);
        HashNodeBase **vector() { return m_first; }

    private:
        HashNodeBase **m_first, **m_last;
        Buckets &operator=(const Buckets &);
    };

    iterator_base insertBase(std::size_t hash, HashNodeBase *node) {
        ++m_size;
        return buckets.insert(hash, node);
    }
    GX_NODISCARD iterator_base beginBase() const;
    GX_NODISCARD iterator_base endBase() const {
        return iterator_base(buckets.end(), buckets.end());
    }
    void eraseBase(iterator_base &pos);
    void rehashBase(Buckets &replacement, std::size_t (*hasher)(HashNodeBase *));
    void destroyNodes(void (*destroyer)(HashNodeBase *));
    void clearBase(void (*destroyer)(HashNodeBase *));

    void swap(HashMapBase &other) GX_NOEXCEPT {
        buckets.swap(other.buckets);
        std::swap(m_size, other.m_size);
    }

    template<typename TR, typename TP> static TR nextMember(TP node) {
        return static_cast<TR>(node->hash_member);
    }

    Buckets buckets;

private:
    std::size_t m_size;
};
} // namespace internal

template<typename T, typename Hash = utils::hash<T>, typename Equal = utils::equal<T> >
class HashNode : public internal::HashNodeBase {
    T m_data;

public:
    typedef Hash hash;   // NOLINT(readability-*)
    typedef Equal equal; // NOLINT(readability-*)
    typedef T value_type;

    explicit HashNode(const T &data) : HashNodeBase(), m_data(data) {}
    T &operator*() { return m_data; }
    const T &operator*() const { return m_data; }
};

template<typename T, typename Node = HashNode<T>, typename Alloc = std::allocator<Node> >
class HashMap : protected internal::HashMapBase {
    typedef typename Node::hash hash;   // NOLINT(readability-*)
    typedef typename Node::equal equal; // NOLINT(readability-*)

public:
    typedef Alloc allocator_type;
    typedef typename Alloc::size_type size_type;
    typedef T value_type;
    typedef T &reference;
    typedef const T &const_reference;
    typedef T *pointer;
    typedef const T *const_pointer;

    class iterator : public iterator_base {
    public:
        typedef HashMap::reference reference;
        typedef HashMap::pointer pointer;

        iterator() GX_DEFAULT;
        reference operator*() const { return **static_cast<Node *>(node()); }
        pointer operator->() const { return &operator*(); }

    private:
        explicit iterator(const iterator_base &base) : iterator_base(base) {}
        friend HashMap;
    };

    class const_iterator : public iterator_base {
    public:
        typedef HashMap::reference reference;
        typedef HashMap::pointer pointer;

        const_iterator() GX_DEFAULT;
        const_reference operator*() const { return **static_cast<Node *>(node()); }
        const_pointer operator->() const { return &operator*(); }

    private:
        explicit const_iterator(const iterator_base &base) : iterator_base(base) {}
        friend HashMap;
    };

    HashMap() GX_NOEXCEPT GX_DEFAULT;
    ~HashMap() {
        destroyNodes(destroyNode);
        typename Alloc::template rebind<internal::HashNodeBase *>::other allocator;
        allocator.deallocate(buckets.vector(), buckets.size());
    }

    using HashMapBase::empty;
    GX_NODISCARD size_type size() const GX_NOEXCEPT { return HashMapBase::size(); }
    GX_NODISCARD size_type capacity() const GX_NOEXCEPT { return HashMapBase::capacity(); }
    GX_NODISCARD const_iterator cbegin() const GX_NOEXCEPT { return const_iterator(beginBase()); }
    GX_NODISCARD const_iterator cend() const GX_NOEXCEPT { return const_iterator(endBase()); }
    GX_NODISCARD const_iterator begin() const GX_NOEXCEPT { return cbegin(); }
    GX_NODISCARD const_iterator end() const GX_NOEXCEPT { return cend(); }
    GX_NODISCARD iterator begin() GX_NOEXCEPT { return iterator(beginBase()); }
    GX_NODISCARD iterator end() GX_NOEXCEPT { return iterator(endBase()); }
    template<typename K> GX_NODISCARD const_iterator find(const K &key) const {
        return const_iterator(find(key, hash()(key)));
    }
    template<typename K> GX_NODISCARD iterator find(const K &key) {
        return iterator(find(key, hash()(key)));
    }
    template<typename K> GX_NODISCARD bool contains(const K &key) const {
        return find(key) != end();
    }

    void clear() { clearBase(destroyNode); }
    void swap(HashMap &other) GX_NOEXCEPT { HashMapBase::swap(other); }

    std::pair<iterator, bool> insert(const T &kv) {
        std::size_t hash = typename Node::hash()(kv);
        iterator_base it = find(kv, hash);
        if (!it.end())
            return std::pair<iterator, bool>(iterator(it), false);
        if (empty() || size() > buckets.size())
            rehash(size_type(max<std::size_t>(size() * 2, 2)));
        allocator_type alloc;
        Node *node = alloc.allocate(1);
        alloc.construct(node, kv);
        return std::pair<iterator, bool>(iterator(insertBase(hash, node)), true);
    }

    iterator erase(const_iterator pos) { erase(iterator(pos)); }
    iterator erase(iterator pos) {
        if (pos.end())
            return pos;
        Node *node = static_cast<Node *>(pos.node());
        eraseBase(pos);
        destroyNode(node);
        return pos;
    }
    template<typename K> size_type erase(const K &key) {
        std::size_t oldSize = size();
        erase(find(key));
        return oldSize - size();
    }

    void rehash(size_type size) {
        struct Helper {
            static std::size_t hasher(internal::HashNodeBase *node) {
                return hash()(**static_cast<Node *>(node));
            }
        };
        typename Alloc::template rebind<internal::HashNodeBase *>::other allocator;
        Buckets replacement(allocator.allocate(size), size);
        rehashBase(replacement, Helper::hasher);
        allocator.deallocate(replacement.vector(), replacement.size());
    }

private:
    template<typename KT> GX_NODISCARD iterator_base find(const KT &key, std::size_t hash) const {
        if (!empty()) {
            Buckets::iterator it = buckets.hashBucket(hash);
            Node *node = static_cast<Node *>(*it), *prev = nullptr;
            for (; node; prev = node, node = nextMember<Node *>(node))
                if (equal()(**node, key))
                    return iterator_base(it, buckets.end(), prev);
        }
        return endBase();
    }

    static void destroyNode(internal::HashNodeBase *node) {
        Node *p = static_cast<Node *>(node);
        allocator_type alloc;
        alloc.destroy(p);
        alloc.deallocate(p, 1);
    }
};
} // namespace intrusive
} // namespace gx
