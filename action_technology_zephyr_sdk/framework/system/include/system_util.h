/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system util interface 
 */

#ifndef __SYSTEM_UTIL_H__
#define __SYSTEM_UTIL_H__

/**
 * @defgroup sys_util_apis System Util APIs
 * @ingroup system_apis
 * @{
 */

/**
 * @brief hex to string.
 *
 * This routine provide function hex to string
 *
 * @param hex point to hex value
 * @param len len of hex value
 * @param str pointer to store string value
 *
 * @retval N/A
 */
void HEXTOSTR(char *hex, int len, char *str);

/**
 * @brief UTF8 to GBK.
 *
 * This routine provide function UTF8 to GBK
 *
 * @param lpUTF8Str point to utf8 value
 * @param lpGBKStr pointer to store GBK value
 * @param len len of UTF8 value
 *
 * @retval 0 success , other failed
 */
int UTF8TOGBK(unsigned char * lpUTF8Str,unsigned char * lpGBKStr, int len);

/**
 * @brief GBK to UTF8 .
 *
 * This routine provide function GBK to UTF8 
 *
 * @param lpGBKStr point to GBK value
 * @param lpUTF8Str pointer to store UTF8 value
 * @param len len of GBK value
 *
 * @retval 0 success , other failed
 */
int GBKTOUTF8(unsigned char * lpGBKStr, unsigned char * lpUTF8Str, int len);

/**
 * @brief unicode to mbcs.
 *
 * This routine provide function convert unicode to mbcs.
 *
 * @param src_ucs pointer to store unicode value
 * @param src_len len of unicode value
 * @param dst_mbcs len of mbcs value
 * @param dst_len mbcs value length
 * @param nation_id nation id
 *
 * @retval 0 success , other failed
 */

int UNICODETOMBCS(unsigned short *src_ucs, int src_len, unsigned char*dst_mbcs, int *dst_len, int nation_id);
/**
 * @brief mbcs to unicode.
 *
 * This routine provide function convert mbcs to  unicode.
 *
 * @param src_mbcs pointer to store mbcs value
 * @param src_len len of mbcs value
 * @param dst_uni len of unicode value
 * @param dst_len unicode value length
 * @param nation_id nation id
 *
 * @retval 0 success , other failed
 */
int MBCSTOUNICODE(unsigned char *src_mbcs, int src_len, unsigned short *dst_uni, int *dst_len, int nation_id);

/**
 * @brief utf8 to unicode.
 *
 * This routine provide function convert utf8 to  unicode.
 *
 * @param utf8 pointer to store utf8 value
 * @param utf8_len len of UTF8 value
 * @param unicode point to unicode value
 * @param punicode_len unicode value length
 *
 * @retval 0 success , other failed
 */
int UTF8TOUNICODE(char *utf8, int utf8_len, unsigned short *unicode, int *punicode_len);

/**
 * @brief unicode to utf8.
 *
 * This routine provide function convert unicode to utf8.
 *
 * @param unicode point to unicode value
 * @param unicode_len unicode value length
 * @param utf8 pointer to store utf8 value
 * @param putf8_len len of UTF8 value
 *
 * @retval 0 success , other failed
 */
int UNICODETOUTF8(unsigned short *unicode, int unicode_len, char *utf8, int *putf8_len);


/**
 * @brief string to unix time stamp
 *
 * This routine provide get unix time stamp from string
 *
 * @param string string of "2017-09-16 21:00:00"
 *
 * @retval unix time stamp
 */
int STRING_TO_UNIX_TIME_STAMP(char * string);

/**
 * @brief get unix time stamp from string
 *
 * This routine provide get unix time stamp from string
 *
 * @param string string of "2017-09-16 21:00:00"
 *
 * @retval unix time stamp
 */
int GET_UNIX_TIME_STAMP(char *string);
/**
 * @} end defgroup sys_util_apis
 */
#endif