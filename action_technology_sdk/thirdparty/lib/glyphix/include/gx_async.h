/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_bindableobject.h"
#include "gx_coreapplication.h"
#include "gx_event.h"
#include "gx_jsvm.h"

namespace gx {
class Applet;

class AsyncSession : public BindableObject {
    GX_OBJECT

public:
    /**
     * Consumes and deletes the AsyncSession.
     * @tparam T The consumer type.
     */
    template<class T> struct Consume {
        T value;
        explicit Consume(const T &value) : value(value) {}
        void operator()(AsyncSession *session) {
            value(session);
            delete session;
        }
    };
    struct Terminate {
        void operator()(AsyncSession *session) { delete session; }
    };

    GX_NODISCARD JsValue promise() const { return resolver().isPromise() ? resolver() : JsValue(); }
    GX_NODISCARD const JsValue &resolver() const { return m_resolver; }
    virtual bool close() { return false; }
    void terminate() { resolve(Terminate()); }
    void abandon() { m_resolver = JsValue(); }
    template<class T> void resolve(const T &message) {
        CoreApp()->postEvent(new Message<T>(this, message));
    }
    GX_NODISCARD virtual int objectKinds(int hint) const;

protected:
    AsyncSession() GX_DEFAULT;
    virtual ~AsyncSession();
    virtual void unbind() GX_FINAL;
    void setResolver(const JsValue &object) { m_resolver = object; }

private:
    template<class T> struct Message : TaskEvent {
        T message;
        AsyncSession *request;
        Message(AsyncSession *req, const T &msg) : message(msg), request(req) {}
        virtual void exec() { message(request); }
    };

    JsValue m_resolver;
};

namespace async {
enum StatusEnums { OK = 0, Error = 1 };

class Status {
public:
    explicit Status(int status = OK) : m_status(status) {}
    GX_NODISCARD bool isOK() const { return m_status == OK; }
    GX_NODISCARD int value() const { return m_status; }

    bool operator==(int x) const { return m_status == x; }
    bool operator==(Status x) const { return m_status == x.m_status; }
    bool operator!=(int x) const { return !(*this == x); }
    bool operator!=(Status x) const { return !(*this == x); }

private:
    int m_status;
};

struct ResultHandler {
    static JsValue setup(const JsValue &object);
    static void success(const JsValue &object, const JsValue &result);
    static void fail(const JsValue &object, const JsValue &message, int code);
    static void postJob(Event *job) { CoreApp()->postAsyncEvent(job); }
};

struct ListenHandler {
    static void callback(const JsValue &object, const JsValue &message);
    static void fail(const JsValue &object, const JsValue &message, int code);
};

template<class T = void> struct Result {
    typedef T value_type;
    Result(Status status = Status()) : m_status(status), m_value() {}
    Result(const T &value, Status status = Status()) : m_status(status), m_value(value) {}
    GX_NODISCARD bool isValue() const { return m_status.isOK(); }
    GX_NODISCARD bool isError() const { return !m_status.isOK(); }
    GX_NODISCARD Status error() const { return m_status; }
    GX_NODISCARD const T &value() const { return m_value; }
    GX_NODISCARD JsValue toJsValue() const { return js_cast(value()); }

private:
    Status m_status;
    T m_value;
};

template<> struct Result<void> {
    typedef void value_type;
    Result(Status status = Status()) : m_status(status) {}
    GX_NODISCARD bool isValue() const { return m_status.isOK(); }
    GX_NODISCARD bool isError() const { return !m_status.isOK(); }
    GX_NODISCARD Status error() const { return m_status; }
    GX_NODISCARD JsValue toJsValue() const { return JsValue(); }

private:
    Status m_status;
};

template<typename T> struct ErrorMessage {
private:
    template<typename TT> struct TestCStr {
        template<typename U, const char *(*)(Status) = &U::errorMessage> struct Check {};
        template<typename U> static char test(Check<U> *);
        template<typename U> static int test(...);
        // NOLINTNEXTLINE(*-identifier-naming)
        static const bool value = sizeof(test<TT>(nullptr)) == sizeof(char);
    };
    template<typename TT> struct TestString {
        template<typename U, String (*)(Status) = &U::errorMessage> struct Check {};
        template<typename U> static char test(Check<U> *);
        template<typename U> static int test(...);
        // NOLINTNEXTLINE(*-identifier-naming)
        static const bool value = sizeof(test<TT>(nullptr)) == sizeof(char);
    };

public:
    template<class R, class U = T>
    typename utils::enable_if<!TestCStr<U>::value && !TestString<U>::value, const char *>::type
    operator()(const Result<R> &) const {
        return "unknown async error";
    }
    template<class R, class U = T>
    typename utils::enable_if<TestCStr<U>::value, const char *>::type
    operator()(const Result<R> &result) const {
        return T::errorMessage(result.error());
    }
    template<class R, class U = T>
    typename utils::enable_if<TestString<U>::value, String>::type //
    operator()(const Result<R> &result) const {
        return T::errorMessage(result.error());
    }
};

template<class T, class H = ResultHandler> class ResultSession : public AsyncSession {
public:
    ResultSession() GX_DEFAULT;
    virtual ~ResultSession() GX_DEFAULT;

    GX_NODISCARD T &client() { return m_client; }
    GX_NODISCARD const T &client() const { return m_client; }
    virtual bool close() {
        abandon();
        return false;
    }
    void setResolver(const JsValue &object) { AsyncSession::setResolver(H().setup(object)); }
    template<class R> void resolve(const Result<R> &result) {
        typedef Return<Result<R> > Ret;
        AsyncSession::resolve(Consume<Ret>(Ret(result)));
    }
    void request(const JsValue &resolver) {
        setResolver(resolver);
        H().postJob(new Resolver(this));
    }
    GX_NODISCARD static ResultSession *session(const T *client) {
        return reinterpret_cast<ResultSession *>(reinterpret_cast<uintptr_t>(client) -
                                                 utils::offset_of(&ResultSession::m_client));
    }

private:
    template<class R> struct Return {
        R ret;
        explicit Return(const R &ret) : ret(ret) {}
        void operator()(AsyncSession *session) {
            const JsValue &resolver = session->resolver();
            if (ret.isValue())
                H().success(resolver, ret.toJsValue());
            else
                H().fail(resolver, ErrorMessage<T>()(ret), ret.error().value());
        }
    };
    struct Resolver : TaskEvent {
        ResultSession *session;
        explicit Resolver(ResultSession *request) : session(request) {}
        template<class R> void resolve(const Result<R> &ret) { session->resolve(ret); }
        virtual void exec() { resolve(session->client().resolve()); }
    };

private:
    T m_client;
};

template<class T, class H = ListenHandler> class ListenSession : public AsyncSession {
public:
    ListenSession() : closed() {}
    virtual ~ListenSession() GX_DEFAULT;

    GX_NODISCARD T &client() { return m_client; }
    GX_NODISCARD const T &client() const { return m_client; }
    GX_NODISCARD bool isClosed() const { return closed; }
    virtual bool close() {
        if (closed)
            return false;
        closed = true;
        m_client.close();
        abandon();
        terminate();
        return true;
    }
    using AsyncSession::setResolver;
    template<class R> void emit(const R &message) { this->resolve(Emit<R>(message)); }
    GX_NODISCARD static ListenSession *session(const T *client) {
        return reinterpret_cast<ListenSession *>(reinterpret_cast<uintptr_t>(client) -
                                                 utils::offset_of(&ListenSession::m_client));
    }

protected:
    bool closed;

private:
    template<class R> struct Emit {
        Result<R> ret;
        explicit Emit(const R &ret) : ret(ret) {}
        void operator()(AsyncSession *session) {
            if (!static_cast<ListenSession *>(session)->isClosed()) {
                const JsValue &resolver = session->resolver();
                if (ret.isValue())
                    H().callback(resolver, ret.toJsValue());
                else
                    H().fail(resolver, ErrorMessage<T>()(ret), ret.error().value());
            }
        }
    };

private:
    T m_client;
};

template<class S, class Applet> static S *make(Applet *applet) {
    S *session = new S;
    applet->bindObject(session);
    return session;
}

class Slot;

namespace detail {
class SignalBase {
public:
    SignalBase() : m_pointer(-1) {}
    ~SignalBase();
    Slot *connect(const JsValue &resolver);
    void disconnect(int id);
    void disconnect(Slot *slot);

protected:
    void notify(const JsValue &value);

private:
    Vector<Slot *> m_pool;
    int m_pointer;
    friend class async::Slot;
};
} // namespace detail

class Slot : public PrimitiveObject {
    GX_OBJECT

public:
    GX_NODISCARD int id() const { return m_id; }
    virtual ~Slot();
    GX_NODISCARD virtual int objectKinds(int hint) const;

private:
    // NOLINTNEXTLINE
    Slot(detail::SignalBase *signal, const JsValue &resolver, int id)
        : m_signal(signal), m_resolver(resolver), m_id(id) {}

    detail::SignalBase *m_signal;
    JsValue m_resolver;
    int m_id;
    friend class detail::SignalBase;
};

template<class T> class Signal : public detail::SignalBase {
public:
    void operator()(const T &value) { CoreApp()->postEvent(new Emit(this, value)); }

private:
    struct Emit : TaskEvent {
        Signal *signal;
        T value;
        Emit(Signal *signal, const T &value) : signal(signal), value(value) {}
        virtual void exec() { signal->notify(js_cast(value)); }
    };
};

inline void detail::SignalBase::disconnect(Slot *slot) {
    if (slot)
        disconnect(slot->id());
}
} // namespace async
} // namespace gx
