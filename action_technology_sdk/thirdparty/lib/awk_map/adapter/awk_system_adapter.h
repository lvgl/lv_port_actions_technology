#ifndef __AWK_SYSTEM_ADAPTER_H__
#define __AWK_SYSTEM_ADAPTER_H__

void* awk_mem_malloc_adapter(size_t size);
void awk_mem_free_adapter(void* ptr);
void* awk_mem_calloc_adapter(size_t count, size_t size);
void* awk_mem_realloc_adapter(void* ptr, size_t size);
void* awk_fast_mem_malloc_adapter(size_t size, int type);
void awk_fast_mem_free_adapter(void* ptr);

void* awk_file_open_adapter(const char *filename, const char *mode);
int awk_file_close_adapter(void* handler);
size_t awk_file_read_adapter(void *ptr, size_t size, void* handler);
size_t awk_file_write_adapter(void *ptr, size_t size, void* handler);
int awk_file_seek_adapter(void *handler, long offset, int where);
int awk_file_flush_adapter(void *handler);
bool awk_file_exists_adapter(const char *path);
int awk_file_remove_adapter(const char *path);
bool awk_file_dir_exists_adapter(const char *path);
int awk_file_mkdir_adapter(const char *path, uint16_t model);
int awk_file_rmdir_adapter(const char *path);
void* awk_file_opendir_adapter(const char *path);
int awk_file_closedir_adapter(void *dir);
bool awk_file_readdir_adapter(void *dir, awk_readdir_result *result);
size_t awk_file_get_size_adapter(const char *path);
long awk_file_get_last_access_adapter(const char *path);
int awk_file_rename_adapter(const char *old_name, const char *new_name);
bool awk_file_unzip_adapter(const char *zip_file, const char *out_dir);

uint64_t awk_get_system_time_adapter(void);
int awk_printf_adapter(const char* __fmt, ...);

uint64_t awk_aos_network_adapter_send_adapter(const awk_http_request_t *request, awk_http_response_callback_t *callback);
void awk_aos_network_adapter_cancel_adapter(uint64_t request_id);

void device_active_callback(bool result, int code, const char* msg);
uint64_t awk_get_thread_id_adapter(void);

#endif
