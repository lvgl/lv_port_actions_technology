/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_atomic.h"
#include "gx_http_reader.h"
#include "gx_http_task.h"
#include "gx_pointer.h"
#include "gx_semaphore.h"
#include "gx_vector.h"

namespace gx {

class StreamBuff {
public:
    explicit StreamBuff(size_t size);
    ~StreamBuff();
    const SharedPointer<StreamBuff> &ref() { return m_ref; }
    void write(const Vector<uint8_t> &data);
    size_t read(uint8_t *buffer, size_t length);
    size_t unused();
    size_t size();
    void clear();
    bool await(int timeout = -1);

private:
    Mutex m_locker{};
    Semaphore m_onRead{};
    Vector<uint8_t> m_buffer;
    Atomic<bool> m_fulled{};
    size_t m_head{};
    size_t m_tail{};
    SharedPointer<StreamBuff> m_ref;
};

class HttpStream {
public:
    enum Status { Opening, Reading, Idled, Ended, Failed };
    explicit HttpStream(int retry = 5);
    ~HttpStream();
    int open(const String &url, size_t offset);
    os::off_t seek(os::off_t pos, int whence);
    os::off_t read(void *buffer, size_t length);
    os::off_t size();
    os::off_t tell();

    void taskPost();
    void taskWait();
    HttpStream::Status taskRead();
    void taskDone(HttpStream::Status status);

private:
    bool retry();

private:
    int m_retry{};
    Mutex m_locker{};
    String m_url{};
    bool m_retrying{};
    Semaphore m_taskDone{};
    HttpReader *m_reader{};
    Atomic<size_t> m_offset{};
    Atomic<size_t> m_length{};
    Atomic<Status> m_status{};
    SharedPointer<StreamBuff> m_buffer;
    SharedPointer<HttpStream> m_stream;
};

class StreamTask : public HttpTask {
public:
    explicit StreamTask(const SharedPointer<HttpStream> &stream);
    ~StreamTask() override;

protected:
    int execute() override;
    bool isDone() override;
    void abort() override;

private:
    Atomic<bool> m_done{};
    Atomic<HttpStream::Status> m_status{};
    SharedPointer<HttpStream> m_stream{};
};

} // namespace gx
