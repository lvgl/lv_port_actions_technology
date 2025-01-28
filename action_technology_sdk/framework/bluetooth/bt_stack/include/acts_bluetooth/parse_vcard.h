/** @file
 *  @brief Bluetooth Parse vcrad
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_PARSE_VCARD_H
#define __BT_PARSE_VCARD_H

#ifdef __cplusplus
extern "C" {
#endif

#define PBAP_VCARD_MAX_CACHED	512
#define PBAP_VCARD_MAX_LINE		20

enum VCRAD_RESULT_TYPE {
	VCARD_TYPE_VERSION            = 0,		/* vCard Version*/
	VCARD_TYPE_FN                 = 1,		/* Formatted Name */
	VCARD_TYPE_N                  = 2,		/* Structured Presentation of Name */
	VCARD_TYPE_PHOTO              = 3,		/* Associated Image or Photo */
	VCARD_TYPE_BDAY               = 4,		/* Birthday*/
	VCARD_TYPE_ADR                = 5,		/* Delivery Address*/
	VCARD_TYPE_LABEL              = 6,		/* Delivery */
	VCARD_TYPE_TEL                = 7,		/* Telephone Number */
	VCARD_TYPE_EMAIL              = 8,		/* Electronic Mail Address */
	VCARD_TYPE_MAILER             = 9,		/* Electronic Mail */
	VCARD_TYPE_TZ                 = 10,		/* Time Zone*/
	VCARD_TYPE_GEO                = 11,		/*Geographic Position */
	VCARD_TYPE_TITLE              = 12,		/* Job */
	VCARD_TYPE_ROLE               = 13,     /*Role within the Organization*/

	VCARD_TYPE_LOGO               = 14,		/* Organization Logo*/
	VCARD_TYPE_AGENT              = 15,		/* vCard of Person Representing */
	VCARD_TYPE_ORG                = 16,		/* Name of Organization */
	VCARD_TYPE_NOTE               = 17,		/* Comments */
	VCARD_TYPE_REV                = 18,		/*Revision*/
	VCARD_TYPE_SOUND              = 19,		/* Pronunciation of Name*/
	VCARD_TYPE_URL                = 20,		/* Uniform Resource Locato */
	VCARD_TYPE_UID                = 21,		/* Unique ID */
	VCARD_TYPE_KEY                = 22,		/* Public Encryption Key*/
	VCARD_TYPE_NICKNAME           = 23,		/* Nickname */
	VCARD_TYPE_CATEGORIES         = 24,		/*Categories */
	VCARD_TYPE_PROID              = 25,		/* Product ID */
	VCARD_TYPE_CLASS              = 26,		/* Class information */
	VCARD_TYPE_SORT_STRING        = 27,     /* String used for sorting operations */

	VCARD_TYPE_X_IRMC_CALL_DATETIME= 28,    /* Time stamp */
	VCARD_TYPE_X_BT_SPEEDDIALKEY  = 29,		/* Speed-dial shortcut */
	VCARD_TYPE_X_BT_UCI           = 30,		/*Uniform Caller Identifier */
	VCARD_TYPE_X_BT_UID           = 31,		/* Bluetooth Contact Unique Identifier */

	VCARD_TYPE_Proprietary_Filter = 39, /*Indicates the usage of a proprietary filter*/
};

struct parse_cached_data {
	uint16_t cached_len;
	uint16_t parse_pos;
	uint8_t cached[PBAP_VCARD_MAX_CACHED];
};

struct parse_vcard_result {
	uint8_t type;
	uint16_t len;
	uint8_t *data;
};

uint16_t pbap_parse_copy_to_cached(uint8_t *data, uint16_t len, struct parse_cached_data *vcached);
void pbap_parse_vcached_move(struct parse_cached_data *vcached);
void pbap_parse_vcached_vcard(struct parse_cached_data *vcached, struct parse_vcard_result *result, uint8_t *result_size);
void pbap_parse_vcached_list(struct parse_cached_data *vcached, struct parse_vcard_result *result, uint8_t *result_size);

#ifdef __cplusplus
}
#endif
#endif /* __BT_PARSE_VCARD_H */
