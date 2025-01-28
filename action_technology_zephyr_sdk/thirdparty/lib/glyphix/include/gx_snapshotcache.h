/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_pixmap.h"
#include "gx_widget.h"
#include "intrusive/gx_cachemap.h"

namespace gx {
class SnapshotCache {
public:
    SnapshotCache();
    GX_NODISCARD std::size_t size() const { return m_cache.size(); }
    GX_NODISCARD std::size_t capacity() const { return m_cache.capacity(); }
    void setCapacity(std::size_t capacity) { return m_cache.setCapacity(capacity); }
    bool flush(const Widget *widget);
    std::size_t flush(std::size_t size);
    GX_NODISCARD bool contains(const Widget *widget) const;
    GX_NODISCARD const Pixmap &lookup(Widget *widget);
    GX_NODISCARD int isNesting() const { return m_nesting; }

private:
    struct Node : SharedValue {
        explicit Node(Widget *widget);
        virtual ~Node();
        Widget *const widget;
        Pixmap pixmap;
    };
    struct Helper {
        typedef const Widget *key_type;
        typedef std::size_t size_type;
        static key_type key(const Node &node);
        static size_type size(const Node &node);
        static size_type release(Node &, size_type);
    };
    typedef intrusive::CacheMap<Node, SharedRef<Node>, Helper> CacheMap;
    CacheMap m_cache;
    uint8_t m_nesting;
};
} // namespace gx
