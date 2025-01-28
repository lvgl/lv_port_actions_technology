/*
 * Copyright (c) 2021  Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC battery and charger driver
 */

#include <stdlib.h>
#include <errno.h>
#include <kernel.h>
#include <sys/byteorder.h>
#include <drivers/power_supply.h>
#include <soc.h>
#include <board.h>

#include "..\bat_charge_private.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(coulo_ext, CONFIG_LOG_DEFAULT_LEVEL);



//i2c access addr
#define	CW2015_SLAVER_ADDR		(0xc4>>1)
#define CW2015_RETRY_CNT        3

//cw2015 reg define
#define REG_VERSION             0x0
#define REG_VCELL               0x2
#define REG_SOC                 0x4
#define REG_RRT_ALERT           0x6
#define REG_CONFIG              0x8
#define REG_MODE                0xA
#define REG_BATINFO             0x10

#define MODE_SLEEP_MASK         (0x3<<6)
#define MODE_SLEEP              (0x3<<6)
#define MODE_NORMAL             (0x0<<6)
#define MODE_QUICK_START        (0x3<<4)
#define MODE_RESTART            (0xf<<0)

#define CONFIG_UPDATE_FLG       (0x1<<1)

#define ATHD                    (0x0<<3)        //ATHD = 0%
#define UI_FULL                 100

#define SIZE_BATINFO            64

#define BATTERY_DOWN_MIN_CHANGE_SLEEP 1800      // the min time allow battery change quantity when run 30min

#define BATTERY_VOLTAGE_AVERAGE_TIME    (2)


/* battery profile info,  need change to match actual battery */
static unsigned char cw_bat_config_info[SIZE_BATINFO] = {
0x15  ,0x4C  ,0x5D  ,0x5D  ,0x5A  ,0x59  ,0x55  ,
0x51  ,0x4E  ,0x48  ,0x46  ,0x41  ,0x3C  ,0x39  ,
0x33  ,0x2D  ,0x25  ,0x1E  ,0x19  ,0x19  ,0x1A  ,
0x2C  ,0x44  ,0x4A  ,0x43  ,0x40  ,0x0C  ,0xCD  ,
0x22  ,0x43  ,0x56  ,0x82  ,0x78  ,0x6F  ,0x62  ,
0x60  ,0x42  ,0x19  ,0x37  ,0x31  ,0x00  ,0x1D  ,
0x59  ,0x85  ,0x8F  ,0x91  ,0x91  ,0x18  ,0x58  ,
0x82  ,0x94  ,0xA5  ,0xFF  ,0xAF  ,0xE8  ,0xCB  ,
0x2F  ,0x7D  ,0x72  ,0xA5  ,0xB5  ,0xC1  ,0x46  ,
0xAE
};

struct acts_coulometer_data {
	const  struct device *i2c_dev;
	const  struct device *this_dev;
	struct k_work coulometer_init_work;

	uint16_t err_count;
	uint16_t capacity_zero_count;
	uint32_t capacity;
	uint32_t voltage;	
    struct k_delayed_work coulometer_getinfo_timer;		
};

struct acts_coulometer_config {
	uint16_t poll_interval_ms;
};

static struct acts_coulometer_data coulometer_acts_ddata;


static bool _cw2015_write_bytes(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
	int i2c_op_ret = 0;
	struct acts_coulometer_data *pmeter = dev->data;
	uint8_t retrycnt = CW2015_RETRY_CNT;
	uint8_t sendBuf[10];

    sendBuf[0]=reg;
	memcpy(sendBuf + 1, data, len);
	
	while (retrycnt--) {
		i2c_op_ret = i2c_write(pmeter->i2c_dev, sendBuf, len+1, CW2015_SLAVER_ADDR);
		if (0 != i2c_op_ret) {
			k_busy_wait(4*1000);
			LOG_ERR("err = %d\n", i2c_op_ret);
		} else {
			LOG_INF("CW2015_W: 0x%x - 0x%x\n", reg, sendBuf[1]);
			return true;
		}
	}

	return false;
}

static bool _cw2015_read_bytes(const struct device *dev, uint8_t reg, uint8_t *value, uint16_t len)
{
	int i2c_op_ret = 0;
	struct acts_coulometer_data *pmeter = dev->data;
	uint8_t retrycnt = CW2015_RETRY_CNT;

	uint8_t sendBuf[4];

	sendBuf[0] = reg;	

	while (retrycnt--) {
		i2c_op_ret = i2c_write(pmeter->i2c_dev, sendBuf, 1, CW2015_SLAVER_ADDR);
		if(0 != i2c_op_ret) {
			k_busy_wait(4*1000);
			LOG_ERR("err = %d\n", i2c_op_ret);
			if (retrycnt == 1) {
				return false;
			}
		} else {
			break;
		}
	}

	retrycnt = CW2015_RETRY_CNT;

	while (retrycnt--) {
		i2c_op_ret = i2c_read(pmeter->i2c_dev, value, len, CW2015_SLAVER_ADDR);
		if(0 != i2c_op_ret) {
			k_busy_wait(4*1000);
			LOG_ERR("err = %d\n", i2c_op_ret);
		} else {
			LOG_INF("CW2015_R: 0x%x - 0x%x\n", reg, *value);
			return true;
		}
	}

    return false;
}

/**
**	update battery profile info , run when cw2015 boot
**	return -1:  i2c r/w err;   return -2:  cw2015 in sleep mode;   return -3:  the profile after writting is not match
**/
static int32_t _cw2015_update_config_info(void)
{
	uint8_t i;
	uint8_t reset_val;
	uint8_t reg_val;
	bool b_ret;
	
	/* make sure no in sleep mode */
	b_ret = _cw2015_read_bytes(coulometer_acts_ddata.this_dev, REG_MODE, &reg_val, 1);
	if(!b_ret) {    
		return -1;	
	}	

	if((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP) {
		return -2;
	}
	
	/* update new battery info */
	for(i = 0; i < SIZE_BATINFO; i++) {
		reg_val = cw_bat_config_info[i];
		b_ret = _cw2015_write_bytes(coulometer_acts_ddata.this_dev, REG_BATINFO+i, &reg_val, 1);
		if(!b_ret) {
			return -1;
		}
	}

	/* readback & check */
	for(i = 0; i < SIZE_BATINFO; i++) {
		b_ret = _cw2015_read_bytes(coulometer_acts_ddata.this_dev, REG_BATINFO+i, &reg_val, 1);
		if(!b_ret) {
			return -1;
		}
		
		if(reg_val != cw_bat_config_info[i]) {
		    LOG_ERR("bat info err: %d, need: 0x%02x, read: 0x%02x", i, cw_bat_config_info[i], reg_val);
			return -3;
		}
	}
	
	/* set cw2015/cw2013 to use new battery info */
	b_ret = _cw2015_read_bytes(coulometer_acts_ddata.this_dev, REG_CONFIG, &reg_val, 1);
	if(!b_ret) {
		return -1;
	}
	reg_val |= CONFIG_UPDATE_FLG;   /* set UPDATE_FLAG */
	reg_val &= 0x07;                /* clear ATHD */
	reg_val |= ATHD;                /* set ATHD */
	b_ret = _cw2015_write_bytes(coulometer_acts_ddata.this_dev, REG_CONFIG, &reg_val, 1);
	if(!b_ret) {
		return -1;
	}
	
	/* reset */
	reset_val = MODE_NORMAL;
	reg_val = MODE_RESTART;
	b_ret = _cw2015_write_bytes(coulometer_acts_ddata.this_dev, REG_MODE, &reg_val, 1);
	if(!b_ret) {
		return -1;
	}
	
	k_busy_wait(10*1000);  //delay 10ms      
	b_ret = _cw2015_write_bytes(coulometer_acts_ddata.this_dev, REG_MODE, &reset_val, 1);	
	if(!b_ret) {
		return -1;
	} 
	k_busy_wait(100);  //delay 100us
	
	return 0;
}


/**
**	cw2015 init function, run when cw2015 boot
**	return:
**	-1: i2c r/w err; -2: cw2015 in sleep mode;
**	-3: the profile after writting is not match; -4: read battery voltage invalid within 3s
**/
static int32_t _cw2015_init(const struct device *dev)
{
	int32_t ret;
	uint8_t i;
	uint8_t reg_val = MODE_NORMAL;
    bool b_ret;

	/* wake up cw2015/13 from sleep mode */
	b_ret = _cw2015_write_bytes(dev, REG_MODE, &reg_val, 1);
	if(!b_ret) {
		LOG_ERR("wake up cw2015 fail!\n");	    
		return -1;		
	}
	
	/* check ATHD if not right */
	b_ret = _cw2015_read_bytes(dev, REG_CONFIG, &reg_val, 1);
	if(!b_ret) {
		LOG_ERR("read cw2015 config reg fail!\n");	    
		return -1;	
	}
	if((reg_val & 0xf8) != ATHD) {
		//"the new ATHD need set"
		reg_val &= 0x07;    /* clear ATHD */
		reg_val |= ATHD;    /* set ATHD */
		b_ret = _cw2015_write_bytes(dev, REG_CONFIG, &reg_val, 1);
		if(!b_ret) {
		    LOG_ERR("clr ATHD fail!\n");	   
			return -1;
		}
	}
	
	/* check config_update_flag if not right */
	b_ret = _cw2015_read_bytes(dev, REG_CONFIG, &reg_val, 1);
	if(!b_ret) {
		LOG_ERR("read cw2015 config reg fail!\n");	    
		return -1;	
	}

	if(!(reg_val & CONFIG_UPDATE_FLG)) {
		//"update flag for new battery info need set"
		ret = _cw2015_update_config_info();
		if(ret!=0) {
			return ret;
		}
	} else {
		for(i = 0; i < SIZE_BATINFO; i++) { 
			b_ret = _cw2015_read_bytes(dev, REG_BATINFO+i, &reg_val, 1);
			if(!b_ret) {
				return -1;
			}
			
			if(cw_bat_config_info[i] != reg_val) {
			    LOG_ERR("bat info err: %d, need: 0x%02x, read: 0x%02x", i, cw_bat_config_info[i], reg_val);
				break;
			}
		}
		
		if(i != SIZE_BATINFO) {
			reg_val = MODE_SLEEP;
			b_ret = _cw2015_write_bytes(dev, REG_MODE, &reg_val, 1);
			if(!b_ret) {
				return -1;
			}
			
			k_busy_wait(30*1000);    //delay 30ms
			
			reg_val = MODE_NORMAL;
			b_ret = _cw2015_write_bytes(dev, REG_MODE, &reg_val, 1);
			if(!b_ret) {
				return -1;
			}
			
			//"update flag for new battery info need set"
			ret = _cw2015_update_config_info();
			if(ret!=0) {
				return ret;
			}
		}
	}
	
	/* check SOC if not eqaul 255 */
	for (i = 0; i < 30; i++) {
		k_busy_wait(100*1000);//delay 100ms
		b_ret = _cw2015_read_bytes(dev, REG_SOC, &reg_val, 1);
		if (!b_ret) {
			return -1;
		} else if (reg_val <= 100) {
			break;
		}
    }
	
    if (i >=30) {
        reg_val = MODE_SLEEP;
		b_ret = _cw2015_write_bytes(dev, REG_MODE, &reg_val, 1);
        return -4;
    } 
	
	return 0;
}

#if 0
void update_usb_online(void)
{
	//fgauge_acts_ddata.usb_online = true;
	fgauge_acts_ddata.usb_online = false;
}
#endif

static int _cw2015_por(void)
{
	bool b_ret;
	uint8_t reset_val = 0;
	int32_t ret;
	struct acts_coulometer_data *pmeter = &coulometer_acts_ddata;

	//enter sleep mode
	reset_val = MODE_SLEEP;             
	b_ret = _cw2015_write_bytes(pmeter->this_dev, REG_MODE, &reset_val, 1);
	if (!b_ret) {
		return -1;
	}
	
	k_busy_wait(30*1000);  //delay 30ms

	//restart
	reset_val = MODE_NORMAL;
	b_ret = _cw2015_write_bytes(pmeter->this_dev, REG_MODE, &reset_val, 1);
	if (!b_ret) {
		return -1;
	}
	
	k_busy_wait(100); //delay 100us
	
	ret = _cw2015_init(pmeter->this_dev);
	if (ret!=0) {
		LOG_ERR("cw2015 reset err!");
	}
	
	return ret;
}


static uint32_t _cw2015_get_batadc(void)
{
	uint8_t reg_val[2] = {0 , 0};
	uint32_t ad_value = 0;
	bool b_ret;

	b_ret = _cw2015_read_bytes(coulometer_acts_ddata.this_dev, REG_VCELL, reg_val, 2);
	if(!b_ret) {
		return 1;
	}
		
	ad_value = (reg_val[0] << 8) + reg_val[1];
	
	return(ad_value);
}

static int _cw2015_get_capacity(void)
{
	uint8_t  reg_val[2];
	uint8_t  cw_capacity;
	uint32_t remainder = 0;
	uint32_t real_SOC = 0;
	uint32_t digit_SOC = 0;
	uint32_t UI_SOC = 0;
	bool b_ret;
	int ret;

	b_ret = _cw2015_read_bytes(coulometer_acts_ddata.this_dev, REG_SOC, reg_val, 2);
	if(!b_ret) {
		return -1;
	}

	real_SOC = reg_val[0];
	digit_SOC = reg_val[1];
	
	cw_capacity = real_SOC;
	
	if ((cw_capacity < 0) || (cw_capacity > 100)) {
        LOG_ERR("get cw_capacity error, cw_capacity = %d", cw_capacity);        
        coulometer_acts_ddata.err_count++;
		if (coulometer_acts_ddata.err_count > 5) { 
			ret = _cw2015_por(); //por ic
			if(ret != 0) {
				return -1;
			}
			
			coulometer_acts_ddata.err_count = 0;               
		}
        return coulometer_acts_ddata.capacity;
    } else {
        coulometer_acts_ddata.err_count = 0;
    }
	
	if(cw_capacity == 0) {		  
		coulometer_acts_ddata.capacity_zero_count++;
		if(coulometer_acts_ddata.capacity_zero_count >= BATTERY_DOWN_MIN_CHANGE_SLEEP) {
		    LOG_ERR("cw2015 detect cap is 0 too long!");
            ret = _cw2015_por(); //por ic
			if(ret != 0) {
				return -1;
			}

			coulometer_acts_ddata.capacity_zero_count = 0;
		}
	} else {
		coulometer_acts_ddata.capacity_zero_count = 0;
	}

	//UI_SOC = ((real_SOC * 256 + digit_SOC) * 100)/ (UI_FULL * 256);
	//remainder = (((real_SOC * 256 + digit_SOC) * 100 * 100) / (UI_FULL * 256)) % 100;
	UI_SOC = real_SOC * 256 + digit_SOC;
 	UI_SOC = ((uint32_t)UI_SOC * 100)/ (UI_FULL * 256);
	remainder = real_SOC * 256 + digit_SOC;
 	remainder = (((uint32_t)remainder * 100 * 100) / (UI_FULL * 256)) % 100;
	
	LOG_INF("real_SOC = %d digit_SOC = %d ui_100 = %d UI_SOC = %d remainder = %d\n", 
	    real_SOC, digit_SOC, UI_FULL, UI_SOC, remainder);
	
	/*aviod swing*/
	if(UI_SOC >= 100){
		UI_SOC = 100;
	}else if ((0 == UI_SOC) && (10 >= remainder)){
		UI_SOC = 0;
	}else{
		if((remainder > 80 || remainder < 20) && (UI_SOC >= (coulometer_acts_ddata.capacity - 1)) && (UI_SOC <= (coulometer_acts_ddata.capacity + 1)))
		{
			UI_SOC = coulometer_acts_ddata.capacity;
		}
	}
	
	return UI_SOC;
}


static void _cw2015_update_capacity(void)
{
	int cw_capacity;
	cw_capacity = _cw2015_get_capacity();
	if((cw_capacity >= 0) && (cw_capacity <= 100) && (coulometer_acts_ddata.capacity != cw_capacity))
	{       
		coulometer_acts_ddata.capacity = cw_capacity;
	}
}

static void _cw2015_adc_sample_buf_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int  buf_size = BAT_ADC_SAMPLE_BUF_SIZE;
    int  i;

    /* sample BAT ADC data in 10ms */
    for (i = 0; i < buf_size; i++)
    {
        bat_charge->bat_adc_sample_buf[i] = (uint16_t)_cw2015_get_batadc();

        k_usleep(10000 / buf_size);
    }

    bat_charge->bat_adc_sample_index = 0;
}

void _cw2015_adc_sample_buf_put(uint32_t bat_adc)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    int buf_size = BAT_ADC_SAMPLE_BUF_SIZE;

    bat_charge->bat_adc_sample_buf[bat_charge->bat_adc_sample_index] = (uint16_t)bat_adc;
    bat_charge->bat_adc_sample_index += 1;

    if (bat_charge->bat_adc_sample_index >= buf_size)
    {
        bat_charge->bat_adc_sample_index = 0;
    }
}

static void _cw2015_bat_volt_selectsort(uint16_t *arr, uint16_t len)
{
	uint16_t i, j, min;

	for (i = 0; i < len-1; i++)	{
		min = i;
		for (j = i+1; j < len; j++) {
			if (arr[min] > arr[j])
				min = j;
		}
		/* swap */
		if (min != i) {
			arr[min] ^= arr[i];
			arr[i] ^= arr[min];
			arr[min] ^= arr[i];
		}
	}
}

uint32_t _cw2015_get_voltage_mv(uint32_t adc_val)
{
    uint32_t mv;

    if (!adc_val) {
        return 0;
    }

	mv = adc_val  * 305 / 1000;

    return mv;
}


/* get the average battery voltage during specified time */
uint32_t _cw2015_get_battery_voltage_by_time(uint32_t sec)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int  buf_size = BAT_ADC_SAMPLE_BUF_SIZE;
    int  index = bat_charge->bat_adc_sample_index;
    int  adc_count = (sec * 1000 / BAT_ADC_SAMPLE_INTERVAL_MS);
    int  bat_adc, volt_mv, i;
    uint16_t tmp_buf[BAT_ADC_SAMPLE_BUF_SIZE] = {0};

    for (i = 0; i < adc_count; i++) {
        if (index > 0) {
            index -= 1;
        } else {
            index = buf_size - 1;
        }

        tmp_buf[i] = bat_charge->bat_adc_sample_buf[index];
    }

    /* sort the ADC data by ascending sequence */
	_cw2015_bat_volt_selectsort(tmp_buf, adc_count);

    /* get the last 4/5 position data */
    bat_adc = tmp_buf[adc_count * 4/5 - 1];

    volt_mv = _cw2015_get_voltage_mv(bat_adc);

    return volt_mv;
}


uint32_t _cw2015_get_battery_percent(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    uint32_t bat_volt = _cw2015_get_battery_voltage_by_time(BATTERY_VOLTAGE_AVERAGE_TIME);
    int i;
    uint32_t level_begin;
    uint32_t level_end;
    uint32_t percent;

    if (bat_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
        return 100;
    }

    for (i = CFG_MAX_BATTERY_LEVEL - 1; i > 0; i--) {
        if (bat_volt >= bat_charge->configs.cfg_bat_level.Level[i])
        {
            break;
        }
    }

    level_begin = bat_charge->configs.cfg_bat_level.Level[i];

    if (i < CFG_MAX_BATTERY_LEVEL - 1) {
        level_end = bat_charge->configs.cfg_bat_level.Level[i+1];
    } else {
        level_end = bat_charge->configs.cfg_charge.Charge_Stop_Voltage;
    }

    if (bat_volt > level_end) {
        bat_volt = level_end;
    }

    percent = 100 * i / CFG_MAX_BATTERY_LEVEL;

    if (bat_volt >  level_begin &&
        bat_volt <= level_end) {
        percent += 10 * (bat_volt - level_begin) / (level_end - level_begin);
    }

    LOG_INF("bat percent: %d", percent);
	
    return percent;
}


static void _cw2015_update_vol(void)
{
	uint32_t cw_batadc;
	
	cw_batadc = _cw2015_get_batadc();
	if(cw_batadc == 1) {
		LOG_ERR("read volage err!\n");
	} else {
		_cw2015_adc_sample_buf_put(cw_batadc);
	}
}

void coulometer_get_info(struct k_work *work)
{
    struct acts_coulometer_data *pmeter = &coulometer_acts_ddata;
	
	_cw2015_update_vol();
	_cw2015_update_capacity();

	//LOG_INF("cw2015 vol: %d; cap: %d\n", coulometer_acts_ddata.voltage, coulometer_acts_ddata.capacity);

	k_delayed_work_submit(&pmeter->coulometer_getinfo_timer, K_MSEC(BAT_ADC_SAMPLE_INTERVAL_MS));
}

static void coulometer_acts_enable(const struct device *dev)
{
    struct acts_coulometer_data *pmeter = dev->data;

	k_delayed_work_submit(&pmeter->coulometer_getinfo_timer, K_MSEC(BAT_ADC_SAMPLE_INTERVAL_MS));
}

static void coulometer_acts_disable(const struct device *dev)
{
    struct acts_coulometer_data *pmeter = dev->data;
	
	k_delayed_work_cancel(&pmeter->coulometer_getinfo_timer);
}

static int coulometer_acts_get_property(const struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = _cw2015_get_battery_voltage_by_time(BATTERY_VOLTAGE_AVERAGE_TIME);
		return 0;
	case POWER_SUPPLY_PROP_CAPACITY:
		//val->intval = coulometer_acts_ddata.capacity;
		val->intval = _cw2015_get_battery_percent();
		return 0;

	default:
		return -EINVAL;
	}
}

static void coulometer_acts_set_property(const struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	//empty
}


static int _cw2015_init_data(void)
{
	uint8_t reg_SOC[2];
	int real_SOC = 0;
	int digit_SOC = 0;
	int UI_SOC = 0;
	bool b_ret;


	b_ret = _cw2015_read_bytes(coulometer_acts_ddata.this_dev, REG_SOC, reg_SOC, 2);
    if (!b_ret) {
    	return -1;
    }
	
	real_SOC = reg_SOC[0];
	digit_SOC = reg_SOC[1];
	//UI_SOC = ((real_SOC * 256 + digit_SOC) * 100)/ (UI_FULL*256);
	UI_SOC = real_SOC * 256 + digit_SOC;
 	UI_SOC = (UI_SOC * 100)/ (UI_FULL * 256);
	if(UI_SOC >= 100) {
		UI_SOC = 100;
	}
	
	coulometer_acts_ddata.capacity = UI_SOC;
	coulometer_acts_ddata.err_count = 0;
	coulometer_acts_ddata.capacity_zero_count = 0;

	_cw2015_adc_sample_buf_init();
	
	return 0;
}

static void _coulometer_init_work(struct k_work *work)
{
	struct acts_coulometer_data *pmeter = &coulometer_acts_ddata;
    uint8_t loop;
    int32_t ret;
	
	for(loop=0; loop<3; loop++) {
	    ret = _cw2015_init(pmeter->this_dev);
		if(ret == 0) {
		    break;
		}
	}
	
	if(loop >= 3) {
	    LOG_INF("CW2015 init fail!\n");
		return; 
	}

    _cw2015_init_data();

    LOG_INF("CW2015 init OK!\n");
	return;	
}


static int _coulometer_acts_init(const struct device *dev)
{
	struct acts_coulometer_data *pmeter = dev->data;

	pmeter->this_dev = (struct device *)dev;
	
	pmeter->i2c_dev = (struct device *)device_get_binding(CONFIG_COULOMETER_I2C_NAME);
	if (!pmeter->i2c_dev) {
		printk("can not access right i2c device\n");
		return -1;
	}

	k_work_init(&pmeter->coulometer_init_work, _coulometer_init_work);
	k_work_submit(&pmeter->coulometer_init_work);
	
	k_delayed_work_init(&pmeter->coulometer_getinfo_timer, coulometer_get_info);

	return 0;
}


static const struct acts_coulometer_config _coulometer_acts_cdata = {
	.poll_interval_ms = BAT_ADC_SAMPLE_INTERVAL_MS,
};

static const struct coulometer_driver_api _coulometer_acts_driver_api = {
	.get_property = coulometer_acts_get_property,
	.set_property = coulometer_acts_set_property,
	.enable = coulometer_acts_enable,
	.disable = coulometer_acts_disable,
};


DEVICE_DEFINE(coulometer, CONFIG_ACTS_EXT_COULOMETER_DEV_NAME, _coulometer_acts_init,
			NULL, &coulometer_acts_ddata, &_coulometer_acts_cdata, POST_KERNEL,
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &_coulometer_acts_driver_api);


