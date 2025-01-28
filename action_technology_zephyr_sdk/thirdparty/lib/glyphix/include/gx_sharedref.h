/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_atomic.h"
#include "gx_utils.h"
#include <utility>

namespace gx {
namespace utils {
template<class T> struct SharedHelper {
    template<class R> static T *duplicate(const R &x) { return new T(static_cast<const T &>(x)); }
    template<class R> static void destroy(R *ptr) { delete static_cast<T *>(ptr); }
};
} // namespace utils

#define GX_SHARED_HELPER_DECL(H, T)                                                                \
    struct H {                                                                                     \
        static T *duplicate(const SharedValue &x);                                                 \
        static void destroy(SharedValue *ptr);                                                     \
    }

#define GX_SHARED_HELPER_IMPL(H, T)                                                                \
    /* NOLINTNEXTLINE(*-pro-type-static-cast-downcast, bugprone-macro-parentheses) */              \
    T *H::duplicate(const SharedValue &x) { return new T(static_cast<const T &>(x)); }             \
    /* NOLINTNEXTLINE(*-pro-type-static-cast-downcast, bugprone-macro-parentheses) */              \
    void H::destroy(SharedValue *ptr) { delete static_cast<T *>(ptr); }

namespace internal {
class SharedRefBase;
}

class SharedValue {
public:
    enum SharedType { Mutable = 0, Immutable = 1 };
    explicit SharedValue(SharedType type = Mutable) : m_refcnt(type << 31) {}
    SharedValue(const SharedValue &other);
    ~SharedValue() GX_DEFAULT;

    GX_NODISCARD bool isShared() const { return m_refcnt > 1; }
    GX_NODISCARD bool isMutable() const { return !(m_refcnt & (1 << 31)); }

protected:
    void setMutable(bool status);

private:
    // bit 31: mutable flag
    // bit [30: 0]: refcnt
    Atomic<uint32_t> m_refcnt;
    friend class internal::SharedRefBase;
};

namespace internal {
class SharedRefBase {
public:
    GX_NODISCARD bool empty() const { return m_ptr == nullptr; }
    void swap(SharedRefBase &ref);
    GX_NODISCARD int use_count() const GX_NOEXCEPT { // NOLINT(readability-*)
        return int(m_ptr->m_refcnt & ~(1 << 31));
    }

    operator bool() const { return !empty(); }
    bool operator==(const SharedRefBase &ref) const { return m_ptr == ref.m_ptr; }
    bool operator!=(const SharedRefBase &ref) const { return m_ptr != ref.m_ptr; }
    bool operator<(const SharedRefBase &ref) const { return m_ptr < ref.m_ptr; }
    bool operator<=(const SharedRefBase &ref) const { return m_ptr <= ref.m_ptr; }
    bool operator>(const SharedRefBase &ref) const { return m_ptr > ref.m_ptr; }
    bool operator>=(const SharedRefBase &ref) const { return m_ptr >= ref.m_ptr; }

protected:
    explicit SharedRefBase(SharedValue *ptr = nullptr);
    SharedRefBase(const SharedRefBase &other);
#ifndef GX_COPMATIBLE_CXX_98
    SharedRefBase(SharedRefBase &&ref) noexcept : m_ptr(ref.m_ptr) { ref.m_ptr = nullptr; }
#endif

    GX_NODISCARD SharedValue *get() const { return m_ptr; }
    void ref() GX_NOEXCEPT { ++m_ptr->m_refcnt; }
    bool deref() GX_NOEXCEPT { return --m_ptr->m_refcnt & ~(1 << 31); }

    template<class H> void assign(const SharedRefBase &other) {
        if (m_ptr == other.m_ptr)
            return;
        if (m_ptr && !deref())
            H().destroy(m_ptr);
        m_ptr = other.m_ptr;
        if (m_ptr)
            ref();
    }
#ifndef GX_COPMATIBLE_CXX_98
    template<class H> void assign(SharedRefBase &&other) noexcept {
        if (m_ptr == other.m_ptr)
            return;
        if (m_ptr && !deref())
            H().destroy(m_ptr);
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
    }
#endif
    template<class H> void reset() {
        if (m_ptr && !deref())
            H().destroy(m_ptr);
        m_ptr = nullptr;
    }
    template<class H> bool detach() {
        if (m_ptr && m_ptr->isShared()) {
            SharedValue *ptr = H().duplicate(*m_ptr);
            deref();
            if ((m_ptr = ptr))
                ref();
        }
        return m_ptr;
    }

private:
    SharedValue *m_ptr;
};
} // namespace internal

template<class T, class H = utils::SharedHelper<T> >
class SharedRef : public internal::SharedRefBase {
public:
    typedef T type;
    typedef T *pointer;
    typedef T &reference;

    SharedRef() GX_DEFAULT;
    explicit SharedRef(T *ptr) : SharedRefBase(static_cast<SharedValue *>(ptr)) {}
    SharedRef(const SharedRef &other) : SharedRefBase(other) {}
#ifndef GX_COPMATIBLE_CXX_98
    SharedRef(SharedRef &&ref) noexcept : SharedRefBase(std::move(ref)) {}
#endif
    ~SharedRef() {
        if (SharedRefBase::get() && !deref())
            H().destroy(SharedRefBase::get());
    }
    void reset() { SharedRefBase::reset<H>(); }
    void assign(const SharedRef &other) { SharedRefBase::assign<H>(other); }

#ifndef GX_COPMATIBLE_CXX_98
    void assign(SharedRef &&other) noexcept { SharedRefBase::assign<H>(other); }
#endif

    GX_NODISCARD pointer get() { return static_cast<pointer>(SharedRefBase::get()); }
    GX_NODISCARD pointer get() const { return static_cast<pointer>(SharedRefBase::get()); }
    bool detach() { return SharedRefBase::detach<H>(); }
    pointer operator->() const { return get(); }
    reference operator*() const { return *operator->(); }
    SharedRef &operator=(const SharedRef &rhs) {
        assign(rhs);
        return *this;
    }
#ifndef GX_COPMATIBLE_CXX_98
    SharedRef &operator=(SharedRef &&rhs) noexcept {
        assign(rhs);
        return *this;
    }
#endif
};

// When copying, the reference count will be cleared, and the copied object must be mutable.
inline SharedValue::SharedValue(const SharedValue &) : m_refcnt() {}

template<typename T> struct utils::hash<SharedRef<T> > {
    std::size_t operator()(const SharedRef<T> &v) { return hash<T *>()(v.get()); }
};
} // namespace gx
