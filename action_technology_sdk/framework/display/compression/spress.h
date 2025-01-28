#ifndef SPRESS_HEADER
#define SPRESS_HEADER

#define SPRESS_DEBUG	    (0)

#include <string.h>
#include <stdint.h>

typedef uint32_t size_t;

typedef struct spress_header {
    uint16_t magic;		/* 0xed2b */
    uint8_t major_version;	/* (0x1) - reject images with higher major versions */
    uint8_t minor_version;	/* (0x0) - allow images with higer minor versions */
    uint8_t file_hdr_sz;	/* file header size */
    uint8_t chunk_hdr_sz;	/* chunk header size */
    uint16_t chunk_sz;	    /* chunk size */
    uint32_t compress_sz;	/* compress file size */
    uint32_t decompress_sz;	/* decompress file size */
} spress_header_t;

#define SPRESS_HEADER_MAGIC	    0xed2b
#define SPRESS_BLK_SIZE         sizeof(uint32_t)
#define SPRESS_BLK_MIN          2
#define SPRESS_BLK_MAX          128

#define CHUNK_TYPE_RAW		0x0
#define CHUNK_TYPE_FILL		0x1
#define CHUNK_TYPE_MASK		0x1

typedef struct chunk_header {
    uint8_t data_sz;	/* size for chunk data (bit0: 0-raw, 1-fill) */
} chunk_header_t;

size_t spr_size_decompressed(const char* source);
size_t spr_size_compressed(const char* source);
size_t spr_compress(const char* source, char* destination, size_t size);
size_t spr_decompress(const char* source, char* destination);

size_t spr_compress_index(const char* source, char* destination, size_t size);
size_t spr_compress_data(const char* source, char* destination, size_t size);
#endif

