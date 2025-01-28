#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(__GNUC__) && !defined(__clang__)
#  define RLE_FORCE_O3  __attribute__((optimize("O3")))
#else
#  define RLE_FORCE_O3
#endif

#define ENC_REPEAT_COUNT 3
#define MAX_SIZE 4

static inline int get_repetition_count(const uint8_t * src, size_t count, size_t size, int line_length)
{
	uint8_t rep_val[MAX_SIZE];
	memcpy(rep_val, src, size);
	src += size;
	count--;

	int length = 1;
	while (count > 0 && length < line_length) {
		uint8_t val[MAX_SIZE];
		memcpy(val, src, size);

		if (memcmp(val, rep_val, size) != 0)
			break;

		src += size;
		count--;
		length++;
	}

	return length;
}

static int get_non_repetition_count(const uint8_t * src, size_t count, size_t size, int line_length)
{
	if (count < 2)
		return count;

	uint8_t v1[MAX_SIZE];
	uint8_t v2[MAX_SIZE];
	memcpy(v1, src, size);
	memcpy(v2, src + size, size);
	src += size * 2;
	count -= 2;

	if (memcmp(v1, v2, size) == 0)
		return 1;

	int length = 1;

	while (count > 0 && length < line_length) {
		uint8_t v3[MAX_SIZE];
		memcpy(v3, src, size);

		if (memcmp(v2, v3, size) == 0)
			break;

		memcpy(v2, v3, size);
		src += size;
		count--;
		length++;
	}

	if (count == 0 && length < line_length)
		length++;

	return length;
}

/**
 * @brief RLE encode
 *
 * @param out_buf pointer to encoded output buffer
 * @param out_size size of output buffer in bytes
 * @param in_buf pointer to input buffer
 * @param in_count size of input buffer in elements
 * @param size size of each element in bytes
 *
 * @retval number of bytes actually encoded if out_buf is not NULL
 * @retval total encoded bytes if out_buf is NULL
 */
int rle_compress(const uint8_t* in_buf, uint8_t * out_buf,  size_t in_count, size_t out_size, size_t size)
{
	int enc_size = 0;

	while (in_count > 0) {
		int count = get_repetition_count(in_buf, in_count, size, 127);
		int copy_len, sign;

		if (count >= ENC_REPEAT_COUNT) {
			copy_len = size;
			sign = 0x80;
		} else {
			count = get_non_repetition_count(in_buf, in_count, size, 127);
			copy_len = size * count;
			sign = 0x00;
		}

		if (out_buf != NULL) {
			if (out_size < copy_len + 1)
				break;

			out_buf[0] = sign | count;
			memcpy(&out_buf[1], in_buf, copy_len);
			out_buf += copy_len + 1;
			out_size -= copy_len + 1;
		}

		enc_size += copy_len + 1;

		in_buf += size * count;
		in_count -= count;
	}

	return enc_size;
}

RLE_FORCE_O3
static inline void fill_repetition(uint8_t * dest, const uint8_t * src, size_t count, size_t size)
{
	if (size == 1) {
		memset(dest, src[0], count);
	} else if (size == 2) {
		if (((uintptr_t)dest & 0x1) == 0) {
			uint16_t val = src[0] | ((uint16_t)src[1] << 8);

			for (int i = count; i > 0; i--) {
				*(uint16_t *)dest = val;
				dest += size;
			}
		} else {
			for (int i = count; i > 0; i--) {
				dest[0] = src[0];
				dest[1] = src[1];
				dest += size;
			}
		}
	} else if (size == 4) {
		if (((uintptr_t)dest & 0x3) == 0) {
			uint32_t val = src[0] | ((uint32_t)src[1] << 8) |
					((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);

			for (int i = count; i > 0; i--) {
				*(uint32_t *)dest = val;
				dest += size;
			}
		} else {
			for (int i = count; i > 0; i--) {
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
				dest[3] = src[3];
				dest += size;
			}
		}
	} else if (size == 3) {
		for (int i = count; i > 0; i--) {
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
			dest += size;
		}
	} else { /* common case */
		for (int i = count; i > 0; i--) {
			memcpy(dest, src, size);
			dest += size;
		}
	}
}

/**
 * @brief RLE decode
 *
 * @param out_buf pointer to output buffer
 * @param out_count size of output buffer in elements
 * @param in_buf pointer to encoded input buffer
 * @param in_size size of input buffer in bytes
 * @param size size of each element in bytes
 *
 * @retval number of bytes actually decoded if out_buf is not NULL
 * @retval total decoded bytes if out_buf is NULL
 */
RLE_FORCE_O3
int rle_decompress(const uint8_t * in_buf, uint8_t * out_buf, int in_size, int out_count, size_t size)
{
	int dec_size = 0;

	if (out_buf == NULL)
		out_count = INT32_MAX;

	while (in_size > 0 && out_count > 0) {
		uint8_t sign = in_buf[0];
		int count = sign & 0x7F;

		if (count > out_count)
			count = out_count;

		out_count -= count;
		dec_size += size * count;

		if ((sign & 0x80) != 0) {
			if (out_buf != NULL) {
				fill_repetition(out_buf, &in_buf[1], count, size);
				out_buf += size * count;
			}

			in_buf += (1 + size);
			in_size -= (1 + size);
		} else {
			if (out_buf != NULL) {
				memcpy(out_buf, &in_buf[1], size * count);
				out_buf += size * count;
			}

			in_buf += (1 + size * count);
			in_size -= (1 + size * count);
		}
	}

	return dec_size;
}
