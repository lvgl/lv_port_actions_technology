/*
 ********************************************************************************
 *                            us212a
 *                (c) Copyright 2002-2012, Actions Co,Ld.
 *                        All Right Reserved
 *
 * FileName: compensation.h     Author:          Date:
 * Description:
 * Others:
 * History:
 * <author>    <time>       <version >    <desc>
 *  kehaitao                   1.0         build this file
 ********************************************************************************
 */

#ifndef __COMPENSATION_PROTOCOL_H__
#define __COMPENSATION_PROTOCOL_H__

///////////////////////////////////////////////////////////////////////////////////
#define K_CMD_EXIT                 0xfa
#define K_CMD_SET_RX_MODE          1
#define K_CMD_SET_FREQ             2
#define K_CMD_DECODE               3
#define K_CMD_SET_GAIN_INDEX       4
#define K_CMD_GET_IQ_DATA          5
#define K_CMD_SET_IQ_DATA_LEN      6
#define K_CMD_CLR_IQ_BUF           7
#define K_CMD_CFG_AGC              8
#define K_CMD_SET_TX_MODE          9
#define K_CMD_SET_ATTEN            10
#define K_CMD_ENCODE               11
#define K_CMD_SET_TX_POWER         12
#define K_CMD_SET_RX_DMA_MODE      13
#define K_CMD_GET_INIT_FREQ        14
#define K_CMD_GET_RF_DATA          15
#define K_CMD_SET_TX_POWER_OFFSET  16
#define K_CMD_SET_PAYLOAD          0x17
#define K_CMD_SET_ACCESSCODE       0x18
#define K_CMD_EXCUTE               0x19
#define K_CMD_EXCUTE_STOP          0x1a

#define K_CMD_DECODE_FT_P_IF_RX    0x20
#define K_CMD_DECODE_FT_N_IF_RX    0x21


#define K_CMD_FRE_HOP          0x30
#define K_CMD_BLE_SWITCH    0x31       //bt switch to ble
#define K_CMD_BT_SWITCH      0x32       //ble switch wo br/edr
#define K_CMD_RX_EXCUTE       0x33
#define K_CMD_RX_EXCUTE_STOP      0x34
#define K_CMD_GET_RX_REPORT      0x35
#define K_CMD_SET_TX_PAYLOAD_LEN      0x36

#define K_CMD_WRITE_EFUSE          0x40
#define K_CMD_READ_EFUSE           0x41

#define K_CMD_READ_LONGTERM_RSSI 0x42   //long term rssi
#define K_CMD_READ_LONGTERM_RSSI_5CHANNEL 0x43
#define K_CMD_READ_RSSI       0x44

//for rf debug
#define K_CMD_REG_WRITE_SINGLE                0x60
#define K_CMD_REG_READ_SINGLE                  0x61
#define K_CMD_REG_WRITE_SINGLE_INRUN     0x62
#define K_CMD_REG_READ_SINGLE_INRUN      0x63
#define K_CMD_REG_WRITE_ALL                      0x64
#define K_CMD_REG_READ_ALL                       0x65
#define K_CMD_REG_WRITE_ALL_INRUN          0x66
#define K_CMD_REG_READ_ALL_INRUN            0x67
#define K_CMD_ADC_DATA_DUMP                  0x68
#define K_CMD_DAC_DATA_DUMP                  0x69

#define K_CMD_GET_RX_CFO_RSSI		  0xF1
/////////////////////////////////////////////////////////////
//rx mode
#define K_RX_MODE_DH1                     (0)
#define K_RX_MODE_2DH1                   (1)
#define K_RX_MODE_3DH1                   (2)
#define K_RX_MODE_LE                        (3)
#define K_RX_MODE_DH1_00001111     (4)
#define K_RX_MODE_DH1_01010101     (5)
#define K_RX_MODE_3DH5                   (6)

#define K_RX_MODE_LE_PN9                 (0x10)
#define K_RX_MODE_LE_00001111         (0x11)
#define K_RX_MODE_LE_01010101         (0x12)

// payload type command
#define K_FCC_PN9           0
#define K_FCC_PN15          1
#define K_FCC_ALL_0         2  // singtone @-modulation_index*5e5Hz for dh1/3/5 packet
#define K_FCC_ALL_1         3  // singtone @modulation_index*5e5Hz  for dh1/3/5 packet
#define K_FCC_00001111      4
#define K_FCC_01010101      5
#define K_PAYLOAD_BYTE_INC  6  // trasmit symbol 0,1,......0xff, 0,1, 0xff

#endif /* __COMPENSATION_PROTOCOL_H__ */
