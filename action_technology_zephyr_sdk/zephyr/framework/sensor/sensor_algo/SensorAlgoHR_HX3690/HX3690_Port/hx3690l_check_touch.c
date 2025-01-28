#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "hx3690l.h"
#include "hx3690l_check_touch.h"

#ifdef TYHX_DEMO
#include "twi_master.h"
#include "SEGGER_RTT.h"
#endif

hx3690_wear_msg_code_t hx3690l_wear_status = MSG_NO_WEAR;
hx3690_wear_msg_code_t hx3690l_wear_status_pre = MSG_NO_WEAR;
extern const int32_t check_mode_unwear_thre;
extern const int32_t check_mode_wear_thre;
//static uint8_t notouch_cnt = 0;
//static uint8_t touch_cnt = 0;

#ifdef CHECK_TOUCH_LIB


SENSOR_ERROR_T hx3690l_check_touch_enable(void)
{
    if (!hx3690l_chip_check()) 
    {
        AGC_LOG("hx3690l check id failed!\r\n");
        return SENSOR_OP_FAILED;
    }

    AGC_LOG("hx3690l check id success!\r\n");

    hx3690l_check_touch_init();
    hx3690l_320ms_timer_cfg(true);
    
    return SENSOR_OK;
}

uint8_t hx3690l_check_touch_read_fifo(int32_t *ir_buf)
{
    uint8_t data_flg = 127;
    int32_t data;
    uint8_t databuf[3];
    uint8_t ii=0;
    uint8_t data_size=0;
    uint8_t count=0;
    int32_t buf[64];
  
    data_size = hx3690l_read_fifo_size();
    count = data_size>>1;
    data_size = count*2;
    
    for(ii=0; ii<data_size; ii++) 
    {
        hx3690l_write_reg(0x17, 0x00); // write any walue to 0x17 will update a new data
        hx3690l_delay_us(100);
        hx3690l_brust_read_reg(0x15, databuf, 3); 
        data_flg = databuf[2]>>5;
        data = (int32_t)(databuf[0]|(databuf[1]<<8)|((databuf[2]&0x1f)<<16));

        if(ii==0)
        {
            if(data_flg ==3)
            {
              ii=3;
              buf[0] = 0;
              buf[1] = 0;
              buf[2] = 0;
            }
            if(data_flg ==2)
            {
              ii=2;
              buf[0] = 0;
              buf[1] = 0;
            }
            if(data_flg ==1)
            {
              ii=1;
              buf[0] = 0;
            }			
        }
		
        if(data_flg == 0) 
        {
            buf[ii]= data;
        } 
        else if(data_flg == 1)
        {
            buf[ii]= data;
        } 
        else if(data_flg == 2)
        {
            buf[ii]= data;
        } 
        else if(data_flg == 3) 
        {
            buf[ii]= data;
        }
    }
    for (ii=0; ii<count; ii++) 
    {
      //DEBUG_PRINTF(0,"%d %d %d\r\n" ,buf[ii*2],buf[ii*2+1],hx3690l_read_reg(0x2c)); 
        if(buf[ii*2]>buf[ii*2+1])
        {        
            ir_buf[ii] = buf[ii*2] - buf[ii*2+1];        
        }
        else
        {
            ir_buf[ii] = 0;
        }  
    }
    return count;
}

uint8_t hx3690l_check_touch_read(ppg_sensor_data_t *s_dat)
{
    uint8_t i=0;
	uint8_t size = 0;
    uint8_t fifo_out_size = 0;
	int32_t *ir_buf =  &(s_dat->ir_data[0]);
	int32_t *s_buf =  &(s_dat->s_buf[0]);
  
    size = hx3690l_read_fifo_size();
    if(size<2)
    {
        return 0;
    }
    
    if (size && size <= 64) 
    {
        fifo_out_size = hx3690l_read_fifo_data(s_buf,size,2);
        s_dat->count = fifo_out_size;
        for (i=0; i<fifo_out_size; i++) 
        {
            ir_buf[i] = abs(s_buf[i*2] - s_buf[i*2+1]);     
            //AGC_LOG("%d/%d %d %d %d\r\n" ,1+i,size,ir_buf[i],s_buf[i*2],s_buf[i*2+1]);         
        }
    }
    return size;
}

hx3690_wear_msg_code_t hx3690l_check_touch_send_data(int32_t *ir_data, uint8_t count)
{ 
    uint8_t ii;
    hx3690_wear_msg_code_t wear_status;
    if(count==0)
    {
        return wear_status;
    }
    for(ii=0; ii<count; ii++)
    {
        wear_status = check_touch_alg(ir_data[ii]);
    }
    return wear_status;
}

hx3690_wear_msg_code_t check_touch_alg(int32_t ir_data)
{
    if(ir_data>check_mode_wear_thre)
    {
        notouch_cnt = 0;
        if(touch_cnt>=2)
        {
            hx3690l_wear_status = MSG_WEAR;
            touch_cnt=0;
        }
        else
        {
            touch_cnt++;
        }
    }
    else if(ir_data<check_mode_unwear_thre)
    {
        touch_cnt = 0;
        if(notouch_cnt>=2)
        {
            hx3690l_wear_status = MSG_NO_WEAR;
            notouch_cnt=0;
        }
        else
        {
            notouch_cnt++;
        }
    }
    return hx3690l_wear_status;
}
#endif
