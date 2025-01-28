/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "intrusive/gx_cachemap.h"

namespace gx {
namespace utils {
template<class T> struct UseCount {
    int operator()(const T &) { return 1; }
};
} // namespace utils

namespace internal {
template<typename FT, typename ST> class PairVirtualCacheMapRef {
    std::pair<FT, ST> m_pair;

public:
    typedef std::pair<FT, ST> pair_type;
    PairVirtualCacheMapRef() : m_pair() {}
    explicit PairVirtualCacheMapRef(const pair_type &pair) : m_pair(pair) {}
    pair_type &operator*() { return m_pair; }
    const pair_type &operator*() const { return m_pair; }
    pair_type *operator->() { return &m_pair; }
    const pair_type *operator->() const { return &m_pair; }
    int use_count() const { return utils::UseCount<ST>()(m_pair.second); } // NOLINT(readability-*)
};
} // namespace internal

template<typename Key, typename T, typename Alloc = std::allocator<std::pair<const Key, T> > >
class CacheMap : public intrusive::CacheMap<std::pair<const Key, T>,
                                            internal::PairVirtualCacheMapRef<const Key, T>,
                                            utils::cache_helper<std::pair<const Key, T> >, Alloc> {
    typedef intrusive::CacheMap<std::pair<const Key, T>,
                                internal::PairVirtualCacheMapRef<const Key, T>,
                                utils::cache_helper<std::pair<const Key, T> >, Alloc>
        Super;

public:
    typedef Key key_type;
    typedef T mapped_type;
    typedef Alloc allocator_type;

    explicit CacheMap(typename Super::size_type capacity = 32) : Super(capacity) {}

    std::pair<typename Super::iterator, bool> put(const typename Super::value_type &value,
                                                  bool evict = true) {
        return Super::put(typename Super::refer_type(value), evict);
    }

    T &operator[](const Key &key) {
        typename Super::iterator it = Super::get(key);
        if (it == Super::end())
            it = Super::put(typename Super::refer_type(typename Super::value_type(key, T()))).first;
        return it->second;
    }
};
} // namespace gx
