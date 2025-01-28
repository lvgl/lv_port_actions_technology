#ifndef __PORTABLE_INCLUDE_SERIAL_H__
#define __PORTABLE_INCLUDE_SERIAL_H__

typedef int (*uart_data_callback_t)(uint8_t *command_data, uint16_t command_size);

void *act_uart_open(int com);
void act_uart_close(void *handle);
size_t act_uart_write(void *handle, const void *buffer, size_t size);
size_t act_uart_read(void *handle, void *buffer, size_t size);

int act_uart_register_data_callback(uart_data_callback_t callback);

#endif /* __PORTABLE_INCLUDE_SERIAL_H__ */
