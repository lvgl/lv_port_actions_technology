/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file input manager interface 
 */

#ifndef __INPUT_MANGER_TYPE_H__
#define __INPUT_MANGER_TYPE_H__

/*
 * Keys and buttons
 */
#define KEY_RESERVED			0
#define KEY_POWER           	1
#define KEY_PREVIOUSSONG    	2
#define KEY_NEXTSONG        	3
#define KEY_VOL          		4
#define KEY_VOLUMEUP        	5
#define KEY_VOLUMEDOWN      	6
#define KEY_MENU            	7
#define KEY_CONNECT         	8
#define KEY_TBD	            	9
#define KEY_MUTE            	10
#define KEY_PAUSE_AND_RESUME 	11
#define KEY_FOLDER_ADD			12
#define KEY_FOLDER_SUB			13
#define KEY_NEXT_VOLADD			14
#define KEY_PREV_VOLSUB			15
#define KEY_NUM0 				16
#define KEY_NUM1				17
#define KEY_NUM2				18
#define KEY_NUM3				19
#define KEY_NUM4				20
#define KEY_NUM5				21
#define KEY_NUM6				22
#define KEY_NUM7				23			
#define KEY_NUM8				24
#define KEY_NUM9				25
#define KEY_CH_ADD				26
#define KEY_CH_SUB				27
#define KEY_PAUSE           	28
#define KEY_RESUME          	29
#define KEY_KONB_CLOCKWISE      30
#define KEY_KONB_ANTICLOCKWISE  31

#define KEY_F1              	40
#define KEY_F2              	41
#define KEY_F3             	 	42
#define KEY_F4              	43
#define KEY_F5              	44
#define KEY_F6              	45
#define KEY_ADFU            	46

/*LRADC combo key value*/
#define KEY_COMBO_1				50
#define KEY_COMBO_2				51
#define KEY_COMBO_3				52

/*Tap key*/
#define KEY_TAP					53       


/* We avoid low common keys in module aliases so they don't get huge. */
#define KEY_MIN_INTERESTING	KEY_MUTE

/**/
#define LINEIN_DETECT		100

/* virtual keys: simulating gesture events */
#define KEY_GESTURE_DOWN    110 /* GESTURE_DROP_DOWN */
#define KEY_GESTURE_UP      111 /* GESTURE_DROP_UP */
#define KEY_GESTURE_LEFT    112 /* GESTURE_DROP_LEFT */
#define KEY_GESTURE_RIGHT   113 /* GESTURE_DROP_RIGHT */

#define KEY_MAX			0x2ff
#define KEY_CNT			(KEY_MAX+1)

/*
 * Event types
 */
#define EV_SYN			0x00
#define EV_KEY			0x01
#define EV_REL			0x02
#define EV_ABS			0x03
#define EV_MSC			0x04
#define EV_SW			0x05
#define EV_SR			0x06  /* Sliding rheostat */
#define EV_LED			0x11
#define EV_SND			0x12
#define EV_REP			0x14
#define EV_FF			0x15
#define EV_PWR			0x16
#define EV_FF_STATUS		0x17
#define EV_MAX			0x1f
#define EV_CNT			(EV_MAX+1)

/** key press type */
typedef enum
{
    /** the max value of key type, it must be less than  MASK_KEY_UP*/
    KEY_TYPE_NULL       	= 0x00000000,
    KEY_TYPE_LONG_DOWN  	= 0x10000000,
    KEY_TYPE_SHORT_DOWN 	= 0x20000000,
    KEY_TYPE_DOUBLE_CLICK 	= 0x40000000,
    KEY_TYPE_HOLD       	= 0x08000000,
    KEY_TYPE_SHORT_UP   	= 0x04000000,
    KEY_TYPE_LONG_UP    	= 0x02000000,
    KEY_TYPE_HOLD_UP    	= 0x01000000,
    KEY_TYPE_LONG6S_UP 		= 0x00800000,
    KEY_TYPE_LONG6S      	= 0x00400000, 
	KEY_TYPE_LONG      		= 0x00200000, 
    KEY_TYPE_TRIPLE_CLICK 	= 0x00100000,
    KEY_TYPE_QUAD_CLICK 	= 0x00080000,
    KEY_TYPE_QUINT_CLICK	= 0x00040000,
    KEY_TYPE_CUSTOMED_SEQUENCE_1 = 0x00020000,  
    KEY_TYPE_CUSTOMED_SEQUENCE_2 = 0x00010000,
    KEY_TYPE_ALL        	= 0xFFFF0000,
}key_type_e;

#define KEY_VALUE(event) ((event) & 0x0000FFFF)
#define KEY_TYPE(event)  ((event) & 0xFFFF0000)
#define KEY_EVENT(value, type) ((value) | (type))

#endif