/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "intrusive/gx_cachemap.h"

namespace gx {
class TextLayout;
class Widget;
class Style;

class TextLayoutCache {
    struct Node;
    struct NodeHelper {
        struct key_type { // NOLINT(*-identifier-naming)
            const Widget *widget;
            uint16_t textId;
            uint16_t refcnt;
            key_type(const Widget *widget, uint16_t textId)
                : widget(widget), textId(textId), refcnt() {}
            bool operator==(const key_type &rhs) const {
                return widget == rhs.widget && textId == rhs.textId;
            }
        };
        typedef std::size_t size_type;
        static const key_type &key(const Node &value);
        static size_type size(const Node &);
        static size_type release(Node &, size_type) { return 0; }
    };

    class NodeRef {
        Node *m_node;

    public:
        explicit NodeRef(Node *node);
        NodeRef(const NodeRef &ref);
        ~NodeRef();
        GX_NODISCARD int use_count() const; // NOLINT(*-identifier-naming)
        GX_NODISCARD Node *get() const { return m_node; }
        operator bool() const { return get(); }
        Node &operator*() const { return *get(); }
        Node *operator->() const { return get(); }

    private:
        NodeRef &operator=(const NodeRef &);
    };

    typedef intrusive::CacheMap<Node, NodeRef, NodeHelper> CacheMap;

public:
    typedef CacheMap::iterator iterator;

    TextLayoutCache();
    GX_NODISCARD std::size_t size() const { return m_cache.size(); }
    GX_NODISCARD std::size_t capacity() const { return m_cache.capacity(); }
    void setCapacity(std::size_t limit);
    GX_NODISCARD std::pair<TextLayout *, bool> get(const Widget *widget, int textId = 0);
    GX_NODISCARD iterator find(const Widget *widget, int textId = 0);
    GX_NODISCARD iterator end() const { return m_cache.end(); }
    void clear();
    bool flush(iterator position);
    bool flush(const Widget *widget, int textId = 0);

    static TextLayout *layout(iterator it);

private:
    CacheMap m_cache;

    friend struct utils::hash<NodeHelper::key_type>;
};
} // namespace gx
