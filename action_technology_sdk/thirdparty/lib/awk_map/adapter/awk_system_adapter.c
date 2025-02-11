#include <kernel.h>
#include <string.h>
#include <os_common_api.h>
#include <kernel.h>
#include <stdio.h>
#include <soc.h>
#include <sys/sys_heap.h>
#include <awk.h>
#include <awk_adapter.h>
#include <awk_system_adapter.h>

#define awk_assert(expression) if(!(expression)) {k_panic();}


static __noinit __aligned(8) char awk_heap_mem[CONFIG_AWK_MEM_POOL_SIZE];
static struct sys_heap awk_heap;

static __noinit __aligned(8) char awk_fast_heap_mem[CONFIG_AWK_FAST_MEM_POOL_SIZE];
static struct sys_heap awk_fast_heap;

void *get_awk_fast_heap(void)
{
	return awk_fast_heap.heap;
}

static struct k_spinlock awk_heap_lock;

void* awk_fast_mem_malloc_adapter(size_t size, int type)
{
	k_spinlock_key_t key;
	void *ptr;
	key = k_spin_lock(&awk_heap_lock);
	ptr = sys_heap_aligned_alloc(&awk_fast_heap, 4, size);
	k_spin_unlock(&awk_heap_lock, key);
	awk_assert(ptr != NULL);
	return ptr;
}

void awk_fast_mem_free_adapter(void* ptr)
{
	printf("awk_fast_mem_free_adapter %p\n", ptr);
	k_spinlock_key_t key;
	key = k_spin_lock(&awk_heap_lock);
	sys_heap_free(&awk_fast_heap, ptr);
	k_spin_unlock(&awk_heap_lock, key);
}

void* awk_mem_malloc_adapter(size_t size)
{
	k_spinlock_key_t key;
	void *ptr;
	key = k_spin_lock(&awk_heap_lock);
	ptr = sys_heap_aligned_alloc(&awk_heap, 4, size);
	k_spin_unlock(&awk_heap_lock, key);
	awk_assert(ptr != NULL);
	return ptr;
}

void awk_mem_free_adapter(void* ptr)
{
	k_spinlock_key_t key;
	key = k_spin_lock(&awk_heap_lock);
	sys_heap_free(&awk_heap, ptr);
	k_spin_unlock(&awk_heap_lock, key);
}

void* awk_mem_calloc_adapter(size_t count, size_t size)
{
	k_spinlock_key_t key;
	void *ptr;
	size_t wanted_size;
	key = k_spin_lock(&awk_heap_lock);
	wanted_size = count * size;
	ptr = sys_heap_aligned_alloc(&awk_heap, 4, wanted_size);
	if (ptr != NULL) {
		memset(ptr, 0, wanted_size);
	}
	k_spin_unlock(&awk_heap_lock, key);
	awk_assert(ptr != NULL);
	return ptr;
}

void* awk_mem_realloc_adapter(void* ptr, size_t size)
{
	k_spinlock_key_t key;
	void *new_ptr;
	key = k_spin_lock(&awk_heap_lock);
	new_ptr = sys_heap_aligned_realloc(&awk_heap, ptr, 4, size);
	k_spin_unlock(&awk_heap_lock, key);
	awk_assert(new_ptr != NULL);
	return new_ptr;
}

static int awk_heap_init(const struct device *arg)
{
	sys_heap_init(&awk_heap, awk_heap_mem, CONFIG_AWK_MEM_POOL_SIZE);
	sys_heap_init(&awk_fast_heap, awk_fast_heap_mem, CONFIG_AWK_FAST_MEM_POOL_SIZE);
	return 0;
}

SYS_INIT(awk_heap_init, APPLICATION, 90);

#include <shell/shell.h>
#include <debug/tracedump.h>

static int awkmem_test(const struct shell *shell, size_t argc, char **argv)
{
	void *ptr = NULL;
	if (strcmp(argv[1], "malloc") == 0) {
		ptr = awk_mem_malloc_adapter((size_t)atoi(argv[2]));
		printk("malloc ptr:%p\n", ptr);
	} else if (strcmp(argv[1], "free") == 0) {
		ptr = (void *)strtoul((const char *)argv[2], NULL, 16);
		awk_mem_free_adapter(ptr);
		printk("free ptr:%p\n", ptr);
	} else if (strcmp(argv[1], "realloc") == 0) {
		ptr = (void *)strtoul((const char *)argv[2], NULL, 16);
		ptr = awk_mem_realloc_adapter(ptr, (size_t)atoi(argv[3]));
		printk("realloc ptr:%p\n", ptr);
	} else if (strcmp(argv[1], "calloc") == 0) {
		ptr = awk_mem_calloc_adapter((size_t)atoi(argv[2]), (size_t)atoi(argv[3]));
		printk("calloc ptr:%p\n", ptr);
	} else if (strcmp(argv[1], "memset") == 0) {
		ptr = (void *)strtoul((const char *)argv[2], NULL, 16);
		memset(ptr, 5, (size_t)atoi(argv[3]));
		printk("memset ptr:%p\n", ptr);
	} else if (strcmp(argv[1], "tracestart") == 0) {
		tracedump_reset();
		tracedump_set_filter((uint32_t)awk_heap_mem, CONFIG_AWK_MEM_POOL_SIZE);
		tracedump_set_enable(3, 1);
	} else if (strcmp(argv[1], "tracedump") == 0) {
		tracedump_dump();
	} else if (strcmp(argv[1], "heapdump") == 0) {
		sys_heap_dump(&awk_heap);
	} else if (strcmp(argv[1], "fastheapdump") == 0) {
		sys_heap_dump(&awk_fast_heap);
	}

	return 0;
}

SHELL_CMD_REGISTER(awkmem, NULL, "Application shell commands", awkmem_test);

/**
* @brief 打开文件的回调,参考fopen
* @return {*}
*/
#include <fs/fs.h>

void* awk_file_open_adapter(const char *filename, const char *mode)
{
	int rc = 0;
	struct fs_file_t *zfp;
	fs_mode_t mode_flag = 0;
	bool is_need_truncate = false;

	zfp = awk_mem_malloc_adapter(sizeof(struct fs_file_t));
	memset(zfp, 0, sizeof(struct fs_file_t));
	if (zfp == NULL) {
		printf("zfp null\n");
		return NULL;
	}
	for (int i=0; i<strlen(mode);i++) {
		switch (mode[i]) {
			case 'r':
				if (awk_file_exists_adapter(filename) == false) {
					goto err_handle;
				}
				mode_flag |= FS_O_READ;
				break;
			case 'w':
				mode_flag |= FS_O_CREATE | FS_O_WRITE;
				if (awk_file_exists_adapter(filename) == true) {
					is_need_truncate = true;
				}
				break;
			case '+':
				mode_flag |= FS_O_RDWR;
				break;
			default:
				break;
		}
	}
	rc = fs_open(zfp, filename, mode_flag);
	if (rc < 0) {
		goto err_handle;
	}
	if (is_need_truncate) {
		fs_truncate(zfp, 0);
	}
	return zfp;
err_handle:
	awk_mem_free_adapter(zfp);
	return NULL;
}
/**
* @brief 关闭文件的回调，参考fclose
* @param {void*} handler
* @return {*}
*/
int awk_file_close_adapter(void* handler)
{
	printf("awk_file_open_close, %p\n", handler);
	int rc = 0;

	if (handler) {
		rc = fs_close(handler);
		awk_mem_free_adapter(handler);
	}

	return rc;
}
/**
* @brief 读文件的回调，参考fread
* @param {void} *ptr buffer
* @param {size_t} size 大小
* @param {size_t} nmembs 块数量
* @param {void*} handler 文件指针
* @return {*}
*/
size_t awk_file_read_adapter(void *ptr, size_t size, void* handler)
{
	sys_trace_void(SYS_TRACE_ID_FATFS_READ);
	//int stc = os_uptime_get_32();
	int rc = 0;
	if (handler == NULL) {
		printf("awk_file_read_adapter failed, handler null!\n");
		return -1;
	}
	rc = fs_read(handler, ptr, size);
	//printf("awk_file_read_adapter, stc:%u, spend %u ms\n", os_uptime_get_32(), os_uptime_get_32() - stc);
	if (rc != size) {
		printf("awk_file_read_adapter failed, rc:%d, size %d\n", rc, size);
	}
	sys_trace_end_call(SYS_TRACE_ID_FATFS_READ);
	return rc;
}
/**
* @brief 写文件时回调，参考fwrite
* @param {void} *ptr buffer
* @param {size_t} size 大小
* @param {size_t} nmembs 块数量
* @param {void*} handler 文件指针
* @return {*}
*/
size_t awk_file_write_adapter(void *ptr, size_t size, void* handler)
{
	int rc = 0;
	//printf("awk_file_write_adapter, %p %d\n", handler, size);
	if (handler == NULL) {
		printf("awk_file_write_adapter failed, handler null!\n");
		return -1;
	}
	rc = fs_write(handler, ptr, size);
	if (rc != size) {
		printf("awk_file_write_adapter failed, rc:%d, size %d\n", rc, size);
	}
	return rc;
}

/**
* @brief 参考fseek
* @param {void} *handler 文件指针
* @param {long} offset 偏移
* @param {int} where 位置
* @return {*}
*/
int awk_file_seek_adapter(void *handler, long offset, int where)
{
	sys_trace_u32x2(SYS_TRACE_ID_FATFS_SEEK, (uint32_t)where, (uint32_t)offset);
	//int stc = os_uptime_get_32();
	int rc;
	//printf("awk_file_seek_adapter in, handler %p where %d, offset %ld\n", handler, where, offset);
	if (handler == NULL) {
		return -1;
	}
	rc = fs_seek(handler, offset, where);
	//printf("awk_file_seek_adapter, stc:%u, spend %u ms\n", os_uptime_get_32(), os_uptime_get_32() - stc);
	sys_trace_end_call(SYS_TRACE_ID_FATFS_SEEK);
	return rc;
}
/**
* @brief 参考fflush
* @param {void} *handler 文件指针
* @return {*}
*/
int awk_file_flush_adapter(void *handler)
{
	//printf("awk_file_flush_adapter in, handler %p\n", handler);
	if (handler == NULL) {
		printf("awk_file_flush_adapter failed, handler null!\n");
		return -1;
	}
	return fs_sync(handler);
}
/**
* @brief 文件是否存在
* @param {char} *path 文件路径
* @return {*}
*/
bool awk_file_exists_adapter(const char *path)
{
	//printf("awk_file_exists_adapter, file name:%s\n", path);
	int rc;
	struct fs_dirent entry;

	rc = fs_stat(path, &entry);
	if (rc < 0 || entry.type != FS_DIR_ENTRY_FILE) {
		printf("awk_file_exists_adapter, return false\n");
		return false;
	} else {
		printf("awk_file_exists_adapter, return true\n");
		return true;
	}
}
/**
* @brief 移除文件，参考remove
* @param {char} *path 路径
* @return {*}
*/
int awk_file_remove_adapter(const char *path)
{
	return fs_unlink(path);
}
/**
* @brief 文件夹是否存在
* @param {char} *path 路径
* @return {*}
*/
bool awk_file_dir_exists_adapter(const char *path)
{
	//printf("awk_file_dir_exists_adapter, file name:%s\n", path);
	int rc;
	struct fs_dirent entry;

	rc = fs_stat(path, &entry);
	if (rc < 0 || entry.type != FS_DIR_ENTRY_DIR) {
		printf("awk_file_dir_exists_adapter, return false\n");
		return false;
	} else {
		return true;
	}
}

/**
* @brief 创建文件夹时回调，参考mkdir
* @param {char} *path 路径
* @param {uint16_t} model 权限
* @return {*}
*/
int awk_file_mkdir_adapter(const char *path, uint16_t model)
{
	return fs_mkdir(path);
}
/**
* @brief 删除文件夹时回调，参考rmdir
* @param {char} *path 路径
* @return {*}
*/
int awk_file_rmdir_adapter(const char *path)
{
	return fs_unlink(path);
}
/**
* @brief 打开文件夹时的回调，参考opendir
* @param {char} *path 路径
* @return {*}
*/
void* awk_file_opendir_adapter(const char *path)
{
	//printf("awk_file_opendir_adapter, path: %s\n", path);
	int rc = 0;
	struct fs_dir_t *zdp;

	zdp = awk_mem_malloc_adapter(sizeof(struct fs_dir_t));
	memset(zdp, 0, sizeof(struct fs_dir_t));
	if (zdp == NULL) {
		return NULL;
	}
	rc = fs_opendir(zdp, path);
	if (rc < 0) {
		awk_mem_free_adapter(zdp);
		return NULL;
	}
	return zdp;
}
/**
* @brief 关闭文件夹时的回调
* @param {void} *dir 文件夹指针
* @return {*}
*/
int awk_file_closedir_adapter(void *dir)
{
	int rc = -1;

	if (dir) {
		rc = fs_closedir(dir);
		awk_mem_free_adapter(dir);
	}

	return rc;
}
/**
* @brief 读文件夹的内容
* @param {void} *dir 文件夹指针
* @param {awk_readdir_result} result 填充读取的结果
* @return {成功或失败}
*/
bool awk_file_readdir_adapter(void *dir, awk_readdir_result *result)
{
	//printf("awk_file_readdir_adapter, handler: %p\n", dir);
	struct fs_dirent entry;
	int rc;
	int file_cnt = 0;

	memset(result, 0, sizeof(awk_readdir_result));

	for (;;) {
		rc = fs_readdir(dir, &entry);
		/* entry.name[0] == 0 means end-of-dir */
		if (rc || entry.name[0] == 0) {
			break;
		}
		printf("%s\n", entry.name);
		file_cnt ++;
		result->nodes = awk_mem_realloc_adapter(result->nodes, sizeof(awk_file_node) * file_cnt);
		memset(&result->nodes[file_cnt - 1], 0, sizeof(awk_file_node));
		strcpy(result->nodes[file_cnt - 1].file_name, entry.name);
		result->nodes[file_cnt - 1].file_size = entry.size;
	}

	result->size = file_cnt;
	#if 0
	printf("result->size:%d\n", result->size);
	printf("result->nodes:\n");
	for (int i=0;i<file_cnt; i++) {
		printf("%s\n", result->nodes[i].file_name);
	}
	#endif
	return true;
}
/**
* @brief 获取文件大小时的回调
* @param {char} *path 路径
* @return {*}
*/
size_t awk_file_get_size_adapter(const char *path)
{
	int rc;
	struct fs_dirent entry;
	rc = fs_stat(path, &entry);
	//printf("awk_file_get_size_adapter, path:%s, size:%d\n", path, entry.size);
	return entry.size;
}
/**
* @brief 获取文件最后访问的回调
* @param {char} *path 路径
* @return {*}
*/
long awk_file_get_last_access_adapter(const char *path)
{
	printf("awk_file_get_last_access_adapter in, path: %s\n", path);
	return 0;
}
/**
* @brief 重命名文件，参考rename
* @param {char} *old_name
* @param {char} *new_name
* @return {*}
*/

int awk_file_rename_adapter(const char *old_name, const char *new_name)
{
	//printf("awk_file_rename_adapter in, old_name:%s, new_name:%s\n", old_name, new_name);
	return fs_rename(old_name, new_name);
}

/**
* @brief  解压适配接口
* @param {char} *zip_file 压缩文件路径
* @param {char} *out_dir 输出文件目录
* @return {*} 成功或失败
*/
bool awk_file_unzip_adapter(const char *zip_file, const char *out_dir)
{
	printf("[%s] in, zip_file: %s, outdir: %s\n", __func__, zip_file, out_dir);
	return false;
}


#include <drivers/rtc.h>
#include <stdio.h>

uint64_t awk_get_system_time_adapter(void)
{
	int ret;
    struct rtc_time rtc_time = {0};
    const struct device *rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
	uint32_t time;

    if (rtc_dev == NULL) {
        return 0;
    }

    ret = rtc_get_time(rtc_dev, &rtc_time);
    if (ret) {
        return 0;
    }

    rtc_tm_to_time(&rtc_time, &time);

    return time;
}

int awk_printf_adapter(const char* __fmt, ...)
{
	char log_buf[512] = {0};
	va_list valist;
	int ret;

    va_start(valist,__fmt);
    ret = vsnprintf(log_buf, sizeof(log_buf), __fmt, valist);
    printf("\n==awk==:%s\n", log_buf);
    va_end(valist);

	return ret;
}


/**
 * @brief 发送请求，请求的处理可以在异步线程中发起
 * @param request aos请求类
 * @param callback 返回结果回调接口
 * @return 请求id 请求id不能重复，可以自增处理
 */
uint64_t awk_aos_network_adapter_send_adapter(const awk_http_request_t *request, awk_http_response_callback_t *callback)
{
	static uint32_t id = 0;
	printf("awk_aos_network_adapter_send_adapter, url: %s\n", request->url);
	id++;
	return id;
}

/**
 * @brief 取消请求，需要在主流程线程中处理
 * @param request_id 需要取消请求的id
 */
void awk_aos_network_adapter_cancel_adapter(uint64_t request_id)
{
	printf("awk_aos_network_adapter_cancel_adapter, in\n");
}

void device_active_callback(bool result, int code, const char* msg)
{
	printf("device_active_callback, %d, code: %d, msg: %s\n", result, code, msg);
}

uint64_t awk_get_thread_id_adapter(void)
{
	uint64_t thread_id = (uint64_t)((uint32_t)k_current_get());
	//printk("awk_get_thread_id_adapter: %llu\n", thread_id);
	return thread_id;
}

