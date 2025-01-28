/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_metadefs.h"
#include "gx_string.h"
#include "gx_utils.h"
#include "gx_vector.h"

namespace gx {
namespace internal {
class PointerBase;
} // namespace internal

class Event;
class Object;
class Method;
class Property;
class MetaObject;

template<> class Vector<Object> {}; // TODO remove this on stable version

class ObjectList : std::allocator<Object *> {
public:
    typedef Object *value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef std::allocator<value_type> allocator_type;
    typedef allocator_type::size_type size_type;
    typedef allocator_type::difference_type difference_type;
    typedef pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    ObjectList() : d() {}
    ~ObjectList() { deallocate(); }
    GX_DEPRECATED_MSG("use the move constructor or pass by reference instead")
    ObjectList(const ObjectList &other) GX_DELETE;
#ifndef GX_COPMATIBLE_CXX_98
    ObjectList(ObjectList &&vec) noexcept : d(vec.d) { vec.d.first = vec.d.second = nullptr; }
#endif
    ObjectList &operator=(const ObjectList &vec) GX_DELETE;

    GX_NODISCARD bool empty() const { return GX_LIKELY(!d.first) || (d.mask & 0xffff0001u) == 1; }
    GX_NODISCARD size_type size() const { return d.mask & 1 ? d.size : localSize(); }
    GX_NODISCARD size_type capacity() const { return d.mask & 1 ? d.capacity & ~1u : 2; }
    GX_NODISCARD iterator begin() { return d.mask & 1 ? d.first : localBuffer(); }
    GX_NODISCARD iterator end() {
        return d.mask & 1 ? d.first + d.size : localBuffer() + localSize();
    }
    GX_NODISCARD const_iterator begin() const { return const_cast<ObjectList *>(this)->begin(); }
    GX_NODISCARD const_iterator end() const { return const_cast<ObjectList *>(this)->end(); }
    GX_NODISCARD const_iterator cbegin() const { return begin(); }
    GX_NODISCARD const_iterator cend() const { return end(); }
    GX_NODISCARD reverse_iterator rbegin() { return reverse_iterator(end()); }
    GX_NODISCARD reverse_iterator rend() { return reverse_iterator(begin()); }
    GX_NODISCARD const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    GX_NODISCARD const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    GX_NODISCARD const_reverse_iterator crbegin() const { return rbegin(); }
    GX_NODISCARD const_reverse_iterator crend() const { return rend(); }
    GX_NODISCARD pointer data() { return begin(); }
    GX_NODISCARD const_pointer data() const { return begin(); }
    GX_NODISCARD reference front() { return *begin(); }
    GX_NODISCARD const_reference front() const { return *begin(); }
    GX_NODISCARD reference back() { return *(end() - 1); }
    GX_NODISCARD const_reference back() const { return *(end() - 1); }

    GX_NODISCARD reference operator[](size_type n) {
        return (d.mask & 1 ? d.first : localBuffer())[n];
    }
    GX_NODISCARD const_reference operator[](size_type n) const {
        return const_cast<ObjectList &>(*this)[n];
    }
    GX_NODISCARD reference at(size_type n) { return (*this)[n]; }
    GX_NODISCARD const_reference at(size_type n) const { return (*this)[n]; }

    void push_back(value_type value); // NOLINT(readability-*)
    void pop_back();                  // NOLINT(readability-*)
    iterator insert(iterator position, value_type value);
    iterator erase(iterator position);
    void clear();

private:
    size_type localSize() const { return GX_LIKELY(!d.first) ? 0 : d.second ? 2 : 1; }
    iterator localBuffer() { return reinterpret_cast<iterator>(&d); }
    void deallocate() {
        if (d.mask & 1)
            allocator_type::deallocate(d.first, d.capacity & ~1u);
    }

    struct {
        pointer first;
        union { // only work when little endian
            struct {
                uint16_t capacity;
                uint16_t size;
            };
            uintptr_t mask;
            pointer second;
        };
    } d;
};

class PrimitiveObject : public NonCopyable<PrimitiveObject> {
    GX_OBJECT

public:
    struct GuardRef;
    //! Object kinds hint flags. These flags are used in the return value of the objectKinds().
    enum ObjectKinds {
        NoneKind = 0, //!< No kind.
        //! The object is a root node, Object::objectKinds(int) supports the RootKind tag.
        RootKind = 1 << 0,
        //! The object is a leaf node, Object::objectKinds(int) supports the LeafKind tag.
        LeafKind = 1 << 1,
        //! The object needs to be deleted explicitly instead of automatically. Objects with this
        //! kind are not recursively deleted when the parent object is deleted.
        //! @see JsVM::newObject(PrimitiveObject *).
        ExplicitDeleteKind = 1 << 2,
        //! Mask for all tags.
        AllKinds = 0xff
    };

    PrimitiveObject();
    virtual ~PrimitiveObject();
    //! Get all object kind hints.
    GX_NODISCARD int objectKinds() const { return objectKinds(AllKinds); }
    /**
     * Gets the specified object kind hint. These hints can help the program manage objects, but
     * are not used in most situations. However, the derived class needs to implement this virtual
     * function correctly if necessary.
     * @param hint The kind mask of interest to the caller, e.g. `RootKind | LeafKind`.
     * @return Returns the kind flags requested by (but not limited to) \b hint. The implementation
     * of this base class just return NoneKind.
     */
    GX_NODISCARD virtual int objectKinds(int hint) const;
    Property property(const char *name);
    Property property(const String &name);
    Method method(const char *name);
    Method method(const String &name);
    template<typename T> GX_NODISCARD bool isInstance() const;
    GX_NODISCARD bool isInstance(const char *className) const;
    GX_NODISCARD bool isInstance(const MetaObject *type) const;
    virtual void deleteLater();

private:
    GuardRef *guardRef();
    uintptr_t m_guard;
    friend class internal::PointerBase;
};

class Object : public PrimitiveObject {
    GX_OBJECT

public:
    struct ShowTree;
    enum { UserObjectFlags = 1 << 0 };
    enum FindOptions { DirectChild, RecursiveChild };

    explicit Object(Object *parent = nullptr);
    virtual ~Object();

    GX_NODISCARD int flags() const { return int(d.flags); }
    GX_NODISCARD bool isWidget() const { return d.isWidget; }
    GX_NODISCARD Object *parent() const { return d.parent; }
    GX_NODISCARD const String &objectName() const { return d.name; }
    //! Get the event filter object.
    GX_NODISCARD Object *eventFilterObject() const { return d.eventFilter; }
    GX_NODISCARD const ObjectList &children() const { return d.children; }
    GX_NODISCARD ObjectList findChildren(const String &name,
                                         FindOptions options = DirectChild) const;
    template<typename T>
    GX_NODISCARD T *findChild(const String &name, FindOptions options = DirectChild) const;
    void setFlags(int flags, bool status = true);
    void setParent(Object *parent);
    void setParent(Object *parent, int position);
    void setObjectName(const String &name) { d.name = name; }
    void setEventFilter(Object *filter) { d.eventFilter = filter; }
    bool sendEvent(Event *event);
    GX_NODISCARD virtual int objectKinds(int hint) const;
    virtual void deleteLater();
    virtual bool event(Event *event);

    GX_PROPERTY(String id, set setObjectName, get objectName)

protected:
    //! Delete all child objects.
    void clearChildren();
    virtual bool eventFilter(Object *receiver, Event *event);

private:
    struct Extra;
    Object *findChild(const String &name, FindOptions options, const MetaObject *metaObject) const;
    void findChildren(ObjectList &result, const String &name, FindOptions options) const;

    struct Private {
        Object *parent;
        ObjectList children;
        String name;
        Object *eventFilter;
        uint32_t flags : 30;
        uint32_t isWidget : 1;
        Private();
    };

    Private d;
    friend class Widget;
};

struct PrimitiveObject::GuardRef {
    int refcnt;
    PrimitiveObject *ptr;
    explicit GuardRef(PrimitiveObject *p);
    GuardRef *ref();
    void deref();
};

struct Object::ShowTree {
    const Object *object;
    explicit ShowTree(const Object *object) : object(object) {}
};

template<typename T> bool PrimitiveObject::isInstance() const {
    return isInstance(&T::staticMetaObject);
}

// NOLINTNEXTLINE(readability-*)
template<typename T> GX_NODISCARD T dyn_cast(PrimitiveObject *object) { // NOLINT(readability-*)
    typedef typename utils::remove_pointer<T>::type TT;
    if (object && object->isInstance(&TT::staticMetaObject))
        return static_cast<T>(object);
    return nullptr;
}

// NOLINTNEXTLINE(readability-*)
template<typename T> GX_NODISCARD T dyn_cast(const PrimitiveObject *object) {
    typedef typename utils::remove_pointer<T>::type TT;
    if (object && object->isInstance(&TT::staticMetaObject))
        return static_cast<T>(object);
    return nullptr;
}

// NOLINTNEXTLINE(readability-*)
template<typename T> GX_NODISCARD T dyn_cast(Object *object) {
    return dyn_cast<T>(static_cast<PrimitiveObject *>(object));
}

// NOLINTNEXTLINE(readability-*)
template<typename T> GX_NODISCARD T dyn_cast(const Object *object) {
    return dyn_cast<T>(static_cast<const PrimitiveObject *>(object));
}

/**
 * @brief Static downcasts a pointer from a base type to a derived type.
 *
 * This function performs a downcast from a base class pointer to a derived class pointer
 * using `static_cast`. It assumes that the object being cast is indeed of the derived type.
 * Unlike \ref dyn_cast, which may include runtime checks and is generally used when such
 * checks are necessary, `static_down_cast` is purely a compile-time operation and does not
 * perform any runtime type verification.
 *
 * @tparam Out The derived type to cast to.
 * @tparam In The base type from which to cast.
 * @param x A value of type `In` that is to be cast to type `Out`.
 * @return The value `x` casted to type `Out`. If the cast is invalid, behavior is undefined.
 *
 * @warning Ensure that the object being cast is actually of type `Out` to avoid undefined behavior.
 * This function is intended to be used in cases where the programmer has external guarantees about
 * the type hierarchy and the nature of the objects involved.
 *
 * @see dyn_cast()
 */
template<typename Out, typename In>
GX_NODISCARD Out static_down_cast(In x) { // NOLINT(*-identifier-naming)
#ifndef GX_COPMATIBLE_CXX_98
    typedef typename utils::remove_pointer<In>::type InType;
    typedef typename utils::remove_pointer<Out>::type OutType;
    GX_STATIC_ASSERT(std::is_base_of<InType, OutType>::value, "Out should be derived from In");
#endif
    return static_cast<Out>(x);
}

inline ObjectList Object::findChildren(const String &name, FindOptions options) const {
    ObjectList result;
    findChildren(result, name, options);
    return result;
}

inline void Object::setFlags(int flags, bool status) {
    if (status)
        d.flags |= flags;
    else
        d.flags &= ~flags;
}

template<typename T> T *Object::findChild(const String &name, FindOptions options) const {
    return static_cast<T *>(findChild(name, options, &T::staticMetaObject));
}

const Logger &operator<<(const Logger &, const PrimitiveObject *);
const Logger &operator<<(const Logger &, const Object *);
const Logger &operator<<(const Logger &, const Object::ShowTree &);
} // namespace gx
