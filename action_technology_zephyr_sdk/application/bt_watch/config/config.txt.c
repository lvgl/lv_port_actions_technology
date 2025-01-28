/*-----------------------------------------------------------------------------
 * 配置数据类定义
 * 类型必须以 CFG_XXX 命名
 * 类成员必须赋值
 *---------------------------------------------------------------------------*/


class CFG_User_Version  // <"用户版本", CFG_CATEGORY_SYSTEM>
{
    cfg_uint8  Version[CFG_MAX_USER_VERSION_LEN] = "ACTIONS_LEOPARD";  // <"版本信息", string>
};


class CFG_Platform_Case  // <"平台方案", CFG_CATEGORY_SYSTEM, readonly>
{
    cfg_uint32  IC_Type    = CFG_IC_TYPE;  // <"IC 类型", hex>
    cfg_uint8   Board_Type = BOARD_TYPE;   // <"板型">

    cfg_uint8   Case_Name[CFG_MAX_CASE_NAME_LEN] = "S6_01010101";  // <"方案名称", string>

    cfg_uint8   Major_Version = 1;  // <"主版本号">
    cfg_uint8   Minor_Version = 0;  // <"次版本号">
};

class CFG_Factory_Settings  // <"固件烧录设置", CFG_CATEGORY_UPGRADE>
{
    cfg_uint8   Keep_User_VRAM_Data_When_UART_Upgrade = NO;    // <"烧录固件时保留用户区数据", CFG_TYPE_BOOL>
    cfg_uint8   Keep_Factory_VRAM_Data_When_ATT_Upgrade = YES;  // <"烧录固件时保留工厂区数据", CFG_TYPE_BOOL>
    cfg_uint8   Erase_Entire_Storage = NO;                     // <"烧录前擦除所有数据", CFG_TYPE_BOOL>
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

    cfg_uint8  BT_Call_Default_Volume  = 8;   // <"蓝牙通话默认音量", 0 ~ 15, slide_bar>
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


class CFG_BT_Call_Quality  // <"通话效果", CFG_CATEGORY_ASQT, adjust_online, asqt>
{
    CFG_Type_MIC_Gain  MIC_Gain =
    {
        .ADC0_Gain = MIC_GAIN_31_5_DB,
    };  // <"MIC 增益", CFG_Type_MIC_Gain, click_popup>

    cfg_uint8  Test_Volume = 8;  // <"测试音量", 0 ~ 15, slide_bar>
};

class CFG_IGSpeech_User_Settings  // <"播放器设置", CFG_CATEGORY_IG_CALL, hide>
{
    CFG_Type_MIC_Gain  MIC_Gain =
    {
        .ADC0_Gain = MIC_GAIN_24_0_DB,
    };  // <"MIC 增益", CFG_Type_MIC_Gain, click_popup>
};

class CFG_BT_Music_DAE  // <"蓝牙音乐音效", CFG_CATEGORY_ASET, adjust_online>
{
    cfg_uint8  Enable_DAE=1;      // <"音效使能", CFG_TYPE_BOOL>
    cfg_uint8  Test_Volume=8;     // <"测试音量", 0 ~ 16, slide_bar>
};



