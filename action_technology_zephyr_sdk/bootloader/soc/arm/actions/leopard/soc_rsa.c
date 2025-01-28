/* rsa.c
**
** Copyright 2012, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of Google Inc. nor the names of its contributors may
**       be used to endorse or promote products derived from this software
**       without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY Google Inc. ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
** EVENT SHALL Google Inc. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <device.h>
#include <soc.h>


#ifndef __ORDER_BIG_ENDIAN__
#define __ORDER_BIG_ENDIAN__ 4321
#endif

#ifndef __ORDER_LITTLE_ENDIAN__
#define __ORDER_LITTLE_ENDIAN__ 1234
#endif




#define KEY_EXPONENT_65537


#define RSANUMBYTES 256           /* 2048 bit key length */
#define RSANUMWORDS (RSANUMBYTES / sizeof(uint32_t))

typedef struct RSAPublicKey {
    uint32_t n0inv;           /* -1 / n[0] mod 2^32 */
    uint32_t n[RSANUMWORDS];  /* modulus as little endian array */
    uint32_t rr[RSANUMWORDS]; /* R^2 as little endian array */
} RSAPublicKey;

#define KEY_LEN                     (4 + 256 + 256)
#define SIG_LEN                     256

#define act_writel  sys_write32
#define act_readl   sys_read32

#define SHA256_DIGEST_SIZE 32


#define CONFIG_SHA256_HARDWARE
#ifdef CONFIG_SHA256_HARDWARE

static __ramfunc unsigned int wait_reg_status(unsigned int reg, unsigned int dat, unsigned int check, unsigned int ms)
{
    unsigned int res, t, t_cycle;
    unsigned int wait_start = sys_read32(T2_CNT);
    res = -1;
	t_cycle = ms*(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC/1000);
    while (1) {
        if((sys_read32(reg) & dat) == check) {
            res = 0;
            break;
        }
		t = sys_read32(T2_CNT)-wait_start;
        if (t > t_cycle) {
            break;
        }
    }
    return res;
}


#define     SHA_MODE_SHA_FIRST                                                5
#define     SHA_MODE_SHA_PADDING                                              4
#define     SHA_MODE_SHA_MODE_e                                               1
#define     SHA_MODE_SHA_MODE_SHIFT                                           0
#define     SHA_MODE_SHA_MODE_MASK                                            (0x3<<0)

#define     SHA_INFIFOCTL_INFIFOIP                                            8
#define     SHA_INFIFOCTL_INFIFO_ERR                                          3
#define     SHA_INFIFOCTL_INFIFO_FULL                                         2
#define     SHA_INFIFOCTL_INFIFODE                                            1
#define     SHA_INFIFOCTL_INFIFOIE                                            0


#define     SHA_CTRL_SHA_END                        31
#define     SHA_CTRL_SHA_LEN_ERROR                  30
#define     SHA_CTRL_SHA_GATE_EN                    4
#define     SHA_CTRL_SHA_CLK_EN                     3
#define     SHA_CTRL_SHA_INT_EN                     2
#define     SHA_CTRL_SHA_RESET                      1
#define     SHA_CTRL_SHA_EN                         0

#define     CMU_DEVCLKEN0_SECLKEN                   16
#define		MRCR0_SEC_RESET  						16

#define SHA_MODE_ALG(x)		((x) << SHA_MODE_SHA_MODE_SHIFT)
#define SHA_MODE_ALG_SHA1	SHA_MODE_ALG(0)
#define SHA_MODE_ALG_SHA256	SHA_MODE_ALG(1)
#define SHA_MODE_ALG_SHA224	SHA_MODE_ALG(2)
#define SHA_MODE_ALG_MASK	SHA_MODE_ALG(3)

#define     RMU_BASE                         0x40000000
#define     MRCR0                            (RMU_BASE+0x00000000)
#define SHA_CTRL          (SE_REG_BASE+0x0200)
#define SHA_MODE          (SE_REG_BASE+0x0204)
#define SHA_LEN           (SE_REG_BASE+0x0208)
#define SHA_INFIFOCTL     (SE_REG_BASE+0x020c)
#define SHA_INFIFO        (SE_REG_BASE+0x0210)
#define SHA_DATAOUT       (SE_REG_BASE+0x0214)
#define SHA_TOTAL_LEN     (SE_REG_BASE+0x0218)

static __ramfunc void se_sha_clk_init(void)
{
	/* enable SE clock & clear SE reset */
	/* enable SE clock & clear SE reset */
	act_writel(act_readl(CMU_DEVCLKEN0) | (1 << CMU_DEVCLKEN0_SECLKEN), CMU_DEVCLKEN0);
	act_writel(act_readl(MRCR0) | (1 << MRCR0_SEC_RESET), MRCR0);
	/* clock source: 24M HOSC */
	act_writel(0, CMU_SECCLK);
	//act_writel(0x102, CMU_SECCLK); // 0x100 = corepll/1, 0x101 = corepll/2  0x102 = corepll/4   0x103 = corepll/8
}

static __ramfunc int se_sha256_init(void)
{
	int err;

	se_sha_clk_init();

	/* enable SHA clock */
	act_writel(1 << SHA_CTRL_SHA_CLK_EN , SHA_CTRL);

	/* Se fifo select */
	act_writel(2, SE_FIFOCTRL);
	
	/* clear DRQ */
	act_writel(0, SHA_INFIFOCTL);
	
	/* reset SHA controller */
	act_writel(act_readl(SHA_CTRL) | (1 << SHA_CTRL_SHA_RESET), SHA_CTRL);

	/* wait reset finished, timeout: 10ms */
	err = wait_reg_status(SHA_CTRL, 1 << SHA_CTRL_SHA_RESET, 0, 10);
	if (err) {
		return -1;
	}

	/* must set to 0 before clear SHA_MODE_PADDING_HW */
	act_writel(0, SHA_LEN);

	/* start data process */
	act_writel((1 << SHA_MODE_SHA_FIRST) | SHA_MODE_ALG_SHA256, SHA_MODE);
	
	return 0;
}

#define     DMA1CTL              (DMA_REG_BASE+0x00000200)
#define     DMA1START            (DMA_REG_BASE+0x00000204)
#define     DMA1SADDR0           (DMA_REG_BASE+0x00000208)
#define     DMA1SADDR1           (DMA_REG_BASE+0x0000020c)
#define     DMA1DADDR0           (DMA_REG_BASE+0x00000210)
#define     DMA1DADDR1           (DMA_REG_BASE+0x00000214)
#define     DMA1BC               (DMA_REG_BASE+0x00000218)
#define     DMA1RC               (DMA_REG_BASE+0x0000021c)

#define   DMA_MAX_LEN	0x80000
static __ramfunc int se_sha256_update(const void* data, int len, int is_last)
{
	int err;
	uint32_t tlen, buf_addr;

	if (!data || !len)
		return -1;

	if (is_last) {
		act_writel(act_readl(SHA_MODE) | (1 << SHA_MODE_SHA_PADDING), SHA_MODE);
	}

	act_writel(len, SHA_LEN);

	/* enable DRQ */
	act_writel(act_readl(SHA_INFIFOCTL) | (1 << SHA_INFIFOCTL_INFIFODE) , SHA_INFIFOCTL);
	
	/* start data process */
	act_writel(act_readl(SHA_CTRL) | (1 << SHA_CTRL_SHA_EN) , SHA_CTRL);

	/* config dma */

	act_writel(0x9100, DMA1CTL);
	act_writel(SHA_INFIFO, DMA1DADDR0);
	buf_addr = (unsigned int)data;
	tlen = len;
	while(tlen){
		act_writel(buf_addr, DMA1SADDR0);
		if(tlen < DMA_MAX_LEN) {
			act_writel(tlen, DMA1BC);
			act_writel(1, DMA1START);
			break;
		}else{
			act_writel(DMA_MAX_LEN, DMA1BC);
			act_writel(1, DMA1START);
			buf_addr += DMA_MAX_LEN;
			tlen -= DMA_MAX_LEN;
			while(act_readl(DMA1START)&0x01){
			}
		}
	}

	/* wait sha caculate end */
	err = wait_reg_status(SHA_CTRL, 1 << SHA_CTRL_SHA_END, 1 << SHA_CTRL_SHA_END, 100);
	if (err) {
	    act_writel(0, DMA1START);	       
		goto exit;
	}
exit:
	/* clear SHA pending, ensure controller is disable */
	act_writel(act_readl(SHA_CTRL) & ~(1 << SHA_CTRL_SHA_EN), SHA_CTRL);
	
	/* clear first flag */
	if (act_readl(SHA_MODE) & (1 << SHA_MODE_SHA_FIRST)) {
		act_writel(act_readl(SHA_MODE) & ~(1 << SHA_MODE_SHA_FIRST), SHA_MODE);
	}

	return err;
}

static __ramfunc void sys_put_le16(uint16_t val, uint8_t dst[2])
{
	dst[0] = val;
	dst[1] = val >> 8;
}

/**
 *  @brief Put a 32-bit integer as little-endian to arbitrary location.
 *
 *  Put a 32-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in little-endian format.
 *
 *  @param val 32-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static __ramfunc void sys_put_le32(uint32_t val, uint8_t dst[4])
{
	sys_put_le16(val, dst);
	sys_put_le16(val >> 16, &dst[2]);
}

static __ramfunc int se_sha256_final(unsigned char output[32])
{
	unsigned int w;
	int i;

	if (!output)
		return -1;

	/* read sha values */
	for (i = 0; i < SHA256_DIGEST_SIZE / 4; i++) {
		w = act_readl(SHA_DATAOUT);
		sys_put_le32(w, &output[i * 4]);
	}

	return 0;
}

__ramfunc const uint8_t* SHA256_hash(const void *input, int len,
             uint8_t* digest)
{
	if (!input || ((unsigned int)input & 0x3)){
		return NULL;
	}
	se_sha256_init();
	se_sha256_update(input, len, 1);
	se_sha256_final(digest);
	return digest;
}



#else


struct HASH_CTX;  // forward decl

typedef struct HASH_VTAB {
  void (* const init)(struct HASH_CTX*);
  void (* const update)(struct HASH_CTX*, const void*, int);
  const uint8_t* (* const final)(struct HASH_CTX*);
  const uint8_t* (* const hash)(const void*, int, uint8_t*);
  int size;
} HASH_VTAB;


typedef struct HASH_CTX {
  HASH_VTAB * f;
  uint64_t count;
  union {
      uint8_t buf[64];
      uint32_t buf32[64 / 4];
  };
  uint32_t state[8];  // upto SHA2
} HASH_CTX;

#define HASH_init(ctx) (ctx)->f->init(ctx)
#define HASH_update(ctx, data, len) (ctx)->f->update(ctx, data, len)
#define HASH_final(ctx) (ctx)->f->final(ctx)
#define HASH_hash(data, len, digest) (ctx)->f->hash(data, len, digest)
#define HASH_size(ctx) (ctx)->f->size


typedef HASH_CTX SHA256_CTX;
void SHA256_init(SHA256_CTX* ctx);
void SHA256_update(SHA256_CTX* ctx, const void* data, int len);
const uint8_t* SHA256_final(SHA256_CTX* ctx);
// Convenience method. Returns digest address.
const uint8_t* SHA256_hash(const void* data, int len, uint8_t* digest);
uint32_t __REV(uint32_t value);
void modpow(void *key_param, void *buffer);

#define ror(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))
#define shr(value, bits) ((value) >> (bits))

uint32_t sha_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 
  };

static __ramfunc void SHA256_Transform(SHA256_CTX* ctx) {
    uint32_t W[64];
    uint32_t A, B, C, D, E, F, G, H;
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    uint32_t* p32 = ctx->buf32;
#else
    uint8_t* p = ctx->buf;
#endif
    int t;

    for(t = 0; t < 16; ++t) {
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        W[t] = __REV(*p32++);
#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
        W[t] = *p32++;
#else
        uint32_t tmp =  *p++ << 24;
        tmp |= *p++ << 16;
        tmp |= *p++ << 8;
        tmp |= *p++;
        W[t] = tmp;
#endif
    }

    for(; t < 64; t++) {
        uint32_t s0 = ror(W[t-15], 7) ^ ror(W[t-15], 18) ^ shr(W[t-15], 3);
        uint32_t s1 = ror(W[t-2], 17) ^ ror(W[t-2], 19) ^ shr(W[t-2], 10);
        W[t] = W[t-16] + s0 + W[t-7] + s1;
    }

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];
    F = ctx->state[5];
    G = ctx->state[6];
    H = ctx->state[7];

    for(t = 0; t < 64; t++) {
        uint32_t s0 = ror(A, 2) ^ ror(A, 13) ^ ror(A, 22);
        uint32_t maj = (A & B) ^ (A & C) ^ (B & C);
        uint32_t t2 = s0 + maj;
        uint32_t s1 = ror(E, 6) ^ ror(E, 11) ^ ror(E, 25);
        uint32_t ch = (E & F) ^ ((~E) & G);
        uint32_t t1 = H + s1 + ch + sha_K[t] + W[t];

        H = G;
        G = F;
        F = E;
        E = D + t1;
        D = C;
        C = B;
        B = A;
        A = t1 + t2;
    }

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
    ctx->state[4] += E;
    ctx->state[5] += F;
    ctx->state[6] += G;
    ctx->state[7] += H;
}

static  HASH_VTAB SHA256_VTAB = {
    SHA256_init,
    SHA256_update,
    SHA256_final,
    SHA256_hash,
    SHA256_DIGEST_SIZE
};

 __ramfunc void SHA256_init(SHA256_CTX* ctx) {
    ctx->f = &SHA256_VTAB;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}


 __ramfunc void SHA256_update(SHA256_CTX* ctx,  const void* data, int len) {
    int i = (int) (ctx->count & 63);
    const uint8_t* p = (const uint8_t*)data;

    ctx->count += len;

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    if (len >= 4 && (i & 0x3) == 0 && ((uint32_t)p & 0x3) == 0) {
        const uint32_t *p32 = (const uint32_t *)p;
        int k = i / 4;
        while (len >= 4) {
            len -= 4;
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
            ctx->buf32[k++] = *p32++;
#else
            ctx->buf32[k++] = __REV(*p32++);
#endif
            if (k == 64 / 4) {
                SHA256_Transform(ctx);
                k = 0;
            }
        }
        i = k * 4;
        p = (const uint8_t *)p32;
    }
#endif

    while (len--) {
        ctx->buf[i++] = *p++;
        if (i == 64) {
            SHA256_Transform(ctx);
            i = 0;
        }
    }
}


  __ramfunc const uint8_t* SHA256_final(SHA256_CTX* ctx) {
    uint8_t *p = ctx->buf;
    uint64_t cnt = ctx->count * 8;
    int i;
	uint8_t buf[2];
	buf[0] = '\x80';
	buf[1] = 0;

    SHA256_update(ctx, buf, 1);
    while ((ctx->count & 63) != 56) {
		buf[0] = '\0';
        SHA256_update(ctx, buf, 1);
    }
    for (i = 0; i < 8; ++i) {
        uint8_t tmp = (uint8_t) (cnt >> ((7 - i) * 8));//64bit ³ý·¨ÓÃµ½ÁËlibÖÐµÄ__aeabi_llsr
        SHA256_update(ctx, &tmp, 1);
    }

    for (i = 0; i < 8; i++) {
        uint32_t tmp = ctx->state[i];
        *p++ = tmp >> 24;
        *p++ = tmp >> 16;
        *p++ = tmp >> 8;
        *p++ = tmp >> 0;
    }

    return ctx->buf;
}

/* Convenience function */
__ramfunc const uint8_t* SHA256_hash(const void* data, int len, uint8_t* digest) {
    SHA256_CTX ctx;
	int i;
    SHA256_init(&ctx);
    SHA256_update(&ctx, data, len);
    //memcpy(digest, SHA256_final(&ctx), SHA256_DIGEST_SIZE);
    SHA256_final(&ctx);
    for(i = 0; i < SHA256_DIGEST_SIZE; i++)
		digest[i] = ctx.buf[i];
    return digest;
}

__ramfunc uint32_t __REV(uint32_t value)
{
	//return __builtin_bswap32(value);
	return ((uint32_t) ((((value) >> 24) & 0xff) | \
				   (((value) >> 8) & 0xff00) | \
				   (((value) & 0xff00) << 8) | \
				   (((value) & 0xff) << 24)));
}

#endif // sha256 config


// SHA-256 of PKCS1.5 signature sha256_padding for 2048 bit, as above.
// At the location of the bytes of the hash all 00 are hashed.
static  uint8_t kExpectedPadSha256Rsa2048[SHA256_DIGEST_SIZE] = {
    0xab, 0x28, 0x8d, 0x8a, 0xd7, 0xd9, 0x59, 0x92,
    0xba, 0xcc, 0xf8, 0x67, 0x20, 0xe1, 0x15, 0x2e,
    0x39, 0x8d, 0x80, 0x36, 0xd6, 0x6f, 0xf0, 0xfd,
    0x90, 0xe8, 0x7d, 0x8b, 0xe1, 0x7c, 0x87, 0x59,
};


void modpow(void *key_param, void *buffer);
// Verify a 2048-bit RSA PKCS1.5 signature against an expected hash.
//
// Returns 1 on successful verification, 0 on failure.
__ramfunc int RSA_verify(const RSAPublicKey *key,
               const uint8_t *signature,
               const uint32_t len,
               const uint8_t *msg,
               const uint32_t msg_len) {
    uint8_t buf[RSANUMBYTES];
    int i, j;
    const uint8_t* padding_hash;
    unsigned char hash[SHA256_DIGEST_SIZE];
    unsigned int hash_len = sizeof(hash);

    if (key == NULL || signature == NULL || msg == NULL) {
        return 0;
    }

    if (msg_len == 0) {
        return 0;
    }

    if (len != sizeof(buf)) {
        return 0;  // Wrong input length.
    }

    for (i = 0; i < len; ++i) {  // Copy input to local workspace.
        buf[i] = signature[i];
    }

    // In-place exponentiation.
    modpow((void *)key, buf);

    SHA256_hash(msg, msg_len, hash);//caclulate_image_hash


    // Xor sha portion, so it all becomes 00 iff equal.
    for (i = len - hash_len, j = 0; i < len; ++i, ++j) {
        buf[i] ^= hash[j];
    }

    // Hash resulting buf, in-place.
    switch (hash_len) {
        case SHA256_DIGEST_SIZE:
            padding_hash = kExpectedPadSha256Rsa2048;
            SHA256_hash(buf, len, buf);
            break;
        default:
            return 0;
    }

    // Compare against expected hash value.
    for (i = 0; i < hash_len; ++i) {
        if (buf[i] != padding_hash[i]) {
            return 0;
        }
    }

    return 1;  // All checked out OK.
}


// a[] -= mod
static __ramfunc  void subM(const RSAPublicKey* key,
                 uint32_t* a) {
    int64_t A = 0;
    int i;
    for (i = 0; i < RSANUMWORDS; ++i) {
        A += (uint64_t)a[i] - key->n[i];
        a[i] = (uint32_t)A;
        A >>= 32;
    }
}

// return a[] >= mod
static __ramfunc  int geM(const RSAPublicKey* key,
               const uint32_t* a) {
    int i;
    for (i = RSANUMWORDS; i;) {
        --i;
        if (a[i] < key->n[i]) return 0;
        if (a[i] > key->n[i]) return 1;
    }
    return 1;  // equal
}

// montgomery c[] += a * b[] / R % mod
static __ramfunc  void montMulAdd(const RSAPublicKey* key,
                       uint32_t* c,
                       const uint32_t a,
                       const uint32_t* b) {
    uint64_t A = (uint64_t)a * b[0] + c[0];
    uint32_t d0 = (uint32_t)A * key->n0inv;
    uint64_t B = (uint64_t)d0 * key->n[0] + (uint32_t)A;
    int i;

    for (i = 1; i < RSANUMWORDS; ++i) {
        A = (A >> 32) + (uint64_t)a * b[i] + c[i];
        B = (B >> 32) + (uint64_t)d0 * key->n[i] + (uint32_t)A;
        c[i - 1] = (uint32_t)B;
    }

    A = (A >> 32) + (B >> 32);

    c[i - 1] = (uint32_t)A;

    if (A >> 32) {
        subM(key, c);
    }
}

// montgomery c[] = a[] * b[] / R % mod
static __ramfunc  void montMul(const RSAPublicKey* key,
                    uint32_t* c,
                    const uint32_t* a,
                    const uint32_t* b) {
    int i;
    for (i = 0; i < RSANUMWORDS; ++i) {
        c[i] = 0;
    }
    for (i = 0; i < RSANUMWORDS; ++i) {
        montMulAdd(key, c, a[i], b);
    }
}

// In-place public exponentiation.
// Input and output big-endian byte array in inout.
__ramfunc  void  modpow(void *key_param, void *buffer) {
    uint32_t a[RSANUMWORDS];
    uint32_t aR[RSANUMWORDS];
    uint32_t aaR[RSANUMWORDS];
    uint32_t* aaa = 0;
    int i;

    RSAPublicKey* key = (RSAPublicKey*)key_param;

    uint8_t* inout =(uint8_t *)buffer;

    // Convert from big endian byte array to little endian word array.
    for (i = 0; i < RSANUMWORDS; ++i) {
        uint32_t tmp =
            (inout[((RSANUMWORDS - 1 - i) * 4) + 0] << 24) |
            (inout[((RSANUMWORDS - 1 - i) * 4) + 1] << 16) |
            (inout[((RSANUMWORDS - 1 - i) * 4) + 2] << 8) |
            (inout[((RSANUMWORDS - 1 - i) * 4) + 3] << 0);
        a[i] = tmp;
    }

#ifdef KEY_EXPONENT_65537
        aaa = aaR;  // Re-use location.
        montMul(key, aR, a, key->rr);  // aR = a * RR / R mod M
        for (i = 0; i < 16; i += 2) {
            montMul(key, aaR, aR, aR);  // aaR = aR * aR / R mod M
            montMul(key, aR, aaR, aaR);  // aR = aaR * aaR / R mod M
        }
        montMul(key, aaa, aR, a);  // aaa = aR * a / R mod M
#elif defined(KEY_EXPONENT_3)
        aaa = aR;  // Re-use location.
        montMul(key, aR, a, key->rr);  /* aR = a * RR / R mod M   */
        montMul(key, aaR, aR, aR);     /* aaR = aR * aR / R mod M */
        montMul(key, aaa, aaR, a);     /* aaa = aaR * a / R mod M */
#else
#error "No valid KEY_EXPONENT_XXX defined"
#endif

    // Make sure aaa < mod; aaa is at most 1x mod too large.
    if (geM(key, aaa)) {
        subM(key, aaa);
    }

    // Convert to bigendian byte array
    for (i = RSANUMWORDS - 1; i >= 0; --i) {
        uint32_t tmp = aaa[i];
        *inout++ = tmp >> 24;
        *inout++ = tmp >> 16;
        *inout++ = tmp >> 8;
        *inout++ = tmp >> 0;
    }
}


__ramfunc int verify_signature(const unsigned char *key, const unsigned char *sig, const unsigned char *data, unsigned int len)
{
    int ret;

    ret = RSA_verify((const RSAPublicKey *)key, sig, SIG_LEN, data, len);
    if (ret == 0) {
        return 1;
    }
    return 0;
}
