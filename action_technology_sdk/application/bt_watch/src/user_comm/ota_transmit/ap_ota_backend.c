#include <kernel.h>
#include <device.h>
#include <thread_timer.h>
#include <app_defines.h>
#include <ota_upgrade.h>
#include <ota_backend.h>
#include <ota_backend_sdcard.h>
#include <ota_backend_bt.h>
#include <mem_manager.h>

#include <kernel.h>
#include <stream.h>
#include <ota_backend.h>
#include <user_comm/ap_status.h>
#include "ap_ota_private.h"

#ifdef CONFIG_BLUETOOTH
#include <bt_manager.h>
#endif

#define APOTA_OTA_SEND_INTERVAL	(5)		/* 5ms */

struct ota_backend_apota {
	struct ota_backend backend;
	io_stream_t stream;
	int stream_opened;
	int report_flag;
	int report;
};

static int apota_offset = 0;
static struct ota_backend_apota *ap_backend = NULL;

int ota_backend_apota_get_offset(void)
{
	return apota_offset;
}

int ota_backend_apota_read(struct ota_backend *backend, int offset, void *buf, int size)
{
	struct ota_backend_apota *backend_apota = CONTAINER_OF(backend,
		struct ota_backend_apota, backend);
	int ret = 0;

	SYS_LOG_INF("offset 0x%x, size %d, buf %p", offset, size, buf);
	apota_offset = offset;
	while (ret != size) {
		if (ap_status_get() != AP_STATUS_OTA_RUNING) {
			SYS_LOG_INF("OTA already exit!");
			return 1;
		}

		ret = stream_read(backend_apota->stream, buf, size);
		if (ret != size) {
			os_sleep(1);
		}
	}
	return 0;
}

int ota_backend_apota_open(struct ota_backend *backend)
{
	struct ota_backend_apota *backend_apota = CONTAINER_OF(backend,
		struct ota_backend_apota, backend);
	int err;

	err = stream_open(backend_apota->stream, MODE_IN_OUT);
	if (err) {
		SYS_LOG_ERR("stream open failed\n");
		return err;
	}

	backend_apota->stream_opened = 1;

	SYS_LOG_INF("open stream %p", backend_apota->stream);

	return 0;
}

int ota_backend_apota_close(struct ota_backend *backend)
{
	struct ota_backend_apota *backend_apota = CONTAINER_OF(backend,
		struct ota_backend_apota, backend);
	int err;

	SYS_LOG_INF("close: type %d", backend->type);

	if (backend_apota->stream) {
		err = stream_close(backend_apota->stream);
		if (err) {
			SYS_LOG_ERR("stream_close Failed");
			return err;
		}
	}

	backend_apota->stream_opened = 0;

	return 0;
}

void ota_backend_apota_exit(struct ota_backend *backend)
{
	struct ota_backend_apota *backend_apota = CONTAINER_OF(backend,
		struct ota_backend_apota, backend);

	if (backend_apota->stream) {
		stream_destroy(backend_apota->stream);
	}

	app_mem_free(backend_apota);
}

int ota_backend_apota_ioctl(struct ota_backend *backend, int cmd, unsigned int param)
{
	struct ota_backend_apota *backend_apota = CONTAINER_OF(backend,
		struct ota_backend_apota, backend);

	SYS_LOG_INF("cmd 0x%x: param %d\n", cmd, param);
	backend_apota->report_flag = 1;

	switch (cmd) {
	case OTA_BACKEND_IOCTL_REPORT_IMAGE_VALID:
		backend_apota->report = param;
		break;
	default:
		SYS_LOG_ERR("unknow cmd 0x%x", cmd);
		return -EINVAL;
	}

	return 0;

}

struct ota_backend_api ota_backend_api_apota = {
	//.init = ota_backend_apota_init,
	.exit = ota_backend_apota_exit,
	.open = ota_backend_apota_open,
	.close = ota_backend_apota_close,
	.read = ota_backend_apota_read,
	.ioctl = ota_backend_apota_ioctl,
};

int ota_backend_apota_write_stream(struct ota_backend *backend, uint8_t *buf, int length)
{
	int ret = 0;
	struct ota_backend_apota *backend_apota = CONTAINER_OF(backend,
		struct ota_backend_apota, backend);


	if (length < 0)
		return EINVAL;

	if (!buf)
		return EINVAL;

	if (backend_apota->stream && backend_apota->stream_opened) {
		ret = stream_write(backend_apota->stream, buf, length);
		if (ret <= 0) {
			SYS_LOG_ERR("[%d], ret %d, length %d stream %p stream_opened %d\n",\
				__LINE__, ret, length, backend_apota->stream, backend_apota->stream_opened);
		}
	} else {
		SYS_LOG_ERR("error!! stream %p, stream_opened %d\n", backend_apota->stream, \
			backend_apota->stream_opened);
	}

	return ret;

}
#if 0
int ota_backend_apota_start(struct ota_backend *backend)
{
	ota_backend_notify_cb_t cb = backend->cb;

	cb(backend, OTA_BACKEND_UPGRADE_STATE, 1);
	return 0;
}

int ota_backend_apota_stop(struct ota_backend *backend)
{
	ota_backend_notify_cb_t cb = backend->cb;

	cb(backend, OTA_BACKEND_UPGRADE_STATE, 0);
	return 0;
}
#endif
int ota_backend_apota_reply(struct ota_backend *backend)
{
	int count = 0;
	struct ota_backend_apota *backend_apota = CONTAINER_OF(backend,
		struct ota_backend_apota, backend);

	while (backend_apota->report_flag == 0) {
		os_sleep(100);
		count++;
		if (count > 10) {
			break;
		}
	}
	SYS_LOG_INF("report %d", backend_apota->report);

	return backend_apota->report;
}

static void apota_connect_cb(int connected)
{
	struct ota_backend *backend;

	SYS_LOG_INF("connect: %d", connected);

	/* avoid connect again after exit */
	if (ap_backend) {
		backend = &ap_backend->backend;
		if (backend->cb) {
			SYS_LOG_INF("call cb %p", backend->cb);
			backend->cb(backend, OTA_BACKEND_UPGRADE_STATE, connected);
		}
	}
}

struct ota_backend *ota_backend_apota_init(ota_backend_notify_cb_t cb,
		struct ota_backend_apota_init_param *param)
{
	struct ota_backend_apota *backend_apota;
	struct apota_stream_init_param init_param;

	SYS_LOG_INF("init");
	if (ap_backend)
	{
		SYS_LOG_ERR("ap_backend alreay exit");
		return &ap_backend->backend;
	}

	backend_apota = app_mem_malloc(sizeof(struct ota_backend_apota));
	if (!backend_apota) {
		SYS_LOG_ERR("malloc failed");
		return NULL;
	}

	memset(backend_apota, 0x0, sizeof(struct ota_backend_apota));
	init_param.read_timeout = APOTA_OTA_SEND_INTERVAL;	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	init_param.write_timeout = OS_NO_WAIT;
	init_param.connect_cb = apota_connect_cb;

	backend_apota->stream = apota_stream_create(&init_param);
	if (!backend_apota->stream) {
		SYS_LOG_ERR("stream create failed\n");
		return NULL;
	}
	ap_backend = backend_apota;

	SYS_LOG_INF("create stream %p", backend_apota->stream);

	ota_backend_init(&backend_apota->backend, OTA_BACKEND_TYPE_CARD,
			 &ota_backend_api_apota, cb);

	return &backend_apota->backend;
}


