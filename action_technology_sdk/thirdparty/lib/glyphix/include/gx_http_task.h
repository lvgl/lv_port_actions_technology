/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#pragma once

#include "gx_atomic.h"
#include "gx_event.h"
#include "gx_mutex.h"
#include "gx_semaphore.h"
#include "gx_vector.h"
#include <queue>

namespace gx {

class HttpTask {
public:
    friend class HttpTaskExecutor;
    enum TaskPriority { HighPriorityTask = 0, NormalPriorityTask, LowPriorityTask, UnknownTask };

    HttpTask();
    virtual ~HttpTask();
    void append();
    void setPriority(TaskPriority priority) { m_priority = priority; }
    TaskPriority priority() const { return m_priority; }

protected:
    virtual int execute() = 0;
    virtual bool isDone() = 0;
    virtual void abort() = 0;

private:
    TaskPriority m_priority = LowPriorityTask;
};

class HttpAsyncTask {
public:
    class TaskState {
    public:
        TaskState() : m_run(false), m_done(1) {}
        void init();
        bool isRun() const { return m_run; }
        void run(bool isRun) { m_run = isRun; }
        void finish() { m_done.notify(); }
        void wait() { m_done.wait(); }

    private:
        bool m_run{};
        Semaphore m_done{};
    };

    HttpAsyncTask();
    ~HttpAsyncTask();
    void setTaskMax(std::size_t count);
    std::size_t taskMax();
    void start(int taskType, int taskId, int priority);
    bool isRun(int taskId);
    void run(int taskId, bool isRun);
    void finish(int taskId);
    void wait();

private:
    Mutex m_lock{};
    Vector<TaskState *> m_state{};
};

class HttpTaskQueue {
public:
    ~HttpTaskQueue();
    void append(HttpTask *task);
    bool remove(HttpTask *task);
    bool find(HttpTask *task);
    bool empty();
    HttpTask *fetch();
    std::size_t count();

private:
    Mutex m_lock{};
    std::deque<HttpTask *> m_deque{};
};

class HttpTaskExecutor {
public:
    HttpTaskExecutor();
    HttpTaskExecutor(const HttpTaskExecutor &) = delete;
    HttpTaskExecutor &operator=(const HttpTaskExecutor &) = delete;
    ~HttpTaskExecutor() { cleanup(); }
    static HttpTaskExecutor &getInstance() {
        static HttpTaskExecutor instance{};
        return instance;
    }
    static void process(int taskType, int taskId) {
        HttpTaskExecutor::getInstance().execute(taskType, taskId);
    }
    void append(HttpTask *task);
    void remove(HttpTask *task);

private:
    void execute(int taskType, int taskId);
    void push(HttpTask *task);
    HttpTask *pull(int taskType, int taskId);
    void cleanup();

private:
    Mutex m_lock{};
    Atomic<bool> m_quiting{};
    HttpAsyncTask m_asyncTask[HttpTask::UnknownTask];
    HttpTaskQueue m_readyTask[HttpTask::UnknownTask];
};

} // namespace gx