#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>


#include "hx3690l.h"
#include "hx3690l_spo2_agc.h"
#include "tyhx_spo2_alg.h"
#include "tyhx_hrs_alg.h"

#ifdef TYHX_DEMO
#include "twi_master.h"
#include "SEGGER_RTT.h"
#endif

#ifdef SPO2_ALG_LIB

extern const uint8_t  hx3690l_spo2_agc_red_idac;  // 6,7,8...
extern const uint8_t  hx3690l_spo2_agc_ir_idac;  // 6,7,8...
extern const uint8_t  hx3690l_spo2_agc_green_idac;  // 6,7,8...
extern const uint8_t  green_led_max_init;
extern const uint8_t  red_led_max_init;
extern const uint8_t  ir_led_max_init;
extern uint8_t low_vbat_flag;

//SPO2_INFRARED_THRES
extern const int32_t  spo2_ir_unwear_thres; 
extern const int32_t  spo2_ir_wear_thres; 
extern const uint8_t  spo2_ir_wear_offset;

static uint8_t s_ppg_state = 0;
static uint8_t s_cal_state = 0;
static int32_t agc_buf[4] = {0};

static uint8_t cal_delay = SPO2_CAL_DELAY_COUNT;
static SPO2_CAL_SET_T  calReg;

static spo2_wear_msg_code_t spo2_wear_status = MSG_SPO2_NO_WEAR;
static spo2_wear_msg_code_t spo2_wear_status_pre = MSG_SPO2_NO_WEAR;

void Init_Spo2_PPG_Calibration_Routine(SPO2_CAL_SET_T *calR,uint8_t led)
{
	calR->flag = CAL_FLG_LED_DR|CAL_FLG_LED_DAC|CAL_FLG_AMB_DAC|CAL_FLG_RF;
	calR->G_LEDDAC = 0;   /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	calR->R_LEDDAC = 0;   /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	calR->IR_LEDDAC = 0;   /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	calR->AMBDAC = 0;   /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	calR->G_RF = 0;       /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
	calR->R_RF = 0;       /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
	calR->IR_RF = 0;       /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
	calR->G_LED = SPO2_CAL_INIT_LED_GREEN;
	calR->R_LED = SPO2_CAL_INIT_LED_RED;
	calR->IR_LED = SPO2_CAL_INIT_LED_IR;
	calR->state = sCalStart;
	calR->int_cnt = 0;
	calR->cur255_cnt =0;
    if(low_vbat_flag == 1)
    {
		calR->green_idac = (hx3690l_spo2_agc_green_idac)>>1;
		calR->red_idac = (hx3690l_spo2_agc_red_idac*3)>>2;
		calR->ir_idac = (hx3690l_spo2_agc_ir_idac*3)>>2;
		calR->green_max_cur = (green_led_max_init)>>1;
		calR->red_max_cur = (red_led_max_init*3)>>2;
		calR->ir_max_cur = (ir_led_max_init*3)>>2;
    }
    else
    {
		calR->green_idac = hx3690l_spo2_agc_green_idac;
		calR->red_idac = hx3690l_spo2_agc_red_idac;
		calR->ir_idac = hx3690l_spo2_agc_ir_idac;
		calR->green_max_cur = green_led_max_init;
		calR->red_max_cur = red_led_max_init;
		calR->ir_max_cur = ir_led_max_init;
    }
}

void Restart_Spo2_PPG_Calibration_Routine(SPO2_CAL_SET_T *calR)
{
    calR->flag = CAL_FLG_LED_DAC;
    calR->state = sCalLed;
    calR->int_cnt = 0;
}

bool hx3690_spo2_check_unwear(int32_t infrared_data)
{
	if(calReg.IR_LEDDAC < spo2_ir_wear_offset)
    {
        spo2_wear_status = MSG_SPO2_NO_WEAR;
        if(spo2_wear_status_pre != MSG_SPO2_NO_WEAR)
        {
            spo2_wear_status_pre = MSG_SPO2_NO_WEAR;
            hx3690l_spo2_low_power();
			return true;
        } 
    }   
	return false;		
}

bool hx3690_spo2_check_wear(int32_t infrared_data)
{
    if(infrared_data > spo2_ir_wear_thres)
    {
        spo2_wear_status = MSG_SPO2_WEAR;
        if(spo2_wear_status_pre != MSG_SPO2_WEAR)
        {
            spo2_wear_status_pre = MSG_SPO2_WEAR;
            tyhx_spo2_alg_open_deep();
			hx3690l_spo2_set_mode(PPG_INIT);
			hx3690l_spo2_set_mode(CAL_INIT);
			return true;
        }  
    }
	return false;			
}


void PPG_Spo2_Calibration_Routine(SPO2_CAL_SET_T *calR, int32_t g_led, int32_t r_led, int32_t amb, int32_t ir_led)
{
    int32_t g_led_temp = 0, r_led_temp = 0, ir_led_temp = 0;

    switch(calR->state)
    {
		case sCalStart: 
			calR->state = sCalLedCur;
			break;
				
        case sCalLedCur:  
			if(spo2_wear_status == MSG_SPO2_NO_WEAR)
			{
				calR->state = sCalFinish;
				break;
			}     
            if(amb>1060000)
			{
				calR->AMBDAC = (amb-1060000)/2600;
			}
			else
			{
				calR->AMBDAC = 0;
			}
            if(r_led>amb+256)
            {
                calR->R_led_step = (r_led-amb)/SPO2_CAL_INIT_LED_RED; 
				r_led_temp = 2600*(hx3690l_spo2_agc_red_idac+calR->AMBDAC)/calR->R_led_step;
				if(r_led_temp>calR->red_max_cur)
				{
					r_led_temp = calR->red_max_cur;
				}
				calR->R_LED = r_led_temp;
				if(calR->R_LED != calR->red_max_cur)
				{
					calR->R_LEDDAC = (hx3690l_spo2_agc_red_idac*3)>>2;
				}
				else
				{
					calR->R_LEDDAC = hx3690l_spo2_agc_red_idac>>1;
				}
            } 
            if(ir_led>amb+256)
            {
                calR->IR_led_step = (ir_led-amb)/SPO2_CAL_INIT_LED_IR; 
				ir_led_temp = 2600*(hx3690l_spo2_agc_ir_idac+calR->AMBDAC)/calR->IR_led_step;
				if(ir_led_temp>calR->ir_max_cur)
				{
					ir_led_temp = calR->ir_max_cur;
				}
				calR->IR_LED = ir_led_temp;
				if(calR->IR_LED != calR->ir_max_cur)
				{
					calR->IR_LEDDAC = (hx3690l_spo2_agc_ir_idac*3)>>2;
				}
				else
				{
					calR->IR_LEDDAC = hx3690l_spo2_agc_ir_idac>>1;
				}
            }  
            if(g_led>amb+256)
            {
                calR->G_led_step = (g_led-amb)/SPO2_CAL_INIT_LED_GREEN; 
				g_led_temp = 2600*(hx3690l_spo2_agc_green_idac+calR->AMBDAC)/calR->G_led_step;
				if(g_led_temp>calR->green_max_cur)
				{
					g_led_temp = calR->green_max_cur;
				}
				calR->G_LED = g_led_temp;
				if(calR->G_LED != calR->green_max_cur)
				{
					calR->G_LEDDAC = (hx3690l_spo2_agc_green_idac*3)>>2;
				}
				else
				{
					calR->G_LEDDAC = hx3690l_spo2_agc_green_idac>>1;
				}
            }		
            calR->G_RF = 6;
            calR->R_RF = 6;
            calR->IR_RF = 6;
            calR->flag = CAL_FLG_LED_DR|CAL_FLG_LED_DAC|CAL_FLG_AMB_DAC|CAL_FLG_RF;						
            calR->state = sCalLed;
            break;
						
        case sCalLed:   
            if(r_led>1600000 && calR->R_LEDDAC < 110)
            {
               calR->R_LEDDAC = calR->R_LEDDAC+9;                                
            }

            if(ir_led>1600000 && calR->IR_LEDDAC < 110)
            {
               calR->IR_LEDDAC = calR->IR_LEDDAC+9;                                
            }
						
			if(g_led>1600000 && calR->G_LEDDAC < 110)
            {
               calR->G_LEDDAC = calR->G_LEDDAC+3;                                
            }

            if(ir_led<=1600000 && r_led<=1600000 && g_led<=1600000)
            {
              calR->state = sCalLed2;
            }
            else if(calR->R_LEDDAC>=110 || calR->IR_LEDDAC >= 110)
            {
              calR->state = sCalLed3;                
            }    
            else
            {
              calR->state = sCalLed;
            }  
            calR->flag = CAL_FLG_LED_DAC;
            break;
						
        case sCalLed2:   
            if(r_led>1300000 && calR->R_LEDDAC < 110)
            {
               calR->R_LEDDAC = calR->R_LEDDAC+3;                                
            }
            else if(r_led<800000 && calR->R_LEDDAC >= 3)
            {
               calR->R_LEDDAC = calR->R_LEDDAC-3;                                
            }
			else if(r_led<800000 && calR->R_LEDDAC > 0)
            {
               calR->R_LEDDAC = calR->R_LEDDAC-1;                                
            }
						
            if(ir_led>1300000 && calR->IR_LEDDAC < 110)
            {
               calR->IR_LEDDAC = calR->IR_LEDDAC+3;                                
            }
            else if(ir_led<800000 && calR->IR_LEDDAC >= 3)
            {
               calR->IR_LEDDAC = calR->IR_LEDDAC-3;                                
            }
			else if(ir_led<800000 && calR->IR_LEDDAC > 0)
            {
               calR->IR_LEDDAC = calR->IR_LEDDAC-1;                                
            }
						
			if(g_led>1300000 && calR->G_LEDDAC < 110)
            {
               calR->G_LEDDAC = calR->G_LEDDAC+3;                                
            }
            else if(g_led<800000 && calR->G_LEDDAC >= 3)
            {
               calR->G_LEDDAC = calR->G_LEDDAC-3;                                
            }
			else if(g_led<800000 && calR->G_LEDDAC > 0)
            {
               calR->G_LEDDAC = calR->G_LEDDAC-1;                                
            }
						
            if(ir_led<1300000 && ir_led>700000 && r_led<1300000 && r_led>700000 && g_led<1300000 && g_led>700000)
            {
              calR->state = sCalFinish;
            }
            else if(calR->R_LEDDAC >= 110 || calR->IR_LEDDAC >= 110 || calR->G_LEDDAC >= 110)
            {
              calR->state = sCalLed3;
            }
            else
            {
              calR->state = sCalLed2;               
            }               
            calR->flag = CAL_FLG_LED_DAC;
            break;
						
        case sCalLed3:   
            if(calR->R_LEDDAC>=110)
            {
              calR->R_LED = (calR->R_LED*3)>>2;
            }
            if(calR->IR_LEDDAC>=110)
            {
              calR->IR_LED = (calR->IR_LED*3)>>2;
            }
			if(calR->G_LEDDAC>=110)
            {
              calR->G_LED = (calR->G_LED*3)>>2;
            }
            calR->flag = CAL_FLG_LED_DR;
            calR->state = sCalLed;
            break;    
						
        default: 
            break;
        
    }
    AGC_LOG("AGC: R_cur=%d,G_cur=%d,Ir_cur=%d,ambDac=%d,R_Dac=%d,G_Dac=%d,Ir_Dac=%d,R_step=%d,G_step=%d,Ir_step=%d,state=%d\r\n",\
            calR->R_LED,calR->G_LED,calR->IR_LED,calR->AMBDAC,calR->R_LEDDAC,calR->G_LEDDAC,calR->IR_LEDDAC,\
            calR->R_led_step,calR->G_led_step,calR->IR_led_step,calR->state); 
}

SPO2_CAL_SET_T PPG_spo2_agc(void)
{
    int32_t r_led_val, amb_val;
    int32_t ir_led_val, green_led_val;
	
    calReg.work = false;
    if (!s_cal_state) 
    {
        return calReg;
    } 
		
	#ifdef INT_MODE     
    calReg.int_cnt ++;
    if(calReg.int_cnt < 8)
    {  
         return calReg;
    }
    calReg.int_cnt = 0;
	hx3690l_gpioint_cfg(false);
	#endif
		
	calReg.work = true;		
	hx3690l_spo2_read_data(agc_buf);
	amb_val = agc_buf[0];
	green_led_val = agc_buf[1];
	r_led_val = agc_buf[2];
	ir_led_val = agc_buf[3];
   
    AGC_LOG("cal dat ch1=%d,ch2=%d,ch3=%d,ch4=%d\r\n", agc_buf[0], agc_buf[1], agc_buf[2], agc_buf[3]);
    
    PPG_Spo2_Calibration_Routine(&calReg, green_led_val, r_led_val, amb_val, ir_led_val);
    
    if (calReg.state == sCalFinish) 
    { 
        hx3690l_spo2_set_mode(CAL_OFF);
        if(spo2_wear_status == MSG_SPO2_NO_WEAR)
        {
            hx3690l_spo2_low_power();
        }
		#if defined(TIMMER_MODE)
		#else
		hx3690l_gpioint_cfg(true);
		#endif
    }
    else 
    {
        hx3690l_spo2_updata_reg();
		#if defined(INT_MODE)
		hx3690l_gpioint_cfg(true);
		#endif
    }
    return  calReg;
}


void hx3690l_spo2_cal_init(void)
{
    uint16_t sample_rate = 200;                      /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;        /*period in clk num, num = Fclk/fs */
    uint8_t data_avg_num = 0;         /* 0 = 1 ; 1 = 2; 2 =4 ; 3 =8 ; 4 =16 ;*/
	
    hx3690l_write_reg(0X1a, (uint8_t)prf_clk_num);    // prf bit<7:0>
    hx3690l_write_reg(0X1b, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>
    hx3690l_write_reg(0X1c, (uint8_t)(prf_clk_num>>16)); // prf bit<23:16>
    hx3690l_write_reg(0X3c, data_avg_num<<4 | data_avg_num );
    hx3690l_write_reg(0X3d, data_avg_num<<4 | data_avg_num );
	
    hx3690l_write_reg(0x13,0x30); //FIFO bypass mode enable
    hx3690l_write_reg(0x23,0x20); //phase4 convertion ready enable
  
    #if defined(INT_MODE)
    hx3690l_write_reg(0x24,0x00);
    #endif

    hx3690l_write_reg(0x51,0x02); //Chip reset
    hx3690l_delay_ms(5); 
    hx3690l_write_reg(0x51,0x00); //Chip state machine work normal 
	hx3690l_delay_ms(5);		
}


         
void hx3690l_spo2_cal_off(void)
{
	uint16_t sample_rate = 25;       /*config the data rate of chip alps2_fm ,uint is Hz*/
	uint8_t data_avg_num = 0;         /* 0 = 1 ; 1 = 2; 2 =4 ; 3 =8 ; 4 =16 ;*/
	uint32_t prf_clk_num = 32000/sample_rate;        /*period in clk num, num = Fclk/fs */
	hx3690l_write_reg(0X1a, (uint8_t)prf_clk_num);    // prf bit<7:0>
	hx3690l_write_reg(0X1b, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>
	hx3690l_write_reg(0X1c, (uint8_t)(prf_clk_num>>16)); // prf bit<23:16>
	hx3690l_write_reg(0X3c, data_avg_num<<4 | data_avg_num );
	hx3690l_write_reg(0X3d, data_avg_num<<4 | data_avg_num );		
	hx3690l_write_reg(0x13,0x31); //FIFO mode enable
	hx3690l_write_reg(0x23,0x00); 
	
	#if defined(INT_MODE)
	hx3690l_write_reg(0x24,0x20); 
	#endif
  
    hx3690l_write_reg(0x51,0x02); //Chip reset
    hx3690l_delay_ms(5);          //Delay for reset time
    hx3690l_write_reg(0X13,0x30);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X13,0x31);
    hx3690l_write_reg(0x51,0x00); //Chip state machine work normal
	hx3690l_delay_ms(5); 
}

void hx3690l_spo2_read_data(int32_t *buf) 
{
	uint8_t  databuf1[6] = {0};
	uint8_t  databuf2[6] = {0};
	uint32_t P1 = 0,P2 = 0 ,P3 = 0 ,P4 =0 ;

    hx3690l_brust_read_reg(0x03, databuf1, 6); 
    hx3690l_brust_read_reg(0x09, databuf2, 6); 		
		
	P1 = ((databuf1[0])|(databuf1[1]<<8)|(databuf1[2]<<16)); 
	P3 = ((databuf1[3])|(databuf1[4]<<8)|(databuf1[5]<<16)); 
	P4 = ((databuf2[0])|(databuf2[1]<<8)|(databuf2[2]<<16));	
	P2 = ((databuf2[3])|(databuf2[4]<<8)|(databuf2[5]<<16));
 
	buf[0] = P1;
	buf[1] = P2;
	buf[2] = P3;
	buf[3] = P4; 
}

void hx3690l_spo2_low_power(void)
{   
    uint16_t sample_rate = 10;                        /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint8_t data_avg_num = 0;         				  /* 0 = 1 ; 1 = 2; 2 =4 ; 3 =8 ; 4 =16 ;*/
    uint32_t prf_clk_num = 32000/sample_rate;         /*period in clk num, num = Fclk/fs */

    uint8_t phase1_led_en = 0;     
    uint8_t phase2_led_en = 0;
    uint8_t phase3_led_en = 0;
    uint8_t phase4_led_en = 1;

    uint8_t phase1_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	uint8_t phase2_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	uint8_t phase3_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	uint8_t phase4_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	
	uint8_t phase1_ldr_cur = 0; 
	uint8_t phase2_ldr_cur = 0;
	uint8_t phase3_ldr_cur = 0;
	uint8_t phase4_ldr_cur = 64;
	
	uint8_t phase1_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
	uint8_t phase2_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
	uint8_t phase3_tia_res = 0; 
	uint8_t phase4_tia_res = 0; 
    
    uint8_t led_on_time = 1;        /* 0 = 32clk=8us ; 1 = 64clk=16us; 2=128clk=32us ; 3 = 256clk=64us ;
                                       4 = 512clk=128us ; 5 = 1024clk=256us; 6= 2048clk=512us; 7 = 4096clk=1024us */
    hx3690l_write_reg(0X6a, 0X00);	//rest int
    hx3690l_delay_ms(10);
    hx3690l_write_reg(0X1a, (uint8_t)prf_clk_num);    // prf bit<7:0>
    hx3690l_write_reg(0X1b, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>
    hx3690l_write_reg(0X1c, (uint8_t)(prf_clk_num>>16)); // prf bit<23:16>
    hx3690l_write_reg(0X3c, data_avg_num<<4 | data_avg_num );
    hx3690l_write_reg(0X3d, data_avg_num<<4 | data_avg_num );	
		
    hx3690l_write_reg(0X2c, phase1_tia_res);
    hx3690l_write_reg(0X2d, phase3_tia_res);
    hx3690l_write_reg(0X2e, phase4_tia_res);
    hx3690l_write_reg(0X2f, phase2_tia_res);
    hx3690l_write_reg(0X30, phase1_ldr_cur);
    hx3690l_write_reg(0X31, phase3_ldr_cur);
    hx3690l_write_reg(0X32, phase4_ldr_cur);
    hx3690l_write_reg(0X33, phase2_ldr_cur);
	hx3690l_write_reg(0X38, phase1_offset_idac);
    hx3690l_write_reg(0X39, phase3_offset_idac);
    hx3690l_write_reg(0X3a, phase4_offset_idac);
    hx3690l_write_reg(0X3b, phase2_offset_idac);   
    hx3690l_write_reg(0X1f, (led_on_time<<4| phase1_led_en<<3 | phase3_led_en<<2 | phase4_led_en<<1 | phase2_led_en) );
    hx3690l_write_reg(0X6a, 0X02);
	hx3690l_write_reg(0X69, 0xa2);	//bit<0>: rc_comb_en bits<1>=rc_rbp_en bits<7>= vcom_clamp_en bits<6:4>= LED_vdesl
		
	#ifdef INT_MODE
	hx3690l_write_reg(0X12,0x14); //fifo almostfull cfg ,max=0x40;
	hx3690l_write_reg(0x13,0x31); //FIFO bypass mode enable
	hx3690l_write_reg(0x23,0x00); //phase convertion ready disable
	#else
	hx3690l_write_reg(0x13,0x31); //FIFO bypass mode enable
	hx3690l_write_reg(0x23,0x00); //phase convertion ready disable
	#endif
		
    hx3690l_write_reg(0x51,0x02); //Chip reset
    hx3690l_delay_ms(5);          //Delay for reset time
    hx3690l_write_reg(0X13, 0x30);
    hx3690l_delay_ms(5);
	hx3690l_write_reg(0X13, 0x31);
	hx3690l_write_reg(0x51,0x00); //Chip state machine work normal
	hx3690l_delay_ms(5);

	calReg.G_LED = phase2_ldr_cur;
	calReg.R_LED =  phase3_ldr_cur;
	calReg.IR_LED =  phase4_ldr_cur;
	calReg.G_LEDDAC = phase2_offset_idac;
	calReg.R_LEDDAC = phase3_offset_idac;
	calReg.IR_LEDDAC = phase4_offset_idac;	

    AGC_LOG(" chip go to low power mode  \r\n" );   
}

void hx3690l_spo2_updata_reg(void)
{
    if (calReg.flag & CAL_FLG_LED_DR) 
    {
		hx3690l_write_reg(0X31, calReg.R_LED);
		hx3690l_write_reg(0X32, calReg.IR_LED);
		hx3690l_write_reg(0X33, calReg.G_LED);
    }
    
    if (calReg.flag & CAL_FLG_LED_DAC) 
    {
		hx3690l_write_reg(0X39, calReg.R_LEDDAC);
		hx3690l_write_reg(0X3a, calReg.IR_LEDDAC);
		hx3690l_write_reg(0X3b, calReg.G_LEDDAC);
    }
    
    if (calReg.flag & CAL_FLG_AMB_DAC) 
    {
        hx3690l_write_reg(0X38, calReg.AMBDAC);
    }
    
    if (calReg.flag & CAL_FLG_RF) 
    {
        hx3690l_write_reg(0X2c, calReg.R_RF);    // phase1 tia feed back resister cfg
        hx3690l_write_reg(0X2d, calReg.R_RF);    // phase3 tia feed back resister cfg
        hx3690l_write_reg(0X2e, calReg.IR_RF);   // phase4 tia feed back resister cfg
        hx3690l_write_reg(0X2f, calReg.G_RF);    // phase2 tia feed back resister cfg
    }
}
void hx3690l_spo2_set_mode(uint8_t mode_cmd)
{
    switch (mode_cmd) 
    {
        case PPG_INIT:
            hx3690l_spo2_ppg_init();
            #if defined(TIMMER_MODE)
            hx3690l_320ms_timer_cfg(true);
                #if defined(GSEN_40MS_TIMMER)
                hx3690l_40ms_timer_cfg(true);
                #endif
            #else
            hx3690l_gpioint_cfg(true);
            #endif
            s_ppg_state = 1;
            AGC_LOG("ppg init mode\r\n");
            break;

        case PPG_OFF:
            hx3690l_ppg_off();
            s_ppg_state = 0;
            AGC_LOG("ppg off mode\r\n");
            break;
				
        case PPG_LED_OFF:
            hx3690l_spo2_low_power();
            s_ppg_state = 0;
            AGC_LOG("ppg led off mode\r\n");
            break;

        case CAL_INIT:
            Init_Spo2_PPG_Calibration_Routine(&calReg,64);
            hx3690l_spo2_cal_init();
            hx3690l_spo2_updata_reg();
            #if defined(TIMMER_MODE)
                #if defined(GSEN_40MS_TIMMER)
                #else
                hx3690l_40ms_timer_cfg(true);
                #endif
            #endif        
            s_cal_state = 1;
            AGC_LOG("cal init mode\r\n"); 
            break;       
				
        case RECAL_INIT:
            Restart_Spo2_PPG_Calibration_Routine(&calReg);
            hx3690l_spo2_cal_init();
            hx3690l_spo2_updata_reg();
            #if defined(TIMMER_MODE)
                #if defined(GSEN_40MS_TIMMER)
                #else
                hx3690l_40ms_timer_cfg(true);
                #endif
            #endif 
            s_cal_state = 1;
            AGC_LOG("recal init mode\r\n");
            break;

        case CAL_OFF:
            #if defined(TIMMER_MODE)
                #if defined(GSEN_40MS_TIMMER)
                #else
                hx3690l_40ms_timer_cfg(false);
                #endif
            #endif
            hx3690l_spo2_cal_off();
            s_cal_state = 0;
            AGC_LOG("cal off mode\r\n");
            break;

        default:
            break;
    }
}

SENSOR_ERROR_T hx3690l_spo2_enable(void)
{
    if (!hx3690l_chip_check()) 
    {
        AGC_LOG("hx3690l check id failed!\r\n");
        return SENSOR_OP_FAILED;
    }

    AGC_LOG("hx3690l check id success!\r\n");

    if (s_ppg_state)
    {
        AGC_LOG("ppg already on!\r\n");
        return SENSOR_OP_FAILED;
    }

    if(!tyhx_spo2_alg_open())
    {
        AGC_LOG("spo2 alg open fail,or dynamic ram not enough!\r\n");
        return SENSOR_OP_FAILED;
    }
    spo2_wear_status = MSG_SPO2_NO_WEAR;
    spo2_wear_status_pre = MSG_SPO2_NO_WEAR;
    hx3690l_spo2_set_mode(PPG_INIT);

    AGC_LOG("hx3690l enable!\r\n");

    return SENSOR_OK;
}

SENSOR_ERROR_T hx3690l_hrspo2_enable(void)
{
    if (!hx3690l_chip_check()) 
    {
        AGC_LOG("hx3690l check id failed!\r\n");
        return SENSOR_OP_FAILED;
    }

    AGC_LOG("hx3690l check id success!\r\n");

    if (s_ppg_state)
    {
        AGC_LOG("ppg already on!\r\n");
        return SENSOR_OP_FAILED;
    }

    if(!tyhx_spo2_alg_open())
    {
        AGC_LOG("spo2 alg open fail,or dynamic ram not enough!\r\n");
        return SENSOR_OP_FAILED;
    }
		if(!tyhx_hrs_alg_open())
    {
        AGC_LOG("spo2 alg open fail,or dynamic ram not enough!\r\n");
        return SENSOR_OP_FAILED;
    }
    spo2_wear_status = MSG_SPO2_NO_WEAR;
    spo2_wear_status_pre = MSG_SPO2_NO_WEAR;
    hx3690l_spo2_set_mode(PPG_INIT);

    AGC_LOG("hx3690l enable!\r\n");

    return SENSOR_OK;
}

void hx3690l_spo2_disable(void)
{
	#if defined(TIMMER_MODE)
	hx3690l_320ms_timer_cfg(false);
	hx3690l_40ms_timer_cfg(false);
	#elif defined(INT_MODE)
	hx3690l_gpioint_cfg(false);
	#endif	
	hx3690l_spo2_set_mode(PPG_OFF);
	s_ppg_state = 0;
	s_cal_state = 0;
	tyhx_spo2_alg_close();    

    AGC_LOG("hx3690l disable!\r\n");
}

void hx3690l_spo2_data_reset(void)
{
	s_ppg_state = 0;
	s_cal_state = 0;
	tyhx_spo2_alg_close();
}

spo2_wear_msg_code_t hx3690_spo2_get_wear_status(void)
{
    return  spo2_wear_status;
}

SPO2_CAL_SET_T get_spo2_agc_status(void)
{
	SPO2_CAL_SET_T cal = {0};

	cal.flag = calReg.flag;
	cal.int_cnt = calReg.int_cnt;
	cal.G_LED = calReg.G_LED;
	cal.R_LED = calReg.R_LED;     
	cal.IR_LED = calReg.IR_LED;     
	cal.R_LEDDAC = calReg.R_LEDDAC; 
	cal.IR_LEDDAC = calReg.IR_LEDDAC; 
	cal.G_LEDDAC = calReg.G_LEDDAC; 
	cal.AMBDAC = calReg.AMBDAC;  
	cal.G_RF = calReg.G_RF;
	cal.R_RF = calReg.R_RF;     
	cal.IR_RF = calReg.IR_RF;  
	cal.G_led_step = calReg.G_led_step;
	cal.R_led_step = calReg.R_led_step;
	cal.IR_led_step = calReg.IR_led_step;
	cal.state = calReg.state;
	cal.green_idac = calReg.green_idac;
	cal.red_idac = calReg.red_idac;
	cal.ir_idac = calReg.ir_idac;

    return cal;
}

void hx3690l_spo2_read_fifo_data(uint8_t read_fifo_size,int32_t *buf)
{
	uint8_t data_flg = 127;
	int32_t data;
	uint8_t databuf[3];
	uint8_t ii=0;
	uint8_t data_size_all = 0;
	data_size_all = hx3690l_read_fifo_size();
    for(ii=0; ii<read_fifo_size; ii++) 
    {        
        hx3690l_write_reg(0x17, 0x00); // write any walue to 0x17 will update a new data
        hx3690l_delay_us(100);
        
        databuf[2]=hx3690l_read_reg(0x17);
        databuf[1]=hx3690l_read_reg(0x16);
        databuf[0]=hx3690l_read_reg(0x15);
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
		
    if(data_size_all==64)
    {
        hx3690l_write_reg(0X13, 0x30);
        hx3690l_delay_ms(5);
        hx3690l_write_reg(0X13, 0x31);
        hx3690l_delay_ms(5);
    }
}

uint8_t hx3690l_spo2_read(ppg_sensor_data_t * s_dat)
{
    uint8_t i = 0;
	int32_t Red_src_data = 0;
	int32_t Ir_src_data = 0;
	int32_t Green_src_data = 0;
	bool recal = false;
	uint8_t size = 0;
    uint8_t fifo_out_size = 0;
	int32_t *red_buf = &(s_dat->red_data[0]);
	int32_t *green_buf = &(s_dat->green_data[0]);
	int32_t *ir_buf = &(s_dat->ir_data[0]); 
	int32_t *s_buf = &(s_dat->s_buf[0]);  
	s_dat->green_cur = calReg.G_LED;	
	s_dat->red_cur = calReg.R_LED;
	s_dat->ir_cur = calReg.IR_LED;

    //AGC_LOG("s_ppg_state: %d,s_cal_state: %d\r\n",s_ppg_state,s_cal_state);
    if (!s_ppg_state || s_cal_state) 
    {
        return 0;
    }    

    size = hx3690l_read_fifo_size();
    //AGC_LOG("ppg data size: %d\r\n", size_byte);
    if(size<4)
    {
        return 0;
    }
		
    if (size && size <= 64) 
    {
        fifo_out_size = hx3690l_read_fifo_data(s_buf,size,4);
        s_dat->count = fifo_out_size;
        for (i=0; i<fifo_out_size; i++) 
        {
            Green_src_data = s_buf[i*4+1] - s_buf[i*4];
			Red_src_data = s_buf[i*4+2] - s_buf[i*4];
            Ir_src_data = s_buf[i*4+3] - s_buf[i*4];

            if ((s_buf[i*4+1]<500000 || s_buf[i*4+1]>1500000) \
                ||(s_buf[i*4+2]<500000 || s_buf[i*4+2]>1500000) \
                ||(s_buf[i*4+3]<500000 || s_buf[i*4+3]>1500000))
            {
                recal = true;               
                if(spo2_wear_status==MSG_SPO2_NO_WEAR)
                {
                     recal = false;
                }
            }            

            red_buf[i] = Red_src_data;
			green_buf[i] = Green_src_data;
            ir_buf[i] = Ir_src_data;
						
            //DEBUG_PRINTF("G_LEDDAC: %d, R_LEDDAC: %d, IR_LEDDAC: %d\r\n", calReg.G_LEDDAC,calReg.R_LEDDAC,calReg.IR_LEDDAC);
            DEBUG_PRINTF("%d/%d %d %d %d %d\r\n" ,1+i,size,s_buf[i*4],s_buf[i*4+1],s_buf[i*4+2],s_buf[i*4+3]);
						
			if(spo2_wear_status==MSG_SPO2_WEAR)
			{
                if(hx3690_spo2_check_unwear(Ir_src_data))
                {
                    return 0;
                }
			}
			else
			{
                if(hx3690_spo2_check_wear(Ir_src_data))
                {
                    return 0;
                }
			}
        }

        if (recal) 
        {
            cal_delay--;
            if (cal_delay <= 0) 
            {
                cal_delay = SPO2_CAL_DELAY_COUNT;
                hx3690l_spo2_set_mode(RECAL_INIT);
            }
        } 
        else 
        {
            cal_delay = SPO2_CAL_DELAY_COUNT;
        }  
    }
    return 1;
}

#endif
