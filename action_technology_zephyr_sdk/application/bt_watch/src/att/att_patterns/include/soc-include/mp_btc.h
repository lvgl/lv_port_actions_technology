#ifndef __MP_BTC_H
#define __MP_BTC_H

//btc function
void mp_btc_init(void);

void mp_btc_deinit(void);

int btdrv_send(uint32_t type, uint8_t *data, uint16_t len);

int mp_btc_read_hci_data(uint8_t *data, uint32_t len);

int mp_btc_rx_init(uint8_t channel);

int mp_btc_rx_begin(void);

int mp_btc_rx_stop(void);


int mp_btc_clear_hci_rx_buffer(void);

#endif

