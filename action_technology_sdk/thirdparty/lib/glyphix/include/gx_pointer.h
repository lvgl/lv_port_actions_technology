/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_object.h"

namespace gx {
namespace internal {
class PointerBase {
public:
    PointerBase(const PointerBase &p);
    ~PointerBase();

    void reset();
    void swap(PointerBase &p) noexcept { std::swap(m_ref, p.m_ref); }
    explicit operator bool() const { return m_ref && m_ref->ptr; }
    bool operator==(const PointerBase &rhs) const { return ptr() == rhs.ptr(); }
    bool operator!=(const PointerBase &rhs) const { return !(*this == rhs); }
    bool operator<(const PointerBase &rhs) const { return ptr() < rhs.ptr(); }
    bool operator<=(const PointerBase &rhs) const { return !(rhs < *this); }
    bool operator>(const PointerBase &rhs) const { return rhs < *this; }
    bool operator>=(const PointerBase &rhs) const { return !(*this < rhs); }

    // It is only used in some occasions that need to optimize memory access, please refer to the
    // documentation for usage.
    PrimitiveObject::GuardRef *moveRef();
    explicit PointerBase(PrimitiveObject::GuardRef *ref) : m_ref(ref) {}

protected:
    PointerBase();
    explicit PointerBase(PrimitiveObject *p);
    GX_NODISCARD PrimitiveObject *ptr() const { return m_ref ? m_ref->ptr : nullptr; }
    void assign(const PointerBase &ptr);

private:
    PrimitiveObject::GuardRef *m_ref;
};

inline PrimitiveObject::GuardRef *PointerBase::moveRef() {
    PrimitiveObject::GuardRef *ref = m_ref;
    m_ref = nullptr;
    return ref;
}

class SharedPointerBase {
public:
    SharedPointerBase(const SharedPointerBase &rhs);
#ifndef GX_COPMATIBLE_CXX_98
    SharedPointerBase(SharedPointerBase &&rhs) noexcept {
        d = rhs.d;
        rhs.d = nullptr;
    }
#endif
    ~SharedPointerBase();
    void syncReset();

protected:
    SharedPointerBase() : d() {}
    explicit SharedPointerBase(void *p);
    GX_NODISCARD void *get() const;
    void assign(const SharedPointerBase &rhs);

private:
    struct Private;
    Private *d;
};
} // namespace internal

template<class T> class Pointer : public internal::PointerBase {
public:
    Pointer() {}
    explicit Pointer(T *p) : PointerBase(const_cast<typename utils::remove_const<T>::type *>(p)) {}
    explicit Pointer(PrimitiveObject::GuardRef *ref) : PointerBase(ref) {}
    Pointer(const Pointer &other) : PointerBase(other) {}

    GX_NODISCARD T *get() const { return static_cast<T *>(ptr()); }
    void assign(const Pointer &ptr) { PointerBase::assign(ptr); }
    void assign(T *ptr) { assign(Pointer(ptr)); }
    Pointer &operator=(const Pointer &rhs) {
        assign(rhs);
        return *this;
    }

    operator T *() const { return get(); }
    T &operator*() const { return *get(); }
    T *operator->() const { return get(); }
};

template<class T> class SharedPointer : public internal::SharedPointerBase {
public:
    SharedPointer() {}
    explicit SharedPointer(T *p)
        : SharedPointerBase(const_cast<typename utils::remove_const<T>::type *>(p)) {}
    SharedPointer(const SharedPointer &other) : SharedPointerBase(other) {}
#ifndef GX_COPMATIBLE_CXX_98
    SharedPointer(SharedPointer &&other) noexcept : SharedPointerBase(std::move(other)) {}
#endif

    GX_NODISCARD T *get() const { return static_cast<T *>(SharedPointerBase::get()); }

    SharedPointer &operator=(const SharedPointer &rhs) {
        assign(rhs);
        return *this;
    }

    operator T *() const { return get(); }
    T &operator*() const { return *get(); }
    T *operator->() const { return get(); }
};
} // namespace gx

template<> inline void ::std::swap(gx::internal::PointerBase &a, gx::internal::PointerBase &b) {
    a.swap(b);
}
