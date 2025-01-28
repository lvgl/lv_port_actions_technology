/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_sharedref.h"
#include <utility>

namespace gx {
class String;

class ByteArray {
public:
    typedef const uint8_t *const_iterator;
    ByteArray() GX_DEFAULT;
    explicit ByteArray(std::size_t size);
    ByteArray(std::initializer_list<uint8_t> data);
    ByteArray(const uint8_t *data, std::size_t size);
    ByteArray(const_iterator first, const_iterator last);
    ByteArray(const ByteArray &array);
    ByteArray &operator=(const ByteArray &array);
    GX_NODISCARD bool isEmpty() const;
    GX_NODISCARD bool isStatic() const;
    GX_NODISCARD std::size_t size() const;
    GX_NODISCARD const_iterator begin() const { return constData(); }
    GX_NODISCARD const_iterator end() const { return constData() + size(); }
    void clear();
    void resize(std::size_t size);
    void push_back(std::uint8_t byte); // NOLINT(readability-*)
    void append(const_iterator first, const_iterator last);
    GX_NODISCARD uint8_t *data();
    GX_NODISCARD const uint8_t *data() const;
    GX_NODISCARD const uint8_t *constData() const { return data(); }
    GX_NODISCARD const char *charData() const {
        return reinterpret_cast<const char *>(constData());
    }
    uint8_t operator[](std::size_t index) const { return constData()[index]; }
    GX_NODISCARD uint8_t *detach();
    GX_NODISCARD uint8_t *detachUnique();
    /**
     * Converts a ByteArray to a String object.
     */
    GX_NODISCARD String toString() const;
    /**
     * Converts a ByteArray to a String object. Equivalent to function toString() const.
     */
    explicit operator String() const;
    ByteArray operator+(const ByteArray &other) const;
    ByteArray &operator+=(const ByteArray &other);

    /**
     * Construct a ByteArray object from an existing memory block .
     * @param data Pointing to the start address of the memory block. The \p data is read-only and
     * writes to copies.
     * @param size The size of the memory block.
     */
    static ByteArray fromStatic(const uint8_t *data, std::size_t size);
    static ByteArray fromString(const char *string);
    static ByteArray fromString(const char *string, std::size_t size);
    static ByteArray fromString(const String &string);

    static void free(void *data);

private:
    class Data;
    GX_SHARED_HELPER_DECL(Helper, Data);
    SharedRef<Data, Helper> d;
};

inline ByteArray::ByteArray(std::initializer_list<uint8_t> data)
    : ByteArray(data.begin(), data.end()) {}

// equal to operators
bool operator==(const ByteArray &left, const ByteArray &right);
inline bool operator!=(const ByteArray &left, const ByteArray &right) { return !(left == right); }

// relational operators
bool operator<(const ByteArray &left, const ByteArray &right);
inline bool operator<=(const ByteArray &left, const ByteArray &right) { return !(right < left); }
inline bool operator>=(const ByteArray &left, const ByteArray &right) { return !(left < right); }
inline bool operator>(const ByteArray &left, const ByteArray &right) { return !(left <= right); }

const Logger &operator<<(const Logger &, const ByteArray &);

inline ByteArray ByteArray::operator+(const ByteArray &other) const {
    return isEmpty() ? other : ByteArray(*this) += other;
}

inline ByteArray &ByteArray::operator+=(const ByteArray &other) {
    append(other.begin(), other.end());
    return *this;
}
} // namespace gx
