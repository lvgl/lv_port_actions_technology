/*!
 * \file
 * \brief     配置文件
 * \details
 * \author
 * \date
 * \copyright Actions
 */

#include <input_manager_type.h>

#define BOARD_LARK                          (4)

#define CFG_SUPPORT_AAP_SETTINGS     1
#define CFG_OPTIMIZE_BT_MUSIC_STUCK  0

#define BOARD_TYPE  BOARD_LARK

/*-----------------------------------------------------------------------------
 * 配置文件中必须使用以下定义的 cfg_xxx 数据类型
 *---------------------------------------------------------------------------*/
typedef signed char   cfg_int8;
typedef signed short  cfg_int16;
typedef signed int  cfg_int32;

typedef unsigned char   cfg_uint8;
typedef unsigned short  cfg_uint16;
typedef unsigned int  cfg_uint32;

/*---------------------------------------------------------------------------*/


/* 常用数值定义
 */
#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif 

#define ENABLE   TRUE
#define DISABLE  FALSE
#define OK       TRUE
#define FAIL     FALSE
#define NONE     0


/* IC 型号定义
 */
#define IC_TYPE_LARK    (1 << 14)


/* 当前选用的 IC 型号
 */
#if (BOARD_TYPE == BOARD_LARK)
    #define CFG_IC_TYPE  IC_TYPE_LARK
#endif

/* 最大配置数定义
 */
#define CFG_MAX_USER_VERSION_LEN    32
#define CFG_MAX_CASE_NAME_LEN       20
#define CFG_MAX_RESERVED_SIZE       255
#define CFG_MAX_GPIO_PINS           79
#define CFG_MAX_LRADC_KEYS          9
#define CFG_MAX_LRADC_COMBO_KEYS    3
#define CFG_MAX_GPIO_KEYS           4
#define CFG_MAX_KEY_FUNC_MAPS       40
#define CFG_MAX_COMBO_KEY_MAPS      8
#define CFG_MAX_LEDS                4
#define CFG_MAX_LED_DISPLAY_MODELS  15
#define CFG_MAX_VOICES              24
#define CFG_MAX_NUMERIC_VOICES      10
#define CFG_MAX_TONES               10
#define CFG_MAX_VOICE_NAME_LEN      9
#define CFG_MAX_VOICE_FMT_LEN       5
#define CFG_MAX_TONE_NAME_LEN       9
#define CFG_MAX_TONE_FMT_LEN        5
#define CFG_MAX_EVENT_NOTIFY        40
#define CFG_MAX_BATTERY_LEVEL       10
#define CFG_MAX_BT_DEV_NAME_LEN     30
#define CFG_MAX_BT_SUFFIX_LEN       10
#define CFG_MAX_BT_PIN_CODE_LEN     6
#define CFG_MAX_BT_SUPPORT_DEVICES  3
#define CFG_MAX_BT_MUSIC_VOLUME     16
#define CFG_MAX_BT_CALL_VOLUME      15
#define CFG_MAX_LINEIN_VOLUME       16
#define CFG_MAX_VOICE_VOLUME        16
#define CFG_MAX_CAP_TEMP_COMP       20
#define CFG_MAX_PEQ_BANDS           14
#define CFG_MAX_UUID_STR_LEN        38
#define CFG_MAX_ADC_NUM             4

#define CFG_GPIO_MFP_PIN_DEF(_gpio_no, _mfp_sel)  \
    (_gpio_no << 0) |  \
    (_mfp_sel << 8)

/*-----------------------------------------------------------------------------
 * 配置枚举类型定义
 * 类型必须以 CFG_XXX 命名
 *---------------------------------------------------------------------------*/


/* 配置分类定义
 */
enum CFG_CATEGORY
{
    CFG_CATEGORY_SYSTEM,          // <"系统">
    CFG_CATEGORY_UPGRADE,         // <"固件烧录设置">
    CFG_CATEGORY_ASQT,            // <"通话调节">
	CFG_CATEGORY_VOLUME_GAIN,     // <"音量及增益",hide>
	CFG_CATEGORY_ASET,            // <"音效调节",hide>
};


enum CFG_TYPE_BOOL
{
    YES = 1,  // <"是">
    NO  = 0,  // <"否">
};

enum CFG_TYPE_VOLUME_GAIN
{
    VOLUME_GAIN_0  =  0x0,   // <"MUTE">
    VOLUME_GAIN_1  =  0x46,  // <" -45.375  dB">
    VOLUME_GAIN_2  =  0x4d,  // <" -42.750  dB">
    VOLUME_GAIN_3  =  0x57,  // <" -39.000  dB">
    VOLUME_GAIN_4  =  0x5d,  // <" -36.750  dB">
    VOLUME_GAIN_5  =  0x63,  // <" -34.500  dB">
    VOLUME_GAIN_6  =  0x69,  // <" -32.250  dB">
    VOLUME_GAIN_7  =  0x6c,  // <" -31.125  dB">
    VOLUME_GAIN_8  =  0x71,  // <" -29.250  dB">
    VOLUME_GAIN_9  =  0x75,  // <" -27.750  dB">
    VOLUME_GAIN_10 =  0x78,  // <" -26.625  dB">
    VOLUME_GAIN_11 =  0x7b,  // <" -25.500  dB">
    VOLUME_GAIN_12 =  0x7e,  // <" -24.375  dB">
    VOLUME_GAIN_13 =  0x81,  // <" -23.250  dB">
    VOLUME_GAIN_14 =  0x84,  // <" -22.125  dB">
    VOLUME_GAIN_15 =  0x86,  // <" -21.375  dB">
    VOLUME_GAIN_16 =  0x89,  // <" -20.250  dB">
    VOLUME_GAIN_17 =  0x8b,  // <" -19.500  dB">
    VOLUME_GAIN_18 =  0x8c,  // <" -19.125  dB">
    VOLUME_GAIN_19 =  0x8d,  // <" -18.750  dB">
    VOLUME_GAIN_20 =  0x8f,  // <" -18.000  dB">
    VOLUME_GAIN_21 =  0x91,  // <" -17.250  dB">
    VOLUME_GAIN_22 =  0x93,  // <" -16.500  dB">
    VOLUME_GAIN_23 =  0x95,  // <" -15.750  dB">
    VOLUME_GAIN_24 =  0x97,  // <" -15.000  dB">
    VOLUME_GAIN_25 =  0x98,  // <" -14.625  dB">
    VOLUME_GAIN_26 =  0x9a,  // <" -13.875  dB">
    VOLUME_GAIN_27 =  0x9c,  // <" -13.125  dB">
    VOLUME_GAIN_28 =  0x9d,  // <" -12.750  dB">
    VOLUME_GAIN_29 =  0x9f,  // <" -12.000  dB">
    VOLUME_GAIN_30 =  0xa0,  // <" -11.625  dB">
    VOLUME_GAIN_31 =  0xa2,  // <" -10.875  dB">
    VOLUME_GAIN_32 =  0xa4,  // <" -10.125  dB">
    VOLUME_GAIN_33 =  0xa5,  // <" -9.750  dB">
    VOLUME_GAIN_34 =  0xa6,  // <" -9.375  dB">
    VOLUME_GAIN_35 =  0xa8,  // <" -8.625  dB">
    VOLUME_GAIN_36 =  0xa9,  // <" -8.250  dB">
    VOLUME_GAIN_37 =  0xaa,  // <" -7.875  dB">
    VOLUME_GAIN_38 =  0xab,  // <" -7.500  dB">
    VOLUME_GAIN_39 =  0xac,  // <" -7.125  dB">
    VOLUME_GAIN_40 =  0xad,  // <" -6.750  dB">
    VOLUME_GAIN_41 =  0xae,  // <" -6.375  dB">
    VOLUME_GAIN_42 =  0xaf,  // <" -6.000  dB">
    VOLUME_GAIN_43 =  0xb0,  // <" -5.625  dB">
    VOLUME_GAIN_44 =  0xb1,  // <" -5.250  dB">
    VOLUME_GAIN_45 =  0xb2,  // <" -4.875  dB">
    VOLUME_GAIN_46 =  0xb3,  // <" -4.500  dB">
    VOLUME_GAIN_47 =  0xb4,  // <" -4.125  dB">
    VOLUME_GAIN_48 =  0xb5,  // <" -3.750  dB">
    VOLUME_GAIN_49 =  0xb6,  // <" -3.375  dB">
    VOLUME_GAIN_50 =  0xb7,  // <" -3.000  dB">
    VOLUME_GAIN_51 =  0xb8,  // <" -2.625  dB">
    VOLUME_GAIN_52 =  0xb9,  // <" -2.250  dB">
    VOLUME_GAIN_53 =  0xba,  // <" -1.875  dB">
    VOLUME_GAIN_54 =  0xbb,  // <" -1.500  dB">
    VOLUME_GAIN_55 =  0xbc,  // <" -1.125  dB">
    VOLUME_GAIN_56 =  0xbd,  // <" -0.750  dB">
    VOLUME_GAIN_57 =  0xbe,  // <" -0.375  dB">
    VOLUME_GAIN_58 =  0xbf,  // <" 0.00   dB">
    VOLUME_GAIN_59 =  0xc0,  // <" 0.375  dB">
    VOLUME_GAIN_60 =  0xc1,  // <" 0.750  dB">
    VOLUME_GAIN_61 =  0xc2,  // <" 1.125  dB">
    VOLUME_GAIN_62 =  0xc3,  // <" 1.500  dB">
    VOLUME_GAIN_63 =  0xc4,  // <" 1.875  dB">
    VOLUME_GAIN_64 =  0xc5,  // <" 2.250  dB">
    VOLUME_GAIN_65 =  0xc6,  // <" 2.625  dB">
    VOLUME_GAIN_66 =  0xc7,  // <" 3.000  dB">
    VOLUME_GAIN_67 =  0xc8,  // <" 3.375  dB">
    VOLUME_GAIN_68 =  0xc9,  // <" 3.750  dB">
    VOLUME_GAIN_69 =  0xca,  // <" 4.125  dB">
    VOLUME_GAIN_70 =  0xcb,  // <" 4.500  dB">
    VOLUME_GAIN_71 =  0xcc,  // <" 4.875  dB">
    VOLUME_GAIN_72 =  0xcd,  // <" 5.250  dB">
    VOLUME_GAIN_73 =  0xce,  // <" 5.625  dB">
    VOLUME_GAIN_74 =  0xcf,  // <" 6.000  dB">
    VOLUME_GAIN_75 =  0xd0,  // <" 6.375  dB">
    VOLUME_GAIN_76 =  0xd1,  // <" 6.750  dB">
    VOLUME_GAIN_77 =  0xd2,  // <" 7.125  dB">
    VOLUME_GAIN_78 =  0xd3,  // <" 7.500  dB">
    VOLUME_GAIN_79 =  0xd4,  // <" 7.875  dB">
};

enum CFG_TYPE_MIC_GAIN
{
    MIC_GAIN_0_0_DB    = 0x0,  // <"0 dB">
    MIC_GAIN_3_0_DB    = 30,  // <"3.0 dB">
    MIC_GAIN_6_0_DB    = 60,  // <"6.0 dB">
    MIC_GAIN_7_5_DB    = 75,  // <"7.5 dB">
    MIC_GAIN_9_0_DB    = 90,  // <"9.0 dB">
    MIC_GAIN_10_5_DB   = 105,  // <"10.5 dB">
    MIC_GAIN_12_0_DB   = 120,  // <"12.0 dB">
    MIC_GAIN_13_5_DB   = 135,  // <"13.5 dB">
    MIC_GAIN_15_0_DB   = 150,  // <"15.0 dB">
    MIC_GAIN_16_5_DB   = 165,  // <"16.5 dB">
    MIC_GAIN_18_0_DB   = 180,  // <"18.0 dB">
    MIC_GAIN_19_5_DB   = 190,  // <"19.5 dB">
    MIC_GAIN_21_0_DB   = 210,  // <"21.0 dB">
    MIC_GAIN_22_5_DB   = 225,  // <"22.5 dB">
    MIC_GAIN_23_0_DB   = 230,  // <"23.0 dB">
    MIC_GAIN_24_0_DB   = 240,  // <"24.0 dB">
    MIC_GAIN_25_0_DB   = 250,  // <"25.0 dB">
    MIC_GAIN_25_5_DB   = 255,  // <"25.5 dB">
    MIC_GAIN_26_0_DB   = 260,  // <"26.0 dB">
    MIC_GAIN_26_5_DB   = 265,  // <"26.5 dB">
    MIC_GAIN_27_0_DB   = 270,  // <"27.0 dB">
    MIC_GAIN_27_5_DB   = 275,  // <"27.5 dB">
    MIC_GAIN_28_0_DB   = 280,  // <"28.0 dB">
    MIC_GAIN_28_5_DB   = 285,  // <"28.5 dB">
    MIC_GAIN_29_0_DB   = 290,  // <"29.0 dB">
    MIC_GAIN_29_5_DB   = 295,  // <"29.5 dB">
    MIC_GAIN_30_0_DB   = 300,  // <"30.0 dB">
    MIC_GAIN_30_5_DB   = 305,  // <"30.5 dB">
    MIC_GAIN_31_0_DB   = 310,  // <"31.0 dB">
    MIC_GAIN_31_5_DB   = 315,  // <"31.5 dB">
    MIC_GAIN_32_0_DB   = 320,  // <"32.0 dB">
    MIC_GAIN_32_5_DB   = 325,  // <"32.5 dB">
    MIC_GAIN_33_0_DB   = 330,  // <"33.0 dB">
    MIC_GAIN_33_5_DB   = 335,  // <"33.5 dB">
    MIC_GAIN_34_0_DB   = 340,  // <"34.0 dB">
    MIC_GAIN_34_5_DB   = 345,  // <"34.5 dB">
    MIC_GAIN_35_0_DB   = 350,  // <"35.0 dB">
    MIC_GAIN_35_5_DB   = 355,  // <"35.5 dB">
    MIC_GAIN_36_0_DB   = 360,  // <"36.0 dB">
    MIC_GAIN_37_0_DB   = 370,  // <"37.0 dB">
    MIC_GAIN_38_0_DB   = 380,  // <"38.0 dB">
    MIC_GAIN_39_0_DB   = 390,  // <"39.0 dB">
    MIC_GAIN_40_0_DB   = 400,  // <"40.0 dB">
    MIC_GAIN_41_0_DB   = 410,  // <"41.0 dB">
    MIC_GAIN_42_0_DB   = 420,  // <"42.0 dB">
    MIC_GAIN_43_0_DB   = 430,  // <"43.0 dB">
    MIC_GAIN_44_0_DB   = 440,  // <"44.0 dB">
    MIC_GAIN_45_0_DB   = 450,  // <"45.0 dB">
    MIC_GAIN_46_0_DB   = 460,  // <"46.0 dB">
    MIC_GAIN_47_0_DB   = 470,  // <"47.0 dB">
    MIC_GAIN_48_0_DB   = 480,  // <"48.0 dB">
    MIC_GAIN_49_0_DB   = 490,  // <"49.0 dB">
    MIC_GAIN_50_0_DB   = 500,  // <"50.0 dB">
    MIC_GAIN_51_0_DB   = 510,  // <"51.0 dB">
    MIC_GAIN_52_0_DB   = 520,  // <"52.0 dB">
    MIC_GAIN_53_0_DB   = 530,  // <"53.0 dB">
    MIC_GAIN_54_0_DB   = 540,  // <"54.0 dB">
    MIC_GAIN_55_0_DB   = 550,  // <"55.0 dB">
    MIC_GAIN_56_0_DB   = 560,  // <"56.0 dB">
    MIC_GAIN_57_0_DB   = 570,  // <"57.0 dB">
    MIC_GAIN_58_0_DB   = 580,  // <"58.0 dB">
    MIC_GAIN_59_0_DB   = 590,  // <"59.0 dB">
    MIC_GAIN_60_0_DB   = 600,  // <"60.0 dB">
    MIC_GAIN_61_0_DB   = 610,  // <"61.0 dB">
    MIC_GAIN_62_0_DB   = 620,  // <"62.0 dB">
    MIC_GAIN_63_0_DB   = 630,  // <"63.0 dB">
    MIC_GAIN_64_0_DB   = 640,  // <"64.0 dB">
    MIC_GAIN_65_0_DB   = 650,  // <"65.0 dB">
    MIC_GAIN_66_0_DB   = 660,  // <"66.0 dB">
    MIC_GAIN_67_0_DB   = 670,  // <"67.0 dB">
    MIC_GAIN_68_0_DB   = 680,  // <"68.0 dB">
    MIC_GAIN_69_0_DB   = 690,  // <"69.0 dB">
    MIC_GAIN_70_0_DB   = 700,  // <"70.0 dB">
    MIC_GAIN_71_0_DB   = 710,  // <"71.0 dB">
    MIC_GAIN_72_0_DB   = 720,  // <"72.0 dB">
    MIC_GAIN_73_0_DB   = 730,  // <"73.0 dB">
    MIC_GAIN_74_0_DB   = 740,  // <"74.0 dB">
    MIC_GAIN_75_0_DB   = 750,  // <"75.0 dB">
    MIC_GAIN_76_0_DB   = 760,  // <"76.0 dB">
    MIC_GAIN_77_0_DB   = 770,  // <"77.0 dB">
    MIC_GAIN_78_0_DB   = 780,  // <"78.0 dB">
    MIC_GAIN_79_0_DB   = 790,  // <"79.0 dB">
    MIC_GAIN_81_0_DB   = 810,  // <"81.0 dB">
    MIC_GAIN_82_5_DB   = 825,  // <"82.5 dB">
    MIC_GAIN_84_0_DB   = 840,  // <"84.0 dB">
};

typedef struct  // <"MIC 增益">
{
    cfg_uint16  ADC0_Gain;    // <"ADC 增益", CFG_TYPE_MIC_GAIN>
} CFG_Type_MIC_Gain;


typedef struct  // <"模拟增益设置">
{
    cfg_uint8   ANALOG_Gain_Poweron;  // <"开机默认模拟增益",      CFG_TYPE_ANALOG_GAIN_MAP, /* NA时为-3DB */>
    cfg_uint8   ANALOG_Gain_Voice;    // <"语音场景模拟增益",      CFG_TYPE_ANALOG_GAIN_MAP, /* NA时跟随开机模拟增益 */>
    cfg_uint8   ANALOG_Gain_BTSpeech; // <"蓝牙通话场景模拟增益",  CFG_TYPE_ANALOG_GAIN_MAP, /* NA时跟随开机模拟增益 */>
    cfg_uint8   ANALOG_GAIN_BTMusic;  // <"蓝牙播歌场景模拟增益",  CFG_TYPE_ANALOG_GAIN_MAP, /* NA时跟随开机模拟增益 */>

} CFG_Type_ANALOG_GAIN_Settings;

typedef struct  // <"DC5V_COM 通讯设置">
{
    cfg_uint8   Enable_DC5V_UART_Comm_Mode;  // <"启用 DC5V_COM 通讯模式", CFG_TYPE_BOOL>
    cfg_uint8   DC5V_UART_Switch_Voltage;    // <"DC5V_COM 切换电压",      CFG_TYPE_DC5V_UART_SWITCH_VOLT>
    cfg_uint8   Redirect_Console_Print;      // <"重定向控制台打印",       CFG_TYPE_BOOL>
    cfg_uint8   DC5V_UART_Parity_Select;     // <"DC5V_COM 奇偶校验位",    CFG_TYPE_UART_PARITY_BIT>
    cfg_uint32  DC5V_UART_Comm_Baudrate;     // <"DC5V_COM 通讯波特率 (bps)">

} CFG_Type_DC5V_UART_Comm_Settings;

enum CFG_TYPE_DC5V_UART_SWITCH_VOLT
{
    DC5V_UART_SWITCH_VOLT_NA    = 0x0,  // <"NA">
    DC5V_UART_SWITCH_VOLT_2_0_V = 0x4,	// <"2.0 V">
    DC5V_UART_SWITCH_VOLT_2_5_V = 0x5,	// <"2.5 V">
    DC5V_UART_SWITCH_VOLT_3_0_V = 0x6,	// <"3.0 V">
    DC5V_UART_SWITCH_VOLT_4_5_V = 0x7,  // <"4.5 V">
};



