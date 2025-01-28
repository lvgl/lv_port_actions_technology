/*
 * Copyright (c) 2019 Actions Semiconductor Co, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __NSM_INTERFACE_H__
#define __NSM_INTERFACE_H__

#include <ctype.h>

enum {
	NSM_BLE_TX_CMD_POWER,
	NSM_BLE_TX_CMD_MODULE1,
	NSM_BLE_TX_CMD_MODULE2,
	NSM_BLE_TX_CMD_MAX,
};

/** @brief 启动BLE TX测试
 *
 *  调用接口后会进入非信令TX模式(tx access code 0x71764129)，系统将无法运行，测试完成后会自动重启.
 *
 *  @param timeout 持续发送TX数据时间，单位秒; 0xFFFF为不重启一直发;
 *  @param channel 发送数据的信道编号 range 0~39.
 *  @param power 发射功率. range 20~43, 对应-10dbm~13dbm
 *  @param cmd 操作码
 *              0x00：测试发送功率、带内辐射
 *              0x01：测试调制特性1
 *              0x02：测试调制特性2、频偏
 *  @param mode BLE mode, range 0~1, 0: BLE 1M, 1: BLE 2M
 *  @return  0  success, -1 fail.
 */
int nsm_ble_tx_test(u16_t timeout, u8_t channel, u8_t power, int cmd, u8_t mode);

/** @brief 启动BLE RX测试
 *
 *  调用接口后会进入非信令RX模式，系统将无法运行，测试完成后会自动重启.
 *
 *  @param timeout 持续接收RX数据的时间，单位秒; 0xFFFF为不重启一直收;
 *  @param channel 接收数据的信道编号 range 0~39.
 *  @param mac , 发数仪器的低四位地址, 数据长度4bytes, 小端模式. 
 *                  如mac = 0x12345678将对应mac[4] = {0x78, 0x56, 0x34, 0x12}
 *  @param mode BLE mode, range 0~1, 0: BLE 1M, 1: BLE 2M
 *  @return  0  success, -1 fail.
 */
int nsm_ble_rx_test(u16_t timeout, u8_t channel, u8_t *mac, u8_t mode);

/** @brief 获取上一次BLE RX测试结果
 *
 *  @return  -1 fail, 其它值 接收到的正确包数.
 */
int nsm_ble_rx_report_get(void);

/** @brief 获取上一次BLE RX测试的RSSI平均值
 *
 *  @return RSSI值.
 */
int8_t nsm_ble_rx_rssi_get(void);

/** @brief 进入BLE TX压测模式，断开BLE，发送不可连接广播
 *   2402MHz(信道0 index 37),2426MHz(信道12 index 38),2480MHz(信道39 index 39), 每60s交替发送广播数据，发包间隔100ms;
 *   |信道0发60s|信道12发60s|信道39发60s|......, 持续2小时;
 *  @return  0  success, -1 fail.
 */
int nsm_ble_tx_stress_normal(void);

/** @brief 进入UART 非信令模式
 *  uart的测试指令参考ACTIONS 308x BT_TEST_command_list.xlsx
 *  @return  0  success, -1 fail.
 */
int nsm_pcba_uart_test(void);

#endif  /* __NSM_INTERFACE_H__ */

