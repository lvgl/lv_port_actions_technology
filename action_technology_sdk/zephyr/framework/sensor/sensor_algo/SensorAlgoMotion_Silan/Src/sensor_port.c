/*******************************************************************************
 * @file    sensor_port.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-7-21
 * @brief   sensor port api
*******************************************************************************/
#include <stdio.h>
#include <sensor_algo.h>
#include <SL_Watch_Pedo_Kcal_Wrist_Sleep_Sway_L_Algorithm.h>

unsigned char SL_SC7A20_I2cAddr_Read(unsigned char iic_addr, unsigned char reg, unsigned char len, unsigned char *buf)
{
    return sensor_hal_read(ID_ACC, reg, buf, len);
}

unsigned char SL_SC7A20_I2cAddr_Write(unsigned char iic_addr, unsigned char reg, unsigned char dat)
{
    return sensor_hal_write(ID_ACC, reg, &dat, 1);
}

unsigned char SL_SC7A20_I2c_Spi_Write(bool sl_spi_iic,unsigned char reg, unsigned char dat)
{
    return sensor_hal_write(ID_ACC, reg, &dat, 1);
}

unsigned char SL_SC7A20_I2c_Spi_Read(bool sl_spi_iic,unsigned char reg, unsigned char len, unsigned char *buf)
{
    return sensor_hal_read(ID_ACC, reg, buf, len);
}
