/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *
 *  \file       bg_ota_main.c
 *  \brief      OTA main fraim
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2018-10-25
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "spp_test_inner.h"

void show_version_through_spp(void)
{
    char temp_str[48];
    int i;
    CFG_Struct_User_Version   usr_ver;
    CFG_Struct_Platform_Case  plat_case;

    app_config_read(CFG_ID_USER_VERSION,
        &usr_ver, 0, sizeof(CFG_Struct_User_Version));

    app_config_read(CFG_ID_PLATFORM_CASE,
        &plat_case, 0, sizeof(CFG_Struct_Platform_Case));

    // should NOT exceed size 48
    i = sprintf(temp_str, "User_Ver  : %s\n", usr_ver.Version);
    spp_test_backend_write(temp_str, i, 10);

    // should NOT exceed size 48
    i = sprintf(temp_str, "Case_Type : %s %d.%d\n",
        plat_case.Case_Name,
        plat_case.Major_Version,
        plat_case.Minor_Version);
    spp_test_backend_write(temp_str, i, 10);

	do
    {
        // wait spp disconnect
        os_sleep(100);
	} while (!tool_is_quitting());
}

