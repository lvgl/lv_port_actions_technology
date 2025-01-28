/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_file.h"
#include "gx_loggerstream.h"
#include "gx_mutex.h"

namespace gx {
class FileLoggerStream : public LoggerStream {
public:
    FileLoggerStream() GX_DEFAULT;
    explicit FileLoggerStream(const String &path);
    bool isValid() const { return m_outfile.isOpened(); }
    virtual State *start(int level, const char *tag);
    virtual void puts(State *state, const char *span);
    virtual State *flush(State *state, bool);

private:
    struct LoggerState : State {
        int level;
        String buffer;
        explicit LoggerState(int level) : level(level), buffer(128) {}
    };
    File m_outfile;
};

class RotateFileLoggerStream : public LoggerStream {
public:
    struct LoggerState;

    explicit RotateFileLoggerStream(const String &path, std::size_t capacity = 8 * 1024 * 1024,
                                    int fileCount = 4);
    ~RotateFileLoggerStream() override;
    void setFeature(Features features, bool enabled) override;
    State *start(int level, const char *tag) override;
    void puts(State *state, const char *span) override;
    State *flush(State *state, bool) override;

private:
    class LoggerFile {
    public:
        explicit LoggerFile(const String &path, std::size_t capacity, int fileCount);
        ~LoggerFile();
        void open();
        void close();
        void write(const String &line);

    private:
        void rotate();

    private:
        Mutex m_lock;
        File m_file;
        std::size_t m_capacity;
        int m_fileCount;
    };

    Features m_features;
    LoggerFile m_file;
    LoggerStream *m_fallback;
};
} // namespace gx
