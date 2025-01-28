#ifndef __PORTABLE_INCLUDE_AT_COMMAND_H__
#define __PORTABLE_INCLUDE_AT_COMMAND_H__
#include <wifi.h>

int at_command_wifi_scan(void);
int at_command_wifi_connect(const uint8_t *wifi, const uint8_t *pwd);
int at_command_wifi_disconnect(void);
int at_command_http_get(const uint8_t *url);
int at_command_http_stop(void);
int at_command_http_get_resume(const uint8_t *url, int offset, int length);
int at_command_http_post(const uint8_t *url, uint8_t *playload, uint32_t playload_size);
int at_command_init(void* uart_handle, wifi_ops_callback_t *ops_callback);

#endif /* __PORTABLE_INCLUDE_AT_COMMAND_H__ */