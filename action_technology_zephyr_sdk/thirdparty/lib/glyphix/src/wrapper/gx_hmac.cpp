/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_cipher.h"
#include "mbedtls/md.h"

namespace gx {
int hmacFindType(const String &type) {
  if (type.startsWith("md5"))
    return MBEDTLS_MD_MD5;
  if (type.startsWith("sha1"))
    return MBEDTLS_MD_SHA1;
  if (type.startsWith("sha224"))
    return MBEDTLS_MD_SHA224;
  if (type.startsWith("sha256"))
    return MBEDTLS_MD_SHA256;
  if (type.startsWith("sha384"))
    return MBEDTLS_MD_SHA384;
  if (type.startsWith("sha512"))
    return MBEDTLS_MD_SHA512;
  return MBEDTLS_MD_NONE;
}

String cipherHmac(const String &key, const String &text, const String &type) {
  int mdType = hmacFindType(type);
  String shaSum(64);
  const mbedtls_md_info_t *info;

  info = mbedtls_md_info_from_type((mbedtls_md_type_t)mdType);
  mbedtls_md_hmac(info, (uint8_t *)key.data(), key.size(),
                  (uint8_t *)text.data(), text.size(),
                  (uint8_t *)shaSum.data());
  int hmacLen = mbedtls_md_get_size(info);
  shaSum.resize(hmacLen);
  return shaSum;
}
} // namespace gx
