/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#pragma once

#include "gx_mutex.h"
#include "platform/gx_os.h"
#include <cstdlib>

namespace gx {

struct HttpClientHeader {
    char *buffer;
    size_t length; /* content header buffer size */
    size_t size; /* maximum support header size */
};

#define HTTPCLIENT_HEADER_BUFSZ   4096
#define HTTPCLIENT_RESPONSE_BUFSZ 4096

enum HTTPCLIENT_STATUS {
    HTTPCLIENT_OK,
    HTTPCLIENT_ERROR,
    HTTPCLIENT_TIMEOUT,
    HTTPCLIENT_NOMEM,
    HTTPCLIENT_NOSOCKET,
    HTTPCLIENT_NOBUFFER,
    HTTPCLIENT_CONNECT_FAILED,
    HTTPCLIENT_DISCONNECT,
    HTTPCLIENT_FILE_ERROR,
};

enum HTTPCLIENT_METHOD {
    HTTPCLIENT_USER_METHOD,
    HTTPCLIENT_GET,
    HTTPCLIENT_POST,
};

class AbstractHttpClient {
public:
    virtual ~AbstractHttpClient() = default;

    virtual int setTimeout(int millisecond) = 0;
    virtual int connect(const char *uri) = 0;
    virtual int sendHeader() = 0;
    virtual int handleResponse() = 0;
    virtual int getBytesAvail() = 0;
    virtual int read(void *buffer, size_t size) = 0;
    virtual int write(const void *buffer, size_t size) = 0;
    /**
     * add fields data to request header data.
     *
     * @param fmt fields format
     *
     * @return >0: data length of successfully added
     *         <0: not enough header buffer size
     */
    int headerFieldsAdd(const char *fmt, ...);
    /**
     * iterate all the field information from the request/response header data.
     *
     * @param iter processing function
     * @param user_handle user Handle
     *
     * @return = nullptr: get fields data failed
     *        != nullptr: success get fields data
     */
    void headerFieldsIter(bool (*iter)(void *user_handle, const char *key, const char *value),
                          void *user_handle);
    /**
     * resolve server address
     *
     * @param m_session http m_session
     * @param res the server address information
     * @param url the input server URI address
     * @param request the pointer to point the request url, for example, /index.html
     *
     * @return 0 on resolve server address OK, others failed
     *
     * URL example:
     * http://www.rt-thread.org
     * http://www.rt-thread.org:80
     * https://www.rt-thread.org/
     * http://192.168.1.1:80/index.htm
     * http://[fe80::1]
     * http://[fe80::1]/
     * http://[fe80::1]/index.html
     * http://[fe80::1]:80/index.html
     */
    int resolveAddress(const char *url);
    /* send HTTP POST/GET request, and get response data */
    int httpGetPosition(const char *uri, int position);

    struct HttpClientHeader header {}; /* httpclient response header information */
    char *host{};   /* server host */
    int port{};   /* server port */

    size_t contentLength{};
    size_t chunkSize{};
    int chunkOffset{};

protected:
    Mutex mMutex;
    char *mReqUrl{}; /* HTTP request address*/
    int respStatus{};
};

} // namespace gx
