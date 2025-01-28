/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"
#include <memory>

namespace gx {
namespace intrusive {
class ListNodeBase {
public:
    ListNodeBase() GX_DEFAULT;

private:
    ListNodeBase *m_prev;
    ListNodeBase *m_next;
    friend class ListBase;
};

class ListBase {
public:
    ListBase();
    ~ListBase();

    GX_NODISCARD std::size_t size() const;

protected:
    class iterator_base {
    public:
        bool operator==(const iterator_base &rhs) const { return m_node == rhs.m_node; }
        bool operator!=(const iterator_base &rhs) const { return !(*this == rhs); }

    protected:
        explicit iterator_base(ListNodeBase *node) : m_node(node) {}
        void next() { m_node = m_node->m_next; }
        void prev() { m_node = m_node->m_prev; }
        GX_NODISCARD ListNodeBase *node() const { return m_node; }

    private:
        ListNodeBase *m_node;
        friend class ListBase;
    };

    GX_NODISCARD iterator_base beginBase() const { return iterator_base(m_first); }
    GX_NODISCARD iterator_base endBase() const { return iterator_base(m_last); }
    void insertBase(iterator_base pos, ListNodeBase *node);
    iterator_base removeBase(iterator_base pos, void destroyer(ListNodeBase *));
    void pushFrontBase(ListNodeBase *node);
    void pushBackBase(ListNodeBase *node);
    void popFrontBase(void destroyer(ListNodeBase *));
    void popBackBase(void destroyer(ListNodeBase *));
    void destroyNodes(void destroyer(ListNodeBase *));

private:
    ListNodeBase *m_first, *m_last;
};

template<typename T> class ListNode : public ListNodeBase {
    T m_data;

public:
    typedef T value_type;

    explicit ListNode(const T &data) : ListNodeBase(), m_data(data) {}
    T &operator*() { return m_data; }
    const T &operator*() const { return m_data; }
};

template<typename T, typename Node = ListNode<T>, typename Alloc = std::allocator<Node> >
class List : public ListBase {
public:
    typedef Alloc allocator_type;
    typedef T value_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;
    class iterator;

    class iterator : public iterator_base {
    public:
        iterator() GX_DEFAULT;
        T &operator*() { return **static_cast<Node *>(node()); }
        T *operator->() { return **static_cast<Node *>(node()); }
        iterator &operator++() {
            next();
            return *this;
        }
        iterator operator++(int) {
            iterator temp = *this;
            next();
            return temp;
        }
        iterator &operator--() {
            prev();
            return *this;
        }
        iterator operator--(int) {
            iterator temp = *this;
            prev();
            return temp;
        }

    private:
        explicit iterator(iterator_base base) : iterator_base(base) {}
        friend class List;
    };
    class const_iterator : public iterator_base {
    public:
        const_iterator() GX_DEFAULT;
        const T &operator*() const { return **static_cast<Node *>(node()); }
        const T *operator->() const { return **static_cast<Node *>(node()); }
        const_iterator &operator++() {
            next();
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator temp = *this;
            next();
            return temp;
        }
        const_iterator &operator--() {
            prev();
            return *this;
        }
        const_iterator operator--(int) {
            const_iterator temp = *this;
            prev();
            return temp;
        }

    private:
        explicit const_iterator(iterator_base base) : iterator_base(base) {}
        explicit const_iterator(iterator base) : iterator_base(base) {}
        friend class List;
    };

    List() GX_DEFAULT;
    ~List() { destroyNodes(destroyNode); }

    GX_NODISCARD const T &front() const { return *begin(); }
    GX_NODISCARD const T &back() const { return *end(); }
    GX_NODISCARD iterator begin() { return beginBase(); }
    GX_NODISCARD iterator end() { return endBase(); }
    GX_NODISCARD iterator begin() const { return iterator(beginBase()); }
    GX_NODISCARD iterator end() const { return iterator(endBase()); }
    void push_front(const T &value) { // NOLINT(readability-*)
        allocator_type alloc;
        Node *node = alloc.allocate(sizeof(Node));
        alloc.construct(node, value);
        pushFrontBase(node);
    }
    void push_back(const T &value) { // NOLINT(readability-*)
        allocator_type alloc;
        Node *node = alloc.allocate(sizeof(Node));
        alloc.construct(node, value);
        pushBackBase(node);
    }
    iterator insert(iterator position, const T &value) {
        allocator_type alloc;
        Node *node = alloc.allocate(sizeof(Node));
        alloc.construct(node, value);
        insertBase(position, node);
        return position;
    }
    iterator remove(iterator position) { return removeBase(position, destroyNode); }

private:
    static void destroyNode(ListNodeBase *node) {
        Node *p = static_cast<Node *>(node);
        allocator_type alloc;
        alloc.destroy(p);
        alloc.deallocate(p, sizeof(Node));
    }
};
} // namespace intrusive
} // namespace gx
