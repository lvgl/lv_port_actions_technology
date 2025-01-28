
#ifndef _FCC_DRV_EXTERN_H_
#define _FCC_DRV_EXTERN_H_

#include "fcc_cmd.h"

#define FCC_WORD_MODE_SHIFT   (0) /* 0 - fcc, 1 - att, 2 - ft */
#define FCC_UART_PORT_SHIFT   (2) /* 0 - uart0, 1 - uart1 */
#define FCC_UART_BAUD_SHIFT   (16) /* baud / 100, 1152 = 115200 */

#define FCC_UART_PORT         (1)
#define FCC_UART_BAUD         (115200)
#define FCC_UART_TX           (15)
#define FCC_UART_RX           (14)

/*************************************************/
typedef struct
{
    unsigned int rx_packet_cnts;
    unsigned int total_error_bits;
    unsigned int packet_len;
    signed short rx_rssi;          // -127 ~ 127 (0  =  0dBm  unit is 0.5 dBm)
    signed short cfo_index;        // -127 ~ 127 ( unit .tbd)
} mp_rx_report_t;

extern int mp_install_fcc_drv(unsigned int para);
extern int mp_uninstall_fcc_drv(void);
extern int mp_bt_packet_receive_process(unsigned int channel, unsigned int packet_counter);
extern int mp_bt_rx_report(mp_rx_report_t *rx_report);
extern int mp_bt_packet_send_process(unsigned int channel, unsigned int power, unsigned int packet_counter);
extern int mp_fcc_drv_send_data_to_transmitter(unsigned char *buf, unsigned int len);
extern int mp_fcc_drv_recv_data_from_transmitter(unsigned char *buf, unsigned int max_len);
extern void mp_task_rx_get_report(void);
extern int mp_bt_packet_receive_start(void);
extern int mp_bt_packet_receive_stop(void);

bool att_bttool_rx_begin(u8_t channel);
bool att_bttool_rx_stop(void);
void att_bttool_clear_rxfifo(void);
int mp_bt_rx_report(mp_rx_report_t *rx_report);
void att_btc_test_init(void);
void att_btc_test_deinit(void);


#define act_test_install_fcc_drv(a)                mp_install_fcc_drv((a))
#define act_test_uninstall_fcc_drv()               mp_uninstall_fcc_drv()
#define act_test_bt_packet_receive_process(a,b)    mp_bt_packet_receive_process((a),(b))
#define act_test_bt_rx_report(a)                   mp_bt_rx_report((a))
#define act_test_bt_packet_send_process(a,b,c)     mp_bt_packet_send_process((a),(b),(c))
#define act_test_fcc_drv_send_data_to_transmitter(a,b)    mp_fcc_drv_send_data_to_transmitter((a),(b))
#define act_test_fcc_drv_recv_data_from_transmitter(a,b)  mp_fcc_drv_recv_data_from_transmitter((a),(b))
#define act_test_bt_packet_receive_start()         mp_bt_packet_receive_start()
#define act_test_bt_packet_receive_stop()          mp_bt_packet_receive_stop()

#endif
