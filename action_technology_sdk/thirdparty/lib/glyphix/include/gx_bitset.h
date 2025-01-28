/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"
#include <cstddef>

namespace gx {
class BitSet {
public:
    BitSet();
    explicit BitSet(int capacity);
    BitSet(const BitSet &other);
#ifndef GX_COPMATIBLE_CXX_98
    BitSet(BitSet &&other) noexcept;
#endif
    ~BitSet();

    GX_NODISCARD int capacity() const { return m_capacity * int(sizeof(std::size_t) * 8); }
    void setCapacity(int capacity);
    void set(int value);
    BitSet &sets(int value) { return set(value), *this; }
    void reset(int value);
    GX_NODISCARD bool test(int value) const;
    GX_NODISCARD bool intersect(const BitSet &other) const;
    void united(const BitSet &other);
    GX_NODISCARD int extra() const { return m_extra; }
    void setExtra(int extra) { m_extra = uint16_t(extra); }
    void clear();

    bool operator[](int value) const { return test(value); }
    bool operator==(const BitSet &other) const;
    bool operator!=(const BitSet &other) const { return !(*this == other); }
    BitSet &operator=(const BitSet &other);
#ifndef GX_COPMATIBLE_CXX_98
    BitSet &operator=(BitSet &&other) noexcept;
#endif
    BitSet &operator|=(const BitSet &other);

private:
    union {
        std::size_t raw;
        std::size_t *vector;
    } m_bits;
    uint16_t m_capacity;
    uint16_t m_extra;
};

inline BitSet &BitSet::operator|=(const BitSet &other) {
    united(other);
    return *this;
}
} // namespace gx
