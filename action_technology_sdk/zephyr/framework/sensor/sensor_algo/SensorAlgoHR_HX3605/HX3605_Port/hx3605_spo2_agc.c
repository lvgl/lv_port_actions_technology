
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>


#include "hx3605.h"
#include "hx3605_spo2_agc.h"
#include "tyhx_spo2_alg.h"

#ifdef TYHX_DEMO
#include "twi_master.h"
#include "SEGGER_RTT.h"
#endif

#ifdef SPO2_ALG_LIB

extern const uint8_t  hx3605_spo2_agc_red_idac;  // 6,7,8...
extern const uint8_t  hx3605_spo2_agc_ir_idac;  // 6,7,8...
extern const uint8_t  red_led_max_init;
extern const uint8_t  ir_led_max_init;
extern uint8_t low_vbat_flag;

//SPO2_INFRARED_THRES
extern const uint8_t  spo2_check_unwear_oft; 
extern const int32_t  spo2_ir_wear_thres; 
extern uint8_t read_fifo_first_flg;


static uint8_t s_ppg_state = 0;
static uint8_t s_cal_state = 0;
static int32_t agc_buf[4] = {0};

static uint8_t cal_delay = CAL_DELAY_COUNT;
static SPO2_CAL_SET_T  calReg;


static hx3605_spo2_wear_msg_code_t spo2_wear_status = MSG_SPO2_INIT;
static hx3605_spo2_wear_msg_code_t spo2_wear_status_pre = MSG_SPO2_INIT;

void hx3605_spo2_ppg_init(void) //20200615 ericy ppg fs=25hz, phase3 conversion ready interupt en
{
   uint16_t sample_rate = 25;                  /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;   /*period in clk num, num = Fclk/fs */
    uint8_t ps0_enable=1;       /*ps0_enable  , 1 mean enable ; 0 mean disable */
    uint8_t ps1_enable=1;       /*ps1_enable  , 1 mean enable ; 0 mean disable */   
    uint8_t ps0_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    uint8_t ps1_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    
    uint8_t ps1_cp_avg_num_sel= 1;
		uint8_t ps0_cp_avg_num_sel= 2;
		uint8_t ps1_avg_num_sel_i2c=0;
		uint8_t ps0_avg_num_sel_i2c=0;
     /***********led open enable***********/
    uint8_t dccancel_ps0_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps0_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    
    uint8_t ir_pden_ps0 =0;     //IR_PDEN_PS0
 //   uint8_t ext_pden_ps0 =0;    ///EXT_PDEN_PS0
    uint8_t pddrive_ps0 =0;     //PDDRIVE_PS0  0-63
	
	  uint8_t ir_pden_ps1 =0;      //IR_PDEN_PS1
//    uint8_t ext_pden_ps1 =0;     //EXT_PDEN_PS1
    uint8_t pddrive_ps1 =5;     //PDDRIVE_PS1  0-63
    uint8_t ps1_interval_i2c =0;    // config 0/5  
    
    uint8_t led_en_begin =1;       // 0 = 2 ; 1 = 4 ; 2 = 8 ; 3 = 16 ;
    uint8_t afe_reset = 1;        //* 0 = 32clk ; 1 = 64clk ; 2 = 128clk ; 3 = 256clk(d) ;
    uint8_t en2rst_delay =2;
		uint8_t init_wait_delay = 2; /* 0 = 31clk ; 1 = 64clk ; 2 = 127clk ; 3 = 255clk(d) ;
                                     4 = 511clk; 5 = 1023clk; 6 = 2047; 7 = 2048clk */   
//		uint16_t thres_level =0;
    uint8_t thres_int =1;    //thres int enable
    uint8_t data_rdy_int =8;    //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1
    
    uint8_t ldrsel_ps1 =IR_LED_SLE;      //ps1 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
    uint8_t ldrsel_ps0 =RED_LED_SLE;      //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
    /***********cap *********/
    uint8_t intcapsel_ps1 =5;   //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf 
    uint8_t intcapsel_ps0 =5;   //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf 
    
    uint8_t led_on_time  = 5;      /* 0 = 32clk2=8us ; 8 = 64clk=16us; 16=128clk=32us ; 32 = 256clk=64us ;
                                     64 = 512clk=128us ; 128= 1024clk=256us; 256= 2048clk=512us*/
    uint8_t force_adc_clk_sel=0;
    uint8_t force_adc_clk_cfg =0;
    uint8_t force_PEN =0;     //phase enable
    uint8_t force_PEN_cfg =0;
    uint8_t force_LED_EN =0;
    uint8_t force_LED_EN_cfg =0;         
    uint8_t force_CKAFEINT_sel =0; 
    uint8_t force_CKAFEINT_cfg =0;
    
    uint8_t PDBIASEN =0;
    uint8_t PDILOADEN =0;
//		uint8_t databuf[3] = {0};

    hx3605_write_reg(0X10, (ps1_enable<<5| ps0_enable<<4|ps1_osr<<2|ps0_osr));	//default 0x00
    hx3605_write_reg(0X11, (uint8_t)prf_clk_num);    // prf bit<7:0>6   default 0x00
    hx3605_write_reg(0X12, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>  default 0x03
    hx3605_write_reg(0X13, (ps1_interval_i2c));   //default 0x00
    hx3605_write_reg(0X14, (led_en_begin<<6|afe_reset<<4|en2rst_delay<<2|init_wait_delay));  //default 0x00   
    hx3605_write_reg(0X15, led_on_time); // led_en_num*8     //default 0x00
    hx3605_write_reg(0X16, (ps1_cp_avg_num_sel<<4)|(ps0_cp_avg_num_sel));  //default 0x00
    hx3605_write_reg(0X17, (ps1_avg_num_sel_i2c<<2)|(ps0_avg_num_sel_i2c));   //default 0x00 
    
    hx3605_write_reg(0X18, (uint8_t)dccancel_ps0_data1);     //default 0x00
    hx3605_write_reg(0X19, (uint8_t)(dccancel_ps0_data1>>8));   //default 0x00
    hx3605_write_reg(0X1a, (uint8_t)dccancel_ps0_data2);   //default 0x00
    hx3605_write_reg(0X1b, (uint8_t)(dccancel_ps0_data2>>8));  //default 0x00
    hx3605_write_reg(0X1c, (uint8_t)dccancel_ps1_data1);   //default 0x00
    hx3605_write_reg(0X1d, (uint8_t)(dccancel_ps1_data1>>8));   //default 0x00
    hx3605_write_reg(0X1e, (uint8_t)dccancel_ps1_data2 );   //default 0x00
    hx3605_write_reg(0X1f, (uint8_t)(dccancel_ps1_data2 >>8));   //default 0x00
    
    hx3605_write_reg(0X20, (ir_pden_ps0<<7|ir_pden_ps0<<6|pddrive_ps0));  //default 0x00
    hx3605_write_reg(0X21, (ir_pden_ps1<<7|ir_pden_ps1<<6|pddrive_ps1));  //default 0x00
    hx3605_write_reg(0X22, (ldrsel_ps1<<4|ldrsel_ps0));      //default 0x00
    hx3605_write_reg(0X23, (intcapsel_ps1<<4|intcapsel_ps0));   //default 0x00
    hx3605_write_reg(0X26, (thres_int<<4|data_rdy_int));   //default 0x0f
    
    //    hx3605_write_reg(0X34, 0X10);  //w_almost_full ENABLE   /default0x04
    
    hx3605_write_reg(0X69, (force_adc_clk_sel<<7|force_adc_clk_cfg<<6|force_PEN<<5|force_PEN_cfg<<4|force_LED_EN<<3|
                             force_LED_EN_cfg<<2|force_CKAFEINT_sel<<1|force_CKAFEINT_cfg));   //default 0x00
    hx3605_write_reg(0Xc2, 0x0b);                         
    hx3605_write_reg(0Xc3, (PDBIASEN<<5|PDILOADEN<<4));   //default 0x27
#ifdef INT_FLOATING
		hx3605_write_reg(0xc0, 0x08);
#endif
     
#if defined(TIMMER_MODE)
        hx3605_write_reg(0x2d,0x83);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
				hx3605_write_reg(0x2e,0x20);			 //watermark
				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf int,04=enable almost full int
#else ///////////INT Mode
				hx3605_write_reg(0x2d,0x00);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf ,04=enable almost full
#endif
        
    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
    hx3605_delay(5);
    
    calReg.red_idac = 0;
    calReg.ir_idac = 0;
    
		read_fifo_first_flg = 1;
}

void Init_Spo2_PPG_Calibration_Routine(SPO2_CAL_SET_T *calR)
{
    calR->flag = CAL_FLG_LED_DR|CAL_FLG_LED_DAC|CAL_FLG_AMB_DAC|CAL_FLG_RF;
    calR->red_idac = 0;   
    calR->ir_idac = 0;   
    calR->amb_idac = 0; 
    calR->ontime = 5; 
    calR->red_cur = SPO2_CAL_INIT_LED_RED;
    calR->ir_cur = SPO2_CAL_INIT_LED_IR;
    calR->state = sCalStart;
    calR->int_cnt = 0;
    calR->cur255_cnt =0;
    if(low_vbat_flag == 1)
    {
        calR->red_max_cur = (red_led_max_init*3)>>2;
        calR->ir_max_cur = (ir_led_max_init*3)>>2;
    }
    else
    {
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

bool hx3605_spo2_check_unwear(void)
{
		if(calReg.ir_idac < spo2_check_unwear_oft)
    {
        spo2_wear_status = MSG_SPO2_NO_WEAR;
        if(spo2_wear_status_pre != MSG_SPO2_NO_WEAR)
        {
            spo2_wear_status_pre = MSG_SPO2_NO_WEAR;
            hx3605_spo2_low_power();
						return true;
        } 
    }    
    return false;
}

bool hx3605_spo2_check_wear(int32_t infrared_data)
{
    if(infrared_data > spo2_ir_wear_thres)
    {
        spo2_wear_status = MSG_SPO2_WEAR;
        if(spo2_wear_status_pre != MSG_SPO2_WEAR)
        {
						//AGC_LOG("hx3605_spo2_check_wear ir = %d\r\n", infrared_data);
            spo2_wear_status_pre = MSG_SPO2_WEAR;
            tyhx_spo2_alg_open_deep();
            hx3605_spo2_set_mode(PPG_INIT);
            hx3605_spo2_set_mode(CAL_INIT);
            return true;
        }  
    }   
    return false;
}


void PPG_Spo2_Calibration_Routine(SPO2_CAL_SET_T *calR, int32_t r_led, int32_t amb, int32_t ir_led)
{
    int32_t r_led_temp = 0, ir_led_temp = 0;
    AGC_LOG("AGC_before: red_cur=%d,ir_cur=%d,amb_idac=%d,red_idac=%d,ir_idac=%d,red_step=%d,ir_step=%d,state=%d\r\n",\
            calR->red_cur,calR->ir_cur,calR->amb_idac,calR->red_idac,calR->ir_idac,calR->red_led_step,calR->ir_led_step,calR->state);
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
            if(amb>270000)
						{
							calR->amb_idac = (amb-270000)/2800;
						}
						else
						{
							calR->amb_idac = 0;
						}
            if(r_led>amb+128)
            {
                calR->red_led_step = (r_led-amb)/SPO2_CAL_INIT_LED_RED; 
								r_led_temp = 4000*(hx3605_spo2_agc_red_idac+calR->amb_idac)/calR->red_led_step;
								if(r_led_temp>calR->red_max_cur)
								{
									r_led_temp = calR->red_max_cur;
								}
								calR->red_cur = r_led_temp;
								if(calR->red_cur != calR->red_max_cur)
								{
									calR->red_idac = (hx3605_spo2_agc_red_idac*3)>>2;
								}
								else
								{
									calR->red_idac = hx3605_spo2_agc_red_idac>>1;
								}
            } 
            if(ir_led>amb+128)
            {
                calR->ir_led_step = (ir_led-amb)/SPO2_CAL_INIT_LED_IR; 
								ir_led_temp = 4000*(hx3605_spo2_agc_ir_idac+calR->amb_idac)/calR->ir_led_step;
								if(ir_led_temp>calR->ir_max_cur)
								{
									ir_led_temp = calR->ir_max_cur;
								}
								calR->ir_cur = ir_led_temp;
								if(calR->ir_cur != calR->ir_max_cur)
								{
									calR->ir_idac = (hx3605_spo2_agc_ir_idac*3)>>2;
								}
								else
								{
									calR->ir_idac = hx3605_spo2_agc_ir_idac>>1;
								}
            }  	
						calR->ontime = 64;
						calR->flag = CAL_FLG_LED_DR|CAL_FLG_LED_DAC|CAL_FLG_AMB_DAC|CAL_FLG_RF;						
            calR->state = sCalLed;
            break;
						
        case sCalLed:   
            if(r_led>450000 && calR->red_idac < 200)
            {
               calR->red_idac = calR->red_idac+6;                                
            }

            if(ir_led>450000 && calR->ir_idac < 200)
            {
               calR->ir_idac = calR->ir_idac+6;                                
            }

            if(ir_led<=450000 && r_led<=450000)
            {
              calR->state = sCalLed2;
            }  
						else if(calR->ir_idac >= 200 || calR->red_idac >= 200)				
						{
							calR->state = sCalFinish;
						}							
            else
            {
              calR->state = sCalLed;
            }  
            calR->flag = CAL_FLG_LED_DAC;
            break;
						
        case sCalLed2:   
            if(r_led>320000 && calR->red_idac < 200)
            {
               calR->red_idac = calR->red_idac+2;                                
            }
						else if(r_led<200000 && calR->red_idac > 1)
            {
               calR->red_idac = calR->red_idac-2;                                
            }
						else if(r_led<200000 && calR->red_idac > 0)
            {
               calR->red_idac = calR->red_idac-1;                                
            }
						
            if(ir_led>320000 && calR->ir_idac < 200)
            {
               calR->ir_idac = calR->ir_idac+2;                                
            }
						else if(ir_led<200000 && calR->ir_idac > 1)
            {
               calR->ir_idac = calR->ir_idac-2;                                
            }
						else if(ir_led<200000 && calR->ir_idac > 0)
            {
               calR->ir_idac = calR->ir_idac-1;                                
            }
						
            if(ir_led<320000 && ir_led>200000 && r_led<320000 && r_led>200000)
            {
              calR->state = sCalFinish;
            }
						else if(calR->ir_idac >= 200 || calR->red_idac >= 200)
            {
              calR->state = sCalFinish;
            }
            else
            {
              calR->state = sCalLed2;               
            }               
            calR->flag = CAL_FLG_LED_DAC;
            break;
				
        default: 
            break;
        
    }
    AGC_LOG("AGC_after: red_cur=%d,ir_cur=%d,amb_idac=%d,red_idac=%d,ir_idac=%d,red_step=%d,ir_step=%d,state=%d\r\n",\
            calR->red_cur,calR->ir_cur,calR->amb_idac,calR->red_idac,calR->ir_idac,calR->red_led_step,calR->ir_led_step,calR->state); 
}

SPO2_CAL_SET_T PPG_spo2_agc(void)
{
    int32_t r_led_val, amb_val;
    int32_t ir_led_val;
	
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
		hx3605_gpioint_cfg(false);
		#endif
		
    calReg.work = true;		
    read_data_packet(agc_buf);
		amb_val = agc_buf[1];
    r_led_val = agc_buf[0];
    ir_led_val = agc_buf[2];
   
    AGC_LOG("cal dat amb=%d,r=%d,ir=%d\r\n", amb_val, r_led_val, ir_led_val);
    
    PPG_Spo2_Calibration_Routine(&calReg, r_led_val, amb_val, ir_led_val);
    
    if (calReg.state == sCalFinish) 
    { 
        hx3605_spo2_set_mode(CAL_OFF);
        if(spo2_wear_status == MSG_SPO2_NO_WEAR)
        {
            hx3605_spo2_low_power();
        }
				#if defined(TIMMER_MODE)
				#else
				hx3605_gpioint_cfg(true);
				#endif
    }
    else 
    {
        hx3605_spo2_updata_reg();
				#if defined(INT_MODE)
				hx3605_gpioint_cfg(true);
				#endif
    }
    return  calReg;
}


void hx3605_spo2_cal_init(void) // 20200615 ericy afe cali offline
{
    uint16_t sample_rate = 200;                      /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;        /*period in clk num, num = Fclk/fs */
	  uint8_t ps1_cp_avg_num_sel= 0;
		uint8_t ps0_cp_avg_num_sel= 0;
		uint8_t thres_int =0;    //thres int enable
		uint8_t data_rdy_int =8;    //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1
	
    hx3605_write_reg(0X11, (uint8_t)prf_clk_num);    // prf bit<7:0>6   default 0x00
    hx3605_write_reg(0X12, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>  default 0x03
		hx3605_write_reg(0X16, (ps1_cp_avg_num_sel<<4)|(ps0_cp_avg_num_sel));  //default 0x00
	
		hx3605_write_reg(0x2d,0x00);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
		hx3605_write_reg(0X26, (thres_int<<4|data_rdy_int));   //default 0x0f
  
    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
		hx3605_delay(5);             //Delay for reset time		
}


         
void hx3605_spo2_cal_off(void) // 20200615 ericy afe cali offline
{
    uint16_t sample_rate = 25;                      /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;        /*period in clk num, num = Fclk/fs */
	  uint8_t ps1_cp_avg_num_sel= 1;
		uint8_t ps0_cp_avg_num_sel= 2;
		uint8_t thres_int =0;    //thres int enable
		uint8_t data_rdy_int =0;    //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1
//		uint8_t databuf[3] = {0};
	
	  hx3605_write_reg(0X11, (uint8_t)prf_clk_num);    // prf bit<7:0>6   default 0x00
    hx3605_write_reg(0X12, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>  default 0x03
		hx3605_write_reg(0X16, (ps1_cp_avg_num_sel<<4)|(ps0_cp_avg_num_sel));  //default 0x00
	
		hx3605_write_reg(0x2d,0x83);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
		hx3605_write_reg(0X26, (thres_int<<4|data_rdy_int));   //default 0x0f

    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
		hx3605_delay(5);
		read_fifo_first_flg = 1;
}

void hx3605_spo2_read_data(int32_t *buf) // 20200615 ericy read reg_data phase1 and phase3 
{
    uint8_t  databuf1[6] = {0};
		uint8_t  databuf2[6] = {0};
    uint32_t P1 = 0,P2 = 0 ,P3 = 0 ,P4 =0 ;

    hx3605_brust_read_reg(0x03, databuf1, 6); 
    hx3605_brust_read_reg(0x09, databuf2, 6); 		
		
    P1 = ((databuf1[0])|(databuf1[1]<<8)|(databuf1[2]<<16)); 
		P3 = ((databuf1[3])|(databuf1[4]<<8)|(databuf1[5]<<16)); 
		P4 = ((databuf2[0])|(databuf2[1]<<8)|(databuf2[2]<<16));	
    P2 = ((databuf2[3])|(databuf2[4]<<8)|(databuf2[5]<<16));
 
    buf[0] = P1;
		buf[1] = P2;
		buf[2] = P3;
		buf[3] = P4; 
}

void hx3605_spo2_low_power(void)
{   
    uint16_t sample_rate = 10;                      /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;        /*period in clk num, num = Fclk/fs */
	  uint8_t ps1_cp_avg_num_sel= 0;
		uint8_t ps0_cp_avg_num_sel= 0;
		uint8_t thres_int =0;    //thres int enable
		uint8_t data_rdy_int =0;    //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1	
		uint8_t led_on_time = 5;
		uint8_t ldrsel_ps1 =IR_LED_SLE;      //ps1 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
    uint8_t ldrsel_ps0 =0;      //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
	
	  uint8_t dccancel_ps0_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps0_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t ir_pden_ps0 =0;     //IR_PDEN_PS0
//    uint8_t ext_pden_ps0 =0;    //EXT_PDEN_PS0
    uint8_t pddrive_ps0 =0;     //PDDRIVE_PS0  0-63
	  uint8_t ir_pden_ps1 =0;     //IR_PDEN_PS0
//    uint8_t ext_pden_ps1 =0;    //EXT_PDEN_PS0
    uint8_t pddrive_ps1 =8;     //PDDRIVE_PS0  0-63
	
		hx3605_write_reg(0x01, 0x01);
		hx3605_delay(5);
		hx3605_write_reg(0x01, 0x00);
		hx3605_delay(5);
	
	  hx3605_write_reg(0X11, (uint8_t)prf_clk_num);    // prf bit<7:0>6   default 0x00
    hx3605_write_reg(0X12, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>  default 0x03
		hx3605_write_reg(0X16, (ps1_cp_avg_num_sel<<4)|(ps0_cp_avg_num_sel));  //default 0x00
		hx3605_write_reg(0X20, (ir_pden_ps0<<7|ir_pden_ps0<<6|pddrive_ps0));  //default 0x00
		hx3605_write_reg(0X21, (ir_pden_ps1<<7|ir_pden_ps1<<6|pddrive_ps1));  //default 0x00
		hx3605_write_reg(0X18, (uint8_t)dccancel_ps0_data1);     //default 0x00
    hx3605_write_reg(0X19, (uint8_t)(dccancel_ps0_data1>>8));   //default 0x00
    hx3605_write_reg(0X1a, (uint8_t)dccancel_ps0_data2);   //default 0x00
    hx3605_write_reg(0X1b, (uint8_t)(dccancel_ps0_data2>>8));  //default 0x00
    hx3605_write_reg(0X1c, (uint8_t)dccancel_ps1_data1);   //default 0x00
    hx3605_write_reg(0X1d, (uint8_t)(dccancel_ps1_data1>>8));   //default 0x00
    hx3605_write_reg(0X1e, (uint8_t)dccancel_ps1_data2 );   //default 0x00
    hx3605_write_reg(0X1f, (uint8_t)(dccancel_ps1_data2 >>8));   //default 0x00
		hx3605_write_reg(0X22, (ldrsel_ps1<<4|ldrsel_ps0));      //default 0x00
		hx3605_write_reg(0X15, led_on_time);

		hx3605_write_reg(0x2d,0x83);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
		hx3605_write_reg(0X26, (thres_int<<4|data_rdy_int));   //default 0x0f

    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
		hx3605_delay(5);
    
    calReg.red_cur = 0;
		calReg.ir_cur = 8;
		calReg.red_idac = 0;
		calReg.ir_idac = 0;

    AGC_LOG(" chip go to low power mode  \r\n" );  
  
}

void hx3605_spo2_updata_reg(void)
{
    if (calReg.flag & CAL_FLG_LED_DR) 
    {
				hx3605_write_reg(0X20, calReg.red_cur);
				hx3605_write_reg(0X21, calReg.ir_cur);
    }
    
    if (calReg.flag & CAL_FLG_LED_DAC) 
    {
			  hx3605_write_reg(0X18, (uint8_t)calReg.red_idac);  
				hx3605_write_reg(0X19, (uint8_t)(calReg.red_idac>>8));
			  hx3605_write_reg(0X1c, (uint8_t)calReg.ir_idac);   
				hx3605_write_reg(0X1d, (uint8_t)(calReg.ir_idac>>8)); 
    }
    
    if (calReg.flag & CAL_FLG_AMB_DAC) 
    {
       	hx3605_write_reg(0X1a, (uint8_t)calReg.amb_idac); 
				hx3605_write_reg(0X1b, (uint8_t)(calReg.amb_idac>>8));
			  hx3605_write_reg(0X1e, (uint8_t)calReg.amb_idac); 
				hx3605_write_reg(0X1f, (uint8_t)(calReg.amb_idac>>8));
    }
    
    if (calReg.flag & CAL_FLG_RF) 
    {
				hx3605_write_reg(0X15, calReg.ontime);
    }
}
void hx3605_spo2_set_mode(uint8_t mode_cmd)
{
    switch (mode_cmd) 
    {
        case PPG_INIT:
            hx3605_spo2_ppg_init();
						#if defined(TIMMER_MODE)
							hx3605_ppg_timer_cfg(true);
							#if defined(GSEN_40MS_TIMMER)
							hx3605_agc_timer_cfg(true);
							#endif
						#else
							hx3605_gpioint_cfg(true);
						#endif
            s_ppg_state = 1;
            AGC_LOG("ppg init mode\r\n");
            break;

        case PPG_OFF:
            hx3605_ppg_off();
            s_ppg_state = 0;
            AGC_LOG("ppg off mode\r\n");
            break;
				
        case PPG_LED_OFF:
            hx3605_spo2_low_power();
            s_ppg_state = 0;
            AGC_LOG("ppg led off mode\r\n");
            break;

        case CAL_INIT:
            Init_Spo2_PPG_Calibration_Routine(&calReg);
            hx3605_spo2_cal_init();
            hx3605_spo2_updata_reg();
            #if defined(TIMMER_MODE)
							#if defined(GSEN_40MS_TIMMER)
							#else
							hx3605_agc_timer_cfg(true);
							#endif
            #endif        
            s_cal_state = 1;
            AGC_LOG("cal init mode\r\n"); 
            break;       
				
        case RECAL_INIT:
            Restart_Spo2_PPG_Calibration_Routine(&calReg);
            hx3605_spo2_cal_init();
            hx3605_spo2_updata_reg();
            #if defined(TIMMER_MODE)
							#if defined(GSEN_40MS_TIMMER)
							#else
							hx3605_agc_timer_cfg(true);
							#endif
            #endif 
            s_cal_state = 1;
            AGC_LOG("recal init mode\r\n");
            break;

        case CAL_OFF:
            #if defined(TIMMER_MODE)
							#if defined(GSEN_40MS_TIMMER)
							#else
							hx3605_agc_timer_cfg(false);
							#endif
            #endif
            hx3605_spo2_cal_off();
            s_cal_state = 0;
            AGC_LOG("cal off mode\r\n");
            break;

        default:
            break;
    }
}

SENSOR_ERROR_T hx3605_spo2_enable(void)
{
    if (!hx3605_chip_check()) 
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
    spo2_wear_status = MSG_SPO2_INIT;
    spo2_wear_status_pre = MSG_SPO2_INIT;
    hx3605_spo2_set_mode(PPG_INIT);

    AGC_LOG("hx3690l enable!\r\n");

    return SENSOR_OK;
}

void hx3605_spo2_disable(void)
{
		#if defined(TIMMER_MODE)
    hx3605_ppg_timer_cfg(false);
    hx3605_agc_timer_cfg(false);
		#elif defined(INT_MODE)
    hx3605_gpioint_cfg(false);
		#endif	
		hx3605_spo2_set_mode(PPG_OFF);
		s_ppg_state = 0;
		s_cal_state = 0;
    tyhx_spo2_alg_close();    

    AGC_LOG("hx3690l disable!\r\n");
}

void hx3605_spo2_data_reset(void)
{
		s_ppg_state = 0;
		s_cal_state = 0;
		tyhx_spo2_alg_close();
}

hx3605_spo2_wear_msg_code_t hx3605_spo2_get_wear_status(void)
{
    return  spo2_wear_status;
}

SPO2_CAL_SET_T get_spo2_agc_status(void)
{
    SPO2_CAL_SET_T cal = {0};

    cal.flag = calReg.flag;
    cal.int_cnt = calReg.int_cnt;
    cal.red_cur = calReg.red_cur;    
    cal.ir_cur = calReg.ir_cur;   
    cal.red_idac = calReg.red_idac;  
    cal.ir_idac = calReg.ir_idac;  
    cal.amb_idac = calReg.amb_idac; 
    cal.ontime = calReg.ontime;  
    cal.red_led_step = calReg.red_led_step;
    cal.ir_led_step = calReg.ir_led_step;
    cal.state = calReg.state;
    return cal;
}

uint8_t hx3605_spo2_read(spo2_sensor_data_t * s_dat)
{
    int32_t Red_src_data;
    int32_t Ir_src_data;
    bool recal = false;
//    uint8_t size = 0;
//   uint8_t size_byte = 0;
    int32_t *red_buf =  &(s_dat->red_data[0]);
    int32_t *ir_buf =  &(s_dat->ir_data[0]);  
    int32_t *s_buf =  &(s_dat->s_buf[0]);   
    s_dat->agc_red =  calReg.red_cur;
		s_dat->agc_ir =  calReg.ir_cur;
		int32_t ps_data[4]={0};
		int32_t ps_red_data = 0;
		int32_t ps_ir_data = 0;
		uint8_t data_count = 0;
    if (!s_ppg_state || s_cal_state) 
    {
        return 0;
    }    
		data_count = hx3605_read_fifo_data(s_buf,2,1); 
		s_dat->count =  data_count; 
		for (uint8_t i=0; i<data_count; i++) 
		{
				Red_src_data = s_buf[i*2];
				Ir_src_data = s_buf[i*2+1];            

				red_buf[i] = 2*Red_src_data;
				ir_buf[i] = 2*Ir_src_data;

				if(spo2_wear_status==MSG_SPO2_NO_WEAR)
				{
						if(hx3605_spo2_check_wear(Ir_src_data))
						{
							return 0;
						}
				} 
				else if(spo2_wear_status==MSG_SPO2_WEAR)
				{
						if(hx3605_spo2_check_unwear())
						{
							return 0;
						}
				}				
				else
				{
						if(hx3605_spo2_check_unwear())
						{
							return 0;
						}
						if(hx3605_spo2_check_wear(Ir_src_data))
						{
							return 0;
						}
				}
		}
		read_data_packet(ps_data);
		ps_red_data = ps_data[0];
		ps_ir_data = ps_data[2];
		if (ps_red_data<150000 || ps_red_data>400000 || ps_ir_data<150000 || ps_ir_data>400000)            
		{
				recal = true;
			 
				if(spo2_wear_status==MSG_SPO2_NO_WEAR)
				{
						 recal = false;
				}                
		}
		if (recal) 
		{
				cal_delay--;
				if (cal_delay <= 0) 
				{
						cal_delay = CAL_DELAY_COUNT;
						hx3605_spo2_set_mode(RECAL_INIT);
				}
		} 
		else 
		{
				cal_delay = CAL_DELAY_COUNT;
		}    
    return 1;
}

#ifdef SPO2_DATA_CALI
static int32_t red_data_fifo[4] = {0};
static int32_t ir_data_fifo[4] = {0};
static int32_t red_dc_temp = 0;
static int32_t ir_dc_temp = 0;
static int32_t red_data_pre = 0;
static int32_t ir_data_pre = 0;
static int32_t red_jump_delta = 0;
static int32_t ir_jump_delta = 0;
static int32_t cali_data_cnt = 0;

void hx3605_spo2_data_cali_init(void)
{
	int ii=0;
	for(ii=0;ii<4;ii++)
	{
		red_data_fifo[ii] = 0;
		ir_data_fifo[ii] = 0;
	}
	red_dc_temp = 0;
	ir_dc_temp = 0;
	red_data_pre = 0;
	ir_data_pre = 0;
	red_jump_delta = 0;
	ir_jump_delta = 0;
	cali_data_cnt = 0;
}

int32_t hx3605_red_data_cali(int32_t red_new_raw_data)
{
    uint8_t ii;
    int32_t red_data_final;
    int32_t red_data_cali;
    hx3605_spo2_wear_msg_code_t touch_status;
    touch_status = hx3605_spo2_get_wear_status();
    if(touch_status == MSG_SPO2_NO_WEAR)
    {
      cali_data_cnt = 0;
      red_data_cali = red_new_raw_data;
      for(ii=0;ii<4;ii++)
      {
        red_data_fifo[ii] = 0;
      }
      red_dc_temp = 0;
    }
    else
    {
      for(ii=3;ii>0;ii--)
      {
          red_data_fifo[ii] = red_data_fifo[ii-1];
      }
      red_data_fifo[0] = red_new_raw_data; 
      if(cali_data_cnt>=25) 
      {
        if ((((red_data_fifo[1] - red_data_fifo[2]) > SOP2_DEGLITCH_THRE) && ((red_data_fifo[1] - red_new_raw_data) > SOP2_DEGLITCH_THRE)) || \
           (((red_data_fifo[1] - red_data_fifo[2]) < -SOP2_DEGLITCH_THRE) && ((red_data_fifo[1] - red_new_raw_data) < -SOP2_DEGLITCH_THRE)))
        { 
            red_new_raw_data = red_data_fifo[2]; 
            red_data_fifo[1] = red_data_fifo[2]; 
        }
        else
        {
            red_new_raw_data = red_data_fifo[1];
        }	
        if ((abs((red_new_raw_data - red_jump_delta) - red_data_pre) > SPO2_REMOVE_JUMP_THRE))
        {
            red_jump_delta = red_new_raw_data - red_data_pre;
        }  
        red_data_cali = red_new_raw_data - red_jump_delta; 
        red_data_pre = red_data_cali;
      }
      else
      {
        red_data_cali = red_data_fifo[1];
        red_data_pre = red_data_fifo[1];
      }
    }
    if(cali_data_cnt<=30)
    {
      red_dc_temp = red_data_cali;
      red_data_final = red_new_raw_data;
    }
    else
    {
      red_dc_temp = (red_dc_temp*31 + red_data_cali)>>5;
      red_data_final = red_data_cali - red_dc_temp + 2608*50*hx3605_spo2_agc_red_idac;
    }
    return red_data_final;
}
int32_t hx3605_ir_data_cali(int32_t ir_new_raw_data)
{
    uint8_t ii;
    int32_t ir_data_final;
    int32_t ir_data_cali;
    hx3605_spo2_wear_msg_code_t touch_status;
    touch_status = hx3605_spo2_get_wear_status();
    if(touch_status == MSG_SPO2_NO_WEAR)
    {
      cali_data_cnt = 0;
      ir_data_cali = ir_new_raw_data;
      for(ii=0;ii<4;ii++)
      {
        ir_data_fifo[ii] = 0;
      }
      ir_dc_temp = 0;
    }
    else
    {
      for(ii=3;ii>0;ii--)
      {
          ir_data_fifo[ii] = ir_data_fifo[ii-1];
      }
      ir_data_fifo[0] = ir_new_raw_data; 
      cali_data_cnt++;
      if(cali_data_cnt>=25) 
      {
        if ((((ir_data_fifo[1] - ir_data_fifo[2]) > SOP2_DEGLITCH_THRE) && ((ir_data_fifo[1] - ir_new_raw_data) > SOP2_DEGLITCH_THRE)) || \
           (((ir_data_fifo[1] - ir_data_fifo[2]) < -SOP2_DEGLITCH_THRE) && ((ir_data_fifo[1] - ir_new_raw_data) < -SOP2_DEGLITCH_THRE)))
        { 
            ir_new_raw_data = ir_data_fifo[2]; 
            ir_data_fifo[1] = ir_data_fifo[2]; 
        }
        else
        {
            ir_new_raw_data = ir_data_fifo[1];
        }	
        if ((abs((ir_new_raw_data - ir_jump_delta) - ir_data_pre) > SPO2_REMOVE_JUMP_THRE))
        {
            ir_jump_delta = ir_new_raw_data - ir_data_pre;
        }  
        ir_data_cali = ir_new_raw_data - ir_jump_delta; 
        ir_data_pre = ir_data_cali;
      }
      else
      {
        ir_data_cali = ir_data_fifo[1];
        ir_data_pre = ir_data_fifo[1];
      }
    }
    if(cali_data_cnt<=30)
    {
      ir_dc_temp = ir_data_cali;
      ir_data_final = ir_new_raw_data;
    }
    else
    {
      ir_dc_temp = (ir_dc_temp*31 + ir_data_cali)>>5;
      ir_data_final = ir_data_cali - ir_dc_temp + 2608*50*hx3605_spo2_agc_ir_idac;
    }
    return ir_data_final;
}
#endif

#endif
