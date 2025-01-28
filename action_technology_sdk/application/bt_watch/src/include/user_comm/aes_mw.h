
#ifndef __AES_MW_H__
#define __AES_MW_H__
#include <os_common_api.h>

#include <user_comm/tinycrypt/aes.h>

#define CBC		0
#define ECB		1

enum
{
	AES_COMPLETE,
    AES_INPUT_DATA_INCORRECT,
    AES_SET_ENCRYPT_KEY_FAIL,
	AES_SET_DECRYPT_KEY_FAIL,
    AES_INPUT_OUTPUT_USE_SAME_BUFFER,   
};

typedef struct
{
	struct tc_aes_key_sched_struct key;
	uint8_t iv[16];
	uint8_t mode;
} AES_CTX;

/*!
    @brief  AES CBC mode pkcs7 Encryption Function.
    @param AES key definition Parameter.
    @param AES key 16-bytes.
    @param Want to be encrypt data.
    @param Want to be encrypt data length.
    @param After encryption Data point.
    @param After encryption Data length.
    @return Encryption Status according "AES Event type"(0:AES_COMPLETE).
*/
uint8_t aes_encrypt_pkcs7(AES_CTX *c, const uint8_t key[16], const uint8_t *in, uint8_t in_len, uint8_t *out, int *out_len);

/*!
    @brief  AES CBC mode pkcs7 Decryption Function.
    @param AES key definition Parameter.
    @param AES key 16-bytes.
    @param Want to be decrypt data.
    @param Want to be decrypt data length.
    @param After decryption Data point.
    @param After decryption Data length.
    @return Decryption Status according "AES Event type"(0:6AES_COMPLETE).
*/
uint8_t aes_decrypt_pkcs7(AES_CTX *c, const uint8_t key[16],const uint8_t *in, uint8_t in_len, uint8_t *out, int *out_len);

#endif /* __AES_MW_H__ */

