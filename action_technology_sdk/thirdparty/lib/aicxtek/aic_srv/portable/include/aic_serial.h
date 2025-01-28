#ifndef __PORTABLE_INCLUDE_SERIAL_H__
#define __PORTABLE_INCLUDE_SERIAL_H__

void *aic_uart_open(int com);
void aic_uart_close(void *handle);
size_t aic_uart_write(void *handle, const void *buffer, size_t size);
size_t aic_uart_read(void *handle, void *buffer, size_t size);

#endif /* __PORTABLE_INCLUDE_SERIAL_H__ */
