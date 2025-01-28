#ifndef _ECC_ACTS_H
#define _ECC_ACTS_H

#define BT_P192_PK_SIZE  48
#define BT_P192_DHK_SIZE 24
#define BT_P256_PK_SIZE  64
#define BT_P256_DHK_SIZE 32

enum {
	PENDING_P192_PUB_KEY,
	PENDING_P192_DHKEY,
	PENDING_P256_PUB_KEY,
	PENDING_P256_DHKEY,
	/* Total number of flags - must be at the end of the enum */
	NUM_ECC_FLAGS,
};

struct bt_gen_p192_pk_rsp {
    uint8_t public_key[48];
    uint8_t private_key[24];
} __packed;

struct bt_gen_p192_dhkey_req {
	uint8_t id;
	uint8_t pad[3];
	uint8_t remote_pk[48];
	uint8_t private_key[24];
} __packed;

struct bt_gen_p192_dhkey_rsp {
	uint8_t id;
	uint8_t pad[3];
	uint8_t dhkey[24];
} __packed;

struct bt_gen_p256_pk_rsp {
    uint8_t public_key[64];
    uint8_t private_key[32];
} __packed;

struct bt_gen_p256_dhkey_req {
	uint8_t id;
	uint8_t pad[3];
    uint8_t remote_pk[64];
    uint8_t private_key[32];
} __packed;

struct bt_gen_p256_dhkey_rsp {
	uint8_t id;
	uint8_t pad[3];
    uint8_t dhkey[32];
} __packed;

int ecc_init(void);

void acts_work_submit(struct k_work *work);

int ecc_gen_p192_pk(uint8_t *public_key, uint8_t *private_key);

int ecc_valid_p192_pk(uint8_t *public_key);

int ecc_gen_p192_dhkey(uint8_t *public_key, uint8_t *private_key, uint8_t *dhkey);

int ecc_gen_p256_pk(uint8_t *public_key, uint8_t *private_key);

int ecc_valid_p256_pk(uint8_t *public_key);

int ecc_gen_p256_dhkey(uint8_t *public_key, uint8_t *private_key, uint8_t *dhkey);

#endif
