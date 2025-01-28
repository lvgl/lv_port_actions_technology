/* keys.h - Bluetooth key handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum {
	BT_KEYS_SLAVE_LTK      = BIT(0),
	BT_KEYS_IRK            = BIT(1),
	BT_KEYS_LTK            = BIT(2),
	BT_KEYS_LOCAL_CSRK     = BIT(3),
	BT_KEYS_REMOTE_CSRK    = BIT(4),
	BT_KEYS_LTK_P256       = BIT(5),

	BT_KEYS_ALL            = (BT_KEYS_SLAVE_LTK | BT_KEYS_IRK | \
				  BT_KEYS_LTK | BT_KEYS_LOCAL_CSRK | \
				  BT_KEYS_REMOTE_CSRK | BT_KEYS_LTK_P256),
};

enum {
	BT_KEYS_ID_PENDING_ADD  = BIT(0),
	BT_KEYS_ID_PENDING_DEL  = BIT(1),
	BT_KEYS_ID_ADDED        = BIT(2),
};

enum {
	BT_KEYS_AUTHENTICATED   = BIT(0),
	BT_KEYS_DEBUG           = BIT(1),
	/* Bit 2 and 3 might accidentally exist in old stored keys */
	BT_KEYS_SC              = BIT(4),
};

struct bt_ltk {
	uint8_t                    rand[8];
	uint8_t                    ediv[2];
	uint8_t                    val[16];
};

struct bt_irk {
	uint8_t                    val[16];
	bt_addr_t               rpa;
};

struct bt_csrk {
	uint8_t                    val[16];
	uint32_t                   cnt;
};

struct bt_keys {
	uint8_t                    id;
	bt_addr_le_t            addr;
	uint8_t                    state;
	uint8_t                    storage_start[0] __aligned(sizeof(void *));
	uint8_t                    enc_size;
	uint8_t                    flags;
	uint16_t                   keys;
	struct bt_ltk           ltk;
	struct bt_irk           irk;
#if defined(CONFIG_BT_SIGNING)
	struct bt_csrk          local_csrk;
	struct bt_csrk          remote_csrk;
#endif /* BT_SIGNING */
#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	struct bt_ltk           slave_ltk;
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */
//#if (defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST))		/* Build lib keep this member */
	uint32_t                   aging_counter;
//#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
};

void bt_keys_foreach(int type, void (*func)(struct bt_keys *keys, void *data),
		     void *data);

struct bt_keys *bt_keys_get_addr(uint8_t id, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_get_type(int type, uint8_t id, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find(int type, uint8_t id, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find_irk(uint8_t id, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find_addr(uint8_t id, const bt_addr_le_t *addr);

void bt_keys_add_type(struct bt_keys *keys, int type);
void bt_keys_clear(struct bt_keys *keys);
int bt_keys_store(struct bt_keys *keys);

enum {
	BT_LINK_KEY_AUTHENTICATED  = BIT(0),
	BT_LINK_KEY_DEBUG          = BIT(1),
	BT_LINK_KEY_SC             = BIT(2),
	BT_LINK_KEY_REPLY          = BIT(3),
};

struct bt_keys_link_key {
	bt_addr_t               addr;
	uint8_t                 storage_start[0]  __aligned(sizeof(void *));
	uint8_t                 flags;
	uint8_t                 val[16];
//#if (defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST))		/* Build lib keep this member */
	uint32_t                aging_counter;
//#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
};

/* This function is used to signal that the key has been used for paring */
/* It updates the aging counter and saves it to flash if configuration option */
/* BT_KEYS_SAVE_AGING_COUNTER_ON_PAIRING is enabled */

/* Actions add start */
/* BR linkkey manager */
void acts_bt_keys_link_key_clear(struct bt_keys_link_key *link_key);
void acts_bt_le_unpair_clear_public_addr(const bt_addr_t *addr);
struct bt_keys_link_key *acts_bt_keys_get_link_key(const bt_addr_t *addr);
struct bt_keys_link_key *acts_bt_keys_find_link_key_store(const bt_addr_t *addr);
void acts_bt_keys_link_key_store(struct bt_keys_link_key *link_key, bool valid_key, bool bonding);
void acts_bt_keys_link_key_clear_store(const bt_addr_t *addr);
void acts_bt_keys_link_key_update_usage(const bt_addr_t *addr);
uint8_t *acts_get_smp_derive_br_linkkey(const bt_addr_t *addr);

/* BLE LTK manager */
void bt_keys_load(void);
void bt_keys_update_usage(uint8_t id, const bt_addr_le_t *addr);
int acts_le_keys_find_link_key(const bt_addr_le_t *addr);
uint8_t acts_le_keys_get_link_key_num(void);
void acts_le_keys_clear_link_key(const bt_addr_le_t *addr);
/* Actions add end */
