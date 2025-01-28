/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2019-2-18-4:09:22             1.0             build this file
 ********************************************************************************/
/*!
 * \file     temp_comp.h
 * \brief
 * \author
 * \version  1.0
 * \date  2019-2-18-4:09:22
 *******************************************************************************/

#ifndef TEMP_COMP_H_
#define TEMP_COMP_H_

#define CFG_MAX_CAP_TEMP_COMP    10

enum CFG_TYPE_CAP_TEMP
{
	CAP_TEMP_NA   = 0x7F,        // <"NA">
	CAP_TEMP_N_40 = 0x100 - 40,  // <"-40">
	CAP_TEMP_N_35 = 0x100 - 35,  // <"-35">
	CAP_TEMP_N_30 = 0x100 - 30,  // <"-30">
	CAP_TEMP_N_25 = 0x100 - 25,  // <"-25">
	CAP_TEMP_N_20 = 0x100 - 20,  // <"-20">
	CAP_TEMP_N_15 = 0x100 - 15,  // <"-15">
	CAP_TEMP_N_10 = 0x100 - 10,  // <"-10">
	CAP_TEMP_N_5  = 0x100 - 5,   // <"-5">
	CAP_TEMP_0    = 0,           // <"0">
	CAP_TEMP_P_5  = 5,           // <"+5">
	CAP_TEMP_P_10 = 10,          // <"+10">
	CAP_TEMP_P_15 = 15,          // <"+15">
	CAP_TEMP_P_20 = 20,          // <"+20">
	CAP_TEMP_P_25 = 25,          // <"+25">
	CAP_TEMP_P_30 = 30,          // <"+30">
	CAP_TEMP_P_35 = 35,          // <"+35">
	CAP_TEMP_P_40 = 40,          // <"+40">
	CAP_TEMP_P_45 = 45,          // <"+45">
	CAP_TEMP_P_50 = 50,          // <"+50">
	CAP_TEMP_P_55 = 55,          // <"+55">
	CAP_TEMP_P_60 = 60,          // <"+60">
	CAP_TEMP_P_65 = 65,          // <"+65">
	CAP_TEMP_P_70 = 70,          // <"+70">
	CAP_TEMP_P_75 = 75,          // <"+75">
	CAP_TEMP_P_80 = 80,          // <"+80">
};

typedef struct  // <"temperature compensation">
{
	uint8_t  Cap_Temp;  // <"temperature C", CFG_TYPE_CAP_TEMP>
	uint8_t   Cap_Comp;  // <"compensation pF", -10.0 ~ 10.0, float_x10>
} CFG_Type_Cap_Temp_Comp;

typedef struct  // <"temperature compensation", CFG_CATEGORY_BLUETOOTH>
{
	uint8_t  Enable_Cap_Temp_Comp;                       // <"enable temperature compensation", CFG_TYPE_BOOL>
	CFG_Type_Cap_Temp_Comp  Table[CFG_MAX_CAP_TEMP_COMP];  // <"temperature compensation", CFG_Type_Cap_Temp_Comp>
} CFG_Struct_Cap_Temp_Comp;

typedef struct
{
	CFG_Struct_Cap_Temp_Comp  configs;
	uint8_t  enabled;
	uint8_t  table_size;
	uint8_t  normal_cap;
	int  last_temp;
} cap_temp_comp_ctrl_t;

#endif /* TEMP_COMP_H_ */
