

#ifndef __DRV_CFG_HEAD_H__
#define __DRV_CFG_HEAD_H__


#define CFG_User_Version_Version                                                (0x01000020)    // id:  1, off:   0, size:  32

#define CFG_Platform_Case_IC_Type                                               (0x02000004)    // id:  2, off:   0, size:   4
#define CFG_Platform_Case_Board_Type                                            (0x02004001)    // id:  2, off:   4, size:   1
#define CFG_Platform_Case_Case_Name                                             (0x02005014)    // id:  2, off:   5, size:  20
#define CFG_Platform_Case_Major_Version                                         (0x02019001)    // id:  2, off:  25, size:   1
#define CFG_Platform_Case_Minor_Version                                         (0x0201a001)    // id:  2, off:  26, size:   1

#define CFG_Factory_Settings_Keep_User_VRAM_Data_When_UART_Upgrade              (0x03000001)    // id:  3, off:   0, size:   1
#define CFG_Factory_Settings_Keep_Factory_VRAM_Data_When_ATT_Upgrade            (0x03001001)    // id:  3, off:   1, size:   1
#define CFG_Factory_Settings_Erase_Entire_Storage                               (0x03002001)    // id:  3, off:   2, size:   1

#define CFG_BT_Call_Volume_Table_Level                                          (0x04000020)    // id:  4, off:   0, size:  32

#define CFG_Voice_Volume_Table_Level                                            (0x05000022)    // id:  5, off:   0, size:  34

#define CFG_Volume_Settings_Voice_Default_Volume                                (0x06000001)    // id:  6, off:   0, size:   1
#define CFG_Volume_Settings_Voice_Min_Volume                                    (0x06001001)    // id:  6, off:   1, size:   1
#define CFG_Volume_Settings_Voice_Max_Volume                                    (0x06002001)    // id:  6, off:   2, size:   1
#define CFG_Volume_Settings_BT_Call_Default_Volume                              (0x06003001)    // id:  6, off:   3, size:   1

#define CFG_BT_Call_Out_DAE_Enable_DAE                                          (0x07000001)    // id:  7, off:   0, size:   1
#define CFG_BT_Call_Out_DAE_Test_Volume                                         (0x07001001)    // id:  7, off:   1, size:   1

#define CFG_BT_Call_MIC_DAE_Enable_DAE                                          (0x08000001)    // id:  8, off:   0, size:   1
#define CFG_BT_Call_MIC_DAE_Test_Volume                                         (0x08001001)    // id:  8, off:   1, size:   1

#define CFG_BT_Call_Quality_MIC_Gain                                            (0x09000002)    // id:  9, off:   0, size:   2
#define CFG_BT_Call_Quality_Test_Volume                                         (0x09002001)    // id:  9, off:   2, size:   1


#endif  // __DRV_CFG_HEAD_H__

