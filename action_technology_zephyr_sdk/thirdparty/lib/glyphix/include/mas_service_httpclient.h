/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#include "gx_abstract_httpclient.h"

#pragma once

namespace gx {

class MasHttpClient : public AbstractHttpClient {
public:
    explicit MasHttpClient(size_t headerSize);
    ~MasHttpClient() override;
    int setTimeout(int millisecond) override;
    int connect(const char *uri) override;
    int sendHeader() override;
    int handleResponse() override;
    int getBytesAvail() override;
    int read(void *buffer, size_t size) override;
    int write(const void *buffer, size_t size) override;

private:
    int m_timeoutMs{};    /* timeout millisecond */
    int m_remoteSocket{}; /* remote rpc httpclient socket number */
    /** number of times data was received, set by event_callback(),
        tested by the receive and select functions */
    short m_rcvevent{};
    /** number of times data was ACKed (free send buffer), set by event_callback(),
        tested by select */
    short m_sendevent{};
    /** error happened for this socket, set by event_callback(), tested by select */
    short m_errevent{};
    int rawRead(void *buffer, size_t size);

    int rawReadWithResult(void *buffer, size_t size);
};

} // namespace gx
