/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"
#include "gx_vector.h"
#include "tiny_base64.h"

namespace gx {
enum DataEncode { RawEncode, Base64Encode, HexEncode };

String base64Encode(const uint8_t *data, const std::size_t size);
String base64Encode(const Vector<uint8_t> &data);
template<class T = String> T base64Decode(const uint8_t *data, const std::size_t size) {
    T temp(size * 4);
    int olen = int(temp.capacity());
    tiny_base64_decode(reinterpret_cast<uint8_t *>(temp.data()), &olen, const_cast<uint8_t *>(data),
                       int(size));
    temp.resize(olen);
    return temp;
}

template<class T = String> T base64Decode(const String &data) {
    return base64Decode<T>(reinterpret_cast<const uint8_t *>(data.data()), data.size());
}
String dataEncode(const String &data, int encode);
} // namespace gx
