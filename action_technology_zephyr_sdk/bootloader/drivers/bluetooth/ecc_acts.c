#include <zephyr.h>
#include <sys/atomic.h>
#include <debug/stack.h>
#include <sys/byteorder.h>
#include <device.h>
#include <kernel.h>
#include <soc.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/hmac_prng.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>

#include "ecc_acts.h"

#ifdef CONFIG_SOC_SERIES_LARK_FPGA
#include <acts_bluetooth/crypto.h>
#define ECC_USE_TRNG 0
#else
#define ECC_USE_TRNG 1
#endif

#if defined(CONFIG_BT_USE_DEBUG_KEYS)
/* based on Core Specification 4.2 Vol 3. Part H 2.3.5.6.1 */
static const uint32_t debug_private_key[8] = {
	0xcd3c1abd, 0x5899b8a6, 0xeb40b799, 0x4aff607b, 0xd2103f50, 0x74c9b3e3,
	0xa3c55f38, 0x3f49f6d4
};

static const uint8_t debug_public_key[64] = {
	0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc, 0xdb, 0xfd, 0xf4, 0xac,
	0x11, 0x91, 0xf4, 0xef, 0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
	0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20, 0x8b, 0xd2, 0x89, 0x15,
	0xd0, 0x8e, 0x1c, 0x74, 0x24, 0x30, 0xed, 0x8f, 0xc2, 0x45, 0x63, 0x76,
	0x5c, 0x15, 0x52, 0x5a, 0xbf, 0x9a, 0x32, 0x63, 0x6d, 0xeb, 0x2a, 0x65,
	0x49, 0x9c, 0x80, 0xdc
};
#endif

#if ECC_USE_TRNG
static struct tc_hmac_prng_struct prng;

static void trng_process(uint8_t *seed)
{
	uint32_t trng_low, trng_high;

	se_trng_init();

	se_trng_process(&trng_low, &trng_high);
	sys_put_le32(trng_low, seed);
	sys_put_le32(trng_high, &seed[4]);

	se_trng_deinit();
}

/* 1 for success, 0 for failure */
static int prng_reseed(struct tc_hmac_prng_struct *h)
{
	uint8_t seed[32];
	int64_t extra;
	size_t i;
	int ret;

	for (i = 0; i < (sizeof(seed) / 8); i++) {
		trng_process(&seed[i * 8]);
	}

	extra = k_uptime_get();

	ret = tc_hmac_prng_reseed(h, seed, sizeof(seed), (uint8_t *)&extra, sizeof(extra));
	if (ret == TC_CRYPTO_FAIL) {
		printk("Failed to re-seed PRNG\n");
	}

	return ret;
}

/* 1 for success, 0 for failure */
static int prng_init(void)
{
	int ret;
	uint8_t rand[8];

	trng_process(rand);

	ret = tc_hmac_prng_init(&prng, rand, sizeof(rand));

	if (ret == TC_CRYPTO_FAIL) {
		printk("Failed to initialize PRNG\n");
		return -EIO;
	}

	return prng_reseed(&prng);
}

/* 1 for success, 0 for failure */
static int ecc_rand(void *buf, size_t len)
{
	int ret;

	ret = tc_hmac_prng_generate(buf, len, &prng);
	if (ret == TC_HMAC_PRNG_RESEED_REQ) {
		ret = prng_reseed(&prng);
		if (ret == TC_CRYPTO_FAIL) {
			return ret;
		}

		ret = tc_hmac_prng_generate(buf, len, &prng);
	}

	return ret;
}
#endif

int default_CSPRNG(uint8_t *dst, unsigned int len)
{
#if ECC_USE_TRNG
	return ecc_rand(dst, len);
#else
	return !bt_rand(dst, len);
#endif
}

int ecc_gen_p192_pk(uint8_t *public_key, uint8_t *private_key)
{
	int rc;

	rc = uECC_make_key(public_key, private_key, &curve_secp192r1);
	if (rc == TC_CRYPTO_FAIL) {
		printk("Failed to create ECC public/private pair\n");
		return -EINVAL;
	}

	return 0;
}

/* 0 for valid, otherwise invalid */
int ecc_valid_p192_pk(uint8_t *public_key)
{
	return uECC_valid_public_key(public_key, &curve_secp192r1);
}

int ecc_gen_p192_dhkey(uint8_t *public_key, uint8_t *private_key, uint8_t *dhkey)
{
	int ret;

	ret = uECC_shared_secret(public_key, private_key, dhkey, &curve_secp192r1);

    if (ret == TC_CRYPTO_FAIL) {
        printk("dhkey gen failed (ret %d)\n", ret);
        return -EINVAL;
    }

    return 0;
}

int ecc_gen_p256_pk(uint8_t *public_key, uint8_t *private_key)
{
	int rc;

	rc = uECC_make_key(public_key, private_key, &curve_secp256r1);
	if (rc == TC_CRYPTO_FAIL) {
		printk("Failed to create ECC public/private pair\n");
		return -EINVAL;
	}

	return 0;
}

/* 0 for valid, otherwise invalid */
int ecc_valid_p256_pk(uint8_t *public_key)
{
	return uECC_valid_public_key(public_key, &curve_secp256r1);
}

int ecc_gen_p256_dhkey(uint8_t *public_key, uint8_t *private_key, uint8_t *dhkey)
{
	int ret;

	ret = uECC_shared_secret(public_key, private_key, dhkey, &curve_secp256r1);

    if (ret == TC_CRYPTO_FAIL) {
        printk("dhkey gen failed (ret %d)\n", ret);
        return -EINVAL;
    }

    return 0;
}

static K_THREAD_STACK_DEFINE(acts_ecc_stack, 1024);
static struct k_work_q acts_ecc_work_q;

void acts_work_submit(struct k_work *work)
{
	k_work_submit_to_queue(&acts_ecc_work_q, work);
}

int ecc_init(void)
{
#if ECC_USE_TRNG
	int ret;

	ret = prng_init();
	if (!ret) {
		return -EIO;
	}
#endif

	k_work_queue_start(&acts_ecc_work_q, acts_ecc_stack, K_THREAD_STACK_SIZEOF(acts_ecc_stack), 14, NULL);

    return 0;
}
