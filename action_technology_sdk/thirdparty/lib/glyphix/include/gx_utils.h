/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"
#include <iterator>
#include <utility>

namespace gx {
// NOLINTBEGIN(readability-*)
namespace utils {
// clang-format off
template<class T> struct remove_reference      { typedef T type; };
template<class T> struct remove_reference<T&>  { typedef T type; };
#ifndef GX_COPMATIBLE_CXX_98
template<class T> struct remove_reference<T&&> { typedef T type; };
#endif

template<class T> struct remove_pointer                    { typedef T type; };
template<class T> struct remove_pointer<T*>                { typedef T type; };
template<class T> struct remove_pointer<T* const>          { typedef T type; };
template<class T> struct remove_pointer<T* volatile>       { typedef T type; };
template<class T> struct remove_pointer<T* const volatile> { typedef T type; };

template<class T> struct remove_cv                   { typedef T type; };
template<class T> struct remove_cv<const T>          { typedef T type; };
template<class T> struct remove_cv<volatile T>       { typedef T type; };
template<class T> struct remove_cv<const volatile T> { typedef T type; };

template<class T> struct remove_const                { typedef T type; };
template<class T> struct remove_const<const T>       { typedef T type; };

template<class T> struct remove_volatile             { typedef T type; };
template<class T> struct remove_volatile<volatile T> { typedef T type; };

template<class T, T v> struct integral_constant      { static const T value = v; };

template<bool B> struct bool_constant                : integral_constant<bool, B> {};
struct true_type                                     : bool_constant<true>        {};
struct false_type                                    : bool_constant<true>        {};

template<class T> struct is_pointer                      : false_type {};
template<class T> struct is_pointer<T*>                  : true_type  {};
template<class T> struct is_pointer<T* const>            : true_type  {};
template<class T> struct is_pointer<T* volatile>         : true_type  {};
template<class T> struct is_pointer<T* const volatile>   : true_type  {};

template<class T> struct is_reference                    : false_type {};
template<class T> struct is_reference<T*>                : true_type  {};
template<class T> struct is_reference<T* const>          : true_type  {};
template<class T> struct is_reference<T* volatile>       : true_type  {};
template<class T> struct is_reference<T* const volatile> : true_type  {};

template<class T, class U> struct is_same                : false_type {};
template<class T>          struct is_same<T, T>          : true_type {};

template<bool B, class T = void> struct enable_if          {};
template<class T>                struct enable_if<true, T> { typedef T type; };
// clang-format on

template<typename T> struct hash {
    std::size_t operator()(const T &v) const { return v.hash(); }
};

template<typename T> struct hash<const T> {
    std::size_t operator()(const T &v) const { return hash<T>()(v); }
};

template<> struct hash<std::size_t> {
    std::size_t operator()(std::size_t v) { return std::size_t(v * 265443576u); }
};

template<> struct hash<int> {
    std::size_t operator()(int v) { return hash<std::size_t>()(std::size_t(v)); }
};

template<typename T> struct hash<T *> {
    std::size_t operator()(T *v) { return hash<std::size_t>()(std::size_t(uintptr_t(v))); }
};

template<typename T1> struct equal {
    template<typename T2> bool operator()(const T1 &a, const T2 &b) const { return a == b; }
};

template<typename T> struct cache_helper {
    typedef typename remove_const<typename T::key_type>::type key_type;
    typedef std::size_t size_type;
    key_type key(const T &value) { return value.key(); }
    size_type size(const T &) { return 1; }
    size_type release(T &, size_type) { return 0; }
};

template<typename FT, typename ST> struct cache_helper<std::pair<FT, ST> > {
    typedef typename remove_const<typename std::pair<FT, ST>::first_type>::type key_type;
    typedef std::size_t size_type;
    key_type key(const std::pair<FT, ST> &value) { return value.first; }
    size_type size(const std::pair<FT, ST> &) { return 1; }
    size_type release(std::pair<FT, ST> &, size_type) { return 0; }
};

template<typename T> GX_CONSTEXPR std::size_t size(const T &c) noexcept { return c.size(); }
template<class T, std::size_t n> GX_CONSTEXPR std::size_t size(const T (&)[n]) noexcept {
    return n;
}

template<typename T, std::size_t n> GX_CONSTEXPR T *begin(T (&a)[n]) noexcept { return a; }
template<typename T> GX_CONSTEXPR typename T::iterator begin(T &c) { return c.begin(); }
template<typename T> GX_CONSTEXPR typename T::const_iterator begin(const T &c) { return c.begin(); }
template<typename T> GX_CONSTEXPR typename T::const_iterator cbegin(const T &c) {
    return c.begin();
}

template<typename T, std::size_t n> GX_CONSTEXPR T *end(T (&a)[n]) noexcept { return a + n; }
template<typename T> GX_CONSTEXPR typename T::iterator end(T &c) { return c.end(); }
template<typename T> GX_CONSTEXPR typename T::const_iterator end(const T &c) { return c.end(); }
template<typename T> GX_CONSTEXPR typename T::const_iterator cend(const T &c) { return c.end(); }

#ifndef GX_COPMATIBLE_CXX_98
template<typename T> typename remove_reference<T>::type &&move(T &&x) noexcept {
    return static_cast<typename remove_reference<T>::type &&>(x);
}
#else
template<typename T> typename T &move(T &x) { return x; }
#endif

template<class Iter, class T, class Compare>
static Iter lower_bound(Iter first, Iter last, T &value, Compare comp) {
    typedef typename std::iterator_traits<Iter>::difference_type Distance;
    Distance len = std::distance(first, last);
    while (len > 0) {
        Distance half = len >> 1;
        Iter middle = first;
        std::advance(middle, half);
        if (comp(*middle, value)) {
            first = middle;
            ++first;
            len = len - half - 1;
        } else
            len = half;
    }
    return first;
}

#ifndef GX_COPMATIBLE_CXX_98
template<typename InputIt, typename ForwardIt>
static ForwardIt uninitialized_move(InputIt first, InputIt last, ForwardIt dest) {
    using Value = typename std::iterator_traits<ForwardIt>::value_type;
    ForwardIt current = dest;
    for (; first != last; ++first, ++current)
        new (current) Value(std::move(*first));
    return current;
}
#endif

template<typename T, typename MT> GX_CONSTEXPR std::ptrdiff_t offset_of(MT T::*member) {
    return reinterpret_cast<std::ptrdiff_t>(&(reinterpret_cast<T *>(0)->*member)); // NOTE: UB!
}
} // namespace utils
// NOLINTEND(readability-*)
} // namespace gx
