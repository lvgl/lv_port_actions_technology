/*
 *  FIPS-180-1 compliant SHA-1 implementation
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
 *  The SHA-1 standard was published by NIST in 1993.
 *
 *  http://www.itl.nist.gov/fipspubs/fip180-1.htm
 */

#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#else
#include "mbedtls/config.h"
#endif

#if defined(MBEDTLS_SHA1_C)

#include "mbedtls/sha1.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"

#include <string.h>

#include "mbedtls/platform.h"

#if defined(MBEDTLS_SHA1_ALT)

/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
}
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

void mbedtls_sha1_init( mbedtls_sha1_context *ctx )
{
    memset( ctx, 0, sizeof( mbedtls_sha1_context ) );
}

void mbedtls_sha1_free( mbedtls_sha1_context *ctx )
{
    if( ctx == NULL )
        return;

    mbedtls_platform_zeroize( ctx, sizeof( mbedtls_sha1_context ) );
}

void mbedtls_sha1_clone( mbedtls_sha1_context *dst,
                         const mbedtls_sha1_context *src )
{
    *dst = *src;
}

#if !defined(MBEDTLS_SHA1_PROCESS_ALT)
int mbedtls_internal_sha1_process( mbedtls_sha1_context *ctx,
                                   const unsigned char data[64] )
{
    struct
    {
        uint32_t temp, W[16], A, B, C, D, E;
    } local;

    GET_UINT32_BE( local.W[ 0], data,  0 );
    GET_UINT32_BE( local.W[ 1], data,  4 );
    GET_UINT32_BE( local.W[ 2], data,  8 );
    GET_UINT32_BE( local.W[ 3], data, 12 );
    GET_UINT32_BE( local.W[ 4], data, 16 );
    GET_UINT32_BE( local.W[ 5], data, 20 );
    GET_UINT32_BE( local.W[ 6], data, 24 );
    GET_UINT32_BE( local.W[ 7], data, 28 );
    GET_UINT32_BE( local.W[ 8], data, 32 );
    GET_UINT32_BE( local.W[ 9], data, 36 );
    GET_UINT32_BE( local.W[10], data, 40 );
    GET_UINT32_BE( local.W[11], data, 44 );
    GET_UINT32_BE( local.W[12], data, 48 );
    GET_UINT32_BE( local.W[13], data, 52 );
    GET_UINT32_BE( local.W[14], data, 56 );
    GET_UINT32_BE( local.W[15], data, 60 );

#define S(x,n) (((x) << (n)) | (((x) & 0xFFFFFFFF) >> (32 - (n))))

#define R(t)                                                    \
    (                                                           \
        local.temp = local.W[( (t) -  3 ) & 0x0F] ^             \
                     local.W[( (t) -  8 ) & 0x0F] ^             \
                     local.W[( (t) - 14 ) & 0x0F] ^             \
                     local.W[  (t)        & 0x0F],              \
        ( local.W[(t) & 0x0F] = S(local.temp,1) )               \
    )

#define P(a,b,c,d,e,x)                                          \
    do                                                          \
    {                                                           \
        (e) += S((a),5) + F((b),(c),(d)) + K + (x);             \
        (b) = S((b),30);                                        \
    } while( 0 )

    local.A = ctx->state[0];
    local.B = ctx->state[1];
    local.C = ctx->state[2];
    local.D = ctx->state[3];
    local.E = ctx->state[4];

#define F(x,y,z) ((z) ^ ((x) & ((y) ^ (z))))
#define K 0x5A827999

    P( local.A, local.B, local.C, local.D, local.E, local.W[0]  );
    P( local.E, local.A, local.B, local.C, local.D, local.W[1]  );
    P( local.D, local.E, local.A, local.B, local.C, local.W[2]  );
    P( local.C, local.D, local.E, local.A, local.B, local.W[3]  );
    P( local.B, local.C, local.D, local.E, local.A, local.W[4]  );
    P( local.A, local.B, local.C, local.D, local.E, local.W[5]  );
    P( local.E, local.A, local.B, local.C, local.D, local.W[6]  );
    P( local.D, local.E, local.A, local.B, local.C, local.W[7]  );
    P( local.C, local.D, local.E, local.A, local.B, local.W[8]  );
    P( local.B, local.C, local.D, local.E, local.A, local.W[9]  );
    P( local.A, local.B, local.C, local.D, local.E, local.W[10] );
    P( local.E, local.A, local.B, local.C, local.D, local.W[11] );
    P( local.D, local.E, local.A, local.B, local.C, local.W[12] );
    P( local.C, local.D, local.E, local.A, local.B, local.W[13] );
    P( local.B, local.C, local.D, local.E, local.A, local.W[14] );
    P( local.A, local.B, local.C, local.D, local.E, local.W[15] );
    P( local.E, local.A, local.B, local.C, local.D, R(16) );
    P( local.D, local.E, local.A, local.B, local.C, R(17) );
    P( local.C, local.D, local.E, local.A, local.B, R(18) );
    P( local.B, local.C, local.D, local.E, local.A, R(19) );

#undef K
#undef F

#define F(x,y,z) ((x) ^ (y) ^ (z))
#define K 0x6ED9EBA1

    P( local.A, local.B, local.C, local.D, local.E, R(20) );
    P( local.E, local.A, local.B, local.C, local.D, R(21) );
    P( local.D, local.E, local.A, local.B, local.C, R(22) );
    P( local.C, local.D, local.E, local.A, local.B, R(23) );
    P( local.B, local.C, local.D, local.E, local.A, R(24) );
    P( local.A, local.B, local.C, local.D, local.E, R(25) );
    P( local.E, local.A, local.B, local.C, local.D, R(26) );
    P( local.D, local.E, local.A, local.B, local.C, R(27) );
    P( local.C, local.D, local.E, local.A, local.B, R(28) );
    P( local.B, local.C, local.D, local.E, local.A, R(29) );
    P( local.A, local.B, local.C, local.D, local.E, R(30) );
    P( local.E, local.A, local.B, local.C, local.D, R(31) );
    P( local.D, local.E, local.A, local.B, local.C, R(32) );
    P( local.C, local.D, local.E, local.A, local.B, R(33) );
    P( local.B, local.C, local.D, local.E, local.A, R(34) );
    P( local.A, local.B, local.C, local.D, local.E, R(35) );
    P( local.E, local.A, local.B, local.C, local.D, R(36) );
    P( local.D, local.E, local.A, local.B, local.C, R(37) );
    P( local.C, local.D, local.E, local.A, local.B, R(38) );
    P( local.B, local.C, local.D, local.E, local.A, R(39) );

#undef K
#undef F

#define F(x,y,z) (((x) & (y)) | ((z) & ((x) | (y))))
#define K 0x8F1BBCDC

    P( local.A, local.B, local.C, local.D, local.E, R(40) );
    P( local.E, local.A, local.B, local.C, local.D, R(41) );
    P( local.D, local.E, local.A, local.B, local.C, R(42) );
    P( local.C, local.D, local.E, local.A, local.B, R(43) );
    P( local.B, local.C, local.D, local.E, local.A, R(44) );
    P( local.A, local.B, local.C, local.D, local.E, R(45) );
    P( local.E, local.A, local.B, local.C, local.D, R(46) );
    P( local.D, local.E, local.A, local.B, local.C, R(47) );
    P( local.C, local.D, local.E, local.A, local.B, R(48) );
    P( local.B, local.C, local.D, local.E, local.A, R(49) );
    P( local.A, local.B, local.C, local.D, local.E, R(50) );
    P( local.E, local.A, local.B, local.C, local.D, R(51) );
    P( local.D, local.E, local.A, local.B, local.C, R(52) );
    P( local.C, local.D, local.E, local.A, local.B, R(53) );
    P( local.B, local.C, local.D, local.E, local.A, R(54) );
    P( local.A, local.B, local.C, local.D, local.E, R(55) );
    P( local.E, local.A, local.B, local.C, local.D, R(56) );
    P( local.D, local.E, local.A, local.B, local.C, R(57) );
    P( local.C, local.D, local.E, local.A, local.B, R(58) );
    P( local.B, local.C, local.D, local.E, local.A, R(59) );

#undef K
#undef F

#define F(x,y,z) ((x) ^ (y) ^ (z))
#define K 0xCA62C1D6

    P( local.A, local.B, local.C, local.D, local.E, R(60) );
    P( local.E, local.A, local.B, local.C, local.D, R(61) );
    P( local.D, local.E, local.A, local.B, local.C, R(62) );
    P( local.C, local.D, local.E, local.A, local.B, R(63) );
    P( local.B, local.C, local.D, local.E, local.A, R(64) );
    P( local.A, local.B, local.C, local.D, local.E, R(65) );
    P( local.E, local.A, local.B, local.C, local.D, R(66) );
    P( local.D, local.E, local.A, local.B, local.C, R(67) );
    P( local.C, local.D, local.E, local.A, local.B, R(68) );
    P( local.B, local.C, local.D, local.E, local.A, R(69) );
    P( local.A, local.B, local.C, local.D, local.E, R(70) );
    P( local.E, local.A, local.B, local.C, local.D, R(71) );
    P( local.D, local.E, local.A, local.B, local.C, R(72) );
    P( local.C, local.D, local.E, local.A, local.B, R(73) );
    P( local.B, local.C, local.D, local.E, local.A, R(74) );
    P( local.A, local.B, local.C, local.D, local.E, R(75) );
    P( local.E, local.A, local.B, local.C, local.D, R(76) );
    P( local.D, local.E, local.A, local.B, local.C, R(77) );
    P( local.C, local.D, local.E, local.A, local.B, R(78) );
    P( local.B, local.C, local.D, local.E, local.A, R(79) );

#undef K
#undef F

    ctx->state[0] += local.A;
    ctx->state[1] += local.B;
    ctx->state[2] += local.C;
    ctx->state[3] += local.D;
    ctx->state[4] += local.E;

    /* Zeroise buffers and variables to clear sensitive data from memory. */
    mbedtls_platform_zeroize( &local, sizeof( local ) );

    return( 0 );
}

#endif /* !MBEDTLS_SHA1_PROCESS_ALT */

#endif /* MBEDTLS_SHA1_ALT */

#endif /* MBEDTLS_SHA1_C */
