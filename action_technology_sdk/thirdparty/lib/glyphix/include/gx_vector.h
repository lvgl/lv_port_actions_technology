/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"
#include <algorithm>
#include <iterator>
#include <memory>

namespace gx {
template<typename T, class Alloc = std::allocator<T> > class Vector : private Alloc {
public:
    typedef T value_type;
    typedef Alloc allocator_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;
    typedef typename Alloc::size_type size_type;
    typedef typename Alloc::difference_type difference_type;
    typedef pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    Vector() noexcept : allocator_type(), m_first(), m_last(), m_end() {}
    explicit Vector(const allocator_type &alloc)
        : allocator_type(alloc), m_first(), m_last(), m_end() {}
    explicit Vector(size_type n, const_reference v = value_type(),
                    const allocator_type &alloc = allocator_type())
        : allocator_type(alloc) {
        initialize(n, v);
    }
    template<class InputIterator>
    Vector(InputIterator first, InputIterator last, const allocator_type &alloc = allocator_type())
        : allocator_type(alloc), m_first(), m_last(), m_end() {
        insert(begin(), first, last);
    }
#ifndef GX_COPMATIBLE_CXX_98
    Vector(std::initializer_list<T> init, const allocator_type &alloc = allocator_type())
        : Vector(init.begin(), init.end(), alloc) {}
#endif
    Vector(const Vector &vec) : allocator_type(vec) {
        m_first = allocator_type::allocate(vec.size());
        m_last = std::uninitialized_copy(vec.m_first, vec.m_last, m_first);
        m_end = m_last;
    }
#ifndef GX_COPMATIBLE_CXX_98
    Vector(Vector &&vec) noexcept : m_first(vec.m_first), m_last(vec.m_last), m_end(vec.m_end) {
        vec.m_first = vec.m_last = vec.m_end = nullptr;
    }
#endif

    ~Vector() {
        destroy(m_first, m_last);
        deallocate();
    }

    Vector &operator=(const Vector &vec) {
        if (&vec != this)
            assign(vec.begin(), vec.end());
        return *this;
    }
#ifndef GX_COPMATIBLE_CXX_98
    Vector &operator=(Vector &&vec) noexcept {
        swap(vec);
        return *this;
    }
#endif

    GX_NODISCARD iterator begin() { return m_first; }
    GX_NODISCARD iterator end() { return m_last; }
    GX_NODISCARD const_iterator begin() const { return m_first; }
    GX_NODISCARD const_iterator end() const { return m_last; }
    GX_NODISCARD const_iterator cbegin() const { return begin(); }
    GX_NODISCARD const_iterator cend() const { return end(); }
    GX_NODISCARD reverse_iterator rbegin() { return reverse_iterator(end()); }
    GX_NODISCARD reverse_iterator rend() { return reverse_iterator(begin()); }
    GX_NODISCARD const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    GX_NODISCARD const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    GX_NODISCARD const_reverse_iterator crbegin() const { return rbegin(); }
    GX_NODISCARD const_reverse_iterator crend() const { return rend(); }
    GX_NODISCARD size_type size() const { return size_type(m_last - m_first); }
    GX_NODISCARD size_type capacity() const { return size_type(m_end - m_first); }
    GX_NODISCARD bool empty() const { return m_first == m_last; }
    reference operator[](size_type n) { return m_first[n]; }
    const_reference operator[](size_type n) const { return m_first[n]; }
    GX_NODISCARD reference at(size_type n) { return m_first[n]; }
    GX_NODISCARD const_reference at(size_type n) const { return m_first[n]; }
    GX_NODISCARD reference front() { return *m_first; }
    GX_NODISCARD const_reference front() const { return *m_first; }
    GX_NODISCARD reference back() { return *(m_last - 1); }
    GX_NODISCARD const_reference back() const { return *(m_last - 1); }
    GX_NODISCARD pointer data() { return m_first; }
    GX_NODISCARD const_pointer data() const { return m_first; }

    void push_back(const_reference value) { // NOLINT(readability-*)
        if (m_last != m_end)
            allocator_type::construct(m_last++, value);
        else
            insert(end(), value);
    }
    void pop_back() { allocator_type::destroy(--m_last); } // NOLINT(readability-*)
    iterator erase(iterator first, iterator last) {
        if (first != last) {
            iterator i = std::copy(last, m_last, first);
            destroy(i, m_last);
            m_last = m_last - (last - first);
        }
        return first;
    }
    const_iterator erase(const_iterator first, const_iterator last) {
        return const_cast<const_iterator>(
            erase(const_cast<iterator>(first), const_cast<iterator>(last)));
    }
    iterator erase(iterator position) {
        if (position + 1 != end())
            std::copy(position + 1, end(), position);
        allocator_type::destroy(--m_last);
        return position;
    }
    const_iterator erase(const_iterator position) {
        return const_cast<const_iterator>(erase(const_cast<iterator>(position)));
    }
    void clear() { erase(begin(), end()); }

    template<class InputIterator> void assign(InputIterator first, InputIterator last) {
        clear();
        insert(begin(), first, last);
    }
    void assign(size_type n, const_reference value) {
        clear();
        insert(begin(), n, value);
    }

    void swap(Vector &vec) {
        std::swap(m_first, vec.m_first);
        std::swap(m_last, vec.m_last);
        std::swap(m_end, vec.m_end);
    }

    void insert(iterator position, size_type n, const_reference value) {
        if (n != 0) {
            if (capacity() >= size() + n) {
                const size_type tails = end() - position;
                if (tails > n) {
                    std::uninitialized_copy(m_last - n, m_last, m_last);
                    std::copy_backward(position, end() - n, m_last);
                    std::fill(position, position + n, value);
                } else {
                    std::uninitialized_fill(m_last, m_last + n - tails, value);
                    std::uninitialized_copy(position, end(), position + n);
                    std::fill(position, end(), value);
                }
                m_last += n;
            } else {
                const size_type len = size() + max(size(), n);
                pointer first = allocator_type::allocate(len);
                pointer last = std::uninitialized_copy(begin(), position, first);
                std::uninitialized_fill(last, last + n, value);
                last = std::uninitialized_copy(position, end(), last + n);
                destroy(m_first, m_last);
                deallocate();
                m_first = first;
                m_last = last;
                m_end = first + len;
            }
        }
    }
    void insert(const_iterator position, size_type n, const_reference value) {
        insert(const_cast<iterator>(position), n, value);
    }

    template<class InputIterator>
    void insert(iterator position, InputIterator first, InputIterator last) {
        if (first != last) {
            size_type n = std::distance(first, last);
            if (capacity() >= size() + n) {
                const size_type tails = end() - position;
                if (tails > n) {
                    std::uninitialized_copy(m_last - n, m_last, m_last);
                    std::copy_backward(position, end() - n, m_last);
                    std::copy(first, last, position);
                } else {
                    InputIterator it(first);
                    std::advance(it, tails);
                    std::uninitialized_copy(it, last, m_last);
                    std::uninitialized_copy(position, end(), position + n);
                    std::copy(first, it, position);
                }
                m_last += n;
            } else {
                const size_type len = size() + max(size(), n);
                pointer firstP = allocator_type::allocate(len);
                pointer lastP = std::uninitialized_copy(begin(), position, firstP);
                lastP = std::uninitialized_copy(first, first + n, lastP);
                lastP = std::uninitialized_copy(position, end(), lastP);
                destroy(m_first, m_last);
                deallocate();
                m_first = firstP;
                m_last = lastP;
                m_end = firstP + len;
            }
        }
    }
    template<class InputIterator>
    void insert(const_iterator position, InputIterator first, InputIterator last) {
        return insert(const_cast<iterator>(position), first, last);
    }

    iterator insert(iterator position, const_reference value) {
        if (m_last != m_end) {
            if (position == end()) {
                allocator_type::construct(m_last++, value);
            } else {
                allocator_type::construct(m_last, *(m_last - 1));
                std::copy_backward(position, end() - 1, m_last);
                ++m_last;
                *position = value;
            }
        } else {
            const size_type len = size() ? size() * 2 : 1;
            iterator pos = position;
            pointer first = allocator_type::allocate(len);
            pointer last = std::uninitialized_copy(begin(), pos, first);
            position = last;
            allocator_type::construct(last, value);
            last = std::uninitialized_copy(pos, end(), last + 1);
            destroy(m_first, m_last);
            deallocate();
            m_first = first;
            m_last = last;
            m_end = m_first + len;
        }
        return position;
    }
    iterator insert(const_iterator position, const_reference value) {
        return insert(const_cast<iterator>(position), value);
    }

    void resize(size_type newSize, const_reference value = value_type()) {
        if (newSize < size())
            erase(begin() + newSize, end());
        else
            insert(end(), newSize - size(), value);
    }

    void reserve(size_type n) {
        if (n > capacity()) {
            pointer first = allocator_type::allocate(n);
            pointer last = std::uninitialized_copy(m_first, m_last, first);
            destroy(m_first, m_last);
            deallocate();
            m_first = first;
            m_last = last;
            m_end = m_first + n;
        }
    }

private:
    void initialize(size_type n, const_reference value) {
        m_first = allocAndFill(n, value);
        m_last = m_first + n;
        m_end = m_last;
    }
    pointer allocAndFill(size_type n, const_reference v) {
        pointer data = allocator_type::allocate(n);
        std::uninitialized_fill(data, data + n, v);
        return data;
    }
    void deallocate() {
        if (m_first)
            allocator_type::deallocate(m_first, m_end - m_first);
    }
    void destroy(pointer first, pointer last) {
        for (; first < last; ++first)
            allocator_type::destroy(first);
    }

private:
    pointer m_first, m_last, m_end;
};

template<class T, class Alloc>
bool operator==(const Vector<T, Alloc> &lhs, const Vector<T, Alloc> &rhs) {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<class T, class Alloc>
bool operator!=(const Vector<T, Alloc> &lhs, const Vector<T, Alloc> &rhs) {
    return !(lhs == rhs);
}

template<class T, class Alloc>
bool operator<(const Vector<T, Alloc> &lhs, const Vector<T, Alloc> &rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<class T, class Alloc>
bool operator<=(const Vector<T, Alloc> &lhs, const Vector<T, Alloc> &rhs) {
    return !(rhs < lhs);
}

template<class T, class Alloc>
bool operator>(const Vector<T, Alloc> &lhs, const Vector<T, Alloc> &rhs) {
    return rhs < lhs;
}

template<class T, class Alloc>
bool operator>=(const Vector<T, Alloc> &lhs, const Vector<T, Alloc> &rhs) {
    return !(lhs < rhs);
}
} // namespace gx
