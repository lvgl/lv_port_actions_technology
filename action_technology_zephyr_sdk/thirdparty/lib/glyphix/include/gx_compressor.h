/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#pragma once

#include "gx_async.h"

namespace gx {

class Compressor {
public:
    enum Type { Gzip };
    virtual ~Compressor() = default;
    virtual int decompress(Vector<uint8_t> &input, Vector<uint8_t> &output) = 0;
};

class GzipCompressor : public Compressor {
public:
    int decompress(Vector<uint8_t> &input, Vector<uint8_t> &output) override;
};

class CompressorFactory {
public:
    static Compressor *getCompressor(Compressor::Type type);
};

} // namespace gx

