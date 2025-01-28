#ifndef __LINUX_FT5X0X_TS_H__
#define __LINUX_FT5X0X_TS_H__

/*--------------------TP's GPIO configure----------------------*/
#define  TP_I2C_SCL_GPIO_CTL   (GPIO0_CTL + 4*TP_I2C_SCL_GPIO)
#define  TP_I2C_SDA_GPIO_CTL   (GPIO0_CTL + 4*TP_I2C_SDA_GPIO) 
#define  TP_RESET_GPIO_CTL     (GPIO0_CTL + 4*TP_RESET_GPIO) 

#define  TP_I2C_SCL_ODAT       (GPIO_ODAT0 + 4*(TP_I2C_SCL_GPIO/32))
#define  TP_I2C_SDA_ODAT       (GPIO_ODAT0 + 4*(TP_I2C_SDA_GPIO/32))
#define  TP_RESET_ODAT         (GPIO_ODAT0 + 4*(TP_RESET_GPIO/32))

#define  TP_I2C_SCL_IDAT       (GPIO_IDAT0 + 4*(TP_I2C_SCL_GPIO/32))
#define  TP_I2C_SDA_IDAT       (GPIO_IDAT0 + 4*(TP_I2C_SDA_GPIO/32))
    
#define  TP_I2C_SCL_BIT        (0x00000001<<(TP_I2C_SCL_GPIO%32))
#define  TP_I2C_SDA_BIT        (0x00000001<<(TP_I2C_SDA_GPIO%32))
#define  TP_RESET_BIT          (0x00000001<<(TP_RESET_GPIO%32))


/*----------------TP's i2c address configure--------------------*/
#define  TP_I2C_ADDR         0x70

/*-------------------TP's window configure----------------------*/
#define LCD_WIDTH         240
#define LCD_HEIGHT        320

/* -- dirver configure -- */
#define CFG_SUPPORT_AUTO_UPG 0
#define CFG_SUPPORT_UPDATE_PROJECT_SETTING  0
#define CFG_SUPPORT_TOUCH_KEY  0    //touch key, HOME, SEARCH, RETURN etc
#define CFG_SUPPORT_READ_LEFT_DATA  0
#define CFG_DBG_RAW_DATA_READ  0

#define CFG_MAX_TOUCH_POINTS  1 //5
#define CFG_NUMOFKEYS 4                
#define CFG_FTS_CTP_DRIVER_VERSION "2.0"

#define FTS_PACKET_LENGTH      	32//128

#define SCREEN_MAX_X    1728
#define SCREEN_MAX_Y    1024
#define PRESS_MAX       255

#define CFG_POINT_READ_BUF  5//(3 + 6 * (CFG_MAX_TOUCH_POINTS))

#define FT5X0X_NAME	"ft5x0x_ts"//"synaptics_i2c_rmi"//"synaptics-rmi-ts"// 

#define KEY_PRESS       1
#define KEY_RELEASE     0

#define MOVE_DISTANCE	  900

enum ft5x0x_ts_regs {
	FT5X0X_REG_THGROUP					= 0x80,     /* touch threshold, related to sensitivity */
	FT5X0X_REG_THPEAK						= 0x81,
	FT5X0X_REG_THCAL						= 0x82,
	FT5X0X_REG_THWATER					= 0x83,
	FT5X0X_REG_THTEMP					= 0x84,
	FT5X0X_REG_THDIFF						= 0x85,				
	FT5X0X_REG_CTRL						= 0x86,
	FT5X0X_REG_TIMEENTERMONITOR			= 0x87,
	FT5X0X_REG_PERIODACTIVE				= 0x88,      /* report rate */
	FT5X0X_REG_PERIODMONITOR			= 0x89,
	FT5X0X_REG_HEIGHT_B					= 0x8a,
	FT5X0X_REG_MAX_FRAME					= 0x8b,
	FT5X0X_REG_DIST_MOVE					= 0x8c,
	FT5X0X_REG_DIST_POINT				= 0x8d,
	FT5X0X_REG_FEG_FRAME					= 0x8e,
	FT5X0X_REG_SINGLE_CLICK_OFFSET		= 0x8f,
	FT5X0X_REG_DOUBLE_CLICK_TIME_MIN	= 0x90,
	FT5X0X_REG_SINGLE_CLICK_TIME			= 0x91,
	FT5X0X_REG_LEFT_RIGHT_OFFSET		= 0x92,
	FT5X0X_REG_UP_DOWN_OFFSET			= 0x93,
	FT5X0X_REG_DISTANCE_LEFT_RIGHT		= 0x94,
	FT5X0X_REG_DISTANCE_UP_DOWN		= 0x95,
	FT5X0X_REG_ZOOM_DIS_SQR				= 0x96,
	FT5X0X_REG_RADIAN_VALUE				=0x97,
	FT5X0X_REG_MAX_X_HIGH                       	= 0x98,
	FT5X0X_REG_MAX_X_LOW             			= 0x99,
	FT5X0X_REG_MAX_Y_HIGH            			= 0x9a,
	FT5X0X_REG_MAX_Y_LOW             			= 0x9b,
	FT5X0X_REG_K_X_HIGH            			= 0x9c,
	FT5X0X_REG_K_X_LOW             			= 0x9d,
	FT5X0X_REG_K_Y_HIGH            			= 0x9e,
	FT5X0X_REG_K_Y_LOW             			= 0x9f,
	FT5X0X_REG_AUTO_CLB_MODE			= 0xa0,
	FT5X0X_REG_LIB_VERSION_H 				= 0xa1,
	FT5X0X_REG_LIB_VERSION_L 				= 0xa2,		
	FT5X0X_REG_CIPHER						= 0xa3,
	FT5X0X_REG_MODE						= 0xa4,
	FT5X0X_REG_PMODE						= 0xa5,	  /* Power Consume Mode		*/	
	FT5X0X_REG_FIRMID						= 0xa6,   /* Firmware version */
	FT5X0X_REG_STATE						= 0xa7,
	FT5X0X_REG_FT5201ID					= 0xa8,
	FT5X0X_REG_ERR						= 0xa9,
	FT5X0X_REG_CLB						= 0xaa,
};

//FT5X0X_REG_PMODE
#define PMODE_ACTIVE        0x00
#define PMODE_MONITOR       0x01
#define PMODE_STANDBY       0x02
#define PMODE_HIBERNATE     0x03


#ifndef ABS_MT_TOUCH_MAJOR
#define ABS_MT_TOUCH_MAJOR	0x30	/* touching ellipse */
#define ABS_MT_TOUCH_MINOR	0x31	/* (omit if circular) */
#define ABS_MT_WIDTH_MAJOR	0x32	/* approaching ellipse */
#define ABS_MT_WIDTH_MINOR	0x33	/* (omit if circular) */
#define ABS_MT_ORIENTATION	0x34	/* Ellipse orientation */
#define ABS_MT_POSITION_X	0x35	/* Center X ellipse position */
#define ABS_MT_POSITION_Y	0x36	/* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE	0x37	/* Type of touching device */
#define ABS_MT_BLOB_ID		0x38	/* Group set of pkts as blob */
#endif /* ABS_MT_TOUCH_MAJOR */

#ifndef ABS_MT_TRACKING_ID
#define ABS_MT_TRACKING_ID 0x39 /* Unique ID of initiated contact */
#endif

#endif
