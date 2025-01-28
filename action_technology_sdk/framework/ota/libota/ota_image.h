/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA firmware image interface
 */

#ifndef __OTA_IMAGE_H__
#define __OTA_IMAGE_H__

struct ota_image;
struct ota_backend;

struct ota_image *ota_image_init(void);
void ota_image_exit(struct ota_image *img);

int ota_image_bind(struct ota_image *img, struct ota_backend *backend);
int ota_image_unbind(struct ota_image *img, struct ota_backend *backend);
struct ota_backend *ota_image_get_backend(struct ota_image *img);
uint32_t ota_image_get_checksum(struct ota_image *img);
uint32_t ota_image_get_datasize(struct ota_image *img);

int ota_image_open(struct ota_image *img);
int ota_image_close(struct ota_image *img);

int ota_image_check_data(struct ota_image *img);
int ota_image_read_prepare(struct ota_image *img, int offset, uint8_t *buf, int size);
int ota_image_read_complete(struct ota_image *img, int offset, uint8_t *buf, int size);
int ota_image_read(struct ota_image *img, int offset, uint8_t *buf, int size);
int ota_image_ioctl(struct ota_image *img, int cmd, unsigned int param);

int ota_image_get_file_length(struct ota_image *img, const char *filename);
int ota_image_get_file_offset(struct ota_image *img, const char *filename);

int ota_image_check_file(struct ota_image *img, const char *filename, const uint8_t *buf, int size);
int ota_image_report_progress(struct ota_image *img, uint32_t pre_xfer_size, bool is_final);
int ota_image_progress_on(struct ota_image *img, uint32_t total_size, uint32_t start_offset);
int ota_image_progress_reset(struct ota_image *img);

#endif /* __OTA_IMAGE_H__ */