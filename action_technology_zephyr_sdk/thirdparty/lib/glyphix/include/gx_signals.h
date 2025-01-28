/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_pointer.h"
#include "gx_utils.h"

namespace gx {
class SignalBase : public NonCopyable<SignalBase> {
protected:
    class Slot;
    typedef void (PrimitiveObject::*method_t)();                // NOLINT(readability-*)
    typedef void (*executor_t)(Slot *slot, void *pa, void *pb); // NOLINT(readability-*)
    SignalBase();
    ~SignalBase();
    void emit(void *p1, void *p2);
    void connect(PrimitiveObject *object, method_t method, executor_t executor);
    void disconnect(PrimitiveObject *object, method_t method);
    void move(PrimitiveObject *from, PrimitiveObject *to, method_t method);

private:
    void erase(Slot *slot);
    bool reorganize();

private:
    Slot *m_slots;
    uint16_t m_executing : 1;
    uint16_t m_capacity : 15;
    uint16_t m_first;
};

class SignalBase::Slot {
public:
    Slot(PrimitiveObject *object, method_t method, executor_t executor);
    void execute(void *pa, void *pb) { m_executor(this, pa, pb); }
    GX_NODISCARD PrimitiveObject *object() const { return m_object; }
    GX_NODISCARD method_t method() const { return m_method; }

private:
    Pointer<PrimitiveObject> m_object;
    method_t m_method;
    executor_t m_executor;
    friend class SignalBase;
};

namespace internal {
class noparam;

class Signal0 : public SignalBase {
public:
    template<typename T> void connect(T *object, void (T::*func)()) {
        typedef void (T::*Func)();
        SignalBase::method_t method = function_pointer_cast<SignalBase::method_t>(func);
        SignalBase::connect(object, method, executor<T, Func>);
    }
    template<typename T> void disconnect(T *obj, void (T::*func)()) {
        SignalBase::disconnect(obj, function_pointer_cast<SignalBase::method_t>(func));
    }
    template<typename T> void move(T *from, T *to, void (T::*func)() = nullptr) {
        SignalBase::move(from, to, function_pointer_cast<SignalBase::method_t>(func));
    }
    void operator()() { SignalBase::emit(nullptr, nullptr); }

private:
    template<typename T, typename F> static void executor(SignalBase::Slot *slot, void *, void *) {
        F fp = function_pointer_cast<F>(slot->method());
        (static_cast<T *>(slot->object())->*fp)();
    }
};

template<typename Tp1> class Signal1 : public Signal0 {
public:
    using Signal0::connect;
    using Signal0::disconnect;
    template<typename T> void connect(T *obj, void (T::*func)(Tp1)) {
        typedef void (T::*Func)(Tp1);
        SignalBase::method_t method = function_pointer_cast<SignalBase::method_t>(func);
        SignalBase::connect(obj, method, executor<T, Func>);
    }
    template<typename T> void connect(T *obj, void (T::*func)(const Tp1 &)) {
        typedef void (T::*Func)(const Tp1 &);
        SignalBase::method_t method = function_pointer_cast<SignalBase::method_t>(func);
        SignalBase::connect(obj, method, executor<T, Func>);
    }
    template<typename T> void disconnect(T *obj, void (T::*func)(Tp1)) {
        SignalBase::disconnect(obj, function_pointer_cast<SignalBase::method_t>(func));
    }
    template<typename T> void move(T *from, T *to, void (T::*func)(Tp1) = nullptr) {
        SignalBase::move(from, to, function_pointer_cast<SignalBase::method_t>(func));
    }
    void operator()(const Tp1 &p1) { SignalBase::emit((void *)&p1, nullptr); }

private:
    template<typename T, typename F> static void executor(SignalBase::Slot *slot, void *a, void *) {
        typedef const typename utils::remove_reference<Tp1>::type Tv1;
        GX_CRT_ASSERT(a != nullptr);
        F fp = function_pointer_cast<F>(slot->method());
        Tv1 &va = *reinterpret_cast<Tv1 *>(a);
        (static_cast<T *>(slot->object())->*fp)(va);
    }
};

template<typename Tp1, typename Tp2> class Signal2 : public Signal1<Tp1> {
public:
    using Signal1<Tp1>::connect;
    using Signal1<Tp1>::disconnect;
    template<typename T> void connect(T *obj, void (T::*func)(Tp1, Tp2)) {
        typedef void (T::*Func)(Tp1, Tp2);
        SignalBase::method_t method = function_pointer_cast<SignalBase::method_t>(func);
        SignalBase::connect(obj, method, executor<T, Func>);
    }
    template<typename T> void connect(T *obj, void (T::*func)(const Tp1 &, Tp2)) {
        typedef void (T::*Func)(const Tp1 &, Tp2);
        SignalBase::method_t method = function_pointer_cast<SignalBase::method_t>(func);
        SignalBase::connect(obj, method, executor<T, Func>);
    }
    template<typename T> void connect(T *obj, void (T::*func)(Tp1, const Tp2 &)) {
        typedef void (T::*Func)(Tp1, const Tp2 &);
        SignalBase::method_t method = function_pointer_cast<SignalBase::method_t>(func);
        SignalBase::connect(obj, method, executor<T, Func>);
    }
    template<typename T> void connect(T *obj, void (T::*func)(const Tp1 &, const Tp2 &)) {
        typedef void (T::*Func)(const Tp1 &, const Tp2 &);
        SignalBase::method_t method = function_pointer_cast<SignalBase::method_t>(func);
        SignalBase::connect(obj, method, executor<T, Func>);
    }
    template<typename T> void disconnect(T *obj, void (T::*func)(Tp1, Tp2)) {
        SignalBase::disconnect(obj, function_pointer_cast<SignalBase::method_t>(func));
    }
    template<typename T> void move(T *from, T *to, void (T::*func)(Tp1, Tp2) = nullptr) {
        SignalBase::move(from, to, function_pointer_cast<SignalBase::method_t>(func));
    }
    void operator()(const Tp1 &p1, const Tp2 &p2) { SignalBase::emit((void *)&p1, (void *)&p2); }

private:
    template<typename T, typename F>
    static void executor(SignalBase::Slot *slot, void *a, void *b) {
        typedef const typename utils::remove_reference<Tp1>::type Tv1;
        typedef const typename utils::remove_reference<Tp2>::type Tv2;
        GX_CRT_ASSERT(a != nullptr && b != nullptr);
        F fp = function_pointer_cast<F>(slot->method());
        Tv1 &va = *reinterpret_cast<Tv1 *>(a);
        Tv2 &vb = *reinterpret_cast<Tv2 *>(b);
        (static_cast<T *>(slot->object())->*fp)(va, vb);
    }
};
} // namespace internal

template<typename Tp1 = void, typename Tp2 = internal::noparam>
class Signal
    : public internal::Signal2<
          typename utils::remove_reference<typename utils::remove_const<Tp1>::type>::type,
          typename utils::remove_reference<typename utils::remove_const<Tp2>::type>::type> {};

template<typename Tp1>
class Signal<Tp1, internal::noparam>
    : public internal::Signal1<
          typename utils::remove_reference<typename utils::remove_const<Tp1>::type>::type> {};

template<> class Signal<void> : public internal::Signal0 {};
} // namespace gx
