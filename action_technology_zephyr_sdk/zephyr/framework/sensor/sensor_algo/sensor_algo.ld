/*
 * Copyright (c) 2024 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_DATA_PROLOGUE(_SENSOR_FUNC_SECTION_NAME,,)
{
	. = ALIGN(4);
	__sensor_func_start = .;
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE
	 *(.cwm_sec.common_aml.p0)
	 *(.cwm_sec.common_amh_proc.p3)
	 *cwm_watch_handupdown*(.text* .rodata*)
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_IN_RAM
	 *CWM_LIB_m4.a:*(.text* .rodata*)
	*libgcc.a:*(.text* .rodata*)
	*libm.a:*(.text* .rodata*)
#endif
#endif
	__sensor_func_end = .;

} GROUP_DATA_LINK_IN(SRAM, ROMABLE_REGION)

__sensor_func_size = __sensor_func_end - __sensor_func_start;
__sensor_func_load_start = LOADADDR(_SENSOR_FUNC_SECTION_NAME);


SECTION_DATA_PROLOGUE(_SENSOR_DATA_SECTION_NAME,,)
{
	. = ALIGN(4);
	__sensor_data_start = .;
	*sensor_algo*(.data*)
	*hr_algo*(.data*)
	*libm.a:*(.data*)
	*libc_nano.a:*(.data*)

#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE
	*CWM_LIB_DML_m4.a:*(.data*)
	*CWM_LIB_m4.a:*(.data*)
#endif
#ifdef CONFIG_SENSOR_ALGO_MOTION_SILAN
	*SL_Watch_Pedo_Kcal_Wrist_Sleep_Sway_L_Algorithm.a:*(.data*)
	*SL_Watch_Pedo_Kcal_Wrist_Sleep_Sway_L_Application*(.data*)
	*sensor_port*(.data*)
#endif
#ifdef CONFIG_SENSOR_ALGO_HR_GH30X
	*hbd_communicate_M4.a:*(.data*)
	*hbd_ctrl_M4.a:*(.data*)
	*gh3011*(.data*)
#endif
#ifdef CONFIG_SENSOR_ALGO_HR_VCARE
	*vcHrAlgoV623ForCM4F.a:*(.data*)
	*vcSpo2AlgoV151_VP60A2_B_v2_ForCM4F.a:*(.data*)
	*vcSportMotionAlgoV11ForCM4F.a:*(.data*)
	*module_heart_vc_int*(.data*)
	*vcHr02*(.data*)
#endif
#ifdef CONFIG_SENSOR_ALGO_HR_VCARE_VC9202
	*lib_BIO_V141_GCCM33_Ubuntu1804_JuXin_01.a:*(.data*)
	*lib_hr_v637_GccM33_Ubuntu104_JuXin_01.a:*(.data*)
	*lib_SP_int_GccM33_Ubuntu1804_JuXin_01.a:*(.data*)
	*lib_spo2_9202VP60A2_Int_GccM33_Ubuntu1804_JuXIn.a:*(.data*)
	*vc9202_driver*(.data*)
	*vc9202_user*(.data*)
#endif
#ifdef CONFIG_SENSOR_ALGO_HR_HX3605
	*m33_3605_small_lib_hrs_spo2.a:*(.data*)
	*hx3605*(.data*)
#endif
#ifdef CONFIG_SENSOR_ALGO_HR_HX3690
	*m33_hx3690l_hrs_spo2_hrv.a:*(.data*)
	*hx3690*(.data*)
#endif
	__sensor_data_end = .;

} GROUP_DATA_LINK_IN(SRAM, ROMABLE_REGION)

__sensor_data_size = __sensor_data_end - __sensor_data_start;
__sensor_data_load_start = LOADADDR(_SENSOR_DATA_SECTION_NAME);


SECTION_PROLOGUE(_SENSOR_BSS_SECTION_NAME,(NOLOAD),)
{
	. = ALIGN(4);
	__sensor_bss_start = .;
	*sensor_algo*(.bss* COMMON)
	*hr_algo*(.bss* COMMON)
	*libm.a:*(.bss* COMMON)
	*libc_nano.a:*(.bss* COMMON)

#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE
	*CWM_LIB_DML_m4.a:*(.bss* COMMON)
	*CWM_LIB_m4.a:*(.bss* COMMON)
#endif
#ifdef CONFIG_SENSOR_ALGO_MOTION_SILAN
	*SL_Watch_Pedo_Kcal_Wrist_Sleep_Sway_L_Algorithm.a:*(.bss* COMMON)
	*SL_Watch_Pedo_Kcal_Wrist_Sleep_Sway_L_Application*(.bss* COMMON)
	*sensor_port*(.bss* COMMON)
#endif
#ifdef CONFIG_SENSOR_ALGO_HR_GH30X
	*hbd_communicate_M4.a:*(.bss* COMMON)
	*hbd_ctrl_M4.a:*(.bss* COMMON)
	*gh3011*(.bss* COMMON)
#endif
#ifdef CONFIG_SENSOR_ALGO_HR_VCARE
	*vcHrAlgoV623ForCM4F.a:*(.bss* COMMON)
	*vcSpo2AlgoV151_VP60A2_B_v2_ForCM4F.a:*(.bss* COMMON)
	*vcSportMotionAlgoV11ForCM4F.a:*(.bss* COMMON)
	*module_heart_vc_int*(.bss* COMMON)
	*vcHr02*(.bss* COMMON)
#endif
#ifdef CONFIG_SENSOR_ALGO_HR_VCARE_VC9202
	*lib_BIO_V141_GCCM33_Ubuntu1804_JuXin_01.a:*(.bss* COMMON)
	*lib_hr_v637_GccM33_Ubuntu104_JuXin_01.a:*(.bss* COMMON)
	*lib_SP_int_GccM33_Ubuntu1804_JuXin_01.a:*(.bss* COMMON)
	*lib_spo2_9202VP60A2_Int_GccM33_Ubuntu1804_JuXIn.a:*(.bss* COMMON)
	*vc9202_driver*(.bss* COMMON)
	*vc9202_user*(.bss* COMMON)
#endif
#ifdef CONFIG_SENSOR_ALGO_HR_HX3605
	*m33_3605_small_lib_hrs_spo2.a:*(.bss* COMMON)
	*hx3605*(.bss* COMMON)
#endif
#ifdef CONFIG_SENSOR_ALGO_HR_HX3690
	*m33_hx3690l_hrs_spo2_hrv.a:*(.bss* COMMON)
	*hx3690*(.bss* COMMON)
#endif
	__sensor_bss_end = .;

} GROUP_LINK_IN(SRAM)

__sensor_bss_size = __sensor_bss_end - __sensor_bss_start;
