/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <stdlib.h>
#include <string.h>
#include <mem_manager.h>
#include "ota_image.h"
#include "ota_manifest.h"
#include <os_common_api.h>

/* the last byte reserved for '\0' */
#define OTA_MANIFSET_FILE_MAX_SIZE	(3072 - 1)
#define XML_TAG_MAX_LEN			31

static int xml_get_data(const char *buf, const char *tag, const char **start, const char **end)
{
	char xml_tag[XML_TAG_MAX_LEN + 4];
	char *find;
	int tag_len;

	SYS_LOG_DBG("try to get tag %s \n",tag);

	tag_len = strlen(tag);
	if (tag_len > XML_TAG_MAX_LEN) {
		return -1;
	}

	/* find <tag> start postion */
	xml_tag[0] = '<';
	memcpy(&xml_tag[1], tag, tag_len);
	xml_tag[tag_len + 1] = '>';
	xml_tag[tag_len + 2] = '\0';

	find = strstr(buf, xml_tag);
	if (!find) {
		SYS_LOG_ERR("cant find tag start %s\n",tag);
		return -1;
	}

	find = find + tag_len + 2;
	if (start)
		*start = find;

	/* find </tag> end postion */
	xml_tag[0] = '<';
	xml_tag[1] = '/';
	memcpy(&xml_tag[2], tag, tag_len);
	xml_tag[tag_len + 2] = '>';
	xml_tag[tag_len + 3] = '\0';

	find = strstr(find, xml_tag);
	if (!find) {
		SYS_LOG_ERR("cant find tag end %s\n",tag);
		return -1;
	}

	if (end)
		*end = find;

	return 0;
}

static int xml_get_int(const char *buf, const char *tag, unsigned int *int_ptr)
{
	const char *start, *stop;
	int err;

	err = xml_get_data(buf, tag, &start, &stop);
	if (err) {
		return err;
	}

	*int_ptr = strtoul(start, (char **)&stop, 0);

	return 0;
}

static int xml_get_str(const char *buf, const char *tag, char *str_ptr, int max_len)
{
	const char *start, *stop;
	int err, len;

	err = xml_get_data(buf, tag, &start, &stop);
	if (err) {
		return err;
	}

	len = (unsigned int)stop - (unsigned int)start;
	if ((len + 1) > max_len)
		return -1;

	memcpy(str_ptr, start, len);
	str_ptr[len] = '\0';

	return 0;
}

const char *ota_file_type_strs[] = {
	"RESERVED",
	"BOOT",
	"SYSTEM",
	"RECOVERY",
	"DATA",
	"TEMP",
	"SYS_PARAM",
};

static int ota_get_file_type(const char *type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ota_file_type_strs); i++) {
		if (0 == memcmp(type, ota_file_type_strs[i], strlen(ota_file_type_strs[i]))) {
			return i;
		}
	}

	return -1;
}

int parse_fw_version(struct fw_version *ver, const char *xml, const char *ver_tag)
{
	const char *ver_seg_start, *ver_seg_end;
	int err;

	err = xml_get_data(xml, ver_tag, &ver_seg_start, &ver_seg_end);
	if (err) {
		SYS_LOG_ERR("cannot found tag <%s>", ver_tag);
		goto failed;
	}

	err = xml_get_int(ver_seg_start, "version_code", &ver->version_code);
	if (err) {
		SYS_LOG_ERR("cannot found tag<version_code>");
		goto failed;
	}

	err = xml_get_int(ver_seg_start, "version_res", &ver->version_res);
	if (err) {
		SYS_LOG_ERR("cannot found tag<version_res>");
		ver->version_res = 0;
	}

	err = xml_get_str(ver_seg_start, "version_name", ver->version_name,
			  sizeof(ver->version_name));
	if (err) {
		SYS_LOG_ERR("cannot found tag <version_name>");
		goto failed;
	}

	err = xml_get_str(ver_seg_start, "board_name", ver->board_name,
			  sizeof(ver->board_name));
	if (err) {
		SYS_LOG_ERR("cannot found tag <board_name>");
		goto failed;
	}

	return 0;
failed:
	return -ENOENT;
}

int ota_manifest_parse_partitions(struct ota_manifest *manifest, const char *xml)
{
	const char *start, *stop, *part_seg_start, *part_seg_end;
	char file_name[13];
	int err, i, part_num, file_size, orig_size;
	uint32_t file_type, file_id, checksum;
#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	uint32_t storage_id = 0;
#endif
	struct ota_file *wfile;

	err = xml_get_int(xml, "partitionsNum", &part_num);
	if (err) {
		SYS_LOG_ERR("cannot found partitionsNum tag");
		goto failed;
	}

	SYS_LOG_INF("part_num: %d\n", part_num);

	if (part_num > OTA_MANIFEST_MAX_FILE_CNT) {
		SYS_LOG_ERR("too many ota file cnt: %d\n", part_num);
		err = -EINVAL;
		goto failed;
	}

	err = xml_get_data(xml, "partitions", &part_seg_start, &part_seg_end);
	if (err) {
		SYS_LOG_ERR("cannot found partitions tag");
		goto failed;
	}

	for (i = 0; i < part_num;  i++) {
		err = xml_get_data(part_seg_start, "partition", &part_seg_start, &part_seg_end);
		if (err) {
			SYS_LOG_ERR("cannot found partition tag");
			goto failed;
		}

		err = xml_get_data(part_seg_start, "type", &start, &stop);
		if (err) {
			SYS_LOG_ERR("cannot get <type> for part %d", i);
			goto failed;
		}

		file_type = ota_get_file_type(start);

		err = xml_get_int(part_seg_start, "file_id", &file_id);
		if (err) {
			SYS_LOG_ERR("cannot get <file_id> for part %d", i);
			goto failed;
		}
	#ifdef CONFIG_OTA_MUTIPLE_STORAGE
		err = xml_get_int(part_seg_start, "storage_id", &storage_id);
		if (err) {
			SYS_LOG_ERR("cannot get <storage_id> for part %d", i);
			goto failed;
		}
	#endif
		err = xml_get_str(part_seg_start, "file_name", file_name, sizeof(file_name));
		if (err) {
			SYS_LOG_ERR("cannot get <file_name> for part %d", i);
			goto failed;
		}

		err = xml_get_int(part_seg_start, "file_size", &file_size);
		if (err) {
			SYS_LOG_ERR("cannot get file <file_size> for part %d", i);
			goto failed;
		}

		err = xml_get_int(part_seg_start, "orig_size", &orig_size);
		if (err) {
			orig_size = file_size;
		}

		err = xml_get_int(part_seg_start, "checksum", &checksum);
		if (err) {
			SYS_LOG_ERR("cannot get <checksum> for part %d", i);
			goto failed;
		}

		SYS_LOG_INF("part file %s: type %d, file_id %d, checksum 0x%x",
			file_name, file_type, file_id, checksum);

		wfile = &manifest->wfiles[manifest->file_cnt];
		wfile->size = file_size;
		wfile->orig_size = orig_size;
		wfile->type = file_type;
		wfile->file_id = file_id;
	#ifdef CONFIG_OTA_MUTIPLE_STORAGE
		wfile->storage_id = storage_id;
	#endif
		wfile->checksum = checksum;
		strncpy(wfile->name, file_name, sizeof(wfile->name));

		manifest->file_cnt++;
		part_seg_start = part_seg_end;
	}

	return 0;

failed:
	return err;
}

int ota_manifest_parse_data(struct ota_manifest *manifest, const char *xml)
{
	int err;

#if defined(CONFIG_OTA_FILE_PATCH) || defined(CONFIG_OTA_RES_PATCH)
	err = parse_fw_version(&manifest->old_fw_ver, xml, "old_firmware_version");
	if (err) {
		SYS_LOG_ERR("cannot found old_firmware_version, is not ota patch fw");
		/* clear old version code */
		memset(&manifest->old_fw_ver, 0x0, sizeof(struct fw_version));
	} else {
		SYS_LOG_INF("Is OTA patch fw, old fw version:");
		fw_version_dump(&manifest->old_fw_ver);
	}
#endif

	err = parse_fw_version(&manifest->fw_ver, xml, "firmware_version");
	if (err) {
		SYS_LOG_ERR("parse fw version failed");
		goto failed;
	}

	SYS_LOG_INF("OTA new fw version:");
	fw_version_dump(&manifest->fw_ver);

	err = ota_manifest_parse_partitions(manifest, xml);
	if (err) {
		SYS_LOG_ERR("parse partitions failed");
		goto failed;
	}

	return 0;

failed:
	return -1;
}

int ota_manifest_check_data(struct ota_image *img, const char *file_name,
			    const char *xml, int size)
{
	int err;

	if (memcmp(xml, "<?xml", 5)) {
		SYS_LOG_ERR("invalid manifest file");
		return -1;
	}

	err = ota_image_check_file(img, file_name, xml, size);
	if (err) {
		SYS_LOG_INF("manifest check err");
		return err;
	}

	return 0;
}

int ota_manifest_check_file_size(struct ota_manifest *manifest, struct ota_image *img)
{
#ifndef CONFIG_OTA_FILE_PATCH
	struct ota_file *file;
	int i, file_len;

	for (i = 0; i < manifest->file_cnt; i++) {
		file = &manifest->wfiles[i];

		file_len = ota_image_get_file_length(img, file->name);
		if (file_len != file->size) {
			SYS_LOG_ERR("file %s: file size 0x%x in manifest file is not equal to image dir 0x%x",
				file->name, file->size, file_len);
			return -EINVAL;
		}
	}
#endif

	return 0;
}

int ota_manifest_parse_file(struct ota_manifest *manifest, struct ota_image *img,
			    const char *file_name)
{
	char *manifest_data;
	int file_size, img_file_offset;
	int err = 0;

	memset(manifest, 0x0, sizeof(struct ota_manifest));

	file_size = ota_image_get_file_length(img, file_name);
	if (file_size > OTA_MANIFSET_FILE_MAX_SIZE) {
		SYS_LOG_INF("cfg file is too big");
		return -1;
	}

	SYS_LOG_INF("manifest file %s length %d", file_name, file_size);

	manifest_data = mem_malloc(file_size + 1);
	if (!manifest_data) {
		SYS_LOG_ERR("failed to malloc %d bytes", file_size + 1);
		return -ENOMEM;
	}

	img_file_offset = ota_image_get_file_offset(img, file_name);
	if (img_file_offset < 0) {
		SYS_LOG_ERR("cannot found file %s in image", file_name);
		goto exit;
	}

	err = ota_image_read(img, img_file_offset, manifest_data, file_size);
	if (err) {
		SYS_LOG_INF("image read %d err %d", file_size, err);
		goto exit;
	}

	err = ota_manifest_check_data(img, file_name, manifest_data, file_size);
	if (err) {
		SYS_LOG_INF("manifest check failed");
		goto exit;
	}

	/* ensure data is string */
	manifest_data[file_size] = '\0';

	err = ota_manifest_parse_data(manifest, manifest_data);
	if (err) {
		SYS_LOG_ERR("failed to read manifest");
		goto exit;
	}

	err = ota_manifest_check_file_size(manifest, img);
	if (err) {
		SYS_LOG_ERR("failed to check file");
		goto exit;
	}
exit:
	mem_free(manifest_data);
	return err;
}

