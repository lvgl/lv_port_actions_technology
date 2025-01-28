/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#pragma once

#include "gx_abstract_httpclient.h"
#include "gx_http_task.h"
#include "gx_async.h"
#include "gx_hashmap.h"
#include "gx_string.h"
#include "gx_compressor.h"

namespace gx {
enum HttpMethod { Get, Post, Put, Delete };
enum HttpContentType { TextPlain, ApplicationJson, ApplicationOctetStream };

class HttpReader {
public:
    struct Progress {
        virtual ~Progress() GX_DEFAULT;
        virtual bool operator()(std::size_t current, std::size_t total, const uint8_t *data,
                                std::size_t size) = 0;
        virtual int decompress() { return 0; };
        virtual Vector<uint8_t> data() { return Vector<uint8_t>(); }
        virtual void setFailed() {}
    };
    struct ProgressWithCache : Progress {
        explicit ProgressWithCache() : m_buffer(){};
        bool operator()(std::size_t current, std::size_t total, const uint8_t *data,
                        std::size_t size) override {
            if (total == 0) // chunked mode
                m_buffer.resize(current + size);
            else if (m_buffer.size() < total)
                m_buffer.resize(total);
            memcpy(m_buffer.data() + current, data, size);
            return true;
        }
        int decompress() override {
            Vector<uint8_t> deBuffer;
            std::unique_ptr<Compressor> gzip{CompressorFactory::getCompressor(Compressor::Gzip)};
            int result = gzip->decompress(m_buffer, deBuffer);
            std::swap(m_buffer, deBuffer);
            return result;
        }
        Vector<uint8_t> data() override { return m_buffer; }

    private:
        Vector<uint8_t> m_buffer;
    };
    explicit HttpReader(const String &reqUrl, HttpMethod method, const String &body,
                        HashMap<String, String> &headers, int timeout);
    ~HttpReader();
    int read(void *buffer, size_t size);
    int readAll(Progress *progress);
    int getResponseHeader(Vector<std::pair<String, String> > &headers);
    bool contentEncodingIsGzip();
    static const char *methodCast(HttpMethod method);
    const String &url() const { return m_url; }
    const String &host() const { return m_host; }
    const String &port() const { return m_port; }
    int status() const { return m_status; }
    size_t chunkSize() const { return m_chunkSize; }
    size_t contentLength() const { return m_contentLength; }
    AbstractHttpClient *session() const { return m_session; }

private:
    String m_url;
    String m_host;
    String m_port;
    int m_status;
    size_t m_contentLength = 0;
    size_t m_chunkSize = 0;
    AbstractHttpClient *m_session{};
};

} // namespace gx