/*-----------------------------------------------------------------------------
 * 配置数据类定义
 * 类型必须以 CFG_XXX 命名
 * 类成员必须赋值
 *---------------------------------------------------------------------------*/


class CFG_User_Version  // <"用户版本", CFG_CATEGORY_SYSTEM>
{
    cfg_uint8  Version[CFG_MAX_USER_VERSION_LEN] = "ACTIONS_LARK";  // <"版本信息", string>
};


class CFG_Platform_Case  // <"平台方案", CFG_CATEGORY_SYSTEM, readonly>
{
    cfg_uint32  IC_Type    = CFG_IC_TYPE;  // <"IC 类型", hex>
    cfg_uint8   Board_Type = BOARD_TYPE;   // <"板型">

    cfg_uint8   Case_Name[CFG_MAX_CASE_NAME_LEN] = "S6_01010101";  // <"方案名称", string>

    cfg_uint8   Major_Version = 1;  // <"主版本号">
    cfg_uint8   Minor_Version = 0;  // <"次版本号">
};


class CFG_Console_UART  // <"控制台串口", CFG_CATEGORY_SYSTEM>
{
    cfg_uint16  TX_Pin   = UART_TX_GPIO_10;  // <"输出管脚", CFG_TYPE_UART_TX_PIN>
    cfg_uint16  RX_Pin   = UART_RX_GPIO_11;  // <"输入管脚", CFG_TYPE_UART_RX_PIN>
    cfg_uint32  Baudrate = 2000000;         // <"波特率 (bps)">

    cfg_uint8   Print_Time_Stamp = YES;  // <"打印时间戳", CFG_TYPE_BOOL>
};


class CFG_System_Settings  // <"系统设置", CFG_CATEGORY_SYSTEM>
{
    cfg_uint16  Support_Features =
    (
    //  SYS_ENABLE_SOFT_WATCHDOG          |
    //  SYS_ENABLE_DC5V_IN_RESET          |
        SYS_ENABLE_DC5VPD_WHEN_DETECT_OUT |
    //  SYS_FRONT_CHARGE_DC5V_OUT_REBOOT  |
        0
    );  // <"系统支持特性", CFG_TYPE_SYS_SUPPORT_FEATURES, multi_select>

    cfg_uint8   Auto_Power_Off_Mode = AUTO_POWOFF_MODE_UNCONNECTED;  // <"自动关机模式", CFG_TYPE_AUTO_POWOFF_MODE>

    cfg_uint16  Auto_Power_Off_Time_Sec = 120;  // <"自动关机时间 (秒)", 0 ~ 900, /* 设置为 0 时禁止自动关机 */>
    cfg_uint16  Auto_Standby_Time_Sec   = 30;   // <"自动待机时间 (秒)", 0 ~ 900, /* 设置为 0 时禁止自动待机 */>

    cfg_uint8   Enable_Voice_Prompt_In_Calling = NO;        // <"通话中允许语音播报提示", CFG_TYPE_BOOL>
    cfg_uint8   Default_Voice_Language = VOICE_LANGUAGE_1;  // <"默认语音语言", CFG_TYPE_VOICE_LANGUAGE>

    cfg_uint8   Linein_Disable_Bluetooth  = NO;  // <"Linein 模式下禁用蓝牙功能", CFG_TYPE_BOOL, hide>
};



class CFG_OTA_Settings  // <"OTA 设置", CFG_CATEGORY_UPGRADE>
{
    cfg_uint8   Enable_Dongle_OTA_Erase_VRAM = NO;     // <"Dongle OTA擦除用户区", CFG_TYPE_BOOL>
    cfg_uint8   Enable_APP_OTA_Erase_VRAM = NO;  	  // <"发射机或APP OTA擦除用户区", CFG_TYPE_BOOL>
    cfg_uint8   Enable_Single_OTA_Without_TWS = NO;    // <"未组队时允许单耳OTA", CFG_TYPE_BOOL>
    cfg_uint8   Enable_Ver_Diff = YES;  // <"左右耳固件版本不同时，允许TWS OTA", CFG_TYPE_BOOL>
    cfg_uint8   Enable_Ver_Low = YES;   // <"关闭版本控制，版本号自动加1", CFG_TYPE_BOOL>
	cfg_uint8	Enable_Poweroff = NO;  // <"OTA完成后关机", CFG_TYPE_BOOL>
    cfg_uint8   Version_Number[12] = "1.0.0";  // <"固件版本号", string, /* 例如 1.6.8, 2.6.3.4 */>
};

class CFG_Factory_Settings  // <"固件烧录设置", CFG_CATEGORY_UPGRADE>
{
    cfg_uint8   Keep_User_VRAM_Data_When_UART_Upgrade = NO;    // <"配置工具串口烧录固件时保留用户区数据", CFG_TYPE_BOOL>
    cfg_uint8   Keep_Factory_VRAM_Data_When_ATT_Upgrade = NO;  // <"ATT 工具烧录固件时保留工厂区数据", CFG_TYPE_BOOL>
};




class CFG_ONOFF_Key  // <"ONOFF 按键", CFG_CATEGORY_KEY>
{
    cfg_uint8   Use_Inner_ONOFF_Key = YES;  // <"使用内部软 ONOFF 按键", CFG_TYPE_BOOL>
    cfg_uint8   Continue_Key_Function_After_Wake_Up = YES;  // <"按键唤醒后允许继续响应按键功能", CFG_TYPE_BOOL, /* (内部软 ONOFF 按键) */>

    cfg_uint8   Key_Value             = VKEY_PLAY;                    // <"ONOFF 键值", CFG_TYPE_KEY_VALUE>
    cfg_uint16  Time_Press_Power_On   = ONOFF_PRESS_POWER_ON_500_MS;  // <"按下开机", CFG_TYPE_ONOFF_PRESS_POWER_ON,   /* (内部软 ONOFF 按键) */>
    cfg_uint8   Time_Long_Press_Reset = ONOFF_LONG_PRESS_RESET_8S;    // <"长按复位", CFG_TYPE_ONOFF_LONG_PRESS_RESET, /* (内部软 ONOFF 按键) */>

    cfg_uint8   Boot_Hold_Key_Func    = BOOT_HOLD_KEY_FUNC_ENTER_PAIR_MODE;  // <"开机长按键功能", CFG_TYPE_BOOT_HOLD_KEY_FUNC>
    cfg_uint16  Boot_Hold_Key_Time_Ms = 2500;                                // <"开机长按键时间 (毫秒)", 500 ~ 8000>
    cfg_uint16  Debounce_Time_Ms      = 40;                                  // <"去抖时间 (毫秒)",       0 ~ 100>

    cfg_uint8   Reboot_After_Boot_Hold_Key_Clear_Paired_List = NO;  // <"开机长按键清除配对列表后自动重启", CFG_TYPE_BOOL>
};


class CFG_LRADC_Keys  // <"LRADC 按键", CFG_CATEGORY_KEY>
{
    CFG_Type_LRADC_Key  Key[CFG_MAX_LRADC_KEYS] =
    {
        { .Key_Value = VKEY_VADD, .ADC_Min = 0,              .ADC_Max = 0xb0   + 0x16c, },
        { .Key_Value = VKEY_VSUB, .ADC_Min = 0x520  - 0x16c, .ADC_Max = 0x520  + 0x16c, },
        { .Key_Value = VKEY_MENU, .ADC_Min = 0x970  - 0x16c, .ADC_Max = 0x970  + 0x16c, },
        { .Key_Value = VKEY_MODE, .ADC_Min = 0xdb0  - 0x16c, .ADC_Max = 0xdb0  + 0x16c, },
    };  // <"按键", CFG_Type_LRADC_Key>

    cfg_uint32  LRADC_Ctrl =
    (
        LRADC_CTRL_1_GPIO_76
    );  // <"LRADC 控制器", CFG_TYPE_LRADC_CTRL>

    cfg_uint8   LRADC_Pull_Up = LRADC_PULL_UP_EXTERNAL;  // <"LRADC 上拉电阻", CFG_TYPE_LRADC_PULL_UP, /* LRADC_CTRL_1 默认使用内部上拉, 其它只能外部上拉 */>

    cfg_uint8   Use_LRADC_Key_Wake_Up = NO;  // <"使用 LRADC 按键唤醒", CFG_TYPE_BOOL, /* LRADC_CTRL_1_GPIO_0 才能支持唤醒 */>
    cfg_uint8   LRADC_Value_Test      = NO;  // <"LRADC 采样值测试", CFG_TYPE_BOOL, /* 通过串口打印 LRADC 采样值 */>
    cfg_uint16  Debounce_Time_Ms      = 40;  // <"去抖时间 (毫秒)",  0 ~ 100>
};


class CFG_GPIO_Keys  // <"GPIO 按键", CFG_CATEGORY_KEY>
{
    CFG_Type_GPIO_Key  Key[CFG_MAX_GPIO_KEYS] =
    {
        {   .Key_Value        = VKEY_NONE,
            .GPIO_Pin         = GPIO_NONE,
            .Pull_Up_Down     = CFG_GPIO_PULL_UP,
            .Active_Level     = GPIO_LEVEL_LOW,
            .Debounce_Time_Ms = 40,
        },
        {   .Key_Value        = VKEY_NONE,
            .GPIO_Pin         = GPIO_NONE,
            .Pull_Up_Down     = CFG_GPIO_PULL_UP,
            .Active_Level     = GPIO_LEVEL_LOW,
            .Debounce_Time_Ms = 40,
        },
        {   .Key_Value        = VKEY_NONE,
            .GPIO_Pin         = GPIO_NONE,
            .Pull_Up_Down     = CFG_GPIO_PULL_UP,
            .Active_Level     = GPIO_LEVEL_LOW,
            .Debounce_Time_Ms = 40,
        },
        {   .Key_Value        = VKEY_NONE,
            .GPIO_Pin         = GPIO_NONE,
            .Pull_Up_Down     = CFG_GPIO_PULL_UP,
            .Active_Level     = GPIO_LEVEL_LOW,
            .Debounce_Time_Ms = 40,
        },
    };  // <"按键", CFG_Type_GPIO_Key>
};


class CFG_Tap_Key  // <"敲击按键", CFG_CATEGORY_KEY>
{
    CFG_Type_Tap_Key_Control  Tap_Key_Control =
    {
        .Tap_Ctrl_Select          = TAP_CTRL_NONE,
        .SDA_Pin                  = GPIO_NONE,
        .SCL_Pin                  = GPIO_NONE,
        .I2C_Pull_Up              = CFG_GPIO_PULL_UP_10K,
        .I2C_Device_Address       = 0x4e,
        .I2C_Bitrate              = 100000,
        .INT1_Pin                 = GPIO_NONE,
        .INT1_Pull_Up_Down        = CFG_GPIO_PULL_UP,
        .INT1_Active_Level        = GPIO_LEVEL_LOW,
        .First_Tap_Sensitivity    = 13,
        .Continue_Tap_Sensitivity = 6,
        .Enable_Single_Tap        = NO,
        .Tap_Key_Tone             = TONE_KEY_SOUND,
        .Support_INT_Wake_Up      = NO,

    };  // <"敲击按键支持", CFG_Type_Tap_Key_Control>
};


class CFG_Key_Threshold  // <"按键响应门限参数", CFG_CATEGORY_KEY>
{
    cfg_uint16  Single_Click_Valid_Ms    = 500;   // <"单击有效时间 (毫秒)",     200 ~ 1000, /* 单击按下到抬起在该时间内有效 */>
    cfg_uint16  Multi_Click_Interval_Ms  = 300;   // <"多击间隔时间 (毫秒)",     100 ~ 500>
    cfg_uint16  Repeat_Start_Delay_Ms    = 500;   // <"重复按键延迟时间 (毫秒)", 0 ~ 1000,   /* 按键按下该时间后开始重复按键 */>
    cfg_uint16  Repeat_Interval_Ms       = 250;   // <"重复按键间隔时间 (毫秒)", 100 ~ 1000>
    cfg_uint16  Long_Press_Time_Ms       = 800;   // <"长按键时间 (毫秒)",       500 ~ 5000>
    cfg_uint16  Long_Long_Press_Time_Ms  = 3000;  // <"超长按键时间 (毫秒)",     1000 ~ 10000>
    cfg_uint16  Very_Long_Press_Time_Ms  = 5000;  // <"极长按键时间 (毫秒)",     1500 ~ 20000>
};


class CFG_Key_Func_Maps  // <"按键功能映射", CFG_CATEGORY_KEY>
{
    CFG_Type_Key_Func_Map  Map[CFG_MAX_KEY_FUNC_MAPS] =
    {
        {   .Key_Func  = KEY_FUNC_POWER_OFF,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_LONG_PRESS,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_ENTER_PAIR_MODE,
            .Key_Value = VKEY_VADD,
            .Key_Event = KEY_EVENT_LONG_PRESS,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_CLEAR_PAIRED_LIST_IN_PAIR_MODE,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_TRIPLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_CLEAR_PAIRED_LIST,
            .Key_Value = VKEY_MODE,
            .Key_Event = KEY_EVENT_LONG_PRESS,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_TWS_PAIR_SEARCH,
            .Key_Value = VKEY_MODE,
            .Key_Event = KEY_EVENT_DOUBLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_START_RECONNECT,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_PLAY_PAUSE,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_SINGLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_PREV_MUSIC,
            .Key_Value = VKEY_VSUB,
            .Key_Event = KEY_EVENT_DOUBLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NEXT_MUSIC,
            .Key_Value = VKEY_VADD,
            .Key_Event = KEY_EVENT_DOUBLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_ADD_MUSIC_VOLUME,
            .Key_Value = VKEY_VADD,
            .Key_Event = KEY_EVENT_SINGLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_SUB_MUSIC_VOLUME,
            .Key_Value = VKEY_VSUB,
            .Key_Event = KEY_EVENT_SINGLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_ADD_CALL_VOLUME,
            .Key_Value = VKEY_VADD,
            .Key_Event = KEY_EVENT_SINGLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_SUB_CALL_VOLUME,
            .Key_Value = VKEY_VSUB,
            .Key_Event = KEY_EVENT_SINGLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_ACCEPT_CALL,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_SINGLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_REJECT_CALL,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_LONG_PRESS,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_HANGUP_CALL,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_SINGLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_KEEP_CALL_RELEASE_3WAY,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_LONG_PRESS,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_HOLD_CALL_ACTIVE_3WAY,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_SINGLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_HANGUP_CALL_ACTIVE_3WAY,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_DOUBLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_SWITCH_CALL_OUT,
            .Key_Value = VKEY_VADD,
            .Key_Event = KEY_EVENT_LONG_PRESS,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_SWITCH_MIC_MUTE,
            .Key_Value = VKEY_VSUB,
            .Key_Event = KEY_EVENT_LONG_PRESS,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_DIAL_LAST_NO,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_DOUBLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_START_VOICE_ASSIST,
            .Key_Value = VKEY_VSUB,
            .Key_Event = KEY_EVENT_TRIPLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_STOP_VOICE_ASSIST,
            .Key_Value = VKEY_PLAY,
            .Key_Event = KEY_EVENT_SINGLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_HID_PHOTO_SHOT,
            .Key_Value = VKEY_VADD,
            .Key_Event = KEY_EVENT_TRIPLE_CLICK,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_HID_CUSTOM_KEY,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func  = KEY_FUNC_NONE,
            .Key_Value = VKEY_NONE,
            .Key_Event = KEY_EVENT_NONE,
            .LR_Device = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
    };  // <"映射", CFG_Type_Key_Func_Map>
};


class CFG_Combo_Key_Func_Maps  // <"组合按键映射", CFG_CATEGORY_KEY>
{
    CFG_Type_Combo_Key_Map  Map[CFG_MAX_COMBO_KEY_MAPS] =
    {
        {   .Key_Func    = KEY_FUNC_NONE,
            .Key_Value_1 = VKEY_NONE,
            .Key_Value_2 = VKEY_NONE,
            .Key_Event   = KEY_EVENT_NONE,
            .LR_Device   = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func    = KEY_FUNC_NONE,
            .Key_Value_1 = VKEY_NONE,
            .Key_Value_2 = VKEY_NONE,
            .Key_Event   = KEY_EVENT_NONE,
            .LR_Device   = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func    = KEY_FUNC_NONE,
            .Key_Value_1 = VKEY_NONE,
            .Key_Value_2 = VKEY_NONE,
            .Key_Event   = KEY_EVENT_NONE,
            .LR_Device   = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func    = KEY_FUNC_NONE,
            .Key_Value_1 = VKEY_NONE,
            .Key_Value_2 = VKEY_NONE,
            .Key_Event   = KEY_EVENT_NONE,
            .LR_Device   = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func    = KEY_FUNC_NONE,
            .Key_Value_1 = VKEY_NONE,
            .Key_Value_2 = VKEY_NONE,
            .Key_Event   = KEY_EVENT_NONE,
            .LR_Device   = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func    = KEY_FUNC_NONE,
            .Key_Value_1 = VKEY_NONE,
            .Key_Value_2 = VKEY_NONE,
            .Key_Event   = KEY_EVENT_NONE,
            .LR_Device   = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func    = KEY_FUNC_NONE,
            .Key_Value_1 = VKEY_NONE,
            .Key_Value_2 = VKEY_NONE,
            .Key_Event   = KEY_EVENT_NONE,
            .LR_Device   = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
        {   .Key_Func    = KEY_FUNC_NONE,
            .Key_Value_1 = VKEY_NONE,
            .Key_Value_2 = VKEY_NONE,
            .Key_Event   = KEY_EVENT_NONE,
            .LR_Device   = KEY_DEVICE_TWS_UNPAIRED | KEY_DEVICE_TWS_PAIRED,
        },
    };  // <"映射", CFG_Type_Combo_Key_Map>
};


class CFG_Customed_Key_Sequence  // <"自定义按键序列设置", CFG_CATEGORY_KEY>
{
    CFG_Type_Customed_Key_Sequence  Customed_Key_Sequence[2] =
    {
        {   .Key_Sequence    = CUSTOMED_KEY_SEQUENCE_NONE,
            .Key_Event_1 = KEY_EVENT_NONE,
            .Key_Event_2 = KEY_EVENT_NONE,
        },
        {   .Key_Sequence    = CUSTOMED_KEY_SEQUENCE_NONE,
            .Key_Event_1 = KEY_EVENT_NONE,
            .Key_Event_2 = KEY_EVENT_NONE,
        },
    };// <"自定义按键序列", CFG_Type_Customed_Key_Sequence>

};


class CFG_LED_Drives  // <"LED 驱动", CFG_CATEGORY_DISPLAY>
{
    CFG_Type_LED_Drive  LED[CFG_MAX_LEDS] =
    {
        { .LED_No = LED_RED,    .GPIO_Pin = LED_GPIO_18,     .Active_Level = GPIO_LEVEL_HIGH, },
        { .LED_No = LED_BLUE,   .GPIO_Pin = LED_GPIO_19,     .Active_Level = GPIO_LEVEL_HIGH, },
        { .LED_No = LED_NULL,   .GPIO_Pin = LED_GPIO_NONE,  .Active_Level = GPIO_LEVEL_HIGH, },
        { .LED_No = LED_NULL,   .GPIO_Pin = LED_GPIO_NONE,  .Active_Level = GPIO_LEVEL_HIGH, },
    };  // <"LED", CFG_Type_LED_Drive>
};


class CFG_LED_Display_Models  // <"LED 显示模式", CFG_CATEGORY_DISPLAY>
{
    CFG_Type_LED_Display_Model  Model[CFG_MAX_LED_DISPLAY_MODELS] =
    {
        {   .Display_Model     = LED_DISPLAY_POWER_ON,
            .Display_LEDs      = LED_BLUE,
            .Disable_LEDs      = NONE,
            .Use_PWM_Control   = NO,
            .Delay_Time_Ms     = 0,
            .ON_Time_Ms        = 200,
            .OFF_Time_Ms       = 200,
            .Flash_Count       = 3,
            .Loop_Count        = 1,
            .Loop_Wait_Time_Ms = 0,
            .Breath_Time_Ms    = 0,
        },
        {   .Display_Model     = LED_DISPLAY_POWER_OFF,
            .Display_LEDs      = LED_RED,
            .Disable_LEDs      = NONE,
            .Use_PWM_Control   = NO,
            .Delay_Time_Ms     = 0,
            .ON_Time_Ms        = 200,
            .OFF_Time_Ms       = 200,
            .Flash_Count       = 3,
            .Loop_Count        = 1,
            .Loop_Wait_Time_Ms = 0,
            .Breath_Time_Ms    = 0,
        },
        {   .Display_Model     = LED_DISPLAY_STANDBY,
            .Display_LEDs      = LED_RED,
            .Disable_LEDs      = LED_BLUE,
            .Use_PWM_Control   = YES,
            .Delay_Time_Ms     = 0,
            .ON_Time_Ms        = 100,
            .OFF_Time_Ms       = 5000,
            .Flash_Count       = 0,
            .Loop_Count        = 0,
            .Loop_Wait_Time_Ms = 0,
            .Breath_Time_Ms    = 0,
        },
        {   .Display_Model     = LED_DISPLAY_CHARGE_START,
            .Display_LEDs      = LED_RED,
            .Disable_LEDs      = NONE,
            .Use_PWM_Control   = NO,
            .Delay_Time_Ms     = 0,
            .ON_Time_Ms        = 1000,
            .OFF_Time_Ms       = 0,
            .Flash_Count       = 0,
            .Loop_Count        = 0,
            .Loop_Wait_Time_Ms = 0,
            .Breath_Time_Ms    = 0,
        },
        {   .Display_Model     = LED_DISPLAY_CHARGE_FULL,
            .Display_LEDs      = LED_BLUE,
            .Disable_LEDs      = NONE,
            .Use_PWM_Control   = NO,
            .Delay_Time_Ms     = 0,
            .ON_Time_Ms        = 1000,
            .OFF_Time_Ms       = 0,
            .Flash_Count       = 0,
            .Loop_Count        = 0,
            .Loop_Wait_Time_Ms = 0,
            .Breath_Time_Ms    = 0,
        },
        {   .Display_Model     = LED_DISPLAY_BT_PAIR_MODE,
            .Display_LEDs      = LED_RED | LED_BLUE,
            .Disable_LEDs      = NONE,
            .Use_PWM_Control   = NO,
            .Delay_Time_Ms     = 250,
            .ON_Time_Ms        = 250,
            .OFF_Time_Ms       = 250,
            .Flash_Count       = 0,
            .Loop_Count        = 0,
            .Loop_Wait_Time_Ms = 0,
            .Breath_Time_Ms    = 0,
        },
        {   .Display_Model     = LED_DISPLAY_BT_WAIT_CONNECT,
            .Display_LEDs      = LED_RED | LED_BLUE,
            .Disable_LEDs      = NONE,
            .Use_PWM_Control   = NO,
            .Delay_Time_Ms     = 500,
            .ON_Time_Ms        = 500,
            .OFF_Time_Ms       = 500,
            .Flash_Count       = 0,
            .Loop_Count        = 0,
            .Loop_Wait_Time_Ms = 0,
            .Breath_Time_Ms    = 0,
        },
        {   .Display_Model     = LED_DISPLAY_BT_CONNECTED,
            .Display_LEDs      = LED_BLUE,
            .Disable_LEDs      = NONE,
            .Use_PWM_Control   = NO,
            .Delay_Time_Ms     = 0,
            .ON_Time_Ms        = 150,
            .OFF_Time_Ms       = 0,
            .Flash_Count       = 1,
            .Loop_Count        = 0,
            .Loop_Wait_Time_Ms = 5000,
            .Breath_Time_Ms    = 0,
        },
        {   .Display_Model     = LED_DISPLAY_BT_UNLINKED,
            .Display_LEDs      = LED_RED | LED_BLUE,
            .Disable_LEDs      = NONE,
            .Use_PWM_Control   = NO,
            .Delay_Time_Ms     = 1000,
            .ON_Time_Ms        = 1000,
            .OFF_Time_Ms       = 1000,
            .Flash_Count       = 0,
            .Loop_Count        = 0,
            .Loop_Wait_Time_Ms = 0,
            .Breath_Time_Ms    = 0,
        },
        {   .Display_Model     = LED_DISPLAY_BT_CALL_INCOMING,
            .Display_LEDs      = LED_BLUE,
            .Disable_LEDs      = NONE,
            .Use_PWM_Control   = NO,
            .Delay_Time_Ms     = 0,
            .ON_Time_Ms        = 150,
            .OFF_Time_Ms       = 150,
            .Flash_Count       = 3,
            .Loop_Count        = 0,
            .Loop_Wait_Time_Ms = 500,
            .Breath_Time_Ms    = 0,
        },
    };  // <"模式", CFG_Type_LED_Display_Model>
};


class CFG_BT_Music_Volume_Table  // <"蓝牙音乐音量分级表", CFG_CATEGORY_VOLUME_GAIN>
{
    cfg_uint16  Level[CFG_MAX_BT_MUSIC_VOLUME + 1] =
    {
        VOLUME_GAIN_0,   // LEVEL_0
        VOLUME_GAIN_4,   // LEVEL_1
        VOLUME_GAIN_6,   // LEVEL_2
        VOLUME_GAIN_9,   // LEVEL_3
        VOLUME_GAIN_12,  // LEVEL_4
        VOLUME_GAIN_15,  // LEVEL_5
        VOLUME_GAIN_18,  // LEVEL_6
        VOLUME_GAIN_21,  // LEVEL_7
        VOLUME_GAIN_24,  // LEVEL_8
        VOLUME_GAIN_27,  // LEVEL_9
        VOLUME_GAIN_30,  // LEVEL_10
        VOLUME_GAIN_33,  // LEVEL_11
        VOLUME_GAIN_36,  // LEVEL_12
        VOLUME_GAIN_39,  // LEVEL_13
        VOLUME_GAIN_42,  // LEVEL_14
        VOLUME_GAIN_45,  // LEVEL_15
        VOLUME_GAIN_48,  // LEVEL_16

    };  // <"分级", CFG_TYPE_VOLUME_GAIN, click_popup>
};


class CFG_BT_Call_Volume_Table  // <"蓝牙通话音量分级表", CFG_CATEGORY_VOLUME_GAIN>
{
    cfg_uint16  Level[CFG_MAX_BT_CALL_VOLUME + 1] =
    {
        VOLUME_GAIN_4,   // LEVEL_0
        VOLUME_GAIN_6,   // LEVEL_1
        VOLUME_GAIN_9,   // LEVEL_2
        VOLUME_GAIN_12,  // LEVEL_3
        VOLUME_GAIN_15,  // LEVEL_4
        VOLUME_GAIN_18,  // LEVEL_5
        VOLUME_GAIN_21,  // LEVEL_6
        VOLUME_GAIN_24,  // LEVEL_7
        VOLUME_GAIN_27,  // LEVEL_8
        VOLUME_GAIN_30,  // LEVEL_9
        VOLUME_GAIN_33,  // LEVEL_10
        VOLUME_GAIN_36,  // LEVEL_11
        VOLUME_GAIN_39,  // LEVEL_12
        VOLUME_GAIN_42,  // LEVEL_13
        VOLUME_GAIN_45,  // LEVEL_14
        VOLUME_GAIN_48,  // LEVEL_15

    };  // <"分级", CFG_TYPE_VOLUME_GAIN, click_popup>
};


class CFG_Linein_Volume_Table  // <"Linein 音量分级表", CFG_CATEGORY_VOLUME_GAIN, hide>
{
    cfg_uint16  Level[CFG_MAX_LINEIN_VOLUME + 1] =
    {
        VOLUME_GAIN_4,   // LEVEL_0
        VOLUME_GAIN_6,   // LEVEL_1
        VOLUME_GAIN_9,   // LEVEL_2
        VOLUME_GAIN_12,  // LEVEL_3
        VOLUME_GAIN_15,  // LEVEL_4
        VOLUME_GAIN_18,  // LEVEL_5
        VOLUME_GAIN_21,  // LEVEL_6
        VOLUME_GAIN_24,  // LEVEL_7
        VOLUME_GAIN_27,  // LEVEL_8
        VOLUME_GAIN_30,  // LEVEL_9
        VOLUME_GAIN_33,  // LEVEL_10
        VOLUME_GAIN_36,  // LEVEL_11
        VOLUME_GAIN_39,  // LEVEL_12
        VOLUME_GAIN_42,  // LEVEL_13
        VOLUME_GAIN_45,  // LEVEL_14
        VOLUME_GAIN_48,  // LEVEL_15
        VOLUME_GAIN_52,  // LEVEL_15
    };  // <"分级", CFG_TYPE_VOLUME_GAIN, click_popup>
};


class CFG_Voice_Volume_Table  // <"语音音量分级表", CFG_CATEGORY_VOLUME_GAIN>
{
    cfg_uint16  Level[CFG_MAX_VOICE_VOLUME + 1] =
    {
        VOLUME_GAIN_4,   // LEVEL_0
        VOLUME_GAIN_6,   // LEVEL_1
        VOLUME_GAIN_9,   // LEVEL_2
        VOLUME_GAIN_12,  // LEVEL_3
        VOLUME_GAIN_15,  // LEVEL_4
        VOLUME_GAIN_18,  // LEVEL_5
        VOLUME_GAIN_21,  // LEVEL_6
        VOLUME_GAIN_24,  // LEVEL_7
        VOLUME_GAIN_27,  // LEVEL_8
        VOLUME_GAIN_30,  // LEVEL_9
        VOLUME_GAIN_33,  // LEVEL_10
        VOLUME_GAIN_36,  // LEVEL_11
        VOLUME_GAIN_39,  // LEVEL_12
        VOLUME_GAIN_42,  // LEVEL_13
        VOLUME_GAIN_45,  // LEVEL_14
        VOLUME_GAIN_48,  // LEVEL_15
        VOLUME_GAIN_52,  // LEVEL_16
    };  // <"分级", CFG_TYPE_VOLUME_GAIN, click_popup>
};


class CFG_Volume_Settings  // <"音量设置", CFG_CATEGORY_VOLUME_GAIN>
{
    cfg_uint8  Voice_Default_Volume    = 8;   // <"语音默认音量",     0 ~ 16, slide_bar, hide>
    cfg_uint8  Voice_Min_Volume        = 6;   // <"语音最小音量",     0 ~ 16, slide_bar>
    cfg_uint8  Voice_Max_Volume        = 12;  // <"语音最大音量",     0 ~ 16, slide_bar>

    cfg_uint8  BT_Music_Default_Volume = 8;   // <"蓝牙音乐默认音量", 0 ~ 16, slide_bar>
    cfg_uint8  BT_Call_Default_Volume  = 8;   // <"蓝牙通话默认音量", 0 ~ 15, slide_bar>
    cfg_uint8  BT_Music_Default_Vol_Ex = 12;  // <"蓝牙音乐默认音量 (用于不支持音量同步的设备)", 0 ~ 16, slide_bar>

    cfg_uint8  Linein_Default_Volume   = 8;   // <"Linein 默认音量",  0 ~ 16, slide_bar, hide>
    cfg_uint8  Linein_Gain  = MIC_GAIN_6_0_DB;  // <"Linein 模拟增益", CFG_TYPE_MIC_GAIN, dev_mode, hide>
};


class CFG_Audio_Settings  // <"音频设置", CFG_CATEGORY_AUDIO>
{
    cfg_uint8  Audio_Out_Mode =
    (
        AUDIO_OUT_MODE_DAC_DIFF
    );  // <"音频输出模式", CFG_TYPE_AUDIO_OUT_MODE>

    CFG_Type_I2S_Select_GPIO  I2STX_Select_GPIO =
    {
        .I2S_MCLK  = I2STX_MCLK_GPIO_NONE,
        .I2S_LRCLK = I2STX_MCLK_GPIO_NONE,
        .I2S_BCLK  = I2STX_MCLK_GPIO_NONE,
        .I2S_DOUT  = I2STX_MCLK_GPIO_NONE,
    };  // <"I2S TX 管脚配置", CFG_Type_I2S_Select_GPIO, click_popup>

    CFG_Type_I2S_Select_GPIO  I2SRX_Select_GPIO =
    {
        .I2S_MCLK  = GPIO_NONE,
        .I2S_LRCLK = GPIO_NONE,
        .I2S_BCLK  = GPIO_NONE,
        .I2S_DOUT  = GPIO_NONE,

    };  // <"I2S RX 管脚配置", CFG_Type_I2S_Select_GPIO, click_popup, hide>

    cfg_uint8  Channel_Select_Mode = CHANNEL_SELECT_L_BY_TWS_PAIR;  // <"声道选择模式", CFG_TYPE_CHANNEL_SELECT_MODE>

    CFG_Type_Channel_Select_GPIO  Channel_Select_GPIO =
    {
        .GPIO_Pin     = GPIO_NONE,
        .Pull_Up_Down = CFG_GPIO_PULL_DOWN,
        .Active_Level = GPIO_LEVEL_LOW,

    };  // <"GPIO 选择声道", CFG_Type_Channel_Select_GPIO, click_popup>

    CFG_Type_Channel_Select_LRADC  Channel_Select_LRADC =
    {
        .LRADC_Ctrl    = LRADC_CTRL_NONE,
        .LRADC_Pull_Up = LRADC_PULL_UP_EXTERNAL,
        .ADC_Min       = 0x00,
        .ADC_Max       = 0x00,

    };  // <"LRADC 选择声道", CFG_Type_Channel_Select_LRADC, click_popup>

    cfg_uint8  TWS_Alone_Audio_Channel = TWS_ALONE_AUDIO_MIX_LR;  // <"TWS 未组对时声道选择", CFG_TYPE_TWS_ALONE_AUDIO_CHANNEL>

    cfg_uint8  L_Speaker_Out = SPEAKER_OUT_ENABLE;  // <"左声道喇叭输出", CFG_TYPE_SPEAKER_OUT_SELECT>
    cfg_uint8  R_Speaker_Out = SPEAKER_OUT_ENABLE;  // <"右声道喇叭输出", CFG_TYPE_SPEAKER_OUT_SELECT>

    cfg_uint32  ADC_Bias_Setting = 0x1a36528a;            // <"ADC BIAS 设置", hex, dev_mode>

    cfg_uint32  DAC_Bias_Setting =
    (
    #if (CFG_IC_TYPE & IC_TYPE_LARK)
        0x9b014964
    #else
        0x9b004964
    #endif
    );  // <"DAC BIAS 设置", hex, dev_mode>

    cfg_uint8  Keep_DA_Enabled_When_Play_Pause = YES;  // <"保持DAC打开状态", CFG_TYPE_BOOL, dev_mode>
    cfg_uint8  Disable_PA_When_Reconnect       = YES;  // <"回连或组对时关闭 PA", CFG_TYPE_BOOL>

    CFG_Type_Extern_PA_Control  Extern_PA_Control[2] =
    {
        {   .PA_Function  = EXTERN_PA_ENABLE,
            .GPIO_Pin     = GPIO_40,
            .Pull_Up_Down = CFG_GPIO_PULL_NONE,
            .Active_Level = GPIO_LEVEL_HIGH,
        },
        {   .PA_Function  = EXTERN_PA_NONE,
            .GPIO_Pin     = GPIO_NONE,
            .Pull_Up_Down = CFG_GPIO_PULL_NONE,
            .Active_Level = GPIO_LEVEL_LOW,
        },
    };  // <"外部 PA 控制", CFG_Type_Extern_PA_Control, click_popup>

    cfg_uint8  AntiPOP_Process_Disable      = NO;  // <"禁止ANTIPOP处理", CFG_TYPE_BOOL>

    cfg_uint8  Pa_Gain =
    (
    #if (CFG_IC_TYPE & IC_TYPE_LARK)
        7
    #else
        0
    #endif
    );  // <"PA增益选择", /* 选择范围[0, 7], 不同模式下增益不同*/>

    cfg_uint8  DMIC01_Channel_Aligning = 0;                  // <"DMIC01 采样沿选择", /* 值为 0 表示 channel_1 是上升沿, channel_2 是下降沿, 值为 1 则相反 */>
    cfg_uint8  DMIC23_Channel_Aligning = 0;                  // <"DMIC23 采样沿选择", /* 值为 0 表示 channel_1 是上升沿, channel_2 是下降沿, 值为 1 则相反 */>

    CFG_Type_DMIC_Select_GPIO  DMIC_Select_GPIO =
    {
        .DMIC01_CLK = DMIC01_CLK_GPIO_NONE,
        .DMIC01_DAT = DMIC01_DAT_GPIO_NONE,
        .DMIC23_CLK = DMIC23_CLK_GPIO_NONE,
        .DMIC23_DAT = DMIC23_DAT_GPIO_NONE,
    };  // <"DMIC 管脚配置", CFG_Type_DMIC_Select_GPIO, click_popup>

    cfg_uint8  Enable_ANC = ANC_FF;     // <"ANC功能使能, 使能后需要配置ANC DMIC GPIO", CFG_TYPE_ANC, dev_mode, /* ANC-FF fix ADC0, ANC-FB fix ADC1*/>
    CFG_Type_DMIC_Select_GPIO  ANCDMIC_Select_GPIO =
    {
        .DMIC01_CLK = DMIC01_CLK_GPIO_NONE,
        .DMIC01_DAT = DMIC01_DAT_GPIO_NONE,
        .DMIC23_CLK = DMIC23_CLK_GPIO_NONE,
        .DMIC23_DAT = DMIC23_DAT_GPIO_NONE,
    };  // <"ANCDMIC 管脚配置", CFG_Type_DMIC_Select_GPIO, click_popup, dev_mode>

    cfg_uint8  Record_Adc_Select = ADC_2;       // <"mic录音通路选择",            CFG_TYPE_ADC, multi_select>
    cfg_uint8  Enable_VMIC = VMIC_0 | VMIC_1 | VMIC_2; // <"是否启用 VMIC", CFG_TYPE_VMIC, multi_select, /* 启用 VMIC 则由 PIN 脚供电, 否则由 AVCC 供电 */>
    cfg_uint8  Hw_Aec_Select = ADC_NONE;       // <"硬件aec，选择'无'则使用软件aec",            CFG_TYPE_ADC, hide>
    cfg_uint8  Tm_Adc_Select = ADC_0;       // <"通透录音通路选择",            CFG_TYPE_ADC>

    CFG_Type_Mic_Config  Mic_Config[CFG_MAX_ADC_NUM] =
    {
        {
            .Adc_Index = ADC_0,
            .Mic_Type   = ADC_TYPE_AMIC,
            .Audio_In_Mode  = AUDIO_IN_MODE_ADC_SINGLE,
        },
        {
            .Adc_Index = ADC_1,
            .Mic_Type   = ADC_TYPE_AMIC,
            .Audio_In_Mode  = AUDIO_IN_MODE_ADC_SINGLE,
        },
        {
            .Adc_Index = ADC_2,
            .Mic_Type   = ADC_TYPE_AMIC,
            .Audio_In_Mode  = AUDIO_IN_MODE_ADC_SINGLE,
        },
        {
            .Adc_Index = ADC_3,
            .Mic_Type   = ADC_TYPE_AMIC,
            .Audio_In_Mode  = AUDIO_IN_MODE_ADC_SINGLE,
        },
    };  // <"麦克风配置", CFG_Type_Mic_Config>

    CFG_Type_ADC_Select_INPUT  ADC_Select_INPUT =
    {
        .ADC_Input_Ch0  =
        (
        #if (CFG_IC_TYPE & IC_TYPE_LARK)
            2
        #else
            1
        #endif
        ),
        .ADC_Input_Ch1 =
        (
        #if (CFG_IC_TYPE & IC_TYPE_LARK)
            8
        #else
            2
        #endif
        ),
        .ADC_Input_Ch2  =
        (
        #if (CFG_IC_TYPE & IC_TYPE_LARK)
            1
        #else
            1
        #endif
        ),
        .ADC_Input_Ch3  =
        (
        #if (CFG_IC_TYPE & IC_TYPE_LARK)
            4
        #else
            0
        #endif
        ),
    };  // <"ADC INPUT管脚配置", CFG_Type_ADC_Select_INPUT, click_popup>


    cfg_uint8  Dual_MIC_Exchange_Enable = NO;  // <"双MIC声道交换使能", CFG_TYPE_BOOL, /* 默认R为拾音MIC,L为降噪MIC */>

    cfg_uint8  Large_Current_Protect_Enable = NO;  // <"Speaker大电流保护使能", CFG_TYPE_BOOL>

    CFG_Type_ANALOG_GAIN_Settings  ANALOG_GAIN_Settings = { 0, };  // <"模拟增益设置", CFG_Type_ANALOG_GAIN_Settings, click_popup>

};


class CFG_Tone_List  // <"提示音列表", CFG_CATEGORY_TONE, readonly>
{
    CFG_Type_Tone_ID_Name  Tone[CFG_MAX_TONES] =
    {
        { TONE_ID1,       "T_ID1",  },
        { TONE_ID2,       "T_ID2",  },
        { TONE_ID3,       "T_ID3",  },
        { TONE_ID4,       "T_ID4",  },
        { TONE_ID5,       "T_ID5",  },
        { TONE_ID6,       "T_ID6",  },
        { TONE_ID7,       "T_ID7",  },

        { TONE_KEY_SOUND, "T_KEY",  },
        { TONE_CALL_RING, "T_RING", },
        { TONE_WARNING,   "T_WARN", },

    };  // <"提示音", CFG_Type_Tone_ID_Name>

    cfg_uint8  Tone_Format_Name[CFG_MAX_TONE_FMT_LEN] = ".act";  // <"文件格式", string>
};


class CFG_Key_Tone  // <"按键音", CFG_CATEGORY_TONE>
{
    cfg_uint8  Key_Tone_Select           = TONE_KEY_SOUND;  // <"选择按键音",   CFG_TYPE_TONE_ID>
    cfg_uint8  Long_Key_Tone_Select      = TONE_NONE;       // <"长按提示音",   CFG_TYPE_TONE_ID>
    cfg_uint8  Long_Long_Key_Tone_Select = TONE_NONE;       // <"超长按提示音", CFG_TYPE_TONE_ID>
    cfg_uint8  Very_Long_Key_Tone_Select = TONE_NONE;       // <"极长按提示音", CFG_TYPE_TONE_ID>
};


class CFG_Voice_List  // <"语音列表", CFG_CATEGORY_VOICE, readonly>
{
    CFG_Type_Voice_ID_Name  Voice[CFG_MAX_VOICES] =
    {
        { VOICE_ID1,             "V_ID1",    },
        { VOICE_ID2,             "V_ID2",    },
        { VOICE_ID3,             "V_ID3",    },
        { VOICE_ID4,             "V_ID4",    },
        { VOICE_ID5,             "V_ID5",    },

        { VOICE_POWER_ON,        "V_POWON",  },
        { VOICE_POWER_OFF,       "V_POWOFF", },
        { VOICE_BAT_LOW,         "V_BATLOW", },
        { VOICE_BAT_TOO_LOW,     "V_BATTLO", },
        { VOICE_BT_PAIR_MODE,    "V_BTPRMD", },
        { VOICE_BT_WAIT_CONNECT, "V_BTWPR",  },
        { VOICE_BT_CONNECTED,    "V_BTCNT",  },
        { VOICE_2ND_CONNECTED,   "V_BTCNT2", },
        { VOICE_BT_DISCONNECTED, "V_BTDSC",  },
        { VOICE_TWS_WAIT_PAIR,   "V_TWSWPR", },
        { VOICE_TWS_CONNECTED,   "V_TWSCNT", },
        { VOICE_TWS_DISCONNECTED,"V_TWSDSC", },
        { VOICE_PLAY,            "V_PLAY",   },
        { VOICE_PAUSE,           "V_PAUSE",  },
        { VOICE_PREV_MUSIC,      "V_PRVMUS", },
        { VOICE_NEXT_MUSIC,      "V_NXTMUS", },
        { VOICE_MIN_VOLUME,      "V_MINVOL", },
        { VOICE_MAX_VOLUME,      "V_MAXVOL", },
    //  { VOICE_LINEIN,          "V_LINEIN", },

    };  // <"语音", CFG_Type_Voice_ID_Name>

    cfg_uint8  Voice_Format_Name[CFG_MAX_VOICE_FMT_LEN] = ".act";  // <"文件格式", string>
};


class CFG_Numeric_Voice_List  // <"数字语音列表", CFG_CATEGORY_VOICE, readonly>
{
    CFG_Type_Numeric_Voice_ID_Name  Voice[CFG_MAX_NUMERIC_VOICES] =
    {
        { VOICE_NO_0, "V_NO_0", },
        { VOICE_NO_1, "V_NO_1", },
        { VOICE_NO_2, "V_NO_2", },
        { VOICE_NO_3, "V_NO_3", },
        { VOICE_NO_4, "V_NO_4", },
        { VOICE_NO_5, "V_NO_5", },
        { VOICE_NO_6, "V_NO_6", },
        { VOICE_NO_7, "V_NO_7", },
        { VOICE_NO_8, "V_NO_8", },
        { VOICE_NO_9, "V_NO_9", },

    };  // <"语音", CFG_Type_Numeric_Voice_ID_Name>
};


class CFG_Event_Notify  // <"事件通知", CFG_CATEGORY_EVENT_NOTIFY>
{
    CFG_Type_Event_Notify  Notify[CFG_MAX_EVENT_NOTIFY] =
    {
        {   .Event_Type   = UI_EVENT_POWER_ON,
            .LED_Display  = LED_DISPLAY_POWER_ON,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_POWER_ON,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_POWER_OFF,
            .LED_Display  = LED_DISPLAY_POWER_OFF,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_POWER_OFF,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_STANDBY,
            .LED_Display  = LED_DISPLAY_STANDBY,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BATTERY_TOO_LOW,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = TONE_WARNING,
            .Voice_Play   = VOICE_BAT_TOO_LOW,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BATTERY_LOW,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = TONE_WARNING,
            .Voice_Play   = VOICE_BAT_LOW,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_CHARGE_START,
            .LED_Display  = LED_DISPLAY_CHARGE_START,
            .LED_Override = LED_OVERRIDE_BACK,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_CHARGE_FULL,
            .LED_Display  = LED_DISPLAY_CHARGE_FULL,
            .LED_Override = LED_OVERRIDE_BACK,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_ENTER_PAIR_MODE,
            .LED_Display  = LED_DISPLAY_BT_PAIR_MODE,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_BT_PAIR_MODE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_CLEAR_PAIRED_LIST,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = TONE_WARNING,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BT_WAIT_CONNECT,
            .LED_Display  = LED_DISPLAY_BT_WAIT_CONNECT,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_BT_WAIT_CONNECT,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BT_CONNECTED,
            .LED_Display  = LED_DISPLAY_BT_CONNECTED,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_BT_CONNECTED,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_2ND_CONNECTED,
            .LED_Display  = LED_DISPLAY_BT_CONNECTED,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_2ND_CONNECTED,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BT_DISCONNECTED,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_BT_DISCONNECTED,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BT_UNLINKED,
            .LED_Display  = LED_DISPLAY_BT_UNLINKED,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_TWS_WAIT_PAIR,
            .LED_Display  = LED_DISPLAY_BT_PAIR_MODE,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_TWS_WAIT_PAIR,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_TWS_CONNECTED,
            .LED_Display  = LED_DISPLAY_BT_CONNECTED,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_TWS_CONNECTED,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_TWS_DISCONNECTED,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_TWS_DISCONNECTED,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BT_MUSIC_PLAY,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BT_MUSIC_PAUSE,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_PREV_MUSIC,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_NEXT_MUSIC,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_MIN_VOLUME,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = TONE_WARNING,
            .Voice_Play   = VOICE_MIN_VOLUME,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_MAX_VOLUME,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = TONE_WARNING,
            .Voice_Play   = VOICE_MAX_VOLUME,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BT_CALL_INCOMING,
            .LED_Display  = LED_DISPLAY_BT_CALL_INCOMING,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = TONE_CALL_RING,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BT_CALL_OUTGOING,
            .LED_Display  = LED_DISPLAY_BT_CALL_INCOMING,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BT_CALL_ONGOING,
            .LED_Display  = LED_DISPLAY_BT_CALL_ONGOING,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_BT_CALL_3WAYIN,
            .LED_Display  = LED_DISPLAY_BT_CALL_INCOMING,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = TONE_CALL_RING,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_VOICE_ASSIST_START,
            .LED_Display  = LED_DISPLAY_BT_CALL_ONGOING,
            .LED_Override = LED_OVERRIDE_FRONT,
            .Tone_Play    = NONE,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_MIC_MUTE_ON,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = TONE_WARNING,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_MIC_MUTE_OFF,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = TONE_WARNING,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_SWITCH_CALL_OUT,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = TONE_WARNING,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_HID_PHOTO_SHOT,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = TONE_WARNING,
            .Voice_Play   = NONE,
            .Options      = NONE,
        },
    #if 0
        {   .Event_Type   = UI_EVENT_ENTER_LINEIN,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_LINEIN,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_LINEIN_PLAY,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_PLAY,
            .Options      = NONE,
        },
        {   .Event_Type   = UI_EVENT_LINEIN_PAUSE,
            .LED_Display  = NONE,
            .LED_Override = NONE,
            .Tone_Play    = NONE,
            .Voice_Play   = VOICE_PAUSE,
            .Options      = NONE,
        },
    #endif
    };  // <"通知", CFG_Type_Event_Notify>
};


class CFG_Battery_Charge  // <"电池充电", CFG_CATEGORY_BATTERY_CHARGE>
{
    cfg_uint8   Select_Charge_Mode = BAT_BACK_CHARGE_MODE;  // <"选择充电模式", CFG_TYPE_BAT_CHARGE_MODE, /* 后台充电模式将保持蓝牙正常工作状态 */>

    cfg_uint8   Charge_Current =
    (
    #if (BOARD_TYPE == BOARD_LARK)
        CHARGE_CURRENT_60_MA

    #else
        CHARGE_CURRENT_300_MA
    #endif
    );  // <"正常充电电流", CFG_TYPE_CHARGE_CURRENT>

    cfg_uint8   Charge_Voltage         = CHARGE_VOLTAGE_4_20_V;               // <"正常充电电压",     CFG_TYPE_CHARGE_VOLTAGE>
    cfg_uint8   Charge_Stop_Mode       = CHARGE_STOP_BY_VOLTAGE_AND_CURRENT;  // <"电池充满阈值选择", CFG_TYPE_CHARGE_STOP_MODE>
    cfg_uint16  Charge_Stop_Voltage    = 4160;                                // <"电池充满阈值电压", 4.05 ~ 4.33, float_x1000, /* (小于等于充电电压 - 0.02V) */>
    cfg_uint8   Charge_Stop_Current    = CHARGE_STOP_CURRENT_20_PERCENT;      // <"电池充满阈值电流", CFG_TYPE_CHARGE_STOP_CURRENT>
    cfg_uint16  Precharge_Stop_Voltage = PRECHARGE_STOP_3_3_V;                // <"低电预充阈值电压", CFG_TYPE_PRECHARGE_STOP_VOLTAGE, hide>

    cfg_uint16  Battery_Check_Period_Sec = 60;   // <"电量检测周期 (秒)",     10 ~ 300>
    cfg_uint16  Charge_Check_Period_Sec  = 300;  // <"正常充电检测周期 (秒)", 60 ~ 600>
    cfg_uint16  Charge_Full_Continue_Sec = 420;  // <"充满延续时间 (秒)",     10 ~ 1800, /* 充电至阈值后继续充电该时间以完全充满 */>

    cfg_uint16  Front_Charge_Full_Power_Off_Wait_Sec = 10;  // <"前台模式充电满后关机等待时间 (秒)", 5 ~ 300>

    cfg_uint16  DC5V_Detect_Debounce_Time_Ms = 300;  // <"DC5V 检测去抖时间 (毫秒)", 0 ~ 1000>
};


class CFG_Charger_Box  // <"充电盒设置", CFG_CATEGORY_BATTERY_CHARGE>
{
    cfg_uint8   Enable_Charger_Box         = NO;                      // <"启用充电盒充电模式",       CFG_TYPE_BOOL>
    cfg_uint8   DC5V_Pull_Down_Current     = DC5VPD_CURRENT_DISABLE;  // <"DC5V 下拉唤醒充电盒",      CFG_TYPE_DC5VPD_CURRENT>
    cfg_uint16  DC5V_Pull_Down_Hold_Ms     = 0;                       // <"DC5V 下拉保持时间 (毫秒)", 0 ~ 2000>
    cfg_uint16  Charger_Standby_Delay_Ms   = 500;                     // <"充电盒待机延迟 (毫秒)",    0 ~ 2000>
    cfg_uint16  Charger_Standby_Voltage    = 3.00f * 1000;            // <"充电盒待机电压",           0.5 ~ 4.0, float_x1000, /* 充电盒待机电压为自身电池电压时可配置为 3.8V */>
    cfg_uint16  Charger_Wake_Delay_Ms      = 500;                     // <"充电盒唤醒延迟 (毫秒)",    0 ~ 2000>
    cfg_uint8   Enable_Battery_Recharge    = NO;                      // <"启用电池复充功能",         CFG_TYPE_BOOL>
    cfg_uint8   Battery_Recharge_Threshold = BAT_RECHARGE_3_8_V;      // <"电池复充阈值电压",         CFG_TYPE_BAT_RECHARGE_THRESHOLD>
    cfg_uint8   Charger_Box_Standby_Current = 0;  					  // <"充电盒待机电流(mA)", 0 ~ 10>

    CFG_Type_DC5V_UART_Comm_Settings  DC5V_UART_Comm_Settings = { 0, }; // <"DC5V_COM 通讯设置", CFG_Type_DC5V_UART_Comm_Settings, click_popup>

    CFG_Type_DC5V_IO_Comm_Settings  DC5V_IO_Comm_Settings = { 0, };  	// <"DC5V_IO 通讯设置", CFG_Type_DC5V_IO_Comm_Settings, click_popup>
};


class CFG_Battery_Level  // <"电量分级", CFG_CATEGORY_BATTERY_CHARGE>
{
    cfg_uint16  Level[CFG_MAX_BATTERY_LEVEL] =
    {
        3.10f * 1000,  // LEVEL_0
        3.40f * 1000,  // LEVEL_1
        3.60f * 1000,  // LEVEL_2
        3.65f * 1000,  // LEVEL_3
        3.70f * 1000,  // LEVEL_4
        3.75f * 1000,  // LEVEL_5
        3.80f * 1000,  // LEVEL_6
        3.90f * 1000,  // LEVEL_7
        4.00f * 1000,  // LEVEL_8
        4.10f * 1000,  // LEVEL_9

    };  // <"分级", 2.80 ~ 4.30, float_x1000>
};


class CFG_Battery_Low  // <"低电电量", CFG_CATEGORY_BATTERY_CHARGE>
{
    cfg_uint16  Battery_Too_Low_Voltage = BATTERY_LOW_3_1_V;  // <"电量不足", 3.00 ~ 3.80, float_x1000, /* 电量不足时会自动关机 */>
    cfg_uint16  Battery_Low_Voltage     = BATTERY_LOW_3_4_V;  // <"电量低",   3.00 ~ 3.80, float_x1000>
    cfg_uint16  Battery_Low_Voltage_Ex  = 0;                  // <"更低电量", 0.00 ~ 3.80, float_x1000>

    cfg_uint16  Battery_Low_Prompt_Interval_Sec = 120;  // <"电量低提示间隔时间 (秒)", 0 ~ 600, /* 设置为 0 时只提示一次 */>
};

class CFG_NTC_Settings  // <"温度调节充电电流", CFG_CATEGORY_BATTERY_CHARGE>
{
    CFG_Type_NTC_Settings  NTC_Settings = { 0, };  // <"NTC 温度调节充电电流", CFG_Type_NTC_Settings>

    CFG_Type_NTC_Range  NTC_Ranges[5] = { { 0, }, };  // <"NTC 温度范围", CFG_Type_NTC_Range,click_popup>
};


class CFG_BT_Device  // <"蓝牙设备", CFG_CATEGORY_BLUETOOTH>
{
    cfg_uint8   BT_Device_Name[CFG_MAX_BT_DEV_NAME_LEN] =
    (
        "ZS302A-Hello"
    );  // <"蓝牙设备名称", string>

    cfg_uint8   Left_Device_Suffix [CFG_MAX_BT_SUFFIX_LEN] = " (L)";  // <"L 左设备名称后缀", string>
    cfg_uint8   Right_Device_Suffix[CFG_MAX_BT_SUFFIX_LEN] = " (R)";  // <"R 右设备名称后缀", string>

    cfg_uint8   BT_Address[6] = { 0x01, 0x00, 0x00, 0xFC, 0x4E, 0xF4 };  // <"蓝牙地址", bt_addr>

    cfg_uint8   Use_Random_BT_Address = YES;       // <"使用随机蓝牙地址", CFG_TYPE_BOOL, /* 通过 MIC 采样噪声生成低 3 字节蓝牙地址 */>
    cfg_uint32  BT_Device_Class       = 0x240404;  // <"蓝牙设备类型", hex, dev_mode>

    cfg_uint8   PIN_Code[CFG_MAX_BT_PIN_CODE_LEN] = "0000";  // <"PIN Code", string, dev_mode, /* 在禁止 SSP 功能时使用 */>

    cfg_uint8   Default_HOSC_Capacity =
    (
    #if (BOARD_TYPE == BOARD_LARK)
        12.0f * 10
    #else
        13.5f * 10
    #endif
    );  // <"缺省频偏电容值 (pF)", 0.0 ~ 24.0, float_x10>

    cfg_uint8   Force_Default_HOSC_Capacity = YES;  // <"总是使用配置的频偏电容值", CFG_TYPE_BOOL>

    cfg_uint8   BT_Max_RF_TX_Power =
    (
    #if (BOARD_TYPE == BOARD_LARK)
        15
    #else
        18
    #endif
    );  // <"蓝牙最大发射功率", 0 ~ 22>

    cfg_uint8   BLE_RF_TX_Power = 8;  // <"BLE 发射功率", 0 ~ 22>

    cfg_uint8   A2DP_Bitpool = 49;    // <"A2DP Bitpool", 2 ~ 53>

    cfg_uint16  Vendor_ID  = 0x03E0;  // <"厂商 ID", hex>
    cfg_uint16  Product_ID = 0x302A;  // <"产品 ID", hex>
    cfg_uint16  Version_ID = 0x0100;  // <"版本 ID", hex>
};


class CFG_BT_Manager  // <"蓝牙管理", CFG_CATEGORY_BLUETOOTH>
{
    cfg_uint32  Support_Features =
    (
        BT_SUPPORT_A2DP                  |
        BT_SUPPORT_A2DP_AAC              |
    //  BT_SUPPORT_A2DP_DTCP             |
    //  BT_SUPPORT_A2DP_SCMS_T           |
        BT_SUPPORT_AVRCP                 |
        BT_SUPPORT_AVRCP_VOLUME_SYNC     |
        BT_SUPPORT_HFP                   |
        BT_SUPPORT_HFP_VOLUME_SYNC       |
        BT_SUPPORT_HFP_BATTERY_REPORT    |
        BT_SUPPORT_HFP_3WAY_CALL         |
        BT_SUPPORT_HFP_PHONEBOOK_NUMBER  |
        BT_SUPPORT_HFP_VOICE_ASSIST      |
        BT_SUPPORT_HFP_CODEC_NEGOTIATION |
        BT_SUPPORT_ENABLE_NREC           |
        BT_SUPPORT_HID                   |
        BT_SUPPORT_TWS                   |
        BT_SUPPORT_ENABLE_SNIFF          |
        BT_SUPPORT_LINKKEY_MISS_REJECT   |
        BT_SUPPORT_DUAL_PHONE_DEV_LINK
    );  // <"蓝牙支持特性", CFG_TYPE_BT_SUPPORT_FEATURES, multi_select, /* 支持 AAC 音频格式时无法启用智能语音识别功能 */>

    cfg_uint8   Support_Device_Number     = 3;  // <"可同时连接设备个数", 1 ~ 3>
    cfg_uint8   Paired_Device_Save_Number = 8;  // <"已配对设备保存个数", 2 ~ 8>

    cfg_uint8   Controller_Test_Mode = BT_CTRL_DISABLE_TEST;  // <"控制器测试模式", CFG_TYPE_BT_CTRL_TEST_MODE>
    cfg_uint8   Enter_BQB_Test_Mode_By_Key = NO;              // <"通过按键进入 BQB 测试模式", CFG_TYPE_BOOL>

    CFG_Type_Auto_Quit_BT_Ctrl_Test  Auto_Quit_BT_Ctrl_Test = { 0, };  // <"自动退出控制器测试模式", CFG_Type_Auto_Quit_BT_Ctrl_Test, click_popup>


    cfg_uint16  Idle_Enter_Sniff_Time_Ms = 5000;  // <"空闲进入 Sniff 模式时间 (毫秒)", 2000 ~ 20000, dev_mode>
    cfg_uint16  Sniff_Interval_Ms        = 500;   // <"Sniff 周期 (毫秒)", 100 ~ 500, dev_mode>
};


class CFG_BT_Pair  // <"蓝牙配对连接", CFG_CATEGORY_BLUETOOTH>
{
    cfg_uint8   Default_State_Discoverable     = YES;  // <"默认状态可被搜索发现",      CFG_TYPE_BOOL>
    cfg_uint16  Default_State_Wait_Connect_Sec = 120;  // <"默认状态等待配对连接 (秒)", 0 ~ 600, /* 设置为 0 时不限时间 */>
    cfg_uint16  Pair_Mode_Duration_Sec         = 120;  // <"配对模式持续时间 (秒)",     0 ~ 600, /* 设置为 0 时不限时间 */>

    cfg_uint8   Disconnect_All_Phones_When_Enter_Pair_Mode = NO;  // <"进入配对模式时断开所有已连接手机设备", CFG_TYPE_BOOL>

    cfg_uint8   Clear_Paired_List_When_Enter_Pair_Mode = NO;   // <"进入配对模式时清除配对列表", CFG_TYPE_BOOL>
    cfg_uint8   Clear_TWS_When_Key_Clear_Paired_List   = YES;  // <"按键清除配对列表同时清除 TWS 组对设备信息", CFG_TYPE_BOOL>

    cfg_uint8   Enter_Pair_Mode_When_Key_Clear_Paired_List = NO;   // <"按键清除配对列表同时进入配对模式", CFG_TYPE_BOOL>
    cfg_uint8   Enter_Pair_Mode_When_Paired_List_Empty     = YES;  // <"配对列表为空时开机进入配对模式",   CFG_TYPE_BOOL>

    cfg_uint8   BT_Not_Discoverable_When_Connected = NO;  // <"蓝牙已连接后关闭可见性", CFG_TYPE_BOOL>
};


class CFG_TWS_Pair  // <"TWS 组对连接", CFG_CATEGORY_BLUETOOTH>
{
    cfg_uint8   TWS_Pair_Key_Mode             = TWS_PAIR_KEY_MODE_ONE;  // <"按键组对模式", CFG_TYPE_TWS_PAIR_KEY_MODE>
    cfg_uint8   Match_Mode                    = TWS_MATCH_NAME;         // <"匹配模式",     CFG_TYPE_TWS_MATCH_MODE>
    cfg_uint8   Match_Name_Length             = 30;                     // <"名称匹配长度", 1 ~ 30>
    cfg_uint16  TWS_Wait_Pair_Search_Time_Sec = 60;                     // <"等待组对搜索时间 (秒)", 5 ~ 600>
    cfg_uint8   TWS_Power_On_Auto_Pair_Search = NO;                     // <"未组对过时开机自动进行组对搜索", CFG_TYPE_BOOL>
};




class CFG_TWS_Advanced_Pair  // <"TWS 高级组对设置", CFG_CATEGORY_BLUETOOTH>
{
    cfg_uint8  Enable_TWS_Advanced_Pair_Mode      = YES;       // <"启用 TWS 高级组对模式",  CFG_TYPE_BOOL>
    cfg_uint8  Check_RSSI_When_TWS_Pair_Search    = NO;     // <"组对搜索时判断信号强度", CFG_TYPE_BOOL>
    cfg_int8   RSSI_Threshold                     = 0;         // <"信号强度阈值",           -120 ~ 0>
    cfg_uint8  Use_Search_Mode_When_TWS_Reconnect = YES;       // <"TWS 回连时使用搜索模式", CFG_TYPE_BOOL>
} ;




class CFG_TWS_Sync  // <"TWS 同步设置", CFG_CATEGORY_BLUETOOTH>
{
    cfg_uint8  Sync_Mode =
    (
        TWS_SYNC_KEY_TONE
    //  TWS_SYNC_POWER_OFF
    );  // <"同步模式", CFG_TYPE_TWS_SYNC_MODE, multi_select>
};


class CFG_BT_Auto_Reconnect  // <"蓝牙自动回连", CFG_CATEGORY_BLUETOOTH>
{
    cfg_uint8   Enable_Auto_Reconnect =
    (
        AUTO_RECONNECT_PHONE_BY_STARTUP |
        AUTO_RECONNECT_PHONE_BY_TIMEOUT

    );  // <"启用自动回连", CFG_TYPE_BT_AUTO_RECONNECT, multi_select>

    cfg_uint16  Reconnect_Phone_Timeout          = 5.0f * 10;  // <"单次连接手机设备超时 (秒)",      0.5 ~ 10.0, float_x10>
    cfg_uint16  Reconnect_Phone_Interval         = 5.0f * 10;  // <"重试回连手机设备间隔时间 (秒)",  0.1 ~ 60.0, float_x10>
    cfg_uint8   Reconnect_Phone_Times_By_Startup = 10;         // <"开机回连手机设备尝试次数",       0 ~ 100, /* 设置为 0 时不限次数 */>
    cfg_uint16  Reconnect_TWS_Timeout            = 5.0f * 10;  // <"单次连接 TWS 设备超时 (秒)",     0.5 ~ 10.0, float_x10>
    cfg_uint16  Reconnect_TWS_Interval           = 5.0f * 10;  // <"重试回连 TWS 设备间隔时间 (秒)", 0.1 ~ 60.0, float_x10>
    cfg_uint8   Reconnect_TWS_Times_By_Startup   = 5;          // <"开机回连 TWS 设备尝试次数",      0 ~ 100, /* 设置为 0 时不限次数 */>
    cfg_uint8   Reconnect_Times_By_Timeout       = 30;         // <"超时断开回连尝试次数",           0 ~ 100, /* 设置为 0 时不限次数 */>

    cfg_uint8   Enter_Pair_Mode_When_Startup_Reconnect_Fail = NO;  // <"开机回连失败时进入配对模式", CFG_TYPE_BOOL>
};


class CFG_BT_HID_Settings  // <"HID 设置", CFG_CATEGORY_BLUETOOTH, dev_mode>
{
    cfg_uint16  HID_Auto_Disconnect_Delay_Sec  = 30;    // <"HID 操作后自动断开延迟时间 (秒)", 0 ~ 600, /* 设置为 0 时不自动断开 */>
    cfg_uint16  HID_Connect_Operation_Delay_Ms = 1000;  // <"HID 连接时操作延迟时间 (毫秒)",   100 ~ 2000>

    cfg_uint8   HID_Custom_Key_Type  = BT_HID_KEY_TYPE_KEYBOARD;  // <"HID 自定义按键类型", CFG_TYPE_BT_HID_KEY_TYPE>
    cfg_uint8   HID_Custom_Key_Value = 0x00;                      // <"HID 自定义按键值",   hex>
};



class CFG_Low_Latency_Settings  // <"低延迟模式设置", CFG_CATEGORY_AUDIO>
{
    cfg_uint8   Default_Low_Latency_Mode = NO;  // <"默认低延迟模式", CFG_TYPE_BOOL>
    cfg_uint8   Save_Low_Latency_Mode = NO;     // <"保存低延迟模式", CFG_TYPE_BOOL, /* 不保存时重新开机恢复默认模式 */>

    cfg_uint16  AAC_Threshold = 0;   // <"AAC 格式播放延迟时间 ms",  0 ~ 150, /* 0 表示默认值 60, 非0时不能小于50 */>
    cfg_uint16  SBC_Threshold = 0;   // <"SBC 格式播放延迟时间 ms",  0 ~ 150, /* 0 表示默认值 60, 非0时不能小于40 */>
    cfg_uint16  MSBC_Threshold = 0;  // <"MSBC 格式播放延迟时间 ms", 0 ~ 150,  /* 0 表示默认值 60, 非0时不能小于30 */>
    cfg_uint16  CVSD_Threshold = 0;  // <"CVSD 格式播放延迟时间 ms", 0 ~ 150,  /* 0 表示默认值 60, 非0时不能小于30 */>
};


class CFG_Transparency_Mode_Settings  // <"通透模式设置", CFG_CATEGORY_AUDIO>
{
    cfg_uint8   Tranparency_En = NO;                 // <"是否使能通透模式", CFG_TYPE_BOOL>
    cfg_uint8   Mic_Channel = MIC_CHANNEL_LEFT;      // <"MIC通道", CFG_TYPE_MIC_CHANNEL_SELECT>
    cfg_int16   Mic_Gain = 0;                        // <"MIC增益(dB)", -60.0 ~ 0.0, float_x10>
    cfg_uint8   Voice_Sample_Mode = 0;               // <"提示音采样率模式", CFG_VOICE_SAMPLE_MODE>
    cfg_uint8   Mic_Sample_Mode = 0;                 // <"MIC采样率模式", CFG_MIC_SAMPLE_MODE>
    cfg_uint8   Mic_Al_Mode = 0;                     // <"MIC算法模式">
};


class CFG_BTMusic_Multi_Dae_Settings  // <"自定义音效使能", CFG_CATEGORY_AUDIO>
{
    cfg_uint8  Enable = NO;        // <"是否打开自定义音效", CFG_TYPE_BOOL, /* 打开后可以在音效调节中自定义蓝牙音乐音效 */>
    cfg_uint8  Cur_Dae_Num = 0;   // <"自定义音效数量", readonly, dev_mode>
    cfg_uint8  Dae_Index = 0;     // <"当前音效序号", readonly, dev_mode>
};


class CFG_BT_Music_Volume_Sync  // <"音量同步", CFG_CATEGORY_BT_MUSIC>
{
    cfg_uint8   Volume_Sync_Only_When_Playing = NO;    // <"只在播放状态下同步音量", CFG_TYPE_BOOL>
    cfg_uint8   Origin_Volume_Sync_To_Remote  = NO;    // <"初始音量同步至远端设备", CFG_TYPE_BOOL, /* (连接时同步) */>
    cfg_uint16  Origin_Volume_Sync_Delay_Ms   = 2500;  // <"初始音量同步延迟时间 (毫秒)", 2000 ~ 5000>
    cfg_uint16  Playing_Volume_Sync_Delay_Ms  = 1000;  // <"播放音量同步延迟时间 (毫秒)", 1000 ~ 3000>
};


class CFG_BT_Music_Stop_Hold  // <"按键停顿时间", CFG_CATEGORY_BT_MUSIC>
{
    cfg_uint16  Key_Pause_Stop_Hold_Ms = 0;  // <"按键暂停时停顿时间 (毫秒)", 0 ~ 5000, /* 暂停后维持停止状态一段时间 (可过滤音乐淡出数据) */>
    cfg_uint16  Key_Prev_Next_Hold_Ms  = 0;  // <"按键上下曲停顿时间 (毫秒)", 0 ~ 5000>
};


class CFG_BT_Two_Device_Play  // <"双手机播放控制", CFG_CATEGORY_BT_MUSIC>
{
    cfg_uint8   Stop_Another_When_One_Playing   = NO;    // <"开始播放时停止另一手机", CFG_TYPE_BOOL>
    cfg_uint8   Resume_Another_When_One_Stopped = NO;    // <"停止播放时恢复另一手机", CFG_TYPE_BOOL>
    cfg_uint16  A2DP_Status_Stopped_Delay_Ms    = 1000;  // <"停止播放状态延迟时间 (毫秒)", 500 ~ 5000>
};


class CFG_BT_Call_Volume_Sync  // <"音量同步", CFG_CATEGORY_BT_CALL>
{
    cfg_uint8   Origin_Volume_Sync_To_Remote = NO;    // <"初始音量同步至远端设备", CFG_TYPE_BOOL, /* (开始通话时同步) */>
    cfg_uint16  Origin_Volume_Sync_Delay_Ms  = 1500;  // <"初始音量同步延迟时间 (毫秒)", 1000 ~ 3000>
};


class CFG_Incoming_Call_Prompt  // <"来电提示", CFG_CATEGORY_BT_CALL>
{
    cfg_uint16  Prompt_Interval_Ms = 1500;  // <"来电提示间隔时间 (毫秒)", 200 ~ 5000>
    cfg_uint8   Play_Phone_Number  = YES;   // <"播报来电号码", CFG_TYPE_BOOL>

    cfg_uint8   BT_Call_Ring_Mode = BT_CALL_RING_MODE_DEFAULT;  // <"来电铃声模式", CFG_TYPE_BT_CALL_RING_MODE>
};


class CFG_Cap_Temp_Comp  // <"温度补偿", CFG_CATEGORY_BLUETOOTH, dev_mode>
{
    cfg_uint8  Enable_Cap_Temp_Comp = NO;  // <"启用温度补偿", CFG_TYPE_BOOL>

    CFG_Type_Cap_Temp_Comp  Table[CFG_MAX_CAP_TEMP_COMP] =
    {
        { CAP_TEMP_N_20, 0.0f * 10 },
        { CAP_TEMP_0,    0.0f * 10 },
        { CAP_TEMP_P_20, 0.0f * 10 },
        { CAP_TEMP_P_25, 0.0f * 10 },
        { CAP_TEMP_P_40, 0.0f * 10 },
        { CAP_TEMP_P_60, 0.0f * 10 },
        { CAP_TEMP_P_75, 0.0f * 10 },

        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },
        { CAP_TEMP_NA,   0.0f * 10 },

    };  // <"温度补偿", CFG_Type_Cap_Temp_Comp>
};


class CFG_Linein_Detect  // <"Linein 检测", CFG_CATEGORY_LINEIN, hide>
{
    cfg_uint8  Detect_Mode = LINEIN_DETECT_NONE;  // <"检测方式", CFG_TYPE_LINEIN_DETECT_MODE>

    CFG_Type_Linein_Detect_GPIO  Detect_GPIO =
    {
        .GPIO_Pin     = GPIO_NONE,
        .Pull_Up_Down = CFG_GPIO_PULL_UP,
        .Active_Level = GPIO_LEVEL_LOW,

    };  // <"GPIO 检测 Linein", CFG_Type_Linein_Detect_GPIO, click_popup>

    CFG_Type_Linein_Detect_LRADC  Detect_LRADC =
    {
        .LRADC_Ctrl    = LRADC_CTRL_NONE,
        .LRADC_Pull_Up = LRADC_PULL_UP_EXTERNAL,
        .ADC_Min       = 0x00,
        .ADC_Max       = 0x00,

    };  // <"LRADC 检测 Linein", CFG_Type_Linein_Detect_LRADC, click_popup>

    cfg_uint16  Debounce_Time_Ms = 300;  // <"去抖时间 (毫秒)",  0 ~ 1000>
};


class CFG_BT_Music_DAE  // <"蓝牙音乐音效", CFG_CATEGORY_ASET, adjust_online>
{
    cfg_uint8  Enable_DAE=1;      // <"音效使能", CFG_TYPE_BOOL>
    cfg_uint8  Test_Volume=8;     // <"测试音量", 0 ~ 16, slide_bar>
};


class CFG_BT_Call_Out_DAE  // <"蓝牙通话输出音效", CFG_CATEGORY_ASET, adjust_online, asqt>
{
    cfg_uint8  Enable_DAE=1;      // <"音效使能", CFG_TYPE_BOOL>
    cfg_uint8  Test_Volume=8;     // <"测试音量", 0 ~ 16, slide_bar>
};


class CFG_BT_Call_MIC_DAE  // <"蓝牙通话 MIC 音效", CFG_CATEGORY_ASET, adjust_online, asqt>
{
    cfg_uint8  Enable_DAE=1;      // <"音效使能", CFG_TYPE_BOOL>
    cfg_uint8  Test_Volume=8;     // <"测试音量", 0 ~ 16, slide_bar>
};


class CFG_Linein_Out_DAE  // <"Linein 输出音效", CFG_CATEGORY_ASET, adjust_online, hide>
{
    cfg_uint8  Enable_DAE=1;      // <"音效使能", CFG_TYPE_BOOL>
    cfg_uint8  Test_Volume=8;     // <"测试音量", 0 ~ 16, slide_bar>
};


class CFG_BT_Call_Quality  // <"通话效果", CFG_CATEGORY_ASQT, adjust_online, asqt>
{
    CFG_Type_MIC_Gain  MIC_Gain =
    {
        .ADC0_Gain = MIC_GAIN_31_5_DB,
        .ADC1_Gain = MIC_GAIN_31_5_DB,
        .ADC2_Gain = MIC_GAIN_31_5_DB,
        .ADC3_Gain = MIC_GAIN_31_5_DB,

    };  // <"MIC 增益", CFG_Type_MIC_Gain, click_popup>

    cfg_uint8  Test_Volume = 8;  // <"测试音量", 0 ~ 15, slide_bar>
};


class CFG_Voice_Player_Param  // <"播放器参数", CFG_CATEGORY_VOICE, dev_mode>
{
    cfg_uint32  VP_Develop_Value1      = 0;    // <"开发者模式参数 1">
    cfg_uint8   VP_WaitData_Time       = 20;   // <"数据不增加时持续等待时间">
    cfg_uint8   VP_WaitData_Empty_Time = 0;    // <"数据为空时持续等待时间 (0 表示一直等待)">
    cfg_uint8   VP_Max_Decode_Count    = 5;    // <"最大解码次数">
    cfg_uint16  VP_Max_PCMBUF_Sampels  = 768;  // <"PCMBUF 持续解码最大门限值">
    cfg_uint16  VP_Het_PCMBUF_Sampels  = 256;  // <"PCMBUF 半空中断门限值">
    cfg_uint16  VP_Hft_PCMBUF_Sampels  = 268;  // <"PCMBUF 半满中断门限值">
    cfg_uint8   VP_Work_Frequency      = 66;   // <"工作频率(包括系统正常工作的频率)">
    cfg_uint8   VP_Module_Frequency    = 30;   // <"模块频率(此模块所需频率增量)">

};


class CFG_Voice_User_Settings  // <"播放器设置", CFG_CATEGORY_VOICE, dev_mode>
{
    cfg_uint16  VP_StartPlay_Threshold = 1;  // <"普通模式开始播放门限值 Bytes">
};


class CFG_Tone_Player_Param  // <"播放器参数", CFG_CATEGORY_TONE, dev_mode>
{
    cfg_uint32  WT_Develop_Value1      = 0;    // <"开发者模式参数 1">
    cfg_uint8   WT_WaitData_Time       = 20;   // <"数据不增加时持续等待时间">
    cfg_uint8   WT_WaitData_Empty_Time = 0;    // <"数据为空时持续等待时间 (0 表示一直等待)">
    cfg_uint8   WT_Max_Decode_Count    = 3;    // <"最大解码次数">
    cfg_uint16  WT_Max_PCMBUF_Sampels  = 500;  // <"PCMBUF 持续解码最大门限值">
    cfg_uint16  WT_Het_PCMBUF_Sampels  = 256;  // <"PCMBUF 半空中断门限值">
    cfg_uint16  WT_Hft_PCMBUF_Sampels  = 268;  // <"PCMBUF 半满中断门限值">
    cfg_uint8   WT_Work_Frequency      = 54;   // <"工作频率(包括系统正常工作的频率)">
    cfg_uint8   WT_Module_Frequency    = 20;   // <"模块频率(此模块所需频率增量)">
};


class CFG_Tone_User_Settings  // <"播放器设置", CFG_CATEGORY_TONE, dev_mode>
{
    cfg_uint16  WT_StartPlay_Threshold = 1;  // <"普通模式开始播放门限值 Bytes">
};


class CFG_Linein_Player_Param  // <"播放器参数", CFG_CATEGORY_LINEIN, dev_mode, hide>
{
    cfg_uint32  LI_Develop_Value1      = 0;    // <"开发者模式参数 1">
    cfg_uint8   LI_WaitData_Time       = 20;   // <"数据不增加时持续等待时间">
    cfg_uint8   LI_WaitData_Empty_Time = 0;    // <"数据为空时持续等待时间 (0 表示一直等待)">
    cfg_uint8   LI_Max_Decode_Count    = 3;    // <"最大解码次数">
    cfg_uint16  LI_Max_PCMBUF_Sampels  = 768;  // <"PCMBUF 持续解码最大门限值">
    cfg_uint16  LI_Het_PCMBUF_Sampels  = 512;  // <"PCMBUF 半空中断门限值">
    cfg_uint16  LI_Hft_PCMBUF_Sampels  = 528;  // <"PCMBUF 半满中断门限值">
    cfg_uint16  LI_StartPlay_Normal    = 100;  // <"普通模式开始播放门限值 Bytes">
    cfg_uint8   LI_Work_Frequency      = 50;   // <"工作频率(包括系统正常工作的频率)">
    cfg_uint8   LI_Module_Frequency    = 20;   // <"模块频率(此模块所需频率增量)">
};


class CFG_Linein_User_Settings  // <"播放器设置", CFG_CATEGORY_LINEIN, hide>
{
    cfg_uint8   LI_INOUT_Mode            = AUX_ADDA; // <"模式选择", CFG_TYPE_AUXIN_OUT_MODE, /* AA 通路无法调节音量 */>
    cfg_uint16  LI_Fadein_Continue_Time  = 500;      // <"淡入持续时间 ms">
    cfg_uint16  LI_Fadeout_Continue_Time = 500;      // <"淡出持续时间 ms">
    cfg_uint16  LI_Playing_CacheData     = 4096;     // <"普通模式缓冲区播放数据量 Bytes">
};


class CFG_BTMusic_Player_Param  // <"播放器参数", CFG_CATEGORY_BT_MUSIC, dev_mode>
{
    cfg_uint32  BM_Develop_Value1 =
    (
    #if (BOARD_TYPE == BOARD_LARK)
        6
    #else
        2
    #endif
    );  // <"开发者模式参数 1">

    cfg_uint8   BM_WaitData_Time          = 200;   // <"数据不增加时持续等待时间">
    cfg_uint8   BM_WaitData_Empty_Time    = 0;     // <"数据为空时持续等待时间 (0 表示一直等待)">
    cfg_uint8   BM_Freq_TWS_Increment     = 0;     // <"TWS 场景频率增量 Mhz">
    cfg_uint8   BM_SBC_Max_Decode_Count   = 2;     // <"SBC 最大解码次数">
    cfg_uint8   BM_AAC_Max_Decode_Count   = 2;     // <"AAC 最大解码次数">
    cfg_uint16  BM_SBC_Max_Sleep_Time     = 1000;  // <"SBC 最大睡眠时间 us">
    cfg_uint16  BM_AAC_Max_Sleep_Time     = 1000;  // <"AAC 最大睡眠时间 us">
    cfg_uint16  BM_TWS_WPlay_Mintime      = 120;   // <"TWS 同时播放等待最小时间间隔 ms">
    cfg_uint16  BM_TWS_WPlay_Maxtime      = 1000;  // <"TWS 同时播放等待最大时间间隔 ms">
    cfg_uint16  BM_TWS_WStop_Mintime      = 60;    // <"TWS 同时停止等待最小时间间隔 (中断时间) ms">
    cfg_uint16  BM_TWS_WStop_Maxtime      = 80;    // <"TWS 同时停止等待最大时间间隔 (超时时间) ms">
    cfg_uint16  BM_TWS_Sync_interval      = 40;    // <"TWS 播放过程中同步时间间隔 pkt">
    cfg_uint16  BM_SBC_Max_PCMBUF_Sampels = 1535;  // <"PCMBUF 持续解码最大门限值 (SBC)">
    cfg_uint16  BM_AAC_Max_PCMBUF_Sampels = 1535;  // <"PCMBUF 持续解码最大门限值 (AAC)">
    cfg_uint16  BM_Het_PCMBUF_Sampels     = 512;   // <"PCMBUF 半空中断门限值">
    cfg_uint16  BM_Hft_PCMBUF_Sampels     = 528;   // <"PCMBUF 半满中断门限值">
    cfg_uint16  BM_StartPlay_Normal       = 200;  // <"普通模式开始播放延迟时间 ms">
    cfg_uint16  BM_StartPlay_TWS          = 1;     // <"TWS 模式开始播放延迟时间 ms">
    cfg_uint8   BM_Work_Frequency_AAC     = 90;   // <"AAC工作频率(包括系统正常工作的频率)">
    cfg_uint8   BM_Module_Frequency_AAC   = 50;   // <"AAC模块频率(此模块所需频率增量)">
    cfg_uint8   BM_Work_Frequency_SBC     = 84;   // <"SBC工作频率(包括系统正常工作的频率)">
    cfg_uint8   BM_Module_Frequency_SBC   = 44;   // <"SBC模块频率(此模块所需频率增量)">
};


class CFG_BTMusic_User_Settings  // <"播放器设置", CFG_CATEGORY_BT_MUSIC>
{
    cfg_uint8   BM_DataWidth = 4;               // <"音频输出位宽选择", /* 值为 2 表示 16bit 输出, 4 表示24bit 输出 */>
    cfg_uint8   BM_ISpeech_PEQ_Enable    = 0;    // <"语音识别场景下播歌 PEQ (CPU 不足时不能开启)", dev_mode>
    cfg_uint16  BM_Fadein_Continue_Time  = 350;  // <"淡入持续时间 ms">
    cfg_uint16  BM_Fadeout_Continue_Time = 100;  // <"淡出持续时间 ms">
    cfg_uint16  BM_SBC_Playing_CacheData = 200;  // <"SBC 格式播放延迟 ms",  40 ~ 300>
    cfg_uint16  BM_AAC_Playing_CacheData = 200;  // <"AAC 格式播放延迟 ms",  50 ~ 300>
};


class CFG_BTSpeech_Player_Param  // <"播放器参数", CFG_CATEGORY_BT_CALL, dev_mode>
{
    cfg_uint32  BS_Develop_Value1 =
    (
    #if (BOARD_TYPE == BOARD_LARK)
        32527
    #else
        15
    #endif
    );  // <"开发者模式参数 1">

    cfg_uint8   BS_WaitData_Time           = 20;    // <"数据不增加时持续等待时间">
    cfg_uint8   BS_WaitData_Empty_Time     = 0;     // <"数据为空时持续等待时间 (0 表示一直等待)">
    cfg_uint8   BS_Max_Decode_Count        = 2;     // <"最大解码次数">
    cfg_uint16  BS_CVSD_Max_Sleep_Time     = 1000;  // <"CVSD 最大睡眠时间 us">
    cfg_uint16  BS_MSBC_Max_Sleep_Time     = 1000;  // <"MSBC 最大睡眠时间 us">
    cfg_uint16  BS_TWS_WPlay_Mintime       = 100;    // <"TWS 同时播放等待最小时间间隔 (中断时间) ms">
    cfg_uint16  BS_TWS_WPlay_Maxtime       = 600;   // <"TWS 同时播放等待最大时间间隔 (超时时间) ms">
    cfg_uint16  BS_TWS_WStop_Mintime       = 60;    // <"TWS 同时停止等待最小时间间隔 (中断时间) ms">
    cfg_uint16  BS_TWS_WStop_Maxtime       = 80;    // <"TWS 同时停止等待最大时间间隔 (超时时间) ms">
    cfg_uint16  BS_TWS_Sync_interval       = 200;   // <"TWS 播放过程中同步时间间隔 pkt">
    cfg_uint16  BS_Max_PCMBUF_Sampels_CVSD = 256;   // <"CVSD 输出端持续解码最大门限值">
    cfg_uint16  BS_Max_PCMBUF_Sampels_MSBC = 512;   // <"MSBC 输出端持续解码最大门限值">
    cfg_uint16  BS_Het_PCMBUF_Sampels      = 256;   // <"PCMBUF 半空中断门限值">
    cfg_uint16  BS_Hft_PCMBUF_Sampels      = 268;   // <"PCMBUF 半满中断门限值">
    cfg_uint16  BS_StartPlay_Normal        = 80;  // <"普通模式开始播放延迟 ms">
    cfg_uint16  BS_StartPlay_TWS           = 30;    // <"TWS 模式开始播放延迟 ms">
    cfg_uint8   BS_Work_Frequency_MSBC     = 102;   // <"MSBC工作频率(包括系统正常工作的频率)">
    cfg_uint8   BS_Module_Frequency_MSBC   = 60;    // <"MSBC模块频率(此模块所需频率增量)">
    cfg_uint8   BS_Work_Frequency_CVSD     = 90;    // <"MSBC工作频率(包括系统正常工作的频率)">
    cfg_uint8   BS_Module_Frequency_CVSD   = 48;    // <"MSBC模块频率(此模块所需频率增量)">
    cfg_uint8   BS_Module_Frequency_TMIC   = 36;    // <"双MIC模块频率(此模块所需频率增量)">
    cfg_uint8   BS_Module_Frequency_PLC    = 36;    // <"PLC模块频率(此模块所需频率增量)">
    cfg_uint8   BS_MIC_Playing_PKTCNT      = 3;     // <"播放过程中控制器队列缓存MIC包数 ">
};


class CFG_BTSpeech_User_Settings  // <"播放器设置", CFG_CATEGORY_BT_CALL>
{
    cfg_uint8   BS_DataWidth = 4;               // <"音频输出位宽选择", /* 值为 2 表示 16bit 输出, 4 表示24bit 输出 */>
    cfg_uint16  BS_Max_Out_Gain = VOLUME_GAIN_40;  // <"最大输出幅度", CFG_TYPE_VOLUME_GAIN, asqt>
    cfg_uint16  BS_Fadein_Continue_Time    = 200;   // <"淡入持续时间 ms">
    cfg_uint16  BS_Fadeout_Continue_Time   = 10;    // <"淡出持续时间 ms", dev_mode>
    cfg_uint16  BS_CVSD_Playing_CacheData  = 80;  // <"CVSD 格式播放延迟 ms",  30 ~ 150>
    cfg_uint16  BS_MSBC_Playing_CacheData  = 80;  // <"MSBC 格式播放延迟 ms",  30 ~ 150>
};


class CFG_IGSpeech_Player_Param  // <"播放器参数", CFG_CATEGORY_IG_CALL, dev_mode, hide>
{
    cfg_uint32  IS_Develop_Value1            = 0;    // <"开发者模式参数 1">
    cfg_uint8   IS_AEC_Asr_NrLevel           = 0;    // <"打断唤醒时的 level, 默认为 0 (aec mode 为 1 时才有效)">
    cfg_uint8   IS_AEC_LowSkip_Enable        = 1;    // <"打断唤醒和正常通话都有效, 默认为 1">
    cfg_uint8   IS_AEC_ShiftProcess          = 130;  // <"AEC 处理后的数据移位操作">
    cfg_uint8   IS_Work_Frequency            = 60;   // <"普通场景的工作频率增量, 理论上只有 VAD">
    cfg_uint8   IS_Module_Frequency_VAD      = 20;   // <"AEC模块频率(此模块所需频率增量)">
    cfg_uint8   IS_Module_Frequency_AEC      = 60;   // <"AEC模块频率(此模块所需频率增量)">
    cfg_uint8   IS_Module_Frequency_ASR      = 30;   // <"ASR模块频率(此模块所需频率增量)">
};


class CFG_IGSpeech_User_Settings  // <"播放器设置", CFG_CATEGORY_IG_CALL, hide>
{
    CFG_Type_MIC_Gain  MIC_Gain =
    {
        .ADC0_Gain = MIC_GAIN_24_0_DB,
        .ADC1_Gain = MIC_GAIN_24_0_DB,
        .ADC2_Gain = MIC_GAIN_24_0_DB,
        .ADC3_Gain = MIC_GAIN_24_0_DB,

    };  // <"MIC 增益", CFG_Type_MIC_Gain, click_popup>
};


class CFG_BLE_Manager  // <"BLE 管理", CFG_CATEGORY_BLE, fixed_size=128>
{
    cfg_uint8  BLE_Enable = NO;  // <"启用 BLE 功能", CFG_TYPE_BOOL>

    cfg_uint8  Use_Advertising_Mode_2_After_Paired = NO;  // <"配对连接过后使用 BLE 广播模式 2", CFG_TYPE_BOOL, /* 配对列表非空且不在配对模式 */>

    cfg_uint8   BLE_Address_Type = CFG_BLE_STATIC_DEVICE_ADDRESS;  // <"BLE 地址类型", CFG_TYPE_BLE_ADDR_TYPE>

    cfg_uint8  Advertising_After_Connected = YES;   // <"经典蓝牙连接后才进行BLE广播", CFG_TYPE_BOOL>

};


class CFG_BLE_Advertising_Mode_1  // <"BLE 广播模式 1", CFG_CATEGORY_BLE>
{
    cfg_uint16  Advertising_Interval_Ms = 500;   // <"广播间隔 (毫秒)", 20 ~ 5000>
    cfg_uint8   Advertising_Type = CFG_ADV_IND;  // <"广播类型", CFG_TYPE_BLE_ADV_TYPE>

    cfg_uint8   BLE_Device_Name[29] =
    (
        "ZS302A-Hello"
    );  // <"BLE 设备名称", string>

    cfg_uint8   Manufacturer_Specific_Data[59] = "";  // <"厂商自定义数据", string>
    cfg_uint8   Service_UUIDs_16_Bit[59]       = "";  // <"服务 UUIDs (16-Bit)",  string>
    cfg_uint8   Service_UUIDs_128_Bit[CFG_MAX_UUID_STR_LEN] = "";  // <"服务 UUIDs (128-Bit)", string>
};


class CFG_BLE_Advertising_Mode_2  // <"BLE 广播模式 2", CFG_CATEGORY_BLE>
{
    cfg_uint16  Advertising_Interval_Ms = 500;   // <"广播间隔 (毫秒)", 20 ~ 5000>
    cfg_uint8   Advertising_Type = CFG_ADV_IND;  // <"广播类型", CFG_TYPE_BLE_ADV_TYPE>

    cfg_uint8   BLE_Device_Name[29] = "";  // <"BLE 设备名称", string>

    cfg_uint8   Manufacturer_Specific_Data[59] = "";  // <"厂商自定义数据", string>
    cfg_uint8   Service_UUIDs_16_Bit[59]       = "";  // <"服务 UUIDs (16-Bit)",  string>
    cfg_uint8   Service_UUIDs_128_Bit[CFG_MAX_UUID_STR_LEN] = "";  // <"服务 UUIDs (128-Bit)", string>
};


class CFG_BLE_Connection_Param  // <"BLE 连接参数", CFG_CATEGORY_BLE>
{
    cfg_uint16  Interval_Min_Ms = 10;    // <"最小间隔 (毫秒)", 8 ~ 1000>
    cfg_uint16  Interval_Max_Ms = 30;    // <"最大间隔 (毫秒)", 8 ~ 1000>
    cfg_uint16  Latency         = 2;     // <"延迟",            0 ~ 100>
    cfg_uint16  Timeout_Ms      = 3000;  // <"超时 (毫秒)",     500 ~ 10000>
};


class CFG_BLE_Pass_Through  // <"BLE 数据透传", CFG_CATEGORY_BLE>
{
    cfg_uint8   Enable_BLE_Pass_Through = NO;  // <"启用 BLE 数据透传", CFG_TYPE_BOOL>

    cfg_uint8   Service_UUID[CFG_MAX_UUID_STR_LEN] = "0366";  // <"服务 UUID",  string>
    cfg_uint8   TX_RX_UUID[CFG_MAX_UUID_STR_LEN]   = "0466";  // <"TX/RX UUID", string>

    cfg_uint16  RX_Buffer_Size = 2048;  // <"RX 缓冲区大小", 128 ~ 4096>
};


class CFG_BT_Link_Quality  // <"链路质量监控", CFG_CATEGORY_BLUETOOTH, dev_mode>
{
    cfg_uint8  Quality_Pre_Value = 200;
    cfg_uint8  Quality_Diff      = 55;
    cfg_uint8  Quality_ESCO_Diff = 20;
    cfg_uint8  Quality_Monitor   = 1;
};


class CFG_BT_Scan_Params  // <"SCAN 参数设置", CFG_CATEGORY_BLUETOOTH, dev_mode>
{
    CFG_Type_BT_Scan_Params  Params[7] =
    {
        { CFG_DEFAULT_INQUIRY_PAGE_SCAN_MODE, 0x12, 0x1000, 0,  0x12, 0x800, 0 },
        { CFG_FAST_PAGE_SCAN_MODE,            0,    0,      0,  0x30, 0x180, 1 },
        { CFG_FAST_PAGE_SCAN_MODE_EX,         0,    0,      0,  0x60, 0x200, 1 },
        { CFG_NORMAL_PAGE_SCAN_MODE,          0,    0,      0,  0x18, 0x380, 1 },
        { CFG_NORMAL_PAGE_SCAN_MODE_S3,       0,    0,      0,  0x12, 0x800, 1 },
        { CFG_NORMAL_PAGE_SCAN_MODE_EX,       0,    0,      0,  0x60, 0x500, 1 },
        { CFG_FAST_INQUIRY_PAGE_SCAN_MODE,    0x60, 0x200,  1,  0x80, 0x480, 1 },

    };  // <"SCAN 参数设置", CFG_Type_BT_Scan_Params>
};


class CFG_App_Music  // <"本地播放设置", CFG_CATEGORY_APP_MUSIC, fixed_size=128, hide>
{
    cfg_uint8  Reserved = 0;  // <"数据", hex>
};


class CFG_Card_Settings  // <"存储卡设置", CFG_CATEGORY_CARD, fixed_size=128, hide>
{
    cfg_uint8  Reserved = 0;  // <"数据", hex>
};


class CFG_USB_Settings  // <"USB 设置", CFG_CATEGORY_USB, fixed_size=128, hide>
{
    cfg_uint8  Reserved = 0;  // <"数据", hex>
};


class CFG_Usr_Reserved_Data  // <"用户保留配置", CFG_CATEGORY_SYSTEM, fixed_size=255>
{
    cfg_uint8  String[128] = { 0, };  // <"字符串", string>

    cfg_uint8  Run_Console_Command[127] = { 0, };  // <"执行控制台命令", string>
};


class CFG_Sys_Reserved_Data  // <"系统更多配置", CFG_CATEGORY_SYSTEM, fixed_size=255, hide>
{
    cfg_uint8  Reserved = 0;  // <"数据", hex>
};


