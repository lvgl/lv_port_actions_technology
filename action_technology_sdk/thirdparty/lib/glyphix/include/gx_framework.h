/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_jsvm.h"

namespace gx {
ByteArray serializedJsValue(const JsValue &value);
JsValue readJsValue(const uint8_t *first, const uint8_t *last);
JsValue readJsValue(const ByteArray &data);

inline JsValue readJsValue(const ByteArray &data) { return readJsValue(data.begin(), data.end()); }

template<typename T> class UniquePtr {
public:
    explicit UniquePtr(T *ptr) : m_ptr(ptr) {}
    ~UniquePtr() { delete m_ptr; }
    T *get() { return m_ptr; }
    const T *get() const { return m_ptr; }
    T &operator*() { return *get(); }
    const T &operator*() const { return *get(); }
    T *operator->() { return get(); }
    const T *operator->() const { return get(); }

private:
    UniquePtr(const UniquePtr &);
    UniquePtr &operator=(const UniquePtr &);

    T *m_ptr;
};
} // namespace gx
