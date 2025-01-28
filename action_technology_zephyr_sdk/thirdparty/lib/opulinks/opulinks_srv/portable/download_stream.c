#include <init.h>
#include <stdio.h>
#include <kernel.h>
#include <acts_ringbuf.h>
#include <file_stream.h>
#include <board_cfg.h>
#include "at_command.h"
#include "serial.h"
#include "spi_slave.h"
#include "wifi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define DOWNLOAD_RW_SIZE (32 * 1024)
static char download_ringbuf_backstore[32 * 8 * 1024];
/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
#define CONFIG_WITE_FILE_Q_STACK_SIZE 2048
struct k_work_q write_file_q;
K_THREAD_STACK_DEFINE(write_file_q_stack, CONFIG_WITE_FILE_Q_STACK_SIZE);
bool write_file_q_inited = false;
/****************************************************************************
 * Private Types
 ****************************************************************************/
struct download_stream_data {
	bool disk_full;
	struct acts_ringbuf ringbuf;
	io_stream_t fstream;
	os_work work;
};
/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void download_work_func(os_work *work)
{
	struct download_stream_data *download_data =
			CONTAINER_OF(work, struct download_stream_data, work);
	u32_t start_time, end_time;
	int space;

	do {
		size_t len = acts_ringbuf_length(&download_data->ringbuf);
		if (len < DOWNLOAD_RW_SIZE)
			break;

		SYS_LOG_DBG("begin writing %u bytes (length=%zu, uptime=%u ms)\n",
				DOWNLOAD_RW_SIZE, len, os_uptime_get_32());

		start_time = os_cycle_get_32();

		space = (int)ROUND_DOWN(stream_get_space(download_data->fstream), 2);

		len = acts_ringbuf_read(&download_data->ringbuf, download_data->fstream,
						min(DOWNLOAD_RW_SIZE, space), (acts_ringbuf_write_fn)stream_write);

		end_time = os_cycle_get_32();
		SYS_LOG_DBG("consumed time=%u ms (uptime=%u ms)\n",
			    k_cyc_to_us_near32(end_time - start_time)/1000, os_uptime_get_32());

		if (len != DOWNLOAD_RW_SIZE) {
			SYS_LOG_WRN("stream_write failed (lost %zu bytes)", DOWNLOAD_RW_SIZE - len);
		}
	} while (0);
}

static int download_stream_init(io_stream_t handle, void *param)
{
	struct download_stream_data *download_data = mem_malloc(sizeof(*download_data));
	if (!download_data)
		return -ENOMEM;

	download_data->fstream = file_stream_create( param);
	if (!download_data->fstream) {
		mem_free(download_data);
		return -ENOENT;
	}

	acts_ringbuf_init(&download_data->ringbuf, download_ringbuf_backstore,
			  sizeof(download_ringbuf_backstore));

	handle->data = download_data;

	if (!write_file_q_inited) {
		k_work_queue_start(&write_file_q, write_file_q_stack, K_THREAD_STACK_SIZEOF(write_file_q_stack), 4, NULL);
		write_file_q_inited = true;
	}
	return 0;
}

static int download_stream_open(io_stream_t handle, stream_mode mode)
{
	struct download_stream_data *download_data = handle->data;

	if (mode != MODE_OUT)
		return -EPERM;

	if (stream_open(download_data->fstream, MODE_IN_OUT))
		return -EPERM;

	os_work_init(&download_data->work, download_work_func);
	return 0;
}

static int download_stream_tell(io_stream_t handle)
{
	struct download_stream_data *download_data = handle->data;
	return stream_tell(download_data->fstream);
}

static int download_stream_get_space(io_stream_t handle)
{
	struct download_stream_data *download_data = handle->data;
	return download_data->disk_full ? 0 : acts_ringbuf_space(&download_data->ringbuf);
}

static int download_stream_write(io_stream_t handle, unsigned char *buf, int num)
{
	struct download_stream_data *download_data = handle->data;
	size_t len;

	num = acts_ringbuf_put(&download_data->ringbuf, buf, num);

	len = acts_ringbuf_length(&download_data->ringbuf);
	if (len >= DOWNLOAD_RW_SIZE) {
		SYS_LOG_DBG("length=%zu (uptime=%u ms)\n", len, os_uptime_get_32());
		os_work_submit_to_queue(&write_file_q, &download_data->work);
	}

	return num;
}

static int download_stream_flush(io_stream_t handle)
{
	struct download_stream_data *download_data = handle->data;
	return stream_flush(download_data->fstream);
}

static int download_stream_close(io_stream_t handle)
{
	struct download_stream_data *download_data = handle->data;

	/* write last data */
	acts_ringbuf_read(&download_data->ringbuf, download_data->fstream,
				acts_ringbuf_length(&download_data->ringbuf),
				(acts_ringbuf_write_fn)stream_write);

	SYS_LOG_INF("totally record %u bytes", (unsigned int)stream_tell(download_data->fstream));
	stream_close(download_data->fstream);
	return 0;
}

static int download_stream_destroy(io_stream_t handle)
{
	struct download_stream_data *download_data = handle->data;
	stream_destroy(download_data->fstream);
	mem_free(download_data);
	return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
const stream_ops_t download_stream_ops = {
	.init = download_stream_init,
	.open = download_stream_open,
	.tell = download_stream_tell,
	.write = download_stream_write,
	.flush = download_stream_flush,
	.get_space = download_stream_get_space,
	.close = download_stream_close,
	.destroy = download_stream_destroy,
};

io_stream_t download_stream_create(const char *param)
{
	return stream_create(&download_stream_ops, (void *)param);
}
