/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#pragma once

#include "gx_atomic.h"
#include "gx_http_reader.h"
#include "gx_http_task.h"
#include "gx_mutex.h"
#include "gx_semaphore.h"
#include "gx_string.h"

namespace gx {

class FetchTask : public HttpTask {
public:
    struct Response {
        int error{};
        int status{};
        String content{};
        Vector<std::pair<String, String> > headers{};
        explicit Response(int error = 0) : error(error), status() {}
        Response(int status, String content)
            : error(), status(status), content(std::move(content)) {}
    };
    typedef void (*Complete)(void *user, Response &response);
    explicit FetchTask(const String &url, HttpMethod method, const String &body,
                       HashMap<String, String> &headers, HttpReader::Progress *progress,
                       Complete complete, void *session, int timeout, int retry = 5);
    ~FetchTask() override;

protected:
    int execute() override;
    bool isDone() override;
    void abort() override;

private:
    int fetch(size_t offset);
    int retry(size_t offset);
    int complete(int result);

private:
    Mutex m_lock{};
    Atomic<bool> m_done{};
    Atomic<bool> m_inited{};
    String m_url{};
    HttpMethod m_method{};
    String m_body{};
    HashMap<String, String> m_headers{};
    int m_timeout{};
    int m_retry{};
    HttpReader *m_reader{};
    HttpReader::Progress *m_progress{};
    Complete m_complete{};
    size_t m_offset{};
    size_t m_length{};
    void *m_session{};
};

} // namespace gx