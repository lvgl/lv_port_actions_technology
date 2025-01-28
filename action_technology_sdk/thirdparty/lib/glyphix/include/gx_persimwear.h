/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_mutex.h"
#include "gx_string.h"
#include "gx_framework.h"

namespace gx {
class PersimWear {
public:
    PersimWear();
    ~PersimWear();
    template<class T> T *installService();
    template<class T> T *service();
    Mutex &mutex();

    static PersimWear *instance() { return m_instance; }

private:
    struct ServiceWrapper {
        virtual ~ServiceWrapper() GX_DEFAULT;
        virtual void *service() = 0;
    };
    typedef void *(*serviceid_t)(); // NOLINT(*-identifier-naming)

    ServiceWrapper *installService(serviceid_t id, ServiceWrapper *wrapper);
    void *service(serviceid_t id);
    template<class T> static void *serviceId();

    struct Private;
    Private *d;

    static PersimWear *m_instance;
};

template<class T> T *PersimWear::installService() {
    struct Wrapper : ServiceWrapper {
        T x;
        virtual void *service() { return &x; }
    };
    return static_cast<T *>(installService(&serviceId<T>, new Wrapper)->service());
}

template<class T> T *PersimWear::service() { return static_cast<T *>(service(&serviceId<T>)); }

template<class T> void *PersimWear::serviceId() {
    return function_pointer_cast<void *>(&serviceId<T>);
}

template<typename T> T *service() { return PersimWear::instance()->service<T>(); }
} // namespace gx
