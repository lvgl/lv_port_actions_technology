#include "spress.h"

static spress_header_t spress_hdr = {
	.magic = SPRESS_HEADER_MAGIC,
	.major_version = 0,
	.minor_version = 1,
	.file_hdr_sz = sizeof(spress_header_t),
	.chunk_hdr_sz = sizeof(chunk_header_t),
	.chunk_sz = 0,
	.compress_sz = 0,
	.decompress_sz = 0,
};

size_t spr_size_decompressed(const char *source)
{
	spress_header_t* hdr = (spress_header_t*)source;
	return hdr->decompress_sz;
}

size_t spr_size_compressed(const char *source)
{
	spress_header_t* hdr = (spress_header_t*)source;
	return hdr->compress_sz;
}

static char* spr_split_next(const char* src, size_t size, uint32_t* praw) {
	uint32_t raw = 0;
	uint32_t cnt = 0;
	uint32_t* ptr = (uint32_t*)src;
	uint32_t val = *ptr;
	uint32_t tmp;

	if (size > SPRESS_BLK_MAX * SPRESS_BLK_SIZE) {
		size = SPRESS_BLK_MAX * SPRESS_BLK_SIZE;
	}

	while ((char*)ptr < (src + size)) {
		tmp = *ptr++;
		if (tmp == val) {
			cnt++;
			if (raw && (cnt >= SPRESS_BLK_MIN)) {
				ptr -= cnt;
				break;
			}
			if (!raw && (cnt >= SPRESS_BLK_MAX)) {
				break;
			}
		} else {
			if (cnt >= SPRESS_BLK_MIN) {
				raw = 0;
				ptr --;
				break;
			}
			raw = 1;
			cnt = 1;
			val = tmp;
		}
	}

	*praw = raw;
	return (char*)ptr;
}

static void spr_copy_data(uint32_t* dst, uint32_t* src, uint32_t cnt) {
	while (cnt-- > 0) {
		*dst++ = *src++;
	}
}

static void spr_fill_data(uint32_t* dst, uint32_t dat, uint32_t cnt) {
	while (cnt-- > 0) {
		*dst++ = dat;
	}
}

size_t spr_compress_index(const char* source, char* destination, size_t size)
{
	size_t rsize = size;
	chunk_header_t* hdr = (chunk_header_t*)(destination + sizeof(spress_header_t));
	char* src = (char*)source;
	char* next;
	uint32_t raw, len;

	// write chunk
	while (rsize > 0) {
		// split chunk
		next = spr_split_next(src, rsize, &raw);
		len = next - src;
		if (raw) {
			// raw
			hdr->data_sz = (len >> 1) | CHUNK_TYPE_RAW;
		} else {
			// fill
			hdr->data_sz = (len >> 1) | CHUNK_TYPE_FILL;
		}
		hdr++;
		src = next;
		rsize -= len;
	}

	// algin process
	hdr = (chunk_header_t*)((char*)hdr + ((SPRESS_BLK_SIZE - ((uintptr_t)hdr & 0x3)) & 0x3));

	// write header
	spress_hdr.chunk_sz = (uint16_t)((char*)hdr - destination - sizeof(spress_header_t));

	return spress_hdr.chunk_sz;
}

size_t spr_compress_data(const char* source, char* destination, size_t size)
{
	size_t rsize = size;
	chunk_header_t* hdr = (chunk_header_t*)(destination + sizeof(spress_header_t));
	char* src = (char*)source;
	char* dst = (char*)hdr + spress_hdr.chunk_sz;
	uint32_t raw, len;

	// write data
	while (rsize > 0) {
		raw = ((hdr->data_sz & CHUNK_TYPE_MASK) == CHUNK_TYPE_RAW);
		len = (hdr->data_sz & ~CHUNK_TYPE_MASK) << 1;
		if (len == 0) {
			len = SPRESS_BLK_MAX * SPRESS_BLK_SIZE;
		}
		if (raw) {
			// raw
			spr_copy_data((uint32_t*)dst, (uint32_t*)src, len / SPRESS_BLK_SIZE);
			dst += len;
		} else {
			// fill
			spr_fill_data((uint32_t*)dst, *(uint32_t*)src, 1);
			dst += SPRESS_BLK_SIZE;
		}
		hdr++;
		src += len;
		rsize -= len;
	}

	// write header
	spress_hdr.compress_sz = (uint32_t)(dst - destination);
	spress_hdr.decompress_sz = size;
	memcpy(destination, &spress_hdr, sizeof(spress_header_t));

	return spress_hdr.compress_sz;
}

size_t spr_compress(const char* source, char* destination, size_t size)
{
	size_t rsize = size;
	chunk_header_t* hdr = (chunk_header_t*)(destination + sizeof(spress_header_t));
	char* src = (char*)source;
	char* dst;
	char* next;
	uint32_t raw, len;

	// write chunk
	while (rsize > 0) {
		// split chunk
		next = spr_split_next(src, rsize, &raw);
		len = next - src;
		if (raw) {
			// raw
			hdr->data_sz = (len >> 1) | CHUNK_TYPE_RAW;
		} else {
			// fill
			hdr->data_sz = (len >> 1) | CHUNK_TYPE_FILL;
		}
		hdr++;
		src = next;
		rsize -= len;
	}

	// algin process
	hdr = (chunk_header_t*)((char*)hdr + ((SPRESS_BLK_SIZE - ((uintptr_t)hdr & 0x3)) & 0x3));

	// write header
	spress_hdr.chunk_sz = (uint16_t)((char*)hdr - destination - sizeof(spress_header_t));

	// write data
	rsize = size;
	src = (char*)source;
	dst = (char*)hdr;
	hdr = (chunk_header_t*)(destination + sizeof(spress_header_t));
	while (rsize > 0) {
		raw = ((hdr->data_sz & CHUNK_TYPE_MASK) == CHUNK_TYPE_RAW);
		len = (hdr->data_sz & ~CHUNK_TYPE_MASK) << 1;
		if (len == 0) {
			len = SPRESS_BLK_MAX * SPRESS_BLK_SIZE;
		}
		if (raw) {
			// raw
			spr_copy_data((uint32_t*)dst, (uint32_t*)src, len / SPRESS_BLK_SIZE);
			dst += len;
		} else {
			// fill
			spr_fill_data((uint32_t*)dst, *(uint32_t*)src, 1);
			dst += SPRESS_BLK_SIZE;
		}
		hdr++;
		src += len;
		rsize -= len;
	}

	// write header
	spress_hdr.compress_sz = (uint32_t)(dst - destination);
	spress_hdr.decompress_sz = size;
	memcpy(destination, &spress_hdr, sizeof(spress_header_t));

	return spress_hdr.compress_sz;
}

size_t spr_decompress(const char* source, char* destination)
{
	spress_header_t* shdr = (spress_header_t*)source;
	size_t rsize = shdr->compress_sz - sizeof(spress_header_t) - shdr->chunk_sz;
	chunk_header_t* hdr = (chunk_header_t*)(source + sizeof(spress_header_t));
	char* src = (char*)source + sizeof(spress_header_t) + shdr->chunk_sz;
	char* dst = destination;
	uint16_t raw, len;

	// write chunk
	while (rsize > 0) {
		// parse chunk
		raw = ((hdr->data_sz & CHUNK_TYPE_MASK) == CHUNK_TYPE_RAW);
		len = (hdr->data_sz & ~CHUNK_TYPE_MASK) << 1;
		if (len == 0) {
			len = SPRESS_BLK_MAX * SPRESS_BLK_SIZE;
		}
		if (raw) {
			// raw
			spr_copy_data((uint32_t*)dst, (uint32_t*)src, len / SPRESS_BLK_SIZE);
			dst += len;
		} else {
			// fill
			spr_fill_data((uint32_t*)dst, *(uint32_t*)src, len / SPRESS_BLK_SIZE);
			dst += len;
			len = SPRESS_BLK_SIZE;
		}
		hdr++;
		src += len;
		rsize -= len;
	}

	return shdr->decompress_sz;
}

