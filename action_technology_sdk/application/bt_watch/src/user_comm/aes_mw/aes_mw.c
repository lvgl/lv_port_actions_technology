

#include <stdlib.h>
#include <string.h>
#include <user_comm/aes_mw.h>

static void cbc128_encrypt(const unsigned char *in, unsigned char *out,
			size_t len, const TCAesKeySched_t key,
			unsigned char ivec[16])
{
	size_t n;
	const unsigned char *iv = ivec;

	while (len) {
		for(n=0; n<16 && n<len; ++n)
			out[n] = in[n] ^ iv[n];
		for(; n<16; ++n)
			out[n] = iv[n];
		//AES_encrypt(out, out, key);
		tc_aes_encrypt(out, out, key);
		iv = out;
		if (len<=16) break;
		len -= 16;
		in  += 16;
		out += 16;
	}
	memcpy(ivec,iv,16);
}

static void cbc128_decrypt(const unsigned char *in, unsigned char *out,
			size_t len, const TCAesKeySched_t key,
			unsigned char ivec[16])
{
	size_t n;
	unsigned char tmp[16];

	while (len) {
		unsigned char c;
		//log_error("addr0xbfd876d0 0x%x",*((int *)0xbfd876d0));
		//AES_decrypt(in, tmp, key);
		tc_aes_decrypt(tmp, in, key);
		//log_error("addr0xbfd876d0 0x%x",*((int *)0xbfd876d0));
		for(n=0; n<16 && n<len; ++n) {
			c = in[n];
			out[n] = tmp[n] ^ ivec[n];
			ivec[n] = c;
		}
		//log_error("addr0xbfd876d0 0x%x",*((int *)0xbfd876d0));
		if (len<=16) {
			for (; n<16; ++n)
				ivec[n] = in[n];
			break;
		}
		//log_error("addr0xbfd876d0 0x%x",*((int *)0xbfd876d0));
		len -= 16;
		in  += 16;
		out += 16;
	}
}

uint8_t aes_encrypt_pkcs7(AES_CTX *c, const uint8_t key[16], const uint8_t *in, uint8_t in_len, uint8_t *out, int *out_len)
{
	uint8_t aes_counter = 0;
	uint8_t offset = 0;
	//uint8_t *cbc_tmp = (uint8_t *)zalloc(16+TC_AES_BLOCK_SIZE);
	
	tc_aes128_set_encrypt_key(&c->key, key);
	
	while(in_len>=16)
	{
		if (ECB == c->mode)
			//AES_encrypt(in+offset, out+offset, &c->key);
			tc_aes_encrypt(out+offset, in+offset, &c->key);
		else
		{
			cbc128_encrypt(in+offset, out+offset, 16, &c->key, c->iv);
			//tc_cbc_mode_encrypt(cbc_tmp, 16+TC_AES_BLOCK_SIZE, in+offset, 16, c->iv, &c->key);
			//memcpy(out+offset, cbc_tmp+TC_AES_BLOCK_SIZE, 16);
		}

		offset += 16;
		in_len -= 16;
		aes_counter++;
	}
	
	*out_len = (aes_counter+1)*16;

	//pkcs7 padding
	uint8_t padding_len = 16 - (in_len % 16);

	memcpy(out+offset, in+offset, in_len);
	
	memset(out+offset+in_len, padding_len, padding_len);

	if (ECB == c->mode)
		//AES_encrypt(in+offset, out+offset, &c->key);
		tc_aes_encrypt(out+offset, in+offset, &c->key);
	else
	{
		cbc128_encrypt(out+offset, out+offset, 16, &c->key, c->iv);
		//tc_cbc_mode_encrypt(cbc_tmp, 16+TC_AES_BLOCK_SIZE, in+offset, 16, c->iv, &c->key);
		//memcpy(out+offset, cbc_tmp+TC_AES_BLOCK_SIZE, 16);
	}
		
	//free(cbc_tmp);
	
	return AES_COMPLETE;
}

uint8_t aes_decrypt_pkcs7(AES_CTX *c, const uint8_t key[16],const uint8_t *in, uint8_t in_len, uint8_t *out, int *out_len)
{
	uint8_t aes_counter = 0;
	uint8_t offset = 0;
	//uint8_t *cbc_tmp = (uint8_t *)zalloc(16+TC_AES_BLOCK_SIZE);
	//AES_set_decrypt_key(key, 128, &c->key);
	tc_aes128_set_decrypt_key(&c->key, key);
	while(in_len > 16) 
	{
		if (ECB == c->mode)
			tc_aes_decrypt(out+offset, in+offset, &c->key);
		else
		{
			cbc128_decrypt(in+offset, out+offset, 16, &c->key, c->iv);
			//tc_cbc_mode_decrypt(out+offset, 16, in+offset, 16+TC_AES_BLOCK_SIZE, c->iv, &c->key);
			//memcpy(out+offset, cbc_tmp+TC_AES_BLOCK_SIZE, 16);
		}
		offset += 16;
		in_len -= 16;
		aes_counter++;
	}
	//Last Time
	if (ECB == c->mode)
		tc_aes_decrypt(out+offset, in+offset, &c->key);
	else
 	{
		cbc128_decrypt(in+offset, out+offset, 16, &c->key, c->iv);
		//tc_cbc_mode_decrypt(out+offset, 16, in+offset, 16+TC_AES_BLOCK_SIZE, c->iv, &c->key);
		//memcpy(out+offset, cbc_tmp+TC_AES_BLOCK_SIZE, 16);
	}
	
	int padding_len = *(out+offset+15);
	
	if (padding_len < 1 || padding_len > 16)
	{
		//free(cbc_tmp);
		SYS_LOG_ERR("padding_len 0x%x",padding_len);
		return 3;
	}
		
	*out_len = (aes_counter+1)*16 - padding_len;
	
	//free(cbc_tmp);

	return AES_COMPLETE;
}
