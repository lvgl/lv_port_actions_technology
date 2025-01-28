/******************************************************************************/
/*                                                                            */
/*    Copyright 2024 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_log.c
 *
 */
/*********************
 *      INCLUDES
 *********************/
#include "aic_portable.h"

/**********************
 *  STATIC VARIABLES
 **********************/
/* Use default log level */
static alog_level_e g_log_level = ALOG_LEVEL_DEFAULT;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int alog_set_level(alog_level_e log_level)
{
    //LOG_I("set log level %d", log_level);
    if ((log_level > ALOG_LEVEL_MAX) || (log_level < ALOG_LEVEL_NONE)) {
        //LOG_I("log level %d is not support", log_level);
        return -1;
    }
    g_log_level = log_level;
    return 0;
}

alog_level_e alog_get_level(void)
{
    return g_log_level;
}

