
#ifndef _BROM_INTERFACE_H_
#define _BROM_INTERFACE_H_

#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>

typedef enum
{
    BOOT_TYPE_NULL = 0,
    BOOT_TYPE_SNOR,
    BOOT_TYPE_SNAND,
    BOOT_TYPE_SDMMC,
    BOOT_TYPE_UART,
    BOOT_TYPE_USB,
    BOOT_TYPE_POWERON,
    BOOT_TYPE_EFUSE,
    BOOT_TYPE_MAX,
} boot_type_e;


#define  BROM_API_ADDR		0x188
typedef struct
{
	//for boot
	unsigned int (*p_mbrc_brec_data_check)(unsigned char* buf, unsigned char digital_sign); /* Ð£Ñéº¯Êý*/
	unsigned int (*p_brom_nor_read)(unsigned int, unsigned int, unsigned char*);            /* spi nor read º¯Êý*/
	unsigned int (*p_brom_snand_read) (unsigned int, unsigned int, unsigned char*);         /* spi nand read º¯Êý*/
	unsigned int (*p_brom_card_read)(unsigned int,unsigned int,unsigned char*);             /* card read º¯Êý*/
	void (*p_brom_uart_launcher)(int);       																                /* UART launcher º¯Êý*/
	void (*p_adfu_launcher)(void);                                                          /* adfu launcher º¯Êý*/
	void (*p_launch)(boot_type_e type, unsigned int run_addr, unsigned int phy_addr);       /* launcher º¯Êý*/
	void* p_spinor_api;
	void* (*p_memset)(void *dst, int val, unsigned int count);
	void* (*p_memcpy)(void *dest, const void *src, unsigned int count);
	int (*p_memcmp)(const void *s1, const void *s2, unsigned int len);
	//int (*p_vsnprintf)(char* buf, int size, unsigned int linesep, const char* fmt, va_list args);
	int (*p_vsnprintf)(void *);/* spinor mem */
	unsigned int (*p_calculate_crc32)(unsigned char *buffer, unsigned int buf_len,	unsigned int crc_initial, int last);
	int (*p_verify_signature)(const unsigned char *key, const unsigned char *sig, const unsigned char *data, unsigned int len);
	unsigned char * (*p_image_tlv_find)(void *image_head, unsigned short it_type, unsigned short *it_len, int is_prot);
	const unsigned char* (*p_SHA256_hash)(const void* data, int len, unsigned char* digest);
	unsigned int (*p_image_checksum32)(void *data, unsigned int len);

}brom_api_t;

#define  p_brom_api   ((brom_api_t *)BROM_API_ADDR)

#ifndef cbprintf_cb
typedef int (*cbprintf_cb)(/* int c, void *ctx */);
#endif

typedef struct
{
	char * (*p_strcpy)(char * d, const char * s);
	char * (*p_strncpy)(char * d, const char * s, size_t n);
	char * (*p_strchr)(const char *s, int c);
	char * (*p_strrchr)(const char *s, int c);
	size_t (*p_strlen)(const char *s);
	size_t (*p_strnlen)(const char *s, size_t maxlen);
	int    (*p_strcmp)(const char *s1, const char *s2);
	int    (*p_strncmp)(const char *s1, const char *s2, size_t n);
	char * (*p_strtok_r)(char *str, const char *sep, char **state);
	char * (*p_strcat)(char * dest,const char * src);
	char * (*p_strncat)(char * d, const char * s,size_t n);
	char * (*p_strstr)(const char *s, const char *find);
	size_t (*p_strspn)(const char *s, const char *accept);
	size_t (*p_strcspn)(const char *s, const char *reject);
	int    (*p_memcmp)(const void *m1, const void *m2, size_t n);
	void * (*p_memmove)(void *d, const void *s, size_t n);
	void * (*p_memcpy)(void * d, const void * s,size_t n);
	void * (*p_memset)(void *buf, int c, size_t n);
	void * (*p_memchr)(const void *s, int c, size_t n);
	int    (*p_cbvprintf)(cbprintf_cb out, void *ctx, const char *format, va_list ap);
	int 	 (*p_vsnprintf)(char * s, size_t len, const char * format, va_list vargs);
}lib_api_t;


#define  pbrom_libc_api   ((lib_api_t *)0x00007030)


struct sw_rotate_config;

typedef struct  {
	/* sw_blend.c */
	void (*p_sw_blend_argb8565_over_rgb565)(void *dst, const void *src,
			uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h);
	void (*p_sw_blend_argb8888_over_rgb565)(void *dst, const void *src,
			uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h);
	void (*p_sw_blend_argb8888_over_argb8888)(void *dst, const void *src,
			uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h);

	/* sw_math.c (angle in 0.1 degree, range [0, 3600] */
	int32_t (*p_sw_cos30)(uint16_t angle);
	int32_t (*p_sw_sin30)(uint16_t angle);
	void (*p_sw_rotate_point32)(int32_t *draw_x, int32_t *draw_y,
			int32_t img_x, int32_t img_y, int32_t pivot_x, int32_t pivot_y, uint16_t angle);
	void (*p_sw_rotate_area16)(
			int16_t *dest_x1, int16_t *dest_y1, int16_t *dest_x2, int16_t *dest_y2,
			int16_t src_x1, int16_t src_y1, int16_t src_x2, int16_t src_y2,
			int16_t pivot_x, int16_t pivot_y, uint16_t angle);

	/* sw_rotate.c */
	void (*p_sw_rotate_configure)(int16_t dst_x, int16_t dst_y, int16_t src_x, int16_t src_y,
			int16_t pivot_x, int16_t pivot_y, uint16_t angle, struct sw_rotate_config *cfg);
	void (*p_sw_rotate_rgb565_over_rgb565)(void *dst, const void *src,
			uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
			int16_t x, int16_t y, uint16_t w, uint16_t h,
			const struct sw_rotate_config *cfg);
	void (*p_sw_rotate_argb8888_over_rgb565)(void *dst, const void *src,
			uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
			int16_t x, int16_t y, uint16_t w, uint16_t h,
			const struct sw_rotate_config *cfg);
	void (*p_sw_rotate_argb8888_over_argb8888)(void *dst, const void *src,
			uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
			int16_t x, int16_t y, uint16_t w, uint16_t h,
			const struct sw_rotate_config *cfg);
} brom_libgui_api_t;

#define p_brom_libgui_api ((brom_libgui_api_t *)0x000070a0)

#endif /* _BROM_INTERFACE_H_ */
