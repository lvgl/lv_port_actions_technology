/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_textencode.h"

namespace gx {
// hmac
String cipherHmac(const String &key, const String &text, const String &type);

// rsa
enum PaddingType { PkcsV15 = 0, OaepWithMd5V21, OaepWithSha1V21, OaepWithSha256V21 };
PaddingType identifyType(const String &type);
String encryptRsa(const String &text, const String &key, int transformation);
String decryptRsa(const String &text, const String &key, int transformation);
String signRsa(const String &text, const String &key, const String &algorithm);
bool verifyRsa(const String &text, const String &key, const String &signText,
               const String &algorithm);
} // namespace gx
