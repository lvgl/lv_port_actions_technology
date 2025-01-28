/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef	_ACTIONS_SOC_ATP_H_
#define	_ACTIONS_SOC_ATP_H_

#ifndef _ASMLANGUAGE

/**
 * @brief atp get rf calib function
 *
 * This function is used to obtain RF calibration parameters
 *
 * @param id 0: get power table version bit
 *           1: get power index
 *           2: get avdd_if adjust param
 *          other value: invalid
 *
 * @param calib_val: return calib value
 * @return 0 if invoked succsess.
 * @return -1 if invoked failed.
 */
extern int soc_atp_get_rf_calib(int id, unsigned int *calib_val);

/**
 * @brief atp get pmu calib function
 *
 * This function is used to obtain pmu calibration parameters
 *
 * @param id 0: get Charger CV param
 *           1: get Charger CC param
 *           2: get 10ua bias current calib param
 *           3: get charge set param
 *           4: get batadc param
 *           5: get sensoradc param
 *           6: get DAC noise floor calibration param
 *           7: get dc5vadc param
 *           8: get charge set1 param
 *           9: get usb ref param
 *           10: get vcci  param
 *           11: get vd12  param
*            12: get PSRAM POWER  param
 *          other value:invalid
 * @param calib_val: return calib value
 * @return 0 if invoked succsess.
 * @return -1 if invoked failed.
 */
extern int soc_atp_get_pmu_calib(int id, unsigned int *calib_val);

/**
 * @brief atp get hosc calib function
 *
 * This function is used to obtain hosc calibration parameters
 *
 * @param id 0: get array0 param
 *           1: get array1 param
 *          other value:invalid
 * @param calib_val: return calib value
 * @return 0 if invoked succsess.
 * @return -1 if invoked failed.
 */
extern int soc_atp_get_hosc_calib(int id, unsigned int *calib_val);

/**
 * @brief atp get fcc param
 *
 * This function is used to obtain fcc parameters
 *
 * @param id 0: get array0 param
 *           1: get array1 param
 *          other value:invalid
 * @param param_val: return param value
 * @return 0 if invoked succsess.
 * @return -1 if invoked failed.
 */
extern int soc_atp_get_fcc_param(int id, unsigned int *param_val);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_ATP_H_	*/
