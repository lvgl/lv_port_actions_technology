#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>


#ifdef TYHX_DEMO
#include "SEGGER_RTT.h"
#include "nrf_delay.h"
#endif

//////////////////////////////


#include "hx3605.h"
#include "hx3605_factory_test.h"

void hx3605_factory_ft_card_test_config(int8_t mode)//ª“ø®10mm∏ﬂ∂»≤‚ ‘≈‰÷√
{
		uint16_t sample_rate = 25;                  /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;   /*period in clk num, num = Fclk/fs */
    uint8_t ps0_enable=1;       /*ps0_enable  , 1 mean enable ; 0 mean disable */
    uint8_t ps1_enable=1;       /*ps1_enable  , 1 mean enable ; 0 mean disable */   
    uint8_t ps0_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    uint8_t ps1_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    
    uint8_t ps1_cp_avg_num_sel= 0;
		uint8_t ps0_cp_avg_num_sel= 1;
		uint8_t ps1_avg_num_sel_i2c=0;
		uint8_t ps0_avg_num_sel_i2c=0;
     /***********led open enable***********/
    uint8_t dccancel_ps0_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps0_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    
    uint8_t ir_pden_ps0 =0;     //IR_PDEN_PS0
//    uint8_t ext_pden_ps0 =0;    //EXT_PDEN_PS0
    uint8_t pddrive_ps0 =15;     //PDDRIVE_PS0  0-63
	
	  uint8_t ir_pden_ps1 =0;      //IR_PDEN_PS1
//    uint8_t ext_pden_ps1 =0;     //EXT_PDEN_PS1
    uint8_t pddrive_ps1 =15;     //PDDRIVE_PS1  0-63
    uint8_t ps1_interval_i2c =0;    // config 0/5  
    
    uint8_t led_en_begin =1;       // 0 = 2 ; 1 = 4 ; 2 = 8 ; 3 = 16 ;
    uint8_t afe_reset = 1;        //* 0 = 32clk ; 1 = 64clk ; 2 = 128clk ; 3 = 256clk(d) ;
    uint8_t en2rst_delay =2;
		uint8_t init_wait_delay = 2; /* 0 = 31clk ; 1 = 64clk ; 2 = 127clk ; 3 = 255clk(d) ;
                                     4 = 511clk; 5 = 1023clk; 6 = 2047; 7 = 2048clk */   
//		uint16_t thres_level =0;
    uint8_t thres_int =1;    //thres int enable
    uint8_t data_rdy_int =8;    //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1
    
    uint8_t ldrsel_ps1 =2;      //ps1 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
		uint8_t ldrsel_ps0 =4;
    uint8_t led_on_time  = 32;      /* 0 = 32clk2=8us ; 8 = 64clk=16us; 16=128clk=32us ; 32 = 256clk=64us ;
                                     64 = 512clk=128us ; 128= 1024clk=256us; 256= 2048clk=512us*/
		if (mode==1)
		{
      ldrsel_ps0 =4;      //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
			led_on_time  = 16;
		}
		else if(mode==2)
		{
			ldrsel_ps0 =1;      //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
			led_on_time  = 8;
		}
    /***********cap *********/
    uint8_t intcapsel_ps1 =6;   //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf 
    uint8_t intcapsel_ps0 =6;   //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf 
    
//    uint8_t led_on_time  = 32;      /* 0 = 32clk2=8us ; 8 = 64clk=16us; 16=128clk=32us ; 32 = 256clk=64us ;
//                                     64 = 512clk=128us ; 128= 1024clk=256us; 256= 2048clk=512us*/
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
		uint8_t databuf[3] = {0};

    hx3605_write_reg(0X10, (ps1_enable<<5| ps0_enable<<4|ps0_osr<<2|ps1_osr));	//default 0x00
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
     
//#if defined(TIMMER_MODE)
//        hx3605_write_reg(0x2d,0x83);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
//				hx3605_write_reg(0x2e,0x20);			 //watermark
//				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
//				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf int,04=enable almost full int
//#elif defined(FIFO_ALMOST_FULL_READ_ENABLE)   
//        hx3605_write_reg(0x2d,0x00);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
//				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
//				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf int,04=enable almost full int
//#else ///////////INT Mode
//				hx3605_write_reg(0x2d,0x00);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
//				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
//				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf ,04=enable almost full
//#endif
        
    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
		hx3605_delay(5);
		hx3605_brust_read_reg(0x31, databuf, 3);
}

void hx3605_factory_ft_leak_light_test_config(int8_t mode)//¬©π‚≤‚ ‘≈‰÷√
{
  		uint16_t sample_rate = 25;                  /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;   /*period in clk num, num = Fclk/fs */
    uint8_t ps0_enable=1;       /*ps0_enable  , 1 mean enable ; 0 mean disable */
    uint8_t ps1_enable=1;       /*ps1_enable  , 1 mean enable ; 0 mean disable */   
    uint8_t ps0_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    uint8_t ps1_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    
    uint8_t ps1_cp_avg_num_sel= 0;
		uint8_t ps0_cp_avg_num_sel= 1;
		uint8_t ps1_avg_num_sel_i2c=0;
		uint8_t ps0_avg_num_sel_i2c=0;
     /***********led open enable***********/
    uint8_t dccancel_ps0_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps0_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    
    uint8_t ir_pden_ps0 =0;     //IR_PDEN_PS0
//    uint8_t ext_pden_ps0 =0;    //EXT_PDEN_PS0
    uint8_t pddrive_ps0 =15;     //PDDRIVE_PS0  0-63
	
	  uint8_t ir_pden_ps1 =0;      //IR_PDEN_PS1
//    uint8_t ext_pden_ps1 =0;     //EXT_PDEN_PS1
    uint8_t pddrive_ps1 =15;     //PDDRIVE_PS1  0-63
    uint8_t ps1_interval_i2c =0;    // config 0/5  
    
    uint8_t led_en_begin =1;       // 0 = 2 ; 1 = 4 ; 2 = 8 ; 3 = 16 ;
    uint8_t afe_reset = 1;        //* 0 = 32clk ; 1 = 64clk ; 2 = 128clk ; 3 = 256clk(d) ;
    uint8_t en2rst_delay =2;
		uint8_t init_wait_delay = 2; /* 0 = 31clk ; 1 = 64clk ; 2 = 127clk ; 3 = 255clk(d) ;
                                     4 = 511clk; 5 = 1023clk; 6 = 2047; 7 = 2048clk */   
//		uint16_t thres_level =0;
    uint8_t thres_int =1;    //thres int enable
    uint8_t data_rdy_int =8;    //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1
    
    uint8_t ldrsel_ps1 =2;      //ps1 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
		uint8_t ldrsel_ps0 =4;
    uint8_t led_on_time  = 32;      /* 0 = 32clk2=8us ; 8 = 64clk=16us; 16=128clk=32us ; 32 = 256clk=64us ;
                                     64 = 512clk=128us ; 128= 1024clk=256us; 256= 2048clk=512us*/
		if (mode==1)
		{
      ldrsel_ps0 =4;      //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
			led_on_time  = 16;
		}
		else if(mode==2)
		{
			ldrsel_ps0 =1;      //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
			led_on_time  = 8;
		}
    /***********cap *********/
    uint8_t intcapsel_ps1 =6;   //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf 
    uint8_t intcapsel_ps0 =6;   //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf 
    
//    uint8_t led_on_time  = 32;      /* 0 = 32clk2=8us ; 8 = 64clk=16us; 16=128clk=32us ; 32 = 256clk=64us ;
//                                     64 = 512clk=128us ; 128= 1024clk=256us; 256= 2048clk=512us*/
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
		uint8_t databuf[3] = {0};

    hx3605_write_reg(0X10, (ps1_enable<<5| ps0_enable<<4|ps0_osr<<2|ps1_osr));	//default 0x00
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
     
//#if defined(TIMMER_MODE)
//        hx3605_write_reg(0x2d,0x83);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
//				hx3605_write_reg(0x2e,0x20);			 //watermark
//				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
//				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf int,04=enable almost full int
//#elif defined(FIFO_ALMOST_FULL_READ_ENABLE)   
//        hx3605_write_reg(0x2d,0x00);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
//				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
//				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf int,04=enable almost full int
//#else ///////////INT Mode
//				hx3605_write_reg(0x2d,0x00);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
//				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
//				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf ,04=enable almost full
//#endif
        
    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
		hx3605_delay(5);
		hx3605_brust_read_reg(0x31, databuf, 3);
	
}

void hx3605_factory_ft_int_test_config(int8_t mode)//÷–∂œ≤‚ ‘≈‰÷√
{
 		uint16_t sample_rate = 25;                  /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;   /*period in clk num, num = Fclk/fs */
    uint8_t ps0_enable=1;       /*ps0_enable  , 1 mean enable ; 0 mean disable */
    uint8_t ps1_enable=1;       /*ps1_enable  , 1 mean enable ; 0 mean disable */   
    uint8_t ps0_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    uint8_t ps1_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    
    uint8_t ps1_cp_avg_num_sel= 0;
		uint8_t ps0_cp_avg_num_sel= 1;
		uint8_t ps1_avg_num_sel_i2c=0;
		uint8_t ps0_avg_num_sel_i2c=0;
     /***********led open enable***********/
    uint8_t dccancel_ps0_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps0_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    
    uint8_t ir_pden_ps0 =0;     //IR_PDEN_PS0
//    uint8_t ext_pden_ps0 =0;    //EXT_PDEN_PS0
    uint8_t pddrive_ps0 =15;     //PDDRIVE_PS0  0-63
	
	  uint8_t ir_pden_ps1 =0;      //IR_PDEN_PS1
//    uint8_t ext_pden_ps1 =0;     //EXT_PDEN_PS1
    uint8_t pddrive_ps1 =15;     //PDDRIVE_PS1  0-63
    uint8_t ps1_interval_i2c =0;    // config 0/5  
    
    uint8_t led_en_begin =1;       // 0 = 2 ; 1 = 4 ; 2 = 8 ; 3 = 16 ;
    uint8_t afe_reset = 1;        //* 0 = 32clk ; 1 = 64clk ; 2 = 128clk ; 3 = 256clk(d) ;
    uint8_t en2rst_delay =2;
		uint8_t init_wait_delay = 2; /* 0 = 31clk ; 1 = 64clk ; 2 = 127clk ; 3 = 255clk(d) ;
                                     4 = 511clk; 5 = 1023clk; 6 = 2047; 7 = 2048clk */   
//		uint16_t thres_level =0;
    uint8_t thres_int =1;    //thres int enable
    uint8_t data_rdy_int =8;    //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1
    
    uint8_t ldrsel_ps1 =2;      //ps1 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
		uint8_t ldrsel_ps0 =4;
		uint8_t led_on_time  = 32;      /* 0 = 32clk2=8us ; 8 = 64clk=16us; 16=128clk=32us ; 32 = 256clk=64us ;
                                     64 = 512clk=128us ; 128= 1024clk=256us; 256= 2048clk=512us*/
		if (mode==1)
		{
      ldrsel_ps0 =4;      //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
			led_on_time  = 16;
		}
		else if(mode==2)
		{
			ldrsel_ps0 =1;      //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
			led_on_time  = 8;
		}
    /***********cap *********/
    uint8_t intcapsel_ps1 =6;   //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf 
    uint8_t intcapsel_ps0 =6;   //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf 
    
//    uint8_t led_on_time  = 32;      /* 0 = 32clk2=8us ; 8 = 64clk=16us; 16=128clk=32us ; 32 = 256clk=64us ;
//                                     64 = 512clk=128us ; 128= 1024clk=256us; 256= 2048clk=512us*/
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
		uint8_t databuf[3] = {0};

    hx3605_write_reg(0X10, (ps1_enable<<5| ps0_enable<<4|ps0_osr<<2|ps1_osr));	//default 0x00
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
    hx3605_write_reg(0X27, 0x00);
//#if defined(TIMMER_MODE)
//        hx3605_write_reg(0x2d,0x83);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
//				hx3605_write_reg(0x2e,0x20);			 //watermark
//				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
//				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf int,04=enable almost full int
//#elif defined(FIFO_ALMOST_FULL_READ_ENABLE)   
//        hx3605_write_reg(0x2d,0x00);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
//				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
//				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf int,04=enable almost full int
//#else ///////////INT Mode
//				hx3605_write_reg(0x2d,0x00);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
//				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
//				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf ,04=enable almost full
//#endif
        
    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
		hx3605_delay(5);
		hx3605_brust_read_reg(0x31, databuf, 3);
	
}

bool hx3605_factory_test_read(int32_t *phase1_data,int32_t *phase2_data,int32_t *phase3_data,int32_t *phase4_data)
{
//    uint8_t  databuf[12] = {0};
//    int32_t P1 = 0, P2 = 0 ,P3 = 0 ,P4 = 0;
//    hx3605_brust_read_reg(0x03, databuf, 12);   
//    P1 = ((databuf[0])|(databuf[1]<<8)|(databuf[2]<<16)); 
//    P3 = ((databuf[3])|(databuf[4]<<8)|(databuf[5]<<16)); 
//    P4 = ((databuf[6])|(databuf[7]<<8)|(databuf[8]<<16)); 
//    P2 = ((databuf[9])|(databuf[10]<<8)|(databuf[11]<<16));   
    uint8_t  databuf1[6] = {0};
    uint8_t  databuf2[6] = {0};
//		uint8_t  databuf3[6] = {0};
//		uint8_t  databuf4[6] = {0};
    uint32_t P1 = 0, P2 = 0, P4 = 0, P3 = 0;
    hx3605_brust_read_reg(0x02, databuf1, 6);
		hx3605_brust_read_reg(0x08, databuf2, 6);

		P1 = ((databuf1[0]) | (databuf1[1] << 8) | (databuf1[2] << 16));
		P2 = ((databuf1[3]) | (databuf1[4] << 8) | (databuf1[5] << 16));
		P3 = ((databuf2[0]) | (databuf2[1] << 8) | (databuf2[2] << 16));
		P4 = ((databuf2[3]) | (databuf2[4] << 8) | (databuf2[5] << 16));
		
		*phase1_data = P1 ;
    *phase2_data = P2 ;	
    *phase3_data = P3 ;
    *phase4_data = P4 ;	
		DEBUG_PRINTF("p1,p2,p3,p4 %d %d %d %d\r\n" , P1, P2, P3, P4);
    return true; 	
}


bool hx3605_chip_check_id(void)
{
    uint8_t i = 0;   
	uint8_t chip_id =0;
    for(i=0;i<10;i++)
    {
        hx3605_write_reg(0x01, 0x00);
        hx3605_delay(5);
        chip_id = hx3605_read_reg(0x00);  
        if (chip_id == 0x25)
        {
            return true;
        }        
    }
    return false;
}

bool hx3605_factroy_test(uint32_t  test_mode)
{
	int32_t hr_gr_buffer[4] ={0,0,0,0};
	int32_t spo2_red_buffer[4] ={0,0,0,0};
	int32_t spo2_ir_buffer[4] ={0,0,0,0};
	int32_t hr_gr_high_buffer[4] ={0,0,0,0};
//	int32_t green_psdif_buffer[4] ={0,0,0,0};
//	int32_t red_psdif_buffer[4] ={0,0,0,0};
//	int32_t ir_psdif_buffer[4] ={0,0,0,0};
	int32_t  phase1_data= 0;
	int32_t  phase2_data= 0;
	int32_t  phase3_data= 0;
	int32_t  phase4_data= 0;

	int32_t  green_led_data= 0;
	int32_t  green_led_data_high = phase2_data - phase1_data;			
	int32_t  red_led_data= 0;
	int32_t  ir_led_data= 0;	
	
	//int32_t green_psdif = 0;
	//int32_t red_psdif = 0;
	//int32_t ir_psdif = 0;
	
	int32_t hr_gr_final = 0;
	int32_t spo2_red_final = 0;
  int32_t spo2_ir_final = 0;
	int32_t testdata_temp = 0;
//	int32_t green_psdif_final = 0;
//	int32_t red_psdif_final = 0;
//	int32_t ir_psdif_final = 0;
	int32_t green_high_final = 0;
	int32_t single_gr_led_judge = 0;
	
	bool nrtn = false;
	
	uint8_t ii = 0;
	uint8_t jj = 0;
	if (hx3605_chip_check_id() == false)
	{
		return false;
	}
	switch(test_mode)
	{
			case LEAK_LIGHT_TEST:
			hx3605_factory_ft_leak_light_test_config(1);
					break;            
			case GRAY_CARD_TEST:
			hx3605_factory_ft_card_test_config(1);
					break;
			case FT_INT_TEST:
			hx3605_factory_ft_int_test_config(1);
					break;
			default:
		break;
	}
	hx3605_delay(100);
	if(test_mode==FT_INT_TEST)
	{
		return 1;
	}
	
	for(ii=0;ii<4;ii++)   //40ms read data
	{
			hx3605_factory_test_read(&phase1_data,&phase2_data,&phase3_data,&phase4_data);		
			red_led_data = phase1_data - phase2_data;	
			ir_led_data = phase3_data - phase4_data;
			spo2_ir_buffer[ii] = ir_led_data;
			spo2_red_buffer[ii] = red_led_data;
		  //DEBUG_PRINTF("%d red_led_data:%d ir_led_data:%d %d %d\r\n", ii, red_led_data, ir_led_data, phase2_data, phase4_data);
			hx3605_delay(40);
	}
	
	switch(test_mode)
	{
			case LEAK_LIGHT_TEST:
			hx3605_factory_ft_leak_light_test_config(2);
					break;            
			case GRAY_CARD_TEST:
			hx3605_factory_ft_card_test_config(2);
					break;
			case FT_INT_TEST:
			hx3605_factory_ft_int_test_config(2);
					break;
			default:
		break;
	}

	for(ii=0;ii<4;ii++)   //40ms read data
	{
			hx3605_factory_test_read(&phase1_data,&phase2_data,&phase3_data,&phase4_data);	
			green_led_data = phase1_data - phase2_data;;		
			hr_gr_buffer[ii] = green_led_data;
		//DEBUG_PRINTF("%d green_led_data:%d %d\r\n", ii, green_led_data, phase2_data);
			hx3605_delay(40);
	}
	for(ii=0;ii<3;ii++)
	{
		for(jj=0;jj<3-ii;jj++)
		{
			if(hr_gr_buffer[jj]>=hr_gr_buffer[jj+1])
			{
				testdata_temp = hr_gr_buffer[jj];
				hr_gr_buffer[jj] = hr_gr_buffer[jj+1];
				hr_gr_buffer[jj+1] = testdata_temp;
			}

			if(spo2_red_buffer[jj]>=spo2_red_buffer[jj+1])
			{
				testdata_temp = spo2_red_buffer[jj];
				spo2_red_buffer[jj] = spo2_red_buffer[jj+1];
				spo2_red_buffer[jj+1] = testdata_temp;
			}
			if(spo2_ir_buffer[jj]>=spo2_ir_buffer[jj+1])
			{
				testdata_temp = spo2_ir_buffer[jj];
				spo2_ir_buffer[jj] = spo2_ir_buffer[jj+1];
				spo2_ir_buffer[jj+1] = testdata_temp;
			}
		}
	}
	
	if(test_mode==GRAY_CARD_TEST)
	{
		hx3605_write_reg(0X20, 0x3f);
		hx3605_delay(40);
		for(ii=0;ii<4;ii++)   //40ms read data
		{
				hx3605_factory_test_read(&phase1_data,&phase2_data,&phase3_data,&phase4_data);	
				green_led_data_high = phase1_data - phase2_data;			
				hr_gr_high_buffer[ii] = green_led_data_high;
			  //DEBUG_PRINTF("%d green_led_data_high:%d %d\r\n", ii, green_led_data_high, phase2_data);
				hx3605_delay(40);
		}
		for(ii=0;ii<3;ii++)
		{
			for(jj=0;jj<3-ii;jj++)
			{
				if(hr_gr_high_buffer[jj]>=hr_gr_high_buffer[jj+1])
				{
					testdata_temp = hr_gr_high_buffer[jj];
					hr_gr_high_buffer[jj] = hr_gr_high_buffer[jj+1];
					hr_gr_high_buffer[jj+1] = testdata_temp;
				}
			}
		}
		//DEBUG_PRINTF("%d %d %d %d %d\r\n", ii, hr_gr_high_buffer[0], hr_gr_high_buffer[1], hr_gr_high_buffer[2], hr_gr_high_buffer[3]);
	}
	
	switch(test_mode)
	{
			case LEAK_LIGHT_TEST:
					hr_gr_final = (hr_gr_buffer[1]+hr_gr_buffer[2])/2;
					spo2_red_final = (spo2_red_buffer[1]+spo2_red_buffer[2])/2;
					spo2_ir_final = (spo2_ir_buffer[1]+spo2_ir_buffer[2])/2;
					if(hr_gr_final<0)
					{
						hr_gr_final = 0;
					}
					if(spo2_red_final<0)
					{
						spo2_red_final = 0;
					}
					if(spo2_ir_final<0)
					{
						spo2_ir_final = 0;
					}
					break;            
			case GRAY_CARD_TEST:
					hr_gr_final = (hr_gr_buffer[1]+hr_gr_buffer[2])/2;
					spo2_red_final = (spo2_red_buffer[1]+spo2_red_buffer[2])/2;
					spo2_ir_final = (spo2_ir_buffer[1]+spo2_ir_buffer[2])/2;
					green_high_final = (hr_gr_high_buffer[1]+hr_gr_high_buffer[2])/2;
					single_gr_led_judge = green_high_final*100/hr_gr_final;
					break;
			default:
					break;
	}
	
	//DEBUG_PRINTF("%d %d %d %d %d\r\n",hr_gr_final, spo2_red_final, spo2_ir_final, green_high_final, single_gr_led_judge);
	  
	return nrtn;

}
