//int ft_printf_inner(const char *fmt, ...);
//#include "efuse.h"

#define printf FT_LOG_I
#define     CMU_DEVCLKEN0_EFUSECLKEN                                          31

#define WPASSWORD		0x40687A57
#define RPASSWORD		0x26467358

//============================= efuse reload to reg ==============================================//
static int EfuseReLoad() 
{
	int VOUT_CTL1_S1M_tmp,VOUT_CTL1_S1_tmp;
	*((REG32)(CMU_DEVCLKEN0)) |= (1<<CMU_DEVCLKEN0_EFUSECLKEN); //enable EFUSE CLOCK
	VOUT_CTL1_S1M_tmp = *((REG32)(VOUT_CTL1_S1M));
	VOUT_CTL1_S1_tmp = *((REG32)(VOUT_CTL1_S1));	
	if(((*((REG32)(VOUT_CTL0)))&0x0f000000)<0x08000000)//real VDD<0.95V
	{
  *((REG32)(VOUT_CTL1_S1)) &= (~0x00007000);
	*((REG32)(VOUT_CTL1_S1M)) = (*((REG32)(VOUT_CTL1_S1M))&(~(0xf)))|0x809;	//vd12>=1.0V vdd=1.0V
	}
	*((REG32)(EFUSE_CTL1)) 	&= ~(1<<EFUSE_CTL1_EN_EFUSE25);//确保EFUSE25已关
	*((REG32)(EFUSE_CTL1))	|= 0x00004000;//设置下拉
	ft_udelay(20);		//wait VDD power stably
		
	//reload to reg
	*((REG32)(EFUSE_CTL0))	|= (1<<EFUSE_CTL0_RE_LOAD);			// reload EFUSE,read to reg
	ft_udelay(10);
	while( 0 == (*((REG32)(EFUSE_CTL0))&0x01));					// wait chip ID data ready
	*((REG32)(EFUSE_DATA0)) = RPASSWORD;	// read password，写data0~3均可
	ft_udelay(1); 
	if( ( (*((REG32)(EFUSE_CTL0))) & (1<<EFUSE_CTL0_RD_PSD_OK) ) == 0 )	// 读密码是否正确
		return 1;
	*((REG32)(VOUT_CTL1_S1)) = VOUT_CTL1_S1_tmp;
	*((REG32)(VOUT_CTL1_S1M)) = VOUT_CTL1_S1M_tmp;
			
	return 0;	
}

static int Efuse_Write_32Bits(uint32 bits,uint32 num)
{
//	return PASSED;//for debug
	volatile uint32 EFUSE_Rdata_before[32]={0};
	volatile uint32 EFUSE_Rdata_after[32]={0};	
	int i = 0;
	int VOUT_CTL1_S1M_tmp,VOUT_CTL1_S1_tmp;
	VOUT_CTL1_S1M_tmp = *((REG32)(VOUT_CTL1_S1M));
	VOUT_CTL1_S1_tmp = *((REG32)(VOUT_CTL1_S1));
	
	//检查参数范围
	if(num>31)
	{
		printf("EFUSE num OUT OF RANGE num = %d\n",num);
		return 1;			
	}
	if(bits==0)
	{
		printf("EFUSE no bits need to burn\n");
		return 0;			
	}
	
	//efuse read	
	if(EfuseReLoad())
		return 1;
	for(i=0;i<32;i++)
	{
		EFUSE_Rdata_before[i]=*((REG32)(EFUSE_DATA0 + 4*i));
	}
	//检查要烧写bit是否已经烧写
	if(bits & EFUSE_Rdata_before[num])
	{
		printf("EFUSE ALREADY BURN: num = %d DATA = %x\n",num,EFUSE_Rdata_before[num]);
		return 1;
	}
	
	//烧写准备
	*((REG32)(CMU_DEVCLKEN0)) |= (1<<CMU_DEVCLKEN0_EFUSECLKEN); //enable EFUSE program and reload-reg CLOCK    
	*((REG32)(VOUT_CTL1_S1)) &= (~0x00007000);
	*((REG32)(VOUT_CTL1_S1M)) = (*((REG32)(VOUT_CTL1_S1M))&(~(0xf)))|0xc0b;	//vd12>=1.2V vdd=1.1V
	ft_udelay(30);		//wait VDD power stably
    
	*((REG32)(EFUSE_CTL2)) = WPASSWORD;
	ft_udelay(1);
	if( (*((REG32)(EFUSE_CTL1)) & 0x00001000 ) != 0x00001000)
		return 1;
	*((REG32)(EFUSE_CTL1)) = 0x566000;//设置默认值并打开EFUSE25
	ft_udelay(800);
    //烧写
	for(i=0;i<32;i++)
	{
		if( bits & (1<<i) )				// 判断某一位是否需要烧写
			{
				*((REG32)(EFUSE_CTL1)) &= 0xfffffc00;
				*((REG32)(EFUSE_CTL1)) |= num*32+i;
				ft_udelay(1);
				while( *((REG32)(EFUSE_CTL1))&(1<<EFUSE_CTL1_PGM_AUTO_EN) );	// 等待烧写完成标志
				*((REG32)(EFUSE_CTL1)) |= (1<<EFUSE_CTL1_PGM_AUTO_EN);			// 开始自动烧写
				while( *((REG32)(EFUSE_CTL1))&(1<<EFUSE_CTL1_PGM_AUTO_EN) );	// 等待烧写完成标志			
			}
	}

	//disable EFUSE25
	*((REG32)(EFUSE_CTL1)) 	&= ~(1<<EFUSE_CTL1_EN_EFUSE25);
	ft_udelay(100);

//check data		
		if(EfuseReLoad())
			return 1;
		for(i=0;i<32;i++)
		{
			EFUSE_Rdata_after[i]=*((REG32)(EFUSE_DATA0 + 4*i));
			if(i!=num)
			{
				if(EFUSE_Rdata_before[i]!=EFUSE_Rdata_after[i])
				{
					printf("EFUSE OTHER BIT CHANGE: num = %d DATA = %x\n",i,EFUSE_Rdata_after[i]);
				}
			}
			else
			{
				if(((~(EFUSE_Rdata_before[i])) & EFUSE_Rdata_after[i]) != bits)
				{
					printf("EFUSE PROGRAM WRONG: num = %d DATA = %x\n",i,EFUSE_Rdata_after[i]);
					return 1;
				}
				else
					printf("EFUSE PROGRAM DONE: num = %d DATA = %x\n",i,EFUSE_Rdata_after[i]);					
			}
		}
	*((REG32)(EFUSE_DATA0)) = 0x1;//写入错误值	
	*((REG32)(CMU_DEVCLKEN0)) &= ~(1<<CMU_DEVCLKEN0_EFUSECLKEN); //disable EFUSE CLOCK
	*((REG32)(VOUT_CTL1_S1)) = VOUT_CTL1_S1_tmp;
	*((REG32)(VOUT_CTL1_S1M)) = VOUT_CTL1_S1M_tmp;
	
	return 0;
}

static uint32 Efuse_Read_32Bits( uint32 num, uint32* efuse_value )
{
	volatile uint32 EFUSE_Rdata=0;
	
	//检查参数范围
	if(num>31)
	{
		printf("EFUSE NUM OUT OF RANGE NUM = %d\n",num);
		return 1;
	}
	
	if(EfuseReLoad())
	{
		printf("EFUSE read ERROR");
		return 1;
	}
	*efuse_value = *((REG32)(EFUSE_DATA0+num*4));
	*((REG32)(EFUSE_DATA0)) = 0x1;//写入错误值
	printf("EFUSE READ: num = %d DATA = %x\n",num,*efuse_value);
	*((REG32)(CMU_DEVCLKEN0)) &= ~(1<<CMU_DEVCLKEN0_EFUSECLKEN); //disable EFUSE CLOCK

	return 0;
}



