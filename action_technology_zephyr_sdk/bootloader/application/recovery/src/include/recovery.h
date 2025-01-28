/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *
 *  \file       
 *  \brief      
 *  \author     
 *  \version    
 *  \date       
 *  ***************************************************************************
 *
 */

#ifndef __RECOVERY_H__
#define __RECOVERY_H__

#include <zephyr/types.h>
#include <stdbool.h>


/**
 *  \brief check ota package flag is true.
 */
bool ota_upgrade_is_allowed(void);


/**
 *  \brief recovery main function.
 */

int recovery_main(void);

/**
 *  \brief ota main function.
 */

int ota_main(void);



#endif  /* __RECOVERY_H__ */

