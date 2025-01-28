/********************************************************************************
 *                              USDK281A
 *                            Module: KERNEL
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>           <version >             <desc>
 *      wuyufan     2015-9-10 09:00     1.0             build this file
 ********************************************************************************/
/*!
 * \file     stub_command.h
 * \brief    内核接口列表
 * \author   wuyufan
 * \version  1.0
 * \date  2015/9/10
 *******************************************************************************/

#ifndef _STUB_COMMAND_H
#define _STUB_COMMAND_H

#define STUB_CMD_OPEN                       (0xFF00)

#define STUB_CMD_BTT_POLL                   (0x0100)

#define STUB_CMD_BTT_ACK                    (0x1FFE)

#define STUB_CMD_BTT_HCI_WRITE              (0x1F01)

#define STUB_CMD_BTT_HCI_READ               (0x1F02)

#define STUB_CMD_BTT_RESET_CONTROLLER       (0x1F03)

#define STUB_CMD_BTT_CLOSE                  (0x0F00)

#define STUB_CMD_BTT_CLOSE_ACK              (0x8F00)

#define STUB_CMD_BTT_DOWNLOAD_PATCH         (0x1F04)

#define STUB_CMD_RESET_HOST                 (0x1F05)

#define STUB_CMD_BTT_READ_TRIM_CAP          (0x1F86)

#define STUB_CMD_BTT_WRITE_TRIM_CAP         (0x1F06)

//旧版ASET工具命令字
#define STUB_CMD_ASET_POLL                  (0x3100)

#define STUB_CMD_ASET_DOWNLOAD_DATA         (0x3200)

#define STUB_CMD_ASET_UPLOAD_DATA           (0x3400)

#define STUB_CMD_ASET_READ_VOL              (0x3500)

//新版ASET工具命令字

//Read Staus
#define STUB_CMD_ASET_READ_STATUS           (0x0300)

#define STUB_CMD_ASET_WRITE_STATUS          (0x0380)

#define STUB_CMD_ASET_WRITE_APPLICATION_PROPERTIES   (0x0390)

//Read Volume
#define STUB_CMD_ASET_READ_VOLUME           (0x0301)

#define STUB_CMD_ASET_WRITE_VOLUME          (0x0381)

#define STUB_CMD_ASET_READ_EQ_DATA          (0x0302)

#define STUB_CMD_ASET_WRITE_EQ_DATA         (0x0382)

#define STUB_CMD_ASET_READ_VBASS_DATA       (0x0303)

#define STUB_CMD_ASET_WRITE_VBASS_DATA      (0x0383)

#define STUB_CMD_ASET_READ_TE_DATA          (0x0304)

#define STUB_CMD_ASET_WRITE_TE_DATA         (0x0384)

#define STUB_CMD_ASET_READ_SURR_DATA        (0x0305)

#define STUB_CMD_ASET_WRITE_SURR_DATA       (0x0385)

#define STUB_CMD_ASET_READ_LIMITER_DATA     (0x0306)

#define STUB_CMD_ASET_WRITE_LIMITER_DATA    (0x0386)

#define STUB_CMD_ASET_READ_SPKCMP_DATA      (0x0307)

#define STUB_CMD_ASET_WRITE_SPKCMP_DATA     (0x0387)

#define STUB_CMD_ASET_READ_MDRC_DATA        (0x0308)

#define STUB_CMD_ASET_WRITE_MDRC_DATA       (0x0388)

#define STUB_CMD_ASET_READ_MDRC2_DATA       (0x0309)

#define STUB_CMD_ASET_WRITE_MDRC2_DATA      (0x0389)




#define STUB_CMD_ASET_READ_SEE_DATA         (0x030C)

#define STUB_CMD_ASET_WRITE_SEE_DATA        (0x038C)

#define STUB_CMD_ASET_READ_SEW_DATA         (0x030D)

#define STUB_CMD_ASET_WRITE_SEW_DATA        (0x038D)

#define STUB_CMD_ASET_READ_SD_DATA          (0x030E)

#define STUB_CMD_ASET_WRITE_SD_DATA         (0x038E)



#define STUB_CMD_ASET_READ_LFRC_DATA_BASE   (0x0311)

#define STUB_CMD_ASET_WRITE_LFRC_DATA_BASE  (0x0391)

#define STUB_CMD_ASET_READ_LFRC1_DATA       (STUB_CMD_ASET_READ_LFRC_DATA_BASE)

#define STUB_CMD_ASET_WRITE_LFRC1_DATA      (STUB_CMD_ASET_WRITE_LFRC_DATA_BASE)

#define STUB_CMD_ASET_READ_LFRC2_DATA       (STUB_CMD_ASET_READ_LFRC_DATA_BASE+1)

#define STUB_CMD_ASET_WRITE_LFRC2_DATA      (STUB_CMD_ASET_WRITE_LFRC_DATA_BASE+1)

#define STUB_CMD_ASET_READ_LFRC3_DATA       (STUB_CMD_ASET_READ_LFRC_DATA_BASE+2)

#define STUB_CMD_ASET_WRITE_LFRC3_DATA      (STUB_CMD_ASET_WRITE_LFRC_DATA_BASE+2)

#define STUB_CMD_ASET_READ_LFRC4_DATA       (STUB_CMD_ASET_READ_LFRC_DATA_BASE+3)

#define STUB_CMD_ASET_WRITE_LFRC4_DATA      (STUB_CMD_ASET_WRITE_LFRC_DATA_BASE+3)



#define STUB_CMD_ASET_READ_RFRC_DATA_BASE   (0x0315)

#define STUB_CMD_ASET_WRITE_RFRC_DATA_BASE  (0x0395)


#define STUB_CMD_ASET_READ_RFRC1_DATA       (STUB_CMD_ASET_READ_RFRC_DATA_BASE)

#define STUB_CMD_ASET_WRITE_RFRC1_DATA      (STUB_CMD_ASET_WRITE_RFRC_DATA_BASE)

#define STUB_CMD_ASET_READ_RFRC2_DATA       (STUB_CMD_ASET_READ_RFRC_DATA_BASE+1)

#define STUB_CMD_ASET_WRITE_RFRC2_DATA      (STUB_CMD_ASET_WRITE_RFRC_DATA_BASE+1)

#define STUB_CMD_ASET_READ_RFRC3_DATA       (STUB_CMD_ASET_READ_RFRC_DATA_BASE+2)

#define STUB_CMD_ASET_WRITE_RFRC3_DATA      (STUB_CMD_ASET_WRITE_RFRC_DATA_BASE+2)

#define STUB_CMD_ASET_READ_RFRC4_DATA       (STUB_CMD_ASET_READ_RFRC_DATA_BASE+3)

#define STUB_CMD_ASET_WRITE_RFRC4_DATA      (STUB_CMD_ASET_WRITE_RFRC_DATA_BASE+3)


#define STUB_CMD_ASET_READ_MAIN_SWITCH      (0x0319)

#define STUB_CMD_ASET_READ_EQ2_DATA         (0x0321)
#define STUB_CMD_ASET_READ_EQ3_DATA         (0x0322)


#define STUB_CMD_ASET_ACK                   (0x03FE)


//兼容标准音效流程增加的宏定义
#define STUB_CMD_AUDIOPP_SELECT                             (0x030F)

#define STUB_CMD_ASET_READ_MDRC_DATA_STANDARD_MODE          (0x032A)

#define STUB_CMD_ASET_READ_MDRC2_DATA_STANDARD_MODE         (0x0329)

#define STUB_CMD_ASET_READ_COMPRESSOR_DATA_STANDARD_MODE    (0x032B)


//ATT工具命令字
typedef enum
{
    STUB_CMD_ATT_START                      = 0x0400,
    STUB_CMD_ATT_STOP                       = 0x0401,
    STUB_CMD_ATT_PAUSE                      = 0x0402,
    STUB_CMD_ATT_RESUME                     = 0x0403,
    STUB_CMD_ATT_GET_TEST_ID                = 0x0404,
    STUB_CMD_ATT_GET_TEST_ARG               = 0x0405,
    STUB_CMD_ATT_REPORT_TRESULT             = 0x0406,
    STUB_CMD_ATT_PRINT_LOG                  = 0x0407,
    STUB_CMD_ATT_FOPEN                      = 0x0408,
    STUB_CMD_ATT_FREAD                      = 0x0409,

    STUB_CMD_ATT_REPORT_PARA                = 0x040C,
    STUB_CMD_ATT_GET_TEST_RESULT            = 0x040D,
    STUB_CMD_ATT_INFORM_STATE               = 0x040E,
}ATT_COMMAND_E;


//waves工具命令字
typedef enum
{
    STUB_CMD_WAVES_ASET_ACK                 = 0x07FE,
    STUB_CMD_WAVES_ASET_READ_COEFF_PROPERTY = 0x0740,
    STUB_CMD_WAVES_ASET_WRITE_COEFF_CONTENT = 0x0741,
}WAVES_COMMAND_E;
#endif

