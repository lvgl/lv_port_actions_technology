/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "dm.h"
#include "gx_async.h"
#include "gx_framework.h"
#include "gx_jsonvalue.h"

namespace gx {
class ExchangeDataWatcher : public SharedValue {
public:
    explicit ExchangeDataWatcher() = default;
    virtual ~ExchangeDataWatcher() = default;
    virtual void notify() = 0;
    bool start();
    String &key() { return m_key; }

protected:
    struct dm_notify *m_notify{};
    struct dm_pool *m_pool{};
    String m_key{};
};

using ExchangeCb = void (*)(const JsonValue &value);
class ExchangeDataCppWatcher : public ExchangeDataWatcher {
public:
    explicit ExchangeDataCppWatcher(struct dm_pool *pool, String key, ExchangeCb callback);
    virtual ~ExchangeDataCppWatcher() override;
    virtual void notify() override;

private:
    ExchangeCb m_callback;
};

class ExchangeDataJsWatcher : public ExchangeDataWatcher {
public:
    explicit ExchangeDataJsWatcher(struct dm_pool *pool, String key);
    virtual ~ExchangeDataJsWatcher() override;
    virtual void notify() override;

    async::Signal<JsonValue> signal;
};

class ExchangeDataManager {
public:
    ExchangeDataManager(const ExchangeDataManager &) = delete;
    ExchangeDataManager &operator=(const ExchangeDataManager &) = delete;
    static ExchangeDataManager *instance() {
        static ExchangeDataManager *instance = nullptr;
        if (instance == nullptr)
            instance = new ExchangeDataManager();
        return instance;
    }
    bool set(const char *key, const JsonValue &value);
    JsonValue get(const char *key);
    bool del(const char *key);

    ExchangeDataJsWatcher *subscribe(const char *key);
    void unsubscribe(ExchangeDataJsWatcher *watcher);

    ExchangeDataCppWatcher *subscribe(const char *key, ExchangeCb callback);
    void unsubscribe(ExchangeDataCppWatcher *watcher);

private:
    bool initialization(const String &name);
    ExchangeDataManager();
    ~ExchangeDataManager();

private:
    struct dm_pool *m_pool{};
    String m_name{};
    HashMap<String, ExchangeDataJsWatcher *> m_watchers{};
};
} // namespace gx
