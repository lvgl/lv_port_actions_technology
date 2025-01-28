/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_sharedref.h"
#include "gx_string.h"
#include "gx_style.h"

namespace gx {
class Style;
class StyleSet;

class Styles {
public:
    enum StateType { Normal = 0, Active, Disabled, Focus, Selected, UnknownState };
    enum PartType { Content = 0, Range, Bar, UnknownPart };

    Styles();
    ~Styles();

    GX_NODISCARD bool isEmpty() const;
    GX_NODISCARD int size() const;
    GX_NODISCARD const Style &get(PartType part = Content, StateType state = Normal) const;
    GX_NODISCARD static PartType part(const String &part);
    GX_NODISCARD static StateType state(const String &state);
    GX_NODISCARD static String partToString(PartType part);
    GX_NODISCARD static String stateToString(StateType state);
    GX_NODISCARD static std::pair<PartType, StateType>
    partState(const String &key = "content.normal");
    void set(const Style &style, PartType part = Content, StateType state = Normal);

private:
    Style &insertStyle(int key);

private:
    class Data;
    GX_SHARED_HELPER_DECL(Helper, Data);
    SharedRef<Data, Helper> d;
};

//! The style set of widget status.
class StyleSet : public NonCopyable<StyleSet> {
public:
    typedef Style *iterator;             // NOLINT(readability-*)
    typedef const Style *const_iterator; // NOLINT(readability-*)

    StyleSet();
    ~StyleSet();

    GX_NODISCARD bool isEmpty() const { return !d || d->isEmpty(); }
    GX_NODISCARD int size() const { return d ? d->size : 0; }
    GX_NODISCARD Style *get(int key);
    GX_NODISCARD const Style *get(int key) const;
    GX_NODISCARD Style &operator[](int key);
    bool remove(int key);

    GX_NODISCARD int bitmap() const { return d ? d->bitmap : 0; }
    GX_NODISCARD iterator begin() { return d ? d->begin() : nullptr; }
    GX_NODISCARD iterator end() { return d ? d->end() : nullptr; }
    GX_NODISCARD const_iterator begin() const { return const_cast<StyleSet *>(this)->begin(); }
    GX_NODISCARD const_iterator end() const { return const_cast<StyleSet *>(this)->end(); }

    GX_NODISCARD static int encodeKey(Styles::PartType part, Styles::StateType state);
    GX_NODISCARD static Styles::PartType decodePart(int key);
    GX_NODISCARD static Styles::StateType decodeState(int key);

private:
    struct Buckets {
        uint16_t bitmap;
        uint8_t size;
        uint8_t capacity;

        ~Buckets();
        bool isEmpty() const { return !size; }
        iterator begin() {
            if (sizeof(Buckets) != sizeof(intptr_t)) { // handle address alignment when needed
                uintptr_t data = reinterpret_cast<uintptr_t>(this + 1);
                return reinterpret_cast<iterator>((data + sizeof(intptr_t) - 1) &
                                                  ~(sizeof(intptr_t) - 1));
            }
            return reinterpret_cast<iterator>(this + 1);
        }
        iterator end() { return begin() + size; }
        iterator lowerBound(int key);
        bool contains(int key) const;
        bool needGrow(int key) const;
        Style *insert(Buckets *old, int key, bool move = true);
        template<typename T = Buckets> static T *create(unsigned bitmap, int size, int capacity);
        template<typename T = Buckets> static T *create(int key);
        template<typename T> static void destroy(T *ptr);
    };
    Style *insertBuckets(int key);

    Buckets *d;
    friend class Styles;
};

inline const Style *StyleSet::get(int key) const { return const_cast<StyleSet *>(this)->get(key); }

inline int StyleSet::encodeKey(Styles::PartType part, Styles::StateType state) {
    return int(part) * Styles::UnknownState + int(state);
}

inline Styles::PartType StyleSet::decodePart(int key) {
    return Styles::PartType(key / Styles::UnknownState);
}

inline Styles::StateType StyleSet::decodeState(int key) {
    return Styles::StateType(key % Styles::UnknownState);
}
} // namespace gx
