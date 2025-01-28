/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_cipher.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pk.h"
#include "gx_logger.h"

namespace gx {
PaddingType identifyType(const String &type) {
  String::const_iterator pos = type.findLast("/");
  String temp = type.substr(pos + 1, type.end());
  if (temp == "PKCS_v15andMGF1Padding")
    return PkcsV15;
  if (temp == "OAEPwithMD5andMGF1Padding")
    return OaepWithMd5V21;
  if (temp == "OAEPwithSHA-1andMGF1Padding")
    return OaepWithSha1V21;
  if (temp == "OAEPwithSHA-256andMGF1Padding")
    return OaepWithSha256V21;
  return OaepWithSha256V21;
}

String encryptRsa(const String &text, const String &key, int transformation) {
  mbedtls_entropy_context entropy = {};
  mbedtls_pk_context pkCtx = {};
  mbedtls_ctr_drbg_context ctrDrbg = {};

  const char *custom = __FUNCTION__;
  Vector<uint8_t> output(1024);
  size_t olen = 0;

  mbedtls_pk_init(&pkCtx);
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctrDrbg);

  // set random number generator
  mbedtls_ctr_drbg_seed(&ctrDrbg, mbedtls_entropy_func, &entropy,
                        (const unsigned char *)custom, std::strlen(custom));

  // parse public key
  mbedtls_pk_parse_public_key(&pkCtx, (const unsigned char *)(key.c_str()),
                              key.length() + 1);

  // check is rsa
  mbedtls_pk_can_do(&pkCtx, MBEDTLS_PK_RSA);

  // set padding mode
  mbedtls_rsa_context *temp = mbedtls_pk_rsa(pkCtx);
  switch (transformation) {
  case PkcsV15:
    mbedtls_rsa_set_padding(temp, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);
    break;
  case OaepWithMd5V21:
    mbedtls_rsa_set_padding(temp, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_MD5);
    break;
  case OaepWithSha1V21:
    mbedtls_rsa_set_padding(temp, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1);
    break;
  case OaepWithSha256V21:
    mbedtls_rsa_set_padding(temp, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
    break;
  default:
    mbedtls_rsa_set_padding(temp, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
    break;
  }

  // do encrypt
  mbedtls_pk_encrypt(&pkCtx,
                     reinterpret_cast<const unsigned char *>(text.c_str()),
                     text.size(), output.data(), &olen, output.capacity(),
                     mbedtls_ctr_drbg_random, &ctrDrbg);

  mbedtls_pk_free(&pkCtx);
  mbedtls_entropy_free(&entropy);
  mbedtls_ctr_drbg_free(&ctrDrbg);

  String result = base64Encode(output.data(), olen);
  return result;
}

String decryptRsa(const String &text, const String &key, int transformation) {
  mbedtls_entropy_context entropy = {};
  mbedtls_pk_context pkCtx = {};
  mbedtls_ctr_drbg_context ctrDrbg = {};

  const char *custom = __FUNCTION__;
  Vector<uint8_t> output(1024);
  size_t olen = 0;

  mbedtls_pk_init(&pkCtx);
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctrDrbg);

  // set random number generator
  mbedtls_ctr_drbg_seed(&ctrDrbg, mbedtls_entropy_func, &entropy,
                        (const unsigned char *)custom, std::strlen(custom));

  // parse private key
#if MBEDTLS_VERSION_NUMBER >= 0x3000000
  mbedtls_pk_parse_key(&pkCtx,
                       reinterpret_cast<const unsigned char *>(key.c_str()),
                       key.length() + 1, nullptr, 0, //
                       mbedtls_ctr_drbg_random, &ctrDrbg);
#else
  mbedtls_pk_parse_key(&pkCtx,
                       reinterpret_cast<const unsigned char *>(key.c_str()),
                       key.length() + 1, nullptr, 0);
#endif

  // check is rsa
  mbedtls_pk_can_do(&pkCtx, MBEDTLS_PK_RSA);

  // set padding mode
  mbedtls_rsa_context *temp = mbedtls_pk_rsa(pkCtx);
  switch (transformation) {
  case PkcsV15:
    mbedtls_rsa_set_padding(temp, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);
    break;
  case OaepWithMd5V21:
    mbedtls_rsa_set_padding(temp, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_MD5);
    break;
  case OaepWithSha1V21:
    mbedtls_rsa_set_padding(temp, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1);
    break;
  case OaepWithSha256V21:
    mbedtls_rsa_set_padding(temp, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
    break;
  default:
    mbedtls_rsa_set_padding(temp, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
    break;
  }

  // get ciphertext
  String input = base64Decode(text);

  // rsa decrypt
  mbedtls_pk_decrypt(&pkCtx,
                     reinterpret_cast<const unsigned char *>(input.c_str()),
                     input.size(), output.data(), &olen, output.capacity(),
                     mbedtls_ctr_drbg_random, &ctrDrbg);

  mbedtls_pk_free(&pkCtx);
  mbedtls_entropy_free(&entropy);
  mbedtls_ctr_drbg_free(&ctrDrbg);

  String decrypt(String::const_iterator(output.begin()),
                 String::const_iterator(output.end()));
  decrypt.resize(std::strlen(decrypt.c_str()));
  return decrypt;
}

static mbedtls_md_type_t getMdType(const String &algorithm) {
  if (algorithm.startsWith("MD5withRSA"))
    return MBEDTLS_MD_MD5;
  else if (algorithm.startsWith("SHA1withRSA"))
    return MBEDTLS_MD_SHA1;
  else if (algorithm.startsWith("SHA512withRSA"))
    return MBEDTLS_MD_SHA512;
  else
    return MBEDTLS_MD_SHA256;
}

String signRsa(const String &text, const String &key, const String &algorithm) {
  mbedtls_entropy_context entropy = {};
  mbedtls_pk_context pkCtx = {};
  mbedtls_ctr_drbg_context ctrDrbg = {};

  int res;
  unsigned char hash[64] = {0};
  Vector<uint8_t> output(1024);
  const char *custom = __FUNCTION__;

  mbedtls_md_type_t type;
  const mbedtls_md_info_t *mdInfo;
  size_t olen = 0;

  // init
  mbedtls_pk_init(&pkCtx);
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctrDrbg);

  // set random number generator
  res =
      mbedtls_ctr_drbg_seed(&ctrDrbg, mbedtls_entropy_func, &entropy,
                            (const unsigned char *)custom, std::strlen(custom));
  if (res != 0) {
    LogError() << "mbedtls_ctr_drbg_seed failed.";
    goto exit;
  }

  // parse private key
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
  res = mbedtls_pk_parse_key(
      &pkCtx, reinterpret_cast<const unsigned char *>(key.c_str()),
      key.length() + 1, nullptr, 0, mbedtls_ctr_drbg_random, &ctrDrbg);
#else
  res = mbedtls_pk_parse_key(
      &pkCtx, reinterpret_cast<const unsigned char *>(key.c_str()),
      key.length() + 1, nullptr, 0);
#endif
  if (res != 0) {
    LogError() << "mbedtls_pk_parse_key failed.";
    goto exit;
  }

  // check is rsa
  res = mbedtls_pk_can_do(&pkCtx, MBEDTLS_PK_RSA);
  if (res == 0)
    goto exit;

  // get md type
  type = getMdType(algorithm);

  // hash input
  mdInfo = mbedtls_md_info_from_type(type);
  res =
      mbedtls_md(mdInfo, reinterpret_cast<const unsigned char *>(text.c_str()),
                 text.size(), hash);
  if (res != 0)
    goto exit;

#if MBEDTLS_VERSION_NUMBER >= 0x03000000
  res = mbedtls_pk_sign(&pkCtx, type, hash, mbedtls_md_get_size(mdInfo),
                        output.data(), output.capacity(), &olen,
                        mbedtls_ctr_drbg_random, &ctrDrbg);
#else
  res =
      mbedtls_pk_sign(&pkCtx, type, hash, mbedtls_md_get_size(mdInfo),
                      output.data(), &olen, mbedtls_ctr_drbg_random, &ctrDrbg);
#endif
  if (res != 0) {
    LogError() << "mbedtls_pk_sign failed.";
    goto exit;
  }

exit:
  mbedtls_pk_free(&pkCtx);
  mbedtls_entropy_free(&entropy);
  mbedtls_ctr_drbg_free(&ctrDrbg);

  // make ciphertext
  String result = base64Encode(output.data(), olen);
  return result;
}

bool verifyRsa(const String &text, const String &key, const String &signText,
               const String &algorithm) {
  mbedtls_pk_context pkCtx = {};
  unsigned char hash[64] = {0};

  int res;
  Vector<uint8_t> encodeText(1024);
  mbedtls_md_type_t type;
  const mbedtls_md_info_t *mdInfo;

  mbedtls_pk_init(&pkCtx);

  res =
      mbedtls_pk_parse_public_key(&pkCtx, (const unsigned char *)(key.c_str()),
                                  key.length() + 1); //
  if (res != 0) {
    LogError() << "rsa verify sign parse public key error.";
    goto exit;
  }

  res = mbedtls_pk_can_do(&pkCtx, MBEDTLS_PK_RSA);
  if (res == 0) {
    LogError() << "rsa verify sign parse key type error.";
    goto exit;
  }

  type = getMdType(algorithm);
  mdInfo = mbedtls_md_info_from_type(type);
  res =mbedtls_md(mdInfo, reinterpret_cast<const unsigned char *>(text.c_str()),
                 text.size(), hash);
  if (res != 0) {
    LogError() << "rsa verify sign md fail.";
    goto exit;
  }

  encodeText = base64Decode<Vector<uint8_t>>(signText);
  res = mbedtls_pk_verify(&pkCtx, type, hash, mbedtls_md_get_size(mdInfo),
                          encodeText.data(), encodeText.size());
  if (res != 0) {
    LogError() << "rsa verify sign process fail.";
    goto exit;
  }
  mbedtls_pk_free(&pkCtx);
  return true;

exit:
  mbedtls_pk_free(&pkCtx);
  return false;
}
} // namespace gx
