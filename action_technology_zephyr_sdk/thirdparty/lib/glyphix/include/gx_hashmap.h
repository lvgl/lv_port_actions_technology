/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "intrusive/gx_hashmap.h"

namespace gx {
namespace internal {
template<typename T, typename Hash> struct PairNodeHash {
    std::size_t operator()(const T &v) const { return Hash()(v.first); }
    template<class K> std::size_t operator()(const K &v) const { return Hash()(v); }
};

template<typename T1, typename Equal> struct PairNodeEqual {
    bool operator()(const T1 &a, const T1 &b) const { return Equal()(a.first, b.first); }
    template<typename T2> bool operator()(const T1 &a, const T2 &b) const {
        return Equal()(a.first, b);
    }
};

template<typename Key, typename T, typename Hash, typename Equal>
class HashMapNode
    : public intrusive::HashNode<std::pair<Key, T>, internal::PairNodeHash<std::pair<Key, T>, Hash>,
                                 internal::PairNodeEqual<std::pair<Key, T>, Equal> > {
    typedef intrusive::HashNode<std::pair<Key, T>, internal::PairNodeHash<std::pair<Key, T>, Hash>,
                                internal::PairNodeEqual<std::pair<Key, T>, Equal> >
        Super;

public:
    explicit HashMapNode(const typename Super::value_type &data) : Super(data) {}
};
} // namespace internal

template<typename Key, typename T, typename Hash = utils::hash<Key>,
         typename Equal = utils::equal<Key>,
         typename Alloc = std::allocator<std::pair<const Key, T> > >
class HashMap
    : public intrusive::HashMap<std::pair<const Key, T>,
                                internal::HashMapNode<const Key, T, Hash, Equal>,
                                typename Alloc::template rebind<
                                    internal::HashMapNode<const Key, T, Hash, Equal> >::other> {
public:
    typedef Key key_type;
    typedef T mapped_type;        // NOLINT(readability-*)
    typedef Hash hasher;          // NOLINT(readability-*)
    typedef Equal key_equal;      // NOLINT(readability-*)
    typedef Alloc allocator_type; // NOLINT(readability-*)

    GX_NODISCARD T &at(const Key &key) { return HashMap::find(key)->second; }
    GX_NODISCARD const T &at(const Key &key) const { return HashMap::find(key)->second; }

    T &operator[](const Key &key) {
        typename HashMap::iterator it = HashMap::find(key);
        if (it == HashMap::end())
            it = HashMap::insert(typename HashMap::value_type(key, T())).first;
        return it->second;
    }
};
} // namespace gx
