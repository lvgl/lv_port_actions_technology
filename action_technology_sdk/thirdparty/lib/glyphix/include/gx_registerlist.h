/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"

namespace gx {
template<class T, T *T::*next = &T::next> class RegisterList {
public:
    RegisterList() : m_list(), m_tail() {}
    ~RegisterList() {
        for (iterator it = begin(); it != end();)
            delete &(*it++);
    }

    void push_back(T *node) { // NOLINT(readability-*)
        GX_ASSERT(node);
        if (!m_list)
            m_list = node;
        if (m_tail)
            m_tail->*next = node;
        m_tail = node;
        node->*next = nullptr;
    }

    class iterator {
    public:
        explicit iterator(T *node = nullptr) : m_node(node) {}
        T &operator*() { return *m_node; }
        T *operator->() { return m_node; }
        iterator &operator++() {
            m_node = m_node->*next;
            return *this;
        }
        iterator operator++(int) {
            iterator tmp = *this;
            m_node = m_node->*next;
            return tmp;
        }
        bool operator==(const iterator &rhs) const { return m_node == rhs.m_node; }
        bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

    private:
        T *m_node;
    };

    GX_NODISCARD iterator begin() const { return iterator(m_list); }
    GX_NODISCARD iterator end() const { return iterator(); }

private:
    T *m_list, *m_tail;
};
} // namespace gx
