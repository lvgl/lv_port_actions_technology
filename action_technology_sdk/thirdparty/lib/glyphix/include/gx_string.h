/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_utils.h"
#include <cstring>
#include <iterator>
#include <string>

namespace gx {
class UnicodeIterator {
public:
    UnicodeIterator() : m_byte(), m_last(), m_code(), m_state() {}
    explicit UnicodeIterator(const char *str)
        : m_byte(reinterpret_cast<const uint8_t *>(str)), m_last(m_byte + std::strlen(str)),
          m_code(), m_state() {}
    explicit UnicodeIterator(const char *first, const char *last)
        : m_byte(reinterpret_cast<const uint8_t *>(first)),
          m_last(reinterpret_cast<const uint8_t *>(last)), m_code(), m_state() {}
    GX_NODISCARD const char *base() const { return reinterpret_cast<const char *>(m_byte); }
    uint32_t operator*() const { return m_code; }
    bool next();

private:
    const uint8_t *m_byte, *m_last;
    uint32_t m_code, m_state;
};

class String {
    class StringData {
    public:
        int refcnt;
        std::size_t size, capacity;
        GX_NODISCARD char *data() { return reinterpret_cast<char *>(this) + sizeof(StringData); }
        GX_NODISCARD const char *data() const {
            return reinterpret_cast<const char *>(this) + sizeof(StringData);
        }
        GX_NODISCARD char *end() { return data() + size; }
        GX_NODISCARD const char *end() const { return data() + size; }
        void ref() { refcnt++; }
        bool deref() { return --refcnt; }
        GX_NODISCARD StringData *copy() const;
        static StringData *alloc(std::size_t size);
        static void free(StringData *data);
    };

public:
    typedef const char *const_iterator;                                   // NOLINT(readability-*)
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator; // NOLINT(readability-*)
    enum CaseSensitivity { CaseSensitive, CaseInsensitive };

    String() : d(nullptr) {}
    String(const char *s) { initialization(s, s ? std::strlen(s) : 0); }
    String(const char *s, std::size_t size) { initialization(s, size); }
    String(const const_iterator &begin, const const_iterator &end) {
        initialization(begin, end - begin);
    }
    String(const std::string &s) { initialization(s.data(), s.size()); }
    /**
     * Construct the string with the specified initial capacity.
     * \param capacity The initial capacity of the string.
     */
    explicit String(std::size_t capacity) { initialization(nullptr, capacity); }
    String(const String &string);
#ifndef GX_COPMATIBLE_CXX_98
    String(String &&string) noexcept : d(string.d) { string.d = nullptr; }
#endif
    ~String();

    // properties getter
    GX_NODISCARD bool empty() const { return !size(); }
    GX_NODISCARD std::size_t length() const { return size(); }
    GX_NODISCARD std::size_t size() const { return d ? d->size : 0; }
    GX_NODISCARD std::size_t capacity() const { return d ? d->capacity : 0; }
    GX_NODISCARD const char *data() const { return d ? d->data() : ""; }
    GX_NODISCARD char *data() { return detach() ? d->data() : nullptr; }
    GX_NODISCARD const char *c_str() const { return data(); } // NOLINT(readability-*)
    GX_NODISCARD std::size_t hash() const { return hash(begin(), end()); }

    // modify operations
    void clear();
    void push_back(int c);            // NOLINT(readability-*)
    void pop_back();                  // NOLINT(readability-*)
    void push_unicode(uint32_t code); // NOLINT(readability-*)
    void append(int c) { push_back(c); }
    void append(const char *str, std::size_t size);
    void append(const char *str);
    void append(const_iterator first, const_iterator last);
    void append(const String &str);
    void insert(std::size_t pos, const char *str, std::size_t size);
    void insert(std::size_t pos, const_iterator first, const_iterator last);
    void insert(std::size_t pos, const String &str);
    void remove(std::size_t first, std::size_t last);
    void remove(const_iterator first, const_iterator last);
    template<class T> void remove(const std::pair<T, T> &range) {
        remove(range.first, range.second);
    }
    void resize(std::size_t size);
    void reserve(std::size_t size);
    GX_NODISCARD String copy() const;

    // iterator
    GX_NODISCARD const_iterator begin() const { return data(); }
    GX_NODISCARD const_iterator end() const { return data() + size(); }
    GX_NODISCARD const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    GX_NODISCARD const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    //! Get the starting byte iterator of the UTF-8 sequence at a given character index.
    GX_NODISCARD const_iterator utf8At(int index) const;
    //! Get the position of the specified character in the starting byte in the UTF-8 sequence.
    GX_NODISCARD int utf8ByteOffset(int index) const;
    //! Returns a UTF-8 encoded byte interval to represent the start and end positions of a given
    //! character range in the byte sequence.
    GX_NODISCARD std::pair<int, int> utf8ByteRange(int firstIndex, int endIndex) const;
    GX_NODISCARD int utf8Length() const;

    GX_NODISCARD UnicodeIterator unicode() const { return UnicodeIterator(begin(), end()); }

    // match & search algorithms
    GX_NODISCARD const_iterator find(int ch, CaseSensitivity sensitive = CaseSensitive) const;
    GX_NODISCARD const_iterator find(const char *substr,
                                     CaseSensitivity sensitive = CaseSensitive) const;
    GX_NODISCARD const_iterator find(const String &substr,
                                     CaseSensitivity sensitive = CaseSensitive) const;
    GX_NODISCARD const_iterator find(const_iterator first, const_iterator last,
                                     CaseSensitivity sensitive = CaseSensitive) const;
    GX_NODISCARD const_iterator findLast(int ch, CaseSensitivity sensitive = CaseSensitive) const;
    GX_NODISCARD const_iterator findLast(const char *substr,
                                         CaseSensitivity sensitive = CaseSensitive) const;
    GX_NODISCARD const_iterator findLast(const String &substr,
                                         CaseSensitivity sensitive = CaseSensitive) const;
    GX_NODISCARD bool contains(int ch, CaseSensitivity sensitive = CaseSensitive) const {
        return find(ch, sensitive) != end();
    }
    GX_NODISCARD bool contains(const char *substr,
                               CaseSensitivity sensitive = CaseSensitive) const {
        return find(substr, sensitive) != end();
    }
    GX_NODISCARD bool contains(const String &substr,
                               CaseSensitivity sensitive = CaseSensitive) const {
        return find(substr, sensitive) != end();
    }
    GX_NODISCARD bool contains(const_iterator first, const_iterator last,
                               CaseSensitivity sensitive = CaseSensitive) const {
        return find(first, last, sensitive) != end();
    }
    GX_NODISCARD bool match(const_iterator first, const_iterator last, std::size_t offset,
                            CaseSensitivity sensitive = CaseSensitive) const;
    GX_NODISCARD bool match(const char *substr, std::size_t offset,
                            CaseSensitivity sensitive = CaseSensitive) const {
        return match(substr, substr + std::strlen(substr), offset, sensitive);
    }
    GX_NODISCARD bool match(const String &substr, std::size_t offset,
                            CaseSensitivity sensitive = CaseSensitive) const {
        return match(substr.begin(), substr.end(), offset, sensitive);
    }
    GX_NODISCARD bool startsWith(const_iterator first, const_iterator last,
                                 CaseSensitivity sensitive = CaseSensitive) const {
        return match(first, last, 0, sensitive);
    }
    GX_NODISCARD bool startsWith(const char *substr,
                                 CaseSensitivity sensitive = CaseSensitive) const {
        return match(substr, 0, sensitive);
    }
    GX_NODISCARD bool startsWith(const String &substr,
                                 CaseSensitivity sensitive = CaseSensitive) const {
        return match(substr, 0, sensitive);
    }
    GX_NODISCARD bool endsWith(const_iterator first, const_iterator last,
                               CaseSensitivity sensitive = CaseSensitive) const {
        return match(first, last, size() - (last - first), sensitive);
    }
    GX_NODISCARD bool endsWith(const char *substr,
                               CaseSensitivity sensitive = CaseSensitive) const {
        const std::size_t sublen = std::strlen(substr);
        const std::size_t len = size();
        return len >= sublen && match(substr, substr + sublen, len - sublen, sensitive);
    }
    GX_NODISCARD bool endsWith(const String &substr,
                               CaseSensitivity sensitive = CaseSensitive) const {
        const std::size_t sublen = substr.size();
        const std::size_t len = size();
        return len >= sublen && match(substr, len - substr.size(), sensitive);
    }

    // substrings
    GX_NODISCARD String substr(const_iterator first, const_iterator last) const;
    GX_NODISCARD String substr(std::size_t position, std::size_t len = std::size_t(-1)) const {
        const const_iterator first = begin() + position;
        return substr(first, len == std::size_t(-1) ? end() : first + len);
    }
    GX_NODISCARD String substr(const const_iterator &position) const {
        return substr(position, end());
    }
    GX_NODISCARD String left(std::size_t position) const {
        return substr(begin(), begin() + position);
    }
    GX_NODISCARD String left(const const_iterator &position) const {
        return substr(begin(), position);
    }
    GX_NODISCARD String right(std::size_t position) const { return substr(position); }
    GX_NODISCARD String right(const const_iterator &position) const { return substr(position); }

    // operators
    int operator[](std::size_t position) const { return data()[position]; }
    String operator+(const String &str) const;
    String operator+(const char *str) const;
    String &operator+=(int c) {
        append(c);
        return *this;
    }
    String &operator+=(const String &s) {
        append(s);
        return *this;
    }
    String &operator+=(const char *s) {
        append(s);
        return *this;
    }
    String &operator<<(char c) { return *this += c; }
    String &operator<<(const String &s) { return *this += s; }
    String &operator<<(const char *s) { return *this += s; }
    explicit operator std::string() const { return std::string(data(), size()); }
    // move assign
    void assign(const String &str);
    void assign(const char *str);
    void assign(const const_iterator &begin, const const_iterator &end);
    String &operator=(const String &str) { // NOLINT(*-self-assignment)
        assign(str);                       // The assign() handles self-assignment.
        return *this;
    }
    String &operator=(const char *str) {
        assign(str);
        return *this;
    }
#ifndef GX_COPMATIBLE_CXX_98
    String &operator=(String &&str) noexcept {
        if (d && !d->deref())
            StringData::free(d);
        d = str.d;
        str.d = nullptr;
        return *this;
    }
#endif

    static std::size_t hash(const char *first, const char *last);

private:
    void initialization(const char *data, std::size_t size);
    bool detach();
    bool exchange(std::size_t size);
    bool expand(std::size_t incr);

private:
    StringData *d;
    friend String operator+(const char *, const String &);
};

String operator+(const char *cstr, const String &str);

// BUG: If a null character is stored in the string, the following
// comparison functions may not get the correct result.

// equal to operators
bool operator==(const String &left, const String &right);

inline bool operator==(const String &left, const char *right) {
    return !std::strcmp(left.begin(), right);
}

inline bool operator==(const char *left, const String &right) { return right == left; }

// not equal to operators
inline bool operator!=(const String &left, const String &right) { return !(left == right); }

inline bool operator!=(const String &left, const char *right) { return !(left == right); }

inline bool operator!=(const char *left, const String &right) { return !(left == right); }

// less than to operators
inline bool operator<(const String &left, const String &right) {
    std::size_t size = min(left.size(), right.size()) + 1;
    return std::memcmp(left.data(), right.data(), size) < 0;
}

inline bool operator<(const String &left, const char *right) {
    return std::strcmp(left.data(), right) < 0;
}

inline bool operator<(const char *left, const String &right) {
    return std::strcmp(left, right.data()) < 0;
}

// less or equal than to operators
inline bool operator<=(const String &left, const String &right) { return !(right < left); }

inline bool operator<=(const String &left, const char *right) { return !(right < left); }

inline bool operator<=(const char *left, const String &right) { return !(right < left); }

// greater or equal to operators
inline bool operator>=(const String &left, const String &right) { return !(left < right); }

inline bool operator>=(const String &left, const char *right) { return !(left < right); }

inline bool operator>=(const char *left, const String &right) { return !(left < right); }

// greater to operators
inline bool operator>(const String &left, const String &right) { return !(left <= right); }

inline bool operator>(const String &left, const char *right) { return !(left <= right); }

inline bool operator>(const char *left, const String &right) { return !(left <= right); }

inline int String::utf8ByteOffset(int index) const {
    return static_cast<int>(utf8At(index) - begin());
}

namespace utils {
template<> struct hash<const char *> {
    std::size_t operator()(const char *v) const { return String::hash(v, v + std::strlen(v)); }
};
} // namespace utils
} // namespace gx
