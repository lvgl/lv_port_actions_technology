/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_loggerstream.h"
#include "gx_string.h"
#include "gx_vector.h"
#include <utility>

// log level filters & log tags
#ifndef GX_LOGGING_LEVEL // The global default log level, defined in the CMake arguments.
#define GX_LOGGING_LEVEL GX_LOG_INFO
#endif
#ifndef GX_LOG_LEVEL
#define GX_LOG_LEVEL GX_LOGGING_LEVEL
#endif
#ifndef GX_LOG_TAG
#define GX_LOG_TAG nullptr
#endif

namespace gx {
namespace logger {
enum FmtFlags {
    hex = 1 << 0,   // NOLINT(readability-*)
    fill0 = 1 << 1, // NOLINT(readability-*)
    left = 1 << 2,  // NOLINT(readability-*)
};

class fmt { // NOLINT(readability-*)
public:
    explicit fmt(int value, int width = 0, int flags = 0);
    explicit fmt(float value, int width = 0, int flags = 0);
    explicit fmt(double value, int width = 0, int flags = 0);
    explicit fmt(long value, int width = 0, int flags = 0);
    explicit fmt(long long value, int width = 0, int flags = 0);
    explicit fmt(unsigned int value, int width = 0, int flags = 0);
    explicit fmt(unsigned long value, int width = 0, int flags = 0);
    explicit fmt(unsigned long long value, int width = 0, int flags = 0);

private:
    char m_buffer[32];
    friend class gx::Logger;
};

class holdspace { // NOLINT(readability-*)
public:
    explicit holdspace(const Logger &logger);
    ~holdspace();

private:
    const Logger &m_logger;
};

struct endl_t {};    // NOLINT(readability-*)
struct nospace_t {}; // NOLINT(readability-*)
struct space_t {};   // NOLINT(readability-*)

GX_CONSTEXPR static const endl_t endl;       // NOLINT(readability-*)
GX_CONSTEXPR static const nospace_t nospace; // NOLINT(readability-*)
GX_CONSTEXPR static const space_t space;     // NOLINT(readability-*)
} // namespace logger

class Logger {
public:
    typedef logger::endl_t endl_t;       // NOLINT(readability-*)
    typedef logger::nospace_t nospace_t; // NOLINT(readability-*)
    typedef logger::space_t space_t;     // NOLINT(readability-*)

    explicit Logger(int level, const char *tag);              // use LoggerStream::stream
    Logger(LoggerStream *stream, int level, const char *tag); // specified stream
    ~Logger();
    const Logger &operator<<(nospace_t) const;
    const Logger &operator<<(space_t) const;
    const Logger &operator<<(endl_t) const;
    const Logger &operator<<(int value) const;
    const Logger &operator<<(float value) const;
    const Logger &operator<<(double value) const;
    const Logger &operator<<(long value) const;
    const Logger &operator<<(long long value) const;
    const Logger &operator<<(unsigned int value) const;
    const Logger &operator<<(unsigned long value) const;
    const Logger &operator<<(unsigned long long value) const;
    const Logger &operator<<(const char *text) const;
    const Logger &operator<<(const String &text) const;
    const Logger &operator<<(bool value) const;
    const Logger &operator<<(const void *ptr) const;
    const Logger &operator<<(const logger::fmt &f) const;

protected:
    char spaceChar() const;
    const Logger &puts(const char *span) const;

private:
    const char *m_tag;
    const uint16_t m_level;
    mutable uint16_t m_noSpaceNested;
    mutable LoggerStream::State *m_state;
    LoggerStream *const m_stream;
};

template<class T1, class T2>
const Logger &operator<<(const Logger &logger, const std::pair<T1, T2> &pair) {
    logger::holdspace spc(logger);
    return logger << "std::pair(" << pair.first << ", " << pair.second << ")";
}

inline const Logger &operator<<(const Logger &logger, const uint8_t *bytes) {
    return logger << reinterpret_cast<const char *>(bytes);
}

inline const Logger &operator<<(const Logger &logger, const std::string &text) {
    return logger << text.c_str();
}

template<class T, class Alloc>
const Logger &operator<<(const Logger &logger, const Vector<T, Alloc> &vector) {
    logger::holdspace spc(logger);
    logger << "Vector{size=" << vector.size();
    std::size_t i = 0, end = min<std::size_t>(8, vector.size());
    for (; i < end; i++)
        logger << ", " << vector[i];
    for (end = vector.size(); i < end; i++)
        (i % 8 == 0 ? logger << "," << logger::endl : logger << ", ") << vector[i];
    logger << "}";
    return logger;
}

namespace logger {
inline holdspace::holdspace(const Logger &logger) : m_logger(logger) { logger << nospace_t(); }
inline holdspace::~holdspace() { m_logger << space_t(); }
} // namespace logger

namespace internal {
struct LoggerFake {
    explicit LoggerFake(int, const char *) {}
    LoggerFake(LoggerStream *, int, const char *) {}
    template<class T> const LoggerFake &operator<<(const T &) const { return *this; }
};

template<bool> struct LoggerSelector {};
template<> struct LoggerSelector<true> {
    typedef Logger logger_type;
};
template<> struct LoggerSelector<false> {
    typedef LoggerFake logger_type;
};

template<int filter, int level> struct LoggerLevel : LoggerSelector<filter <= level>::logger_type {
    typedef typename LoggerSelector<filter <= level>::logger_type logger_type;
    explicit LoggerLevel(const char *tag = GX_LOG_TAG) : logger_type(level, tag) {}
    explicit LoggerLevel(LoggerStream *stream, const char *tag = GX_LOG_TAG)
        : logger_type(stream, level, tag) {}
};
} // namespace internal

typedef internal::LoggerLevel<GX_LOG_LEVEL, GX_LOG_DEBUG> LogDebug;
typedef internal::LoggerLevel<GX_LOG_LEVEL, GX_LOG_INFO> LogInfo;
typedef internal::LoggerLevel<GX_LOG_LEVEL, GX_LOG_WARN> LogWarn;
typedef internal::LoggerLevel<GX_LOG_LEVEL, GX_LOG_ERROR> LogError;
typedef internal::LoggerLevel<GX_LOG_LEVEL, GX_LOG_FATAL> LogFatal;
} // namespace gx
