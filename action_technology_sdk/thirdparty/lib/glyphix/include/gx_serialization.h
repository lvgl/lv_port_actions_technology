/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_bytearray.h"
#include "gx_string.h"

namespace gx {
class BinaryReader {
public:
    BinaryReader(const uint8_t *first, const uint8_t *last);
    GX_NODISCARD bool isEnd() const { return m_pointer == m_last; }
    GX_NODISCARD uint8_t readU8();
    GX_NODISCARD uint16_t readU16();
    GX_NODISCARD uint32_t readU32();
    GX_NODISCARD uint64_t readU64();
    GX_NODISCARD float readFloat();
    GX_NODISCARD double readDouble();
    GX_NODISCARD String readString();
    GX_NODISCARD int tell() const { return static_cast<int>(m_pointer - m_first); }
    //! Seek to offset relative to the current position in the stream.
    void seek(int offset);
    //! Seek to absolute position in the stream.
    void lseek(int offset);

private:
    const uint8_t *m_first, *m_pointer, *m_last;
};

class BinaryWriter {
public:
    explicit BinaryWriter(ByteArray &data) : m_data(data) {}
    void writeU8(int x) { m_data.push_back(uint8_t(x)); }
    void writeU16(int x);
    void writeU32(uint32_t x);
    void writeU64(uint64_t x);
    void writeDouble(double x);
    void writeString(const char *s, std::size_t size);
    void writeString(const String &string);
    void writeString(const char *s);

private:
    ByteArray &m_data;
};
} // namespace gx
