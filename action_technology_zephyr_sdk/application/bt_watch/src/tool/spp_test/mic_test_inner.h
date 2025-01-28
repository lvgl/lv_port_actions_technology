/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2020 Actions Semiconductor. All rights reserved.
 *
 *  \file       mic_test_inner.h
 *  \brief      Actions MIC test API
 *  \author     zhouxl
 *  \version    1.10
 *  \date       2020-08-01
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#ifndef __MIC_TEST_INNER_H__
#define __MIC_TEST_INNER_H__

#ifndef _ASSEMBLER_
#include "spp_test_inner.h"


#define MIC_TEST_HEAD_MAGIC     (0x05)

typedef enum __MIC_TEST_COMMAND
{
    ENABLE_L_MIC_RAW_TEST       = 0x81,
    ENABLE_R_MIC_RAW_TEST       = 0x82,
    ENABLE_BOTH_MIC_ENC_TEST    = 0x85,
}MIC_TEST_COMMAND;

typedef enum __SPK_TEST_COMMAND
{
    CLOSE_EFFECT_EQ_CMD    = 0x91,  /*close effect eq*/
    CLOSE_SPK_EQ_CMD       = 0x92,  /*close speaker eq*/
    SPK_EQ_TEST_CMD        = 0x93,  /*test but not save*/
    SPK_EQ_SET_CMD         = 0x94,  /*save eq*/
}SPK_TEST_COMMAND;

typedef struct
{
    uint8_t head[2];
    uint16_t length;
    uint8_t reserved[2];
    uint8_t cmd;                  ///< command, \ref in MIC_TEST_COMMAND
    uint8_t reserved1;
}mic_test_cmd_packet_t;

#define MIC_TEST_CMD_LEN        (sizeof(mic_test_cmd_packet_t))


typedef struct
{
    uint8_t head[6];
    uint8_t ack_cmd;
    uint8_t reserved[2];
}mic_test_rsp_packet_t;

#endif
#endif  // __MIC_TEST_INNER_H__


