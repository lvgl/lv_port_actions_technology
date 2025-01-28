/**
 * \file des.h
 *
 *  Based on TropicSSL: Copyright (C) 2017 Shanghai Real-Thread Technology Co., Ltd
 *
 *  Based on XySSL: Copyright (C) 2006-2008  Christophe Devine
 *
 *  Copyright (C) 2009  Paul Bakker <polarssl_maintainer at polarssl dot org>
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of PolarSSL or XySSL nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef TINY_CRYPT_DES_H__
#define TINY_CRYPT_DES_H__

#define DES_ENCRYPT     1
#define DES_DECRYPT     0

#define TINY_ERR_DES_INVALID_INPUT_LENGTH              -0x0032  /**< The data input has an invalid length. */

#define TINY_DES_KEY_SIZE    8

/**
 * \brief          DES context structure
 */
typedef struct
{
    unsigned long sk[32];            /*!<  DES subkeys       */
}
tiny_des_context;

/**
 * \brief          Triple-DES context structure
 */
typedef struct
{
    unsigned long sk[96];            /*!<  3DES subkeys      */
}
tiny_des3_context;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Set key parity on the given key to odd.
 *
 * \param key      8-byte secret key
 */
void tiny_des_key_set_parity(unsigned char key[TINY_DES_KEY_SIZE]);

/**
 * \brief          Check that key parity on the given key is odd.
 *
 * \param key      8-byte secret key
 *
 * \return         0 is parity was ok, 1 if parity was not correct.
 */
int tiny_des_key_check_key_parity(const unsigned char key[TINY_DES_KEY_SIZE]);

/**
 * \brief          DES key schedule (56-bit, encryption)
 *
 * \param ctx      DES context to be initialized
 * \param key      8-byte secret key
 */
void tiny_des_setkey_enc(tiny_des_context *ctx, unsigned char key[TINY_DES_KEY_SIZE]);

/**
 * \brief          DES key schedule (56-bit, decryption)
 *
 * \param ctx      DES context to be initialized
 * \param key      8-byte secret key
 */
void tiny_des_setkey_dec(tiny_des_context *ctx, unsigned char key[TINY_DES_KEY_SIZE]);

/**
 * \brief          Triple-DES key schedule (112-bit, encryption)
 *
 * \param ctx      3DES context to be initialized
 * \param key      16-byte secret key
 */
void tiny_des3_set2key_enc(tiny_des3_context *ctx,
                           unsigned char key[TINY_DES_KEY_SIZE * 2]);

/**
 * \brief          Triple-DES key schedule (112-bit, decryption)
 *
 * \param ctx      3DES context to be initialized
 * \param key      16-byte secret key
 */
void tiny_des3_set2key_dec(tiny_des3_context *ctx,
                           unsigned char key[TINY_DES_KEY_SIZE * 2]);

/**
 * \brief          Triple-DES key schedule (168-bit, encryption)
 *
 * \param ctx      3DES context to be initialized
 * \param key      24-byte secret key
 */
void tiny_des3_set3key_enc(tiny_des3_context *ctx,
                           unsigned char key[TINY_DES_KEY_SIZE * 3]);

/**
 * \brief          Triple-DES key schedule (168-bit, decryption)
 *
 * \param ctx      3DES context to be initialized
 * \param key      24-byte secret key
 */
void tiny_des3_set3key_dec(tiny_des3_context *ctx,
                           unsigned char key[TINY_DES_KEY_SIZE * 3]);

/**
 * \brief          DES-ECB block encryption/decryption
 *
 * \param ctx      DES context
 * \param input    64-bit input block
 * \param output   64-bit output block
 */
void tiny_des_crypt_ecb(tiny_des_context *ctx,
                        unsigned char input[8],
                        unsigned char output[8]);

/**
 * \brief          DES-CBC buffer encryption/decryption
 *
 * \param ctx      DES context
 * \param mode     TINY_DES_ENCRYPT or TINY_DES_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 * 
 * \return         0 if successful, or TINY_ERR_DES_INVALID_INPUT_LENGTH
 */
int tiny_des_crypt_cbc(tiny_des_context *ctx,
                       int mode,
                       int length,
                       unsigned char iv[8],
                       unsigned char *input,
                       unsigned char *output);

/**
 * \brief          3DES-ECB block encryption/decryption
 *
 * \param ctx      3DES context
 * \param input    64-bit input block
 * \param output   64-bit output block
 */
void tiny_des3_crypt_ecb(tiny_des3_context *ctx,
                         unsigned char input[8],
                         unsigned char output[8]);

/**
 * \brief          3DES-CBC buffer encryption/decryption
 *
 * \param ctx      3DES context
 * \param mode     TINY_DES_ENCRYPT or TINY_DES_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 *
 * \return         0 if successful, or TINY_ERR_DES_INVALID_INPUT_LENGTH
 */
int tiny_des3_crypt_cbc(tiny_des3_context *ctx,
                        int mode,
                        int length,
                        unsigned char iv[8],
                        unsigned char *input,
                        unsigned char *output);

#ifdef __cplusplus
}
#endif

#endif /* des.h */
