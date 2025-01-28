/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_logger.h"
#include "gx_mutex.h"
#include "gx_time.h"
#include "platform/gx_time.h"
#include <posix/pthread.h>
#include <cstdio>

#define LOG_DEBUG_PROMPT "\x1b[90m"
#define LOG_INFO_PROMPT  ""
#define LOG_WARN_PROMPT  "\x1b[33m"
#define LOG_ERROR_PROMPT "\x1b[31m"
#define LOG_END_LINE     "\x1b[0m\n"

static gx::String showTimestamp() {
    int64_t ts = gx::os::timestamp_ms();
    gx::TimeFields tm = gx::localtime(ts / 1000);
    int msec = int(ts % 1000), year = tm.year + 1900;
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.4d-%02d-%02d %02d:%02d:%02d.%.3d ", //
             year, tm.month + 1, tm.monthDay, tm.hour, tm.minutes, tm.seconds, msec);
    return gx::String(buffer);
}

class StdoutLoggerStream : public gx::LoggerStream {
public:
    struct LoggerState : State {
        int level;
        gx::String buffer;
        explicit LoggerState(int level) : level(level), buffer(256) {}
    };

    virtual State *start(int level, const char *tag) {
        auto *state = new LoggerState(level);
        const char *prompt;
        switch (level) {
        case GX_LOG_DEBUG: prompt = LOG_DEBUG_PROMPT; break;
        case GX_LOG_INFO: prompt = LOG_INFO_PROMPT; break;
        case GX_LOG_WARN: prompt = LOG_WARN_PROMPT; break;
        case GX_LOG_ERROR:
        case GX_LOG_FATAL:
        default: prompt = LOG_ERROR_PROMPT; break;
        }
        state->buffer += prompt;
        state->buffer << "gx " << showTimestamp();
        if (tag)
            state->buffer << '[' << tag << "] ";
        char th_name[32] = {0};
        pthread_getname_np(pthread_self(), th_name, sizeof(th_name));
        state->buffer << th_name << ": ";
        return state;
    }

    virtual void puts(State *state, const char *span) {
        static_cast<LoggerState *>(state)->buffer += span;
    }
    virtual State *flush(State *state, bool) {
        static_cast<LoggerState *>(state)->buffer += LOG_END_LINE;
        gx::LockGuard<gx::Mutex> locker(m_mutex);
        std::fputs(static_cast<LoggerState *>(state)->buffer.c_str(), stdout);
        std::fflush(stdout);
        if (static_cast<LoggerState *>(state)->level == GX_LOG_FATAL)
            abort();
        delete static_cast<LoggerState *>(state);
        return nullptr;
    }

private:
    gx::Mutex m_mutex;
};

static StdoutLoggerStream instance;
gx::LoggerStream *gx::LoggerStream::stream = &instance;
