/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Production test main.
 */

#include "fcc_test_inner.h"
#include "rbuf_uart.c"

#define RESET_BTHUB				56

//#define ATT_BUILD_ONLY	(1)

#define TIMEOUT_LONG_SUPPORT (1)
static uint8_t fcc_test_mode;
static uint8_t fcc_uart_mode_stop_log;
static uint8_t sdk_run_mode;
static const struct owl_uart_dev *g_uart_dev;
struct ft_env_var global_ft_env_var  = {0};
static struct ft_env_var *self;
static struct owl_uart_dev config_uart_dev;
static uint8_t g_uart_number = 0;
static uint8_t g_uart_txio = 28;
static uint8_t g_uart_rxio = 29;

#include "efuse.c"		/* efuse use self */

#ifndef ATT_BUILD_ONLY
static const uint32_t bak_reg_addr[] = {
	RMU_MRCR0,
	RMU_MRCR1,
	DCDC_VD12_CTL,
	DCDC_VC18_CTL,
	CK64M_CTL,
	CHG_CTL_SVCC,
	CMU_DEVCLKEN0,
	CMU_DEVCLKEN1,
	CMU_S1CLKCTL,
	CMU_S1BTCLKCTL,
	HOSC_CTL,
	LOSC_CTL,
	CMU_MEMCLKEN0,
	CMU_MEMCLKEN1,
	CMU_MEMCLKSRC0,
	CMU_MEMCLKSRC1,
	MEMORY_CTL,
	JTAG_CTL,
	DEBUGIE0,
	DEBUGOE0,

	/* Watch dog */
	WD_CTL,

	/* Timer */
	CMU_TIMER1CLK,
	T1_CTL,
	T1_VAL,

	/* Uart */
	UART0_CTL,
	UART0_BR,
	UART1_CTL,
	UART1_BR,
	UART2_CTL,
	UART2_BR,
	/*Watchdog*/
	WD_CTL
};

#define BAK_REG_CNT			ARRAY_SIZE(bak_reg_addr)
static uint32_t bak_reg_value[BAK_REG_CNT];

static void fcc_reg_bak(void)
{
	int i;

	for (i=0; i<BAK_REG_CNT; i++) {
		bak_reg_value[i] = *((REG32)(bak_reg_addr[i]));
	}
}

static void fcc_reg_resume(void)
{
	int i;

	for (i=0; i<BAK_REG_CNT; i++) {
		*((REG32)(bak_reg_addr[i])) = bak_reg_value[i];
	}
}
#endif

static void watchdog_disable(void)
{
    *(REG32)WD_CTL = 0x00;
}

/*************************************************
*	soc clock module
**************************************************/
static void owl_clk_set(int clk_id, int enable)
{
	unsigned long reg;
	unsigned int bit;

	reg = CMU_DEVCLKEN0 + (clk_id / 32) * 4;
	bit = clk_id % 32;

	if (enable) {
		sys_clrsetbits(reg, 1 << bit, 1 << bit);
	} else {
		sys_clrsetbits(reg, 1 << bit, 0);
	}
}

static void clk_enable(int clk_id)
{
	owl_clk_set(clk_id, 1);
}

static void clk_disable(int clk_id)
{
	owl_clk_set(clk_id, 0);
}

/*************************************************
*	soc reset module
****************************************************/
static void reset_and_enable_set(int rst_id, int assert)
{
	unsigned long reg;
	unsigned int bit;

	reg = RMU_MRCR0 + (rst_id / 32) * 4;
	bit = rst_id % 32;

	if (assert) {
		sys_clrsetbits(reg, 1 << bit, 0);
	} else {
		sys_clrsetbits(reg, 1 << bit, 1 << bit);
	}
}

static void reset_assert(int rst_id)
{
	reset_and_enable_set(rst_id, 1);
}

static void reset_deassert(int rst_id)
{
	reset_and_enable_set(rst_id, 0);
}

static void reset_and_enable(int rst_id)
{
	reset_assert(rst_id);
	ft_udelay(1);
	reset_deassert(rst_id);
}

static void reset_and_enable_clk(int rst_id, int clk_id)
{
	reset_assert(rst_id);
	clk_enable(clk_id);
	ft_udelay(1);
	reset_deassert(rst_id);
}

static uint32 timer_cnt_get(void)
{
	if ((sdk_run_mode == 0) && (fcc_test_mode != FCC_BT_ATT_MODE)) {
		return TIMER_MAX_VALUE - act_readl(T1_CNT);
	} else {
		uint32_t value;

		value = ft_get_time_ms();
		value *= 1000;
		return USEC_TO_COUNT(value);
	}
}

static unsigned int timestamp_get_us(void)
{
	return COUNT_TO_USEC(timer_cnt_get());
}

static void udelay(unsigned int us)
{
	long tmo = USEC_TO_COUNT(us);
	ulong now, last = timer_cnt_get();

	while (tmo > 0) {
		now = timer_cnt_get();
		if (now > last)	/* normal (non rollover) */
			tmo -= now - last;
		else		/* rollover */
			tmo -= TIMER_MAX_VALUE - last + now;
		last = now;
	}
}

static void mdelay(unsigned int msec)
{
    msec <<= 1;
	while (msec-- != 0){
		ft_udelay(500);
    }
}

static void timestamp_init(void)
{
	reset_deassert(CLOCK_ID_TIMER1);
	clk_enable(CLOCK_ID_TIMER1);

	act_writel(0x0, CMU_TIMER1CLK);  // hosc
	/* reset timer */
	act_writel(0x1, T1_CTL);		//stop and clear pending
	act_writel(TIMER_MAX_VALUE, T1_VAL);
	/* enable timer */
	act_writel(0x24, T1_CTL);
}

/*************************************************
*	uart module
**************************************************/
static const struct owl_uart_dev owl_uart_ports[] = {
	/*uart0*/
	{0, 0, CLOCK_ID_UART0, RESET_ID_UART0, UART0_REG_BASE, UART0_BR,
		{{GPIOX_CTL(10), 0xFFFF, 0x4025},{GPIOX_CTL(11), 0xFFFF, 0x4125},}},
	{0, 1, CLOCK_ID_UART0, RESET_ID_UART0, UART0_REG_BASE, UART0_BR,
		{{GPIOX_CTL(28), 0xFFFF, 0x4025},{GPIOX_CTL(29), 0xFFFF, 0x4125},}},
};

static const struct owl_uart_dev *match_uart_port(int id, int mfp)
{
	const struct owl_uart_dev *dev;
	int i, num;

	num = sizeof(owl_uart_ports) / sizeof(owl_uart_ports[0]);

	for (i = 0; i < num; i++) {
		dev = &owl_uart_ports[i];
		if (dev->chan == id && dev->mfp == mfp) {
			return dev;
		}
	}
	return NULL;
}

static int init_uart_port(const struct owl_uart_dev *dev, unsigned int baud)
{
	const struct uart_reg_config *cfg;
	int i;
	unsigned int div;

	for (i = 0; i < UART_REG_CONFIG_NUM; i++) {
		cfg = &dev->reg_cfg[i];
		if (cfg->mask != 0)
			sys_clrsetbits(cfg->reg, cfg->mask, cfg->val);
	}

	sys_writel(0xf0000003, dev->base + UART_CTL);
	div = CONFIG_HOSC_CLK_MHZ*1000000/baud;
	sys_writel(div | div << 16, dev->uart_clk);
	sys_writel(0xf0008003, dev->base + UART_CTL);

	return 0;
}

static void uart_init(int id, int mfp, int baud)
{
	const struct owl_uart_dev *dev;

	if(fcc_test_mode == FCC_UART_MODE) {
		config_uart_dev.chan = 0;
		config_uart_dev.mfp = 1;
		if (1 == g_uart_number) {
			config_uart_dev.clk_id = CLOCK_ID_UART1;
			config_uart_dev.rst_id = RESET_ID_UART1;
			config_uart_dev.base = UART1_REG_BASE;
			config_uart_dev.uart_clk = UART1_BR;
			config_uart_dev.reg_cfg[0].reg = GPIOX_CTL(g_uart_txio);
			config_uart_dev.reg_cfg[0].val = 0x4126;
			config_uart_dev.reg_cfg[1].reg = GPIOX_CTL(g_uart_rxio);
			config_uart_dev.reg_cfg[1].val = 0x4126;
		} else if (2 == g_uart_number) {
			config_uart_dev.clk_id = CLOCK_ID_UART2;
			config_uart_dev.rst_id = RESET_ID_UART2;
			config_uart_dev.base = UART2_REG_BASE;
			config_uart_dev.uart_clk = UART2_BR;
			config_uart_dev.reg_cfg[0].reg = GPIOX_CTL(g_uart_txio);
			config_uart_dev.reg_cfg[0].val = 0x4127;
			config_uart_dev.reg_cfg[1].reg = GPIOX_CTL(g_uart_rxio);
			config_uart_dev.reg_cfg[1].val = 0x4127;
		} else {
			config_uart_dev.clk_id = CLOCK_ID_UART0;
			config_uart_dev.rst_id = RESET_ID_UART0;
			config_uart_dev.base = UART0_REG_BASE;
			config_uart_dev.uart_clk = UART0_BR;
			config_uart_dev.reg_cfg[0].reg = GPIOX_CTL(g_uart_txio);
			config_uart_dev.reg_cfg[0].val = 0x4125;
			config_uart_dev.reg_cfg[1].reg = GPIOX_CTL(g_uart_rxio);
			config_uart_dev.reg_cfg[1].val = 0x4125;
		}
		config_uart_dev.reg_cfg[0].mask = 0xFFFF;
		config_uart_dev.reg_cfg[1].mask = 0xFFFF;
		config_uart_dev.reg_cfg[2].reg = 0;
		config_uart_dev.reg_cfg[2].mask = 0;
		config_uart_dev.reg_cfg[2].val = 0;
		dev = &config_uart_dev;
	} else {
		dev = match_uart_port(id, mfp);
		if (!dev) {
			return;
		}
	}

	clk_enable(dev->clk_id);
	reset_and_enable(dev->rst_id);
	init_uart_port(dev, baud);
	g_uart_dev = dev;
}

static void uart_puts(char *s, unsigned int len)
{
	const struct owl_uart_dev *dev = g_uart_dev;

	if (dev == NULL) {
		return;
	}

	while (len != 0) {
		while (sys_readl(dev->base + UART_STAT) & UART_STAT_UTBB);
		sys_writel(*s, dev->base + UART_TXDAT);
		s++;
		len--;
	}
}

static int uart_get(uint8_t *value)
{
	const struct owl_uart_dev *dev = g_uart_dev;

	if (dev == NULL) {
		return -1;
	}

	if (sys_readl(dev->base + UART_STAT) & UART_STAT_RFEM) {
		return -1;
	} else {
		*value = (uint8_t)act_readl(dev->base + UART_RXDAT);
		return 0;
	}
}


/*************************************************
*	printf module
****************************************************/
#define	CFG_PBSIZE		0x100
static char printbuffer[CFG_PBSIZE];

#define _SIGN     1  // 有符号
#define _ZEROPAD  2  // 0 前缀
#define _LARGE    4  // 大写

/*!
 * \brief 是否为十进制数字.
 */
#define isdigit(_c)  (('0' <= (_c)) && ((_c) <= '9'))

/*!
 * \brief 是否为小写字母.
 */
#define islower(_c)  (('a' <= (_c)) && ((_c) <= 'z'))

/*!
 * \brief 转换为大写字母.
 */
#define toupper(_c)  (islower(_c) ? (((_c) - 'a') + 'A') : (_c))

#define _putc(_str, _end, _ch)  \
do                              \
{                               \
    if (_str < _end)            \
    {				\
        *_str = _ch;            \
        _str++;                 \
    }				\
}                               \
while (0)

//抽象出来的格式化输出函数,需要注意在windows平台和unix平台对于换行符的处理是不一样的
//windows换行符要求\r\n,而unix平台换行符为\n
//因此多传入一个参数做这种控制
//%c:输出字符
//%s:输出字符串
//%x:输出十六进制数据
static int vsnprintf(char* buf, int size, uint32 linesep, const char* fmt, va_list args)
{
    char*  str = buf;
    char*  end = buf + size - 1;

    if (end < buf - 1)
        end = (char*)-1;

    for (; *fmt != '\0'; fmt++)
    {
        uint_t  flags;
        int     width;
        int     precision;

        uint_t  number;
        uint_t  base;

        char    num_str[16];
        int     num_len;
        int     sign;

        if (*fmt != '%')
        {
            if(linesep == LINESEP_FORMAT_WINDOWS)
            {
                //windows平台在\n前面插入\r
                if(*fmt == '\n')
                    _putc(str, end, '\r');
            }
            _putc(str, end, *fmt);
            continue;
        }

        fmt++;

        flags = 0;
        width = 0;
        precision = 0;
        base = 10;

        if (*fmt == '0')
        {
            flags |= _ZEROPAD;
            fmt++;
        }

        while (isdigit(*fmt))
        {
            width = (width * 10) + (*fmt - '0');
            fmt++;
        }

        if (*fmt == '.')
        {
            fmt++;

            while (isdigit(*fmt))
            {
                precision = (precision * 10) + (*fmt - '0');
                fmt++;
            }

            if (width < precision)
                width = precision;

            flags |= _ZEROPAD;
        }

        switch (*fmt)
        {
            case 'c':
            {
                uchar_t  ch = (uchar_t)va_arg(args, int);

                _putc(str, end, ch);
                continue;
            }

            case 's':
            {
                char*  s = va_arg(args, char*);

                if (s == NULL)
                {
                    char*  rodata_s = "<NULL>";
                    s = rodata_s;
                }

                while (*s != '\0')
				{
                    if(linesep == LINESEP_FORMAT_WINDOWS)
                    {
                        if( (*s == '\r') && (*(s+1) != '\n') ) 		/* 如果字串变量中碰到回车\r(0x0d)而后面没有跟着换行\n(0x0a)，加上 */
                        {
                            _putc( str, end, '\r' );
                            _putc( str, end, '\n' );
                            s++;
                        }
                        else
                        {
                            _putc(str, end, *s);
                            s++;
                        }
                    }
                    else
                    {
						_putc(str, end, *s);
                        s++;
                    }
				}

                continue;
            }

            case 'X':
                flags |= _LARGE;

            case 'x':
            case 'p':
                base = 16;
                break;

            /* integer number formats - set up the flags and "break" */
            case 'o':
                base = 8;
                break;

            case 'd':
            case 'i':
                flags |= _SIGN;

            case 'u':
                break;

            case '%':
            {
                _putc(str, end, '%');
                continue;
            }

            default:
                continue;
        }

        number = va_arg(args, uint_t);

        sign = 0;
        num_len = 0;

        if (flags & _SIGN)
        {
            if ((int)number < 0)
            {
                number = -(int)number;

                sign = '-';
                width -= 1;
            }
        }

        if (number == 0)
        {
            num_str[num_len++] = '0';
        }
        else
        {
            const char*  digits = "0123456789abcdef";

            while (number != 0)
            {
                char  ch = digits[number % base];

                if (flags & _LARGE)
                    ch = toupper(ch);

                num_str[num_len++] = ch;
                number /= base;
            }
        }

        width -= num_len;

        if (!(flags & _ZEROPAD))
        {
            while (width-- > 0)
                _putc(str, end, ' ');
        }

        if (sign != 0)
            _putc(str, end, sign);

        if (flags & _ZEROPAD)
        {
            while (width-- > 0)
                _putc(str, end, '0');
        }

        while (num_len-- > 0)
            _putc(str, end, num_str[num_len]);
    }

    if (str <= end)
        *str = '\0';

    else if (size > 0)
        *end = '\0';

    return (str - buf);
}

static void fcc_printf(const char *fmt, ...)
{
	va_list args;
	unsigned int cnt;

	if ((fcc_test_mode == FCC_UART_MODE) && fcc_uart_mode_stop_log) {
		return;
	}

	va_start(args, fmt);
	cnt = vsnprintf(printbuffer, CFG_PBSIZE, 0, fmt, args);
	va_end(args);

	if (printbuffer[cnt - 1] == '\n') {
		printbuffer[cnt - 1] = '\r';
		printbuffer[cnt] = '\n';
		cnt++;
		printbuffer[cnt] = '\0';
	}
	uart_puts(printbuffer, cnt);
}

#ifndef ATT_BUILD_ONLY
#define     HOSC_CTL_HOSC_CAP_SHIFT                                           17
#define     HOSC_CTL_HOSC_CAP_MASK                                            (0xFF<<17)

static void hosc_adjust_cap(void)
{
	unsigned char hosc_cap;
	int efuse_hosc_cap;

	//temp1=256:263 //temp1=264:271
	ft_efuse_read_32bits(8,(uint32*)&efuse_hosc_cap);
	FT_LOG_I(" efuse_8 = %x\n", efuse_hosc_cap);
	if( ( (efuse_hosc_cap>>(256+1*8-32*8)) & 0xff ) != 0)
	{
		hosc_cap = (efuse_hosc_cap>>(256+1*8-32*8)) & 0xff;
		FT_LOG_I(" efuse_hosc_cap1 = %x\n", hosc_cap);
	}else if( (  (efuse_hosc_cap>>(256+0*8-32*8)) & 0xff ) != 0)
	{
		hosc_cap = (efuse_hosc_cap>>(256+0*8-32*8)) & 0xff;
		FT_LOG_I(" efuse_hosc_cap0 = %x\n", hosc_cap);
	}else
	{
		hosc_cap = 0x64;
		FT_LOG_I(" efuse_hosc_cap_default = %x\n", hosc_cap);
	}

	IO_WRITE(HOSC_CTL,( IO_READ(HOSC_CTL) & (~(0xFF << HOSC_CTL_HOSC_CAP_SHIFT)))
				|(hosc_cap << HOSC_CTL_HOSC_CAP_SHIFT));
}

static void fcc_soc_init(void)
{
	int tmp;

	// watchdog_disable();
	*((REG32)(WD_CTL)) |= (1 << 0); //feed
	timestamp_init();
	reset_assert(RESET_ID_SPIMT0);
	reset_assert(RESET_ID_SPIMT1);
	reset_assert(RESET_ID_I2CMT0);
	reset_assert(RESET_ID_I2CMT1);

	if(fcc_test_mode == FCC_UART_MODE) {
		uart_init(0, 1, 115200);
	} else {
		uart_init(0, 1, 2000000);
	}

	FT_LOG_I("\nVer1.0.002-BT FCC (build %s %s)\n", __DATE__, __TIME__);
	FT_LOG_I("FCC Test Mode: %d\n", fcc_test_mode);
	FT_LOG_I("soc init\n");
	//hosc_adjust_cap();

	//********** CK802 RC64M, calibration // *************************
	IO_WRITE(CMU_S1CLKCTL,0x7);
	IO_WRITE(CMU_S1BTCLKCTL,0x7);
	IO_WRITE(CK64M_CTL, 0x80201400);
	ft_udelay(20);
	IO_WRITE(CK64M_CTL, 0x8020140c); 
	ft_udelay(20);
	FT_LOG_I("Start wait RC64M ready\n");
	while(!(IO_READ(CK64M_CTL)&(1<<12))); //cal done and write back
	FT_LOG_I("Wait RC64M ready finish\n");

	tmp = (IO_READ(CK64M_CTL) & (0x7f<<25))>>25;
	tmp -= 1;
	IO_WRITE(CK64M_CTL, (0x80201000 | (tmp<<4)));
	ft_udelay(20);
}
#endif

#define shareram_802_m4f_base SHARERAM_BASE_ADDR

static volatile unsigned int *ft_end, *ft_result;

static volatile unsigned int *cmd_reg_write;
static unsigned int *cmd_reg_write_addr, *cmd_reg_write_val;

static volatile unsigned int *cmd_reg_read;
static unsigned int *cmd_reg_read_addr, *cmd_reg_read_val;

static volatile unsigned int *cmd_efuse_write;
static unsigned int *cmd_efuse_write_addr, *cmd_efuse_write_val;

static volatile unsigned int *cmd_efuse_read;
static unsigned int  *cmd_efuse_read_addr, *cmd_efuse_read_val;

static volatile unsigned int *cmd_bt_test_end;

static unsigned int *cmd_rf_printf, *cmd_rf_printf_addr;

static void share_cmd_add_init(void)
{
	ft_end = (unsigned int *)(shareram_802_m4f_base + 0*4);
	ft_result = (unsigned int *)(shareram_802_m4f_base + 1*4);
	*ft_end = 0;
	*ft_result =0;

	cmd_reg_write =(unsigned int *)(shareram_802_m4f_base + 2*4);
	cmd_reg_write_addr =(unsigned int *)(shareram_802_m4f_base + 3*4);
	cmd_reg_write_val =(unsigned int *)(shareram_802_m4f_base + 4*4);
	*cmd_reg_write = 0;

	cmd_reg_read =(unsigned int *)(shareram_802_m4f_base + 5*4);
	cmd_reg_read_addr =(unsigned int *)(shareram_802_m4f_base + 6*4);
	cmd_reg_read_val =(unsigned int *)(shareram_802_m4f_base + 7*4);
	*cmd_reg_read = 0;

	cmd_efuse_write =(unsigned int *)(shareram_802_m4f_base + 8*4);
	cmd_efuse_write_addr =(unsigned int *)(shareram_802_m4f_base + 9*4);
	cmd_efuse_write_val =(unsigned int *)(shareram_802_m4f_base + 10*4);
	*cmd_efuse_write = 0;

	cmd_efuse_read =(unsigned int *)(shareram_802_m4f_base + 11*4);
	cmd_efuse_read_addr =(unsigned int *)(shareram_802_m4f_base + 12*4);
	cmd_efuse_read_val =(unsigned int *)(shareram_802_m4f_base + 13*4);
	*cmd_efuse_read = 0;

	cmd_bt_test_end =(unsigned int *)(shareram_802_m4f_base + 14*4);
	*cmd_bt_test_end = 0;

	cmd_rf_printf=(unsigned int *)(shareram_802_m4f_base + 1024*4);
	cmd_rf_printf_addr =(unsigned int *)(shareram_802_m4f_base + 1028*4);
	*cmd_rf_printf = 0;
}

//#define FCC_DEBUG_DUMP_REG	1

#ifdef FCC_DEBUG_DUMP_REG
static void fcc_test_dump_reg(void)
{
	uint32 addr, i;

	FT_LOG_I("fcc_debug_dump_reg\n");

	FT_LOG_I("CMU DIGITAL\n");
	for (i=0; i<70; i++) {
		addr = 0x40001000 + 4*i;
		FT_LOG_I("addr 0x%x = 0x%x\n", addr, *((REG32)(addr)));
	}

	FT_LOG_I("CMU ANALOG\n");
	for (i=0; i<30; i++) {
		addr = 0x40000100 + 4*i;
		FT_LOG_I("addr 0x%x = 0x%x\n", addr, *((REG32)(addr)));
	}

	FT_LOG_I("PMUVDD\n");
	for (i=0; i<19; i++) {
		addr = 0x40004000 + 4*i;
		FT_LOG_I("addr 0x%x = 0x%x\n", addr, *((REG32)(addr)));
	}

	FT_LOG_I("PMUSVCC\n");
	for (i=0; i<9; i++) {
		addr = 0x40004100 + 4*i;
		FT_LOG_I("addr 0x%x = 0x%x\n", addr, *((REG32)(addr)));
	}
}
#endif

static int fcc_test_loop(uint8_t *bt_param, uint8_t *rx_report)
{
	uint8_t value, report_flag = 0, report_cnt = 0, test_time_value = 1;
	uint32_t start_time_cnt = 0, timeout_cnt = 0, curr_time_cnt, value32;

#ifdef FCC_DEBUG_DUMP_REG
	fcc_test_dump_reg();
#endif
#ifdef TIMEOUT_LONG_SUPPORT
	uint8_t timeout_long = 0;
	uint16_t value_long = 0;
	uint32_t last_time_cnt_long = 0;
	uint32_t cur_time_cnt_long = 0;
	uint16_t sec_time = 0;
	uint32_t usec_during = 0;
#endif

	/* BT_FORCE=0, BT_PG=1,  BT_FORCE=0 */
	*((REG32)(PWRGATE_DIG)) &= (~(0x7 << 27));
	*((REG32)(PWRGATE_DIG)) |= (0x1 << 28);
	*((REG32)(RAM_DEEPSLEEP)) &= (~(0x03000000));		/* Disable BT RAM0/1 deepsleep */

	*((REG32)(RMU_MRCR1)) &= (~(1<<MRCR1_BT_HUB_RESET));
	ft_udelay(10);

	share_cmd_add_init();

	if ((sdk_run_mode == 0) && (fcc_test_mode != FCC_BT_ATT_MODE)) {
		*((REG32)(CMU_MEMCLKEN0)) = 0xffffffff;
		*((REG32)(CMU_MEMCLKEN1)) = 0xffffffff;
		*((REG32)(CMU_MEMCLKSRC0)) = 0x0;
		*((REG32)(CMU_MEMCLKSRC1)) = 0x0;
	} else {
		//bt rom/ram clock source cpu
		*((REG32)(CMU_MEMCLKSRC1)) &= ~(0x1<<16);
		ft_udelay(5);
	}

	*((REG32)(MEMORY_CTL)) |= 0x1<<4;   			// bt cpu boot from btram0

#if 0	/* Load bt bin code */
	unsigned int i, *p
	p = (unsigned int *)0x2FF20000; 			//BT_RAM0
	for(i=0;i<sizeof(code_in_btram0)/4;i++)
	{
		*(p+i) = code_in_btram0[i];
	}
#endif
	// disable btcore
	reset_assert(RESET_BTHUB);
	ft_udelay(10);

	ft_load_fcc_bin();

	ft_udelay(5);

	/******************for BT*********************************************/
	//BT clk enable
	*((REG32)(CMU_DEVCLKEN1)) |= (0x1F<<24);
	*((REG32)(CMU_S1CLKCTL)) |=(0x7<<0);
	*((REG32)(CMU_S1BTCLKCTL)) |=(0x7<<0);

	/*
	*((REG32)(CMU_MEMCLKSRC0)) |= (2<<CMU_MEMCLKSRC0_RAM11CLKSRC_SHIFT)\
								| (2<<CMU_MEMCLKSRC0_RAM12CLKSRC_SHIFT)\
								| (2<<CMU_MEMCLKSRC0_RAM13CLKSRC_SHIFT);
	*/
	*((REG32)(CMU_MEMCLKSRC0)) |= (1<<CMU_MEMCLKSRC0_SRAMCLKSRC_SHIFT);
	*((REG32)(CMU_MEMCLKSRC1)) |= (1<<CMU_MEMCLKSRC1_BTRAMCLKSRC);
	FT_LOG_I("test5:\n");

	if ((sdk_run_mode == 0) && (fcc_test_mode != FCC_BT_ATT_MODE)) {
		/* Disable gpio debug */
		(*((REG32)(DEBUGIE0))) &= ~(1<<(28));
		(*((REG32)(DEBUGIE0))) &= ~(1<<(29));
		(*((REG32)(DEBUGOE0))) &= ~(1<<(28));
		(*((REG32)(DEBUGOE0))) &= ~(1<<(29));
	}

#if 0
	/***************************************************************/
	ft_udelay(5);
	*((REG32)(JTAG_CTL)) = 0;	//disable m4f ejtag
	*((REG32)(JTAG_CTL)) |= (0x0<<JTAG_CTL_BTSWMAP_SHIFT);	//gpio14/15
	// *((REG32)(JTAG_CTL)) |= (0x01<<JTAG_CTL_BTSWMAP_SHIFT);	//gpio28/29
	// *((REG32)(JTAG_CTL)) |= (0x02<<JTAG_CTL_BTSWMAP_SHIFT);	//gpio61/62
	// *((REG32)(JTAG_CTL)) |= (0x03<<JTAG_CTL_BTSWMAP_SHIFT);	//gpio75/76
	*((REG32)(JTAG_CTL)) |= (1<<JTAG_CTL_BTSWEN);	//enable JTAG for simulation
	*((REG32)(JTAG_CTL)) |= (0x0<<JTAG_CTL_SWMAP_SHIFT);  //gpio28/29
	*((REG32)(JTAG_CTL)) |= (0x3<<JTAG_CTL_SWMAP_SHIFT);  //gpio75/76
	*((REG32)(JTAG_CTL)) |= (1<<JTAG_CTL_SWEN);
#endif

	/* Init rbuf_uart */
	rbuf_uart_init(CPU_TO_BT_ADDR);
	rbuf_uart_init(BT_TO_CPU_ADDR);
	ft_udelay(100);

	//re-enable btcore 
	reset_deassert(RESET_BTHUB);

	/* BT reset */
	//*((REG32)(RMU_MRCR1)) &= (~(1<<MRCR1_BT_HUB_RESET));
	//ft_udelay(10);
	//*((REG32)(RMU_MRCR1)) |= (1<<MRCR1_BT_HUB_RESET);

	if (fcc_test_mode == FCC_BT_TX_MODE) {
#ifndef ATT_BUILD_ONLY
		/*
		* bt_param(For tx mode):
		*					byte0 : bt_mode 0 or 1	//0: BR/EDR TEST, 1: BLE TEST
		*					byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
		*					byte2 : channel //tx channel	 (0-79)
		*					byte3 : tx_power_mode //tx power	   (0-43)
		*					byte4 : tx_mode //tx mode, DH1/DH3/DH5	  (9-19, !=12)
		*					byte5 : payload_mode //payload mode   (0-6)
		*					byte6 : excute mode // excute mode (0-2)
		*					byte7 : test time // unit : s
		*/
		if (bt_param[0] == 0) {
			rbuf_uart_write(CPU_TO_BT_ADDR, 0x32);		/* K_CMD_BT_SWITCH      0x32 */
		} else {
			rbuf_uart_write(CPU_TO_BT_ADDR, 0x31);		/* K_CMD_BLE_SWITCH    0x31 */
			FT_LOG_I("K_CMD_BLE_SWITCH %x\n", bt_param[1]);
			rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[1]);
		}

		FT_LOG_I("K_CMD_SET_FREQ %x, %x %x %x %x\n", bt_param[2],bt_param[3],bt_param[4],bt_param[5],bt_param[6]);

		rbuf_uart_write(CPU_TO_BT_ADDR, 0x02);			/* K_CMD_SET_FREQ             2 */
		rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[2]);

		rbuf_uart_write(CPU_TO_BT_ADDR, 12);			/* K_CMD_SET_TX_POWER         12 */
		rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[3]);

		rbuf_uart_write(CPU_TO_BT_ADDR, 0x9);			/* K_CMD_SET_TX_MODE          9 */
		rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[4]);

		rbuf_uart_write(CPU_TO_BT_ADDR, 0x17);			/* K_CMD_SET_PAYLOAD          0x17 */
		rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[5]);

		rbuf_uart_write(CPU_TO_BT_ADDR, 0x19);			/* K_CMD_EXCUTE               0x19 */
		rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[6]);

#ifdef TIMEOUT_LONG_SUPPORT
		value_long = bt_param[8];
		value_long = ((value_long << 8) | bt_param[7]);

		if (0 == value_long) {
			value_long = 1;
		}
		if (value_long > 0xFF) {
			timeout_long = 1;
			//
			last_time_cnt_long = timer_cnt_get();
		} else {
			timeout_long = 0;
			test_time_value = (uint8_t)value_long;
			FT_LOG_I("BT time value %d\n", test_time_value);
			timeout_cnt = USEC_TO_COUNT((test_time_value * 1000 * 1000));
			start_time_cnt = timer_cnt_get();
		}
#else
		value = bt_param[7];
		if (value == 0) {
			value = 1;
		}
		test_time_value = value;

		FT_LOG_I("BT time value %d\n", value);
		timeout_cnt = USEC_TO_COUNT((value * 1000 * 1000));
		start_time_cnt = timer_cnt_get();
#endif
#endif
	} else if (fcc_test_mode == FCC_BT_RX_MODE) {
#ifndef ATT_BUILD_ONLY
		/*
		* bt_param(For rx mode):
		*					byte0 : bt_mode 0 or 1	//0: BR/EDR TEST, 1: BLE TEST
		*					byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
		*					byte[2-5]:  access code
		*					byte6 : channel //rx channel	 (0-79)
		*					byte7 : rx_mode //rx mode (0~11, 0x10~0x12)
		*					byte8 : excute mode // excute mode, 0: one packet; 1:continue
		*					byte9 : test time // unit : s
		*/
		if (bt_param[0] == 0) {
			rbuf_uart_write(CPU_TO_BT_ADDR, 0x32);		/* K_CMD_BT_SWITCH      0x32 */

			rbuf_uart_write(CPU_TO_BT_ADDR, 0x37);		/* K_CMD_SET_LAP             0x37 */
			rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[2]);
			rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[3]);
			rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[4]);
			rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[5]);
		} else {
			rbuf_uart_write(CPU_TO_BT_ADDR, 0x31);		/* K_CMD_BLE_SWITCH    0x31 */
			FT_LOG_I("K_CMD_BLE_SWITCH %x\n", bt_param[1]);
			rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[1]);

			rbuf_uart_write(CPU_TO_BT_ADDR, 0x18);		/* K_CMD_SET_ACCESSCODE       0x18 */
			FT_LOG_I("mac 0x%x%x%x%x\n", bt_param[5],bt_param[4],bt_param[3],bt_param[2]);
			rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[2]);
			rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[3]);
			rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[4]);
			rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[5]);
		}

		rbuf_uart_write(CPU_TO_BT_ADDR, 0x02);			/* K_CMD_SET_FREQ             2 */
		FT_LOG_I("K_CMD_SET_FREQ %x\n", bt_param[6]);
		rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[6]);

		rbuf_uart_write(CPU_TO_BT_ADDR, 0x01);			/* K_CMD_SET_RX_MODE          1 */
		FT_LOG_I("K_CMD_SET_RX_MODE %x\n", bt_param[7]);
		rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[7]);

		rbuf_uart_write(CPU_TO_BT_ADDR, 0x33);			/* K_CMD_RX_EXCUTE       0x33 */
		FT_LOG_I("K_CMD_RX_EXCUTE %x\n", bt_param[8]);
		rbuf_uart_write(CPU_TO_BT_ADDR, bt_param[8]);

#ifdef TIMEOUT_LONG_SUPPORT
		value_long = bt_param[10];
		value_long = ((value_long << 8) | bt_param[9]);

		if (0 == value_long) {
			value_long = 1;
		}
		if (value_long > 0xFF) {
			timeout_long = 1;
			//
			last_time_cnt_long = timer_cnt_get();
		} else {
			timeout_long = 0;
			test_time_value = (uint8_t)value_long;
			FT_LOG_I("BT time value %d\n", test_time_value);
			timeout_cnt = USEC_TO_COUNT((test_time_value * 1000 * 1000));
			start_time_cnt = timer_cnt_get();
		}
#else
		value = bt_param[9];
		if (value == 0) {
			value = 1;
		}
		test_time_value = value;

		FT_LOG_I("BT time value %d\n", value);
		timeout_cnt = USEC_TO_COUNT((value * 1000 * 1000));
		start_time_cnt = timer_cnt_get();
#endif
#endif
	} else if (fcc_test_mode == FCC_BT_ATT_MODE) {
		/* Make bt core init and notify cpu exit loop */
		rbuf_uart_write(CPU_TO_BT_ADDR, 0xfa);		/* #define K_CMD_EXIT                 0xfa */
	}

	while(1)
	{
#ifndef ATT_BUILD_ONLY
		if (fcc_test_mode == FCC_BT_TX_MODE) {
			while (1) {
				if (rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, &value)) {
					break;
				} else {
					FT_LOG_I("BT mode rsp 0x%x\n", value);
				}
			}

	#ifdef TIMEOUT_LONG_SUPPORT
			if (1 == timeout_long) {
				//
				if (0xFFFF != value_long) {
					cur_time_cnt_long = timer_cnt_get();
					//FT_LOG_I("cur_time_cnt_long 0x%x\n",cur_time_cnt_long);
					if (cur_time_cnt_long > last_time_cnt_long) {
						usec_during += COUNT_TO_USEC(cur_time_cnt_long - last_time_cnt_long);
					} else {
						usec_during += COUNT_TO_USEC(TIMER_MAX_VALUE - last_time_cnt_long + cur_time_cnt_long);
					}
					last_time_cnt_long = cur_time_cnt_long;

					if (usec_during > (2*940*1000)) {
						sec_time += (usec_during/(940*1000));
						usec_during = 0;
						FT_LOG_I("sec_time %d.\n",sec_time);
						if (sec_time >= value_long) {
							FT_LOG_I("BT TX test time finish\n");
							break;
						}
					}
				}
			} else {
				curr_time_cnt = timer_cnt_get();
				if ((curr_time_cnt - start_time_cnt) > timeout_cnt) {
					FT_LOG_I("BT tx test time finish\n");
					break;
				}
			}
	#else
			curr_time_cnt = timer_cnt_get();
			if (test_time_value != 0xFF) {
				if ((curr_time_cnt - start_time_cnt) > timeout_cnt) {
					FT_LOG_I("BT tx test time finish\n");
					break;
				}
			}
	#endif
		} else if (fcc_test_mode == FCC_BT_RX_MODE) {
			if (report_flag == 0) {
				while (1) {
					if (rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, &value)) {
						break;
					} else {
						FT_LOG_I("BT mode rsp 0x%x\n", value);
					}
				}
			} else {
				while (1) {
					if (rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, &value)) {
						break;
					} else {
						FT_LOG_I("BT get reposrt rsp 0x%x\n", value);
						/* 1byte respone for stop,  1byte respone for get report, 16byte report */
						if (report_cnt >= 2) {
							rx_report[report_cnt - 2] = value;
						}
						report_cnt++;
					}
				}
			}

			if (report_cnt == 18) {
				FT_LOG_I("BT rx get report finish report_cnt %d\n", report_cnt);
				break;
			}

#ifdef TIMEOUT_LONG_SUPPORT
			if (1 == timeout_long) {
				//
				if (0xFFFF != value_long) {
					cur_time_cnt_long = timer_cnt_get();
					//FT_LOG_I("cur_time_cnt_long 0x%x\n",cur_time_cnt_long);
					if (cur_time_cnt_long > last_time_cnt_long) {
						usec_during += COUNT_TO_USEC(cur_time_cnt_long - last_time_cnt_long);
					} else {
						usec_during += COUNT_TO_USEC(TIMER_MAX_VALUE - last_time_cnt_long + cur_time_cnt_long);
					}
					last_time_cnt_long = cur_time_cnt_long;
					
					if (usec_during > (2*940*1000)) {
						if (1 == report_flag) {
							FT_LOG_I("BT RX get report finish\n");
							break;
						}

						sec_time += (usec_during/(940*1000));
						usec_during = 0;
						FT_LOG_I("sec_time %d.\n",sec_time);
						if (sec_time >= value_long) {
							rbuf_uart_write(CPU_TO_BT_ADDR, 0x34);		/* K_CMD_RX_EXCUTE_STOP 	 0x34 */
							rbuf_uart_write(CPU_TO_BT_ADDR, 0x35);		/* K_CMD_GET_RX_REPORT		0x35 */
							FT_LOG_I("BT RX finish and get report\n");
							report_flag = 1;
						}
					}
				}
			} else {
				curr_time_cnt = timer_cnt_get();
				if (report_flag == 0) {
					if ((curr_time_cnt - start_time_cnt) > timeout_cnt) {
						rbuf_uart_write(CPU_TO_BT_ADDR, 0x34);		/* K_CMD_RX_EXCUTE_STOP 	 0x34 */
						rbuf_uart_write(CPU_TO_BT_ADDR, 0x35);		/* K_CMD_GET_RX_REPORT		0x35 */
						FT_LOG_I("BT rx finish and get report\n");
						timeout_cnt += USEC_TO_COUNT((1000 * 1000));	/* 1s (bt rx timeout 500ms) */
						report_flag = 1;
					}
				} else {
					if ((curr_time_cnt - start_time_cnt) > timeout_cnt) {
						FT_LOG_I("BT rx get report finish\n");
						break;
					}
				}
			}
	#else
			curr_time_cnt = timer_cnt_get();
			if (report_flag == 0) {
				if ((curr_time_cnt - start_time_cnt) > timeout_cnt) {
					rbuf_uart_write(CPU_TO_BT_ADDR, 0x34);		/* K_CMD_RX_EXCUTE_STOP 	 0x34 */
					rbuf_uart_write(CPU_TO_BT_ADDR, 0x35);		/* K_CMD_GET_RX_REPORT		0x35 */
					FT_LOG_I("BT rx finish and get report\n");
					timeout_cnt += USEC_TO_COUNT((1000 * 1000));	/* 1s (bt rx timeout 500ms) */
					report_flag = 1;
				}
			} else {
				if ((curr_time_cnt - start_time_cnt) > timeout_cnt) {
					FT_LOG_I("BT rx get report finish\n");
					break;
				}
			}
	#endif
		} else if (fcc_test_mode == FCC_UART_MODE) {
			while (1) {
				if (uart_get(&value)) {
					break;
				} else {
					fcc_uart_mode_stop_log = 1;
					rbuf_uart_write(CPU_TO_BT_ADDR, value);
				}
			}

			while (1) {
				if (rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, &value)) {
					break;
				} else {
					uart_puts((char *)&value, 1);
				}
			}
		} else if (fcc_test_mode == FCC_BT_ATT_MODE) {
			/* Do nothing */
		}
#endif
		if(*cmd_reg_write == 0x88888888)
		{
			FT_LOG_I("cmd_reg_write_addr= %x\n", *cmd_reg_write_addr );
			FT_LOG_I("cmd_reg_write_val= %d\n", *cmd_reg_write_val );
			*((REG32)(*cmd_reg_write_addr)) = *cmd_reg_write_val;
			*cmd_reg_write = 0;
		}

		if(*cmd_reg_read == 0x88888888)
		{
			*cmd_reg_read_val = *((REG32)(*cmd_reg_read_addr));
			FT_LOG_I("cmd_reg_read_addr= %x\n", *cmd_reg_read_addr );
			FT_LOG_I("cmd_reg_read_val= %d\n", *cmd_reg_read_val );
			*cmd_reg_read = 0;
		}

		if(*cmd_efuse_write == 0x88888888)
		{
			FT_LOG_I("cmd_efuse_write_addr= %x\n", *cmd_efuse_write_addr );
			FT_LOG_I("cmd_efuse_write_val= %x\n", *cmd_efuse_write_val );
			ft_efuse_write_32bits( *cmd_efuse_write_val,*cmd_efuse_write_addr);
			*cmd_efuse_write = 0;
		}

		if(*cmd_efuse_read == 0x88888888)
		{
			ft_efuse_read_32bits(*cmd_efuse_read_addr, cmd_efuse_read_val);
			FT_LOG_I("cmd_efuse_read_addr= %x\n", *cmd_efuse_read_addr );
			FT_LOG_I("cmd_efuse_read_val= %x\n", *cmd_efuse_read_val );
			*cmd_efuse_read = 0;
		}

		if(*cmd_rf_printf == 0x88888888)
		{
			FT_LOG_I((const char *)cmd_rf_printf_addr);
			*cmd_rf_printf = 0;
		}

		if(*cmd_bt_test_end == 0x88888888)
		{
			FT_LOG_I("cmd_bt_test_end\n" );
			*cmd_bt_test_end = 0;
		#ifndef ATT_BUILD_ONLY
			uint8_t cnt = 0;
			while (cnt < 10) {
				// maximum wait 20ms to end, get all data.
				if (rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, &value)) {
					ft_udelay(2*1000);
				} else {
					uart_puts((char *)&value, 1);
					break;
				}

				cnt++;
			}
		#endif
			break;
		}

		*((REG32)(WD_CTL)) |= (1 << 0); //feed

		if (sdk_run_mode) {
			ft_mdelay(1);
		}
	}

#ifndef ATT_BUILD_ONLY
	if (fcc_test_mode == FCC_BT_RX_MODE) {
		if (report_cnt == 18) {
			uint8_t cnt = 0;
			int8_t rssi = 0;

			rbuf_uart_write(CPU_TO_BT_ADDR, 0x44);		/* RSSI GET             0x44 */
			while (cnt < 10) {
				// maximum wait 20ms to end, get all data.
				if (rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, &value)) {
					ft_udelay(2*1000);
				} else {
					FT_LOG_I("BT rx ack %d\n", value);
					ft_udelay(1*1000);
					rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, (uint8_t *)&rssi);
					rx_report[16] = rssi;
					FT_LOG_I("BT rx RSSI %d\n", rssi);
					break;
				}

				cnt++;
			}

			/* 1byte respone for stop,  1byte respone for get report, 16byte report */
			value32 = rx_report[0] | (rx_report[1] << 8) | (rx_report[2] << 16) | (rx_report[3] << 24);
			FT_LOG_I("BT rx err bit 0x%x\n", value32);
			value32 = rx_report[4] | (rx_report[5] << 8) | (rx_report[6] << 16) | (rx_report[7] << 24);
			FT_LOG_I("BT rx bit cnt 0x%x\n", value32);
			value32 = rx_report[8] | (rx_report[9] << 8) | (rx_report[10] << 16) | (rx_report[11] << 24);
			FT_LOG_I("BT rx err pkt 0x%x\n", value32);
			value32 = rx_report[12] | (rx_report[13] << 8) | (rx_report[14] << 16) | (rx_report[15] << 24);
			FT_LOG_I("BT rx pkt cnt 0x%x\n", value32);
		} else {
			FT_LOG_I("BT rx get report error %d\n", report_cnt);
			return FAILED;
		}
	}
#endif
	return PASSED;
}

/*************************************************
* Description:  fcc test function entry
* Input: mode	0--uart test mode, for pcba
*				1--bt tx test mode, for demo
*				2--bt rx test mode, for demo
*                           3--bt att mode, for att test
* bt_param(For tx mode):
*					byte0 : bt_mode 0 or 1  //0: BR/EDR TEST, 1: BLE TEST
*					byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
*					byte2 : channel //tx channel     (0-79)
*					byte3 : tx_power_mode //tx power       (0-43)
*					byte4 : tx_mode //tx mode, DH1/DH3/DH5    (9-19, !=12)
*					byte5 : payload_mode //payload mode   (0-6)
*					byte6 : excute mode // excute mode (0-2)
*					byte7 : test time // unit : s
* bt_param(For rx mode):
*					byte0 : bt_mode 0 or 1  //0: BR/EDR TEST, 1: BLE TEST
*					byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
*					byte[2-5]:  access code
*					byte6 : channel //rx channel     (0-79)
*					byte7 : rx_mode //rx mode (0~11, 0x10~0x12)
*					byte8 : excute mode // excute mode, 0: one packet; 1:continue
*					byte9 : test time // unit : s
* rx_report(Only for rx mode):
*					buffer[16]: 16byte report.
* Return:  0:success; 1:failed
****************************************************/
int fcc_test_main(uint8_t mode, uint8_t *bt_param, uint8_t *rx_report)
{
	int ret;

	if (global_ft_env_var.ft_load_fcc_bin == NULL) {
		return FAILED;
	}

	if (mode == FCC_BT_TX_MODE) {
		if (bt_param == NULL) {
			return FAILED;
		}
	} else if (mode == FCC_BT_RX_MODE) {
		if (bt_param == NULL || rx_report == NULL) {
			return FAILED;
		}
	} else if (mode > FCC_BT_ATT_MODE) {
		return FAILED;
	}

	if (mode == FCC_BT_ATT_MODE) {
		if ((global_ft_env_var.ft_printf == NULL) ||
			(global_ft_env_var.ft_udelay == NULL) ||
			(global_ft_env_var.ft_mdelay == NULL) ||
			(global_ft_env_var.ft_get_time_ms == NULL)) {
			return FAILED;
		}
	}

	fcc_test_mode = mode;
	fcc_uart_mode_stop_log = 0;

#ifndef ATT_BUILD_ONLY
	if ((fcc_test_mode == FCC_BT_TX_MODE) || (fcc_test_mode == FCC_BT_RX_MODE)) {
		if (global_ft_env_var.ft_printf && global_ft_env_var.ft_udelay &&
			global_ft_env_var.ft_mdelay && global_ft_env_var.ft_get_time_ms) {
			sdk_run_mode = 1;
		}
	}

	if ((sdk_run_mode == 0) && (fcc_test_mode != FCC_BT_ATT_MODE)) {
	    global_ft_env_var.ft_printf = fcc_printf;
		global_ft_env_var.ft_udelay = udelay;
		global_ft_env_var.ft_mdelay = mdelay;
	}
#endif

	if (global_ft_env_var.ft_efuse_write_32bits == NULL) {
		global_ft_env_var.ft_efuse_write_32bits = Efuse_Write_32Bits;
	}

	if (global_ft_env_var.ft_efuse_read_32bits == NULL) {
		global_ft_env_var.ft_efuse_read_32bits = Efuse_Read_32Bits;
	}

	self = &global_ft_env_var;

	if ((sdk_run_mode == 0) && (fcc_test_mode != FCC_BT_ATT_MODE)) {
	#ifndef ATT_BUILD_ONLY
		fcc_reg_bak();
		fcc_soc_init();
	#endif
	} else {
		FT_LOG_I("\nVer1.0.002-BT FCC (build %s %s)\n", __DATE__, __TIME__);
		FT_LOG_I("FCC Test Mode: %d\n", fcc_test_mode);
		if (fcc_test_mode == FCC_BT_ATT_MODE) {
			FT_LOG_I("FCC prodution test ATT mode\n");
		} else {
			FT_LOG_I("FCC prodution test with sdk running\n");
		}
		//hosc_adjust_cap();		/* sdk or att will set cap */
	}

	ret = fcc_test_loop(bt_param, rx_report);

#ifndef ATT_BUILD_ONLY
	if (fcc_test_mode != FCC_BT_ATT_MODE) {
		FT_LOG_I("BT FCC/FT test %s\n", ret ? "fail" : "pass");
	}

	if ((sdk_run_mode == 0) && (fcc_test_mode != FCC_BT_ATT_MODE)) {
		if(fcc_test_mode == FCC_UART_MODE) {
			uart_init(0, 1, 2000000);
		}
		fcc_reg_resume();
		if (1 == g_uart_number) {
			sys_write32(0xffffffff, UART1_STA);  // fix uart mode fail to exit.
		} else if (2 == g_uart_number) {
			sys_write32(0xffffffff, UART2_STA);
		} else {
			sys_write32(0xffffffff, UART0_STA);
		}
	}
#endif
	return ret ? FAILED : PASSED;
}

/*************************************************
* Description:  fcc test send cmd and parameter, only use in BT_ATT_MODE
* Input: cmd    buffer store command and paremeter;
* Input: len       command and paremeter length;
* Input: wait_finish  1: wait cmd send finish, 0: not wait;
* Return:  0:success; 1:failed
****************************************************/
int fcc_test_send_cmd(uint8_t *cmd, uint8_t len, uint8_t wait_finish)
{
	uint8_t i, value;
	uint32_t report_timeout_us = 0;
	int ret = PASSED;

	if (fcc_test_mode != FCC_BT_ATT_MODE) {
		return FAILED;
	}

	for (i=0; i<len; i++) {
		//FT_LOG_I("FCC send cmd 0x%x\n", cmd[i]);
		rbuf_uart_write(CPU_TO_BT_ADDR, cmd[i]);
	}

	if (wait_finish) {
		rbuf_uart_write(CPU_TO_BT_ADDR, 0xfa);		/* #define K_CMD_EXIT                 0xfa */
	} else {
		ft_udelay(500);
		goto send_eixt;
	}

	while (1) {
		if(*cmd_reg_write == 0x88888888)
		{
			FT_LOG_I("cmd_reg_write_addr= %x\n", *cmd_reg_write_addr);
			FT_LOG_I("cmd_reg_write_val= %d\n", *cmd_reg_write_val);
			*((REG32)(*cmd_reg_write_addr)) = *cmd_reg_write_val;
			*cmd_reg_write = 0;
		}

		if(*cmd_reg_read == 0x88888888)
		{
			*cmd_reg_read_val = *((REG32)(*cmd_reg_read_addr));
			FT_LOG_I("cmd_reg_read_addr= %x\n", *cmd_reg_read_addr);
			FT_LOG_I("cmd_reg_read_val= %d\n", *cmd_reg_read_val);
			*cmd_reg_read = 0;
		}

		if(*cmd_efuse_write == 0x88888888)
		{
			FT_LOG_I("cmd_efuse_write_addr= %x\n", *cmd_efuse_write_addr);
			FT_LOG_I("cmd_efuse_write_val= %x\n", *cmd_efuse_write_val);
			ft_efuse_write_32bits( *cmd_efuse_write_val,*cmd_efuse_write_addr);
			*cmd_efuse_write = 0;
		}

		if(*cmd_efuse_read == 0x88888888)
		{
			ft_efuse_read_32bits(*cmd_efuse_read_addr, cmd_efuse_read_val);
			FT_LOG_I("cmd_efuse_read_addr= %x\n", *cmd_efuse_read_addr);
			FT_LOG_I("cmd_efuse_read_val= %x\n", *cmd_efuse_read_val);
			*cmd_efuse_read = 0;
		}

		if(*cmd_rf_printf == 0x88888888)
		{
			FT_LOG_I((const char *)cmd_rf_printf_addr);
			*cmd_rf_printf = 0;
		}

		if(*cmd_bt_test_end == 0x88888888)
		{
			FT_LOG_I("cmd_bt_test_end\n" );
			*cmd_bt_test_end = 0;
			break;
		}

		rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, &value);		/* Drop rsp */
		ft_udelay(10);
		report_timeout_us += 10;
		if (report_timeout_us > (1000*1000)) {
			ret = FAILED;
			break;
		}
	}

send_eixt:
	while (1) {
		if (rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, &value)) {
			break;
		} else {
			//FT_LOG_I("BT rsp 0x%x\n", value);
		}
	}

	//FT_LOG_I("FCC send cmd finish\n");
	return ret;
}

/*************************************************
* Description:  fcc test clear rx buffer, only use in BT_ATT_MODE
* Return:  None
****************************************************/
void fcc_test_clear_rx_buf(void)
{
	uint8_t value;

	if (fcc_test_mode != FCC_BT_ATT_MODE) {
		return;
	}

	while (1) {
		if (rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, &value)) {
			break;
		}
	}
}

/*************************************************
* Description:  fcc test get report, only use in BT_ATT_MODE
* Input: cmd    get report command;
* Output: rx_report   buffer for output report;
* Input: report_len  get report length;
* Return:  0:success; 1:failed
****************************************************/
int fcc_test_get_report(uint8_t cmd, uint8_t *rx_report, uint8_t report_len)
{
	uint8_t report_cnt = 0, value;
	uint32_t report_timeout_us = 0;

	if (fcc_test_mode != FCC_BT_ATT_MODE) {
		return FAILED;
	}

	//FT_LOG_I("FCC get report cmd 0x%x\n", cmd);
	rbuf_uart_write(CPU_TO_BT_ADDR, cmd);

	while (1) {
		if(*cmd_reg_write == 0x88888888)
		{
			FT_LOG_I("cmd_reg_write_addr= %x\n", *cmd_reg_write_addr);
			FT_LOG_I("cmd_reg_write_val= %d\n", *cmd_reg_write_val);
			*((REG32)(*cmd_reg_write_addr)) = *cmd_reg_write_val;
			*cmd_reg_write = 0;
		}

		if(*cmd_reg_read == 0x88888888)
		{
			*cmd_reg_read_val = *((REG32)(*cmd_reg_read_addr));
			FT_LOG_I("cmd_reg_read_addr= %x\n", *cmd_reg_read_addr);
			FT_LOG_I("cmd_reg_read_val= %d\n", *cmd_reg_read_val);
			*cmd_reg_read = 0;
		}

		if(*cmd_efuse_write == 0x88888888)
		{
			FT_LOG_I("cmd_efuse_write_addr= %x\n", *cmd_efuse_write_addr);
			FT_LOG_I("cmd_efuse_write_val= %x\n", *cmd_efuse_write_val);
			ft_efuse_write_32bits( *cmd_efuse_write_val,*cmd_efuse_write_addr);
			*cmd_efuse_write = 0;
		}

		if(*cmd_efuse_read == 0x88888888)
		{
			ft_efuse_read_32bits(*cmd_efuse_read_addr, cmd_efuse_read_val);
			FT_LOG_I("cmd_efuse_read_addr= %x\n", *cmd_efuse_read_addr);
			FT_LOG_I("cmd_efuse_read_val= %x\n", *cmd_efuse_read_val);
			*cmd_efuse_read = 0;
		}

		if(*cmd_rf_printf == 0x88888888)
		{
			FT_LOG_I((const char *)cmd_rf_printf_addr);
			*cmd_rf_printf = 0;
		}

		if(*cmd_bt_test_end == 0x88888888)
		{
			FT_LOG_I("cmd_bt_test_end\n" );
			*cmd_bt_test_end = 0;
			break;
		}

		while (1) {
			if (rbuf_uart_read_no_wait(BT_TO_CPU_ADDR, &value)) {
				ft_udelay(10);
				report_timeout_us += 10;
				break;
			} else {
				//FT_LOG_I("BT get reposrt rsp 0x%x\n", value);
				/* 1byte respone for get report, report_len byte report */
				if (report_cnt >= 1) {
					rx_report[report_cnt - 1] = value;
				}
				report_cnt++;

				if (report_cnt == (report_len + 1)) {
					break;
				}
			}
		}
		if ((report_cnt == (report_len + 1)) || (report_timeout_us > (100*1000))) {
			if (report_timeout_us > (100*1000)) {
				FT_LOG_I("BT get report timeout\n");
			}
			break;
		}
	}

	if (report_cnt == (report_len + 1)) {
		//FT_LOG_I("BT rx get report %d\n", report_len);
		// uint8_t i;
		//for (i=0; i<report_len; i++) {
		//	FT_LOG_I("Report %d = 0x%x\n", i, rx_report[i]);
		//}
		return PASSED;
	} else {
		FT_LOG_I("BT rx get report error %d\n", report_cnt);
		return FAILED;
	}
}

/*************************************************
* Description:  fcc test deinit, only use in BT_ATT_MODE
* Return:  0:success; 1:failed
****************************************************/
int fcc_test_deinit(void)
{
	if (fcc_test_mode != FCC_BT_ATT_MODE) {
		return FAILED;
	}

	fcc_test_mode = FCC_BT_EXIT;
	*((REG32)(RMU_MRCR1)) &= (~(1<<MRCR1_BT_HUB_RESET));

	return PASSED;
}

int fcc_test_uart_set(uint8_t uart_number, uint8_t uart_txio, uint8_t uart_rxio)
{
	g_uart_number = uart_number;
	g_uart_txio = uart_txio;
	g_uart_rxio = uart_rxio;
	return 0;
}
