#define CHIP_ID_TBL_NUM 5
struct nor_delaychain_tbl {
	uint16_t vdd_volt;
	uint8_t delay;
};
struct id_nor_delaychain_tbl {
	uint32_t chip_id;
	uint32_t max_clk; //MHZ
	const struct nor_delaychain_tbl tbl[CHIP_ID_TBL_NUM];
};


static const  struct id_nor_delaychain_tbl chipid_dl_tbl[] = {
	{
		.chip_id = 0x0, /*default use*/
		.max_clk = CONFIG_SPI_FLASH_FREQ_MHZ,
		.tbl = {
			{950, CONFIG_SPI_FLASH_DELAY_CHAIN},
			{1000, CONFIG_SPI_FLASH_DELAY_CHAIN},
			{1100, CONFIG_SPI_FLASH_DELAY_CHAIN},
			{1150, CONFIG_SPI_FLASH_DELAY_CHAIN},
			{1200, CONFIG_SPI_FLASH_DELAY_CHAIN},
		},
	},

	{
		.chip_id = 0x1840c8,	/*MD25Q128E*/
		.max_clk = CONFIG_SPI_FLASH_FREQ_MHZ,
		.tbl = {
			{950, CONFIG_SPI_FLASH_DELAY_CHAIN},
			{1000, CONFIG_SPI_FLASH_DELAY_CHAIN},
			{1100, CONFIG_SPI_FLASH_DELAY_CHAIN},
			{1150, CONFIG_SPI_FLASH_DELAY_CHAIN},
			{1200, CONFIG_SPI_FLASH_DELAY_CHAIN},
		},
	},

	{
		.chip_id = 0x1940c8,  	/*GD25Q256E*/
		.max_clk = CONFIG_SPI_FLASH_FREQ_MHZ,
		.tbl = {
			{950, 29},
			{1000, 29},
			{1100, 30},
			{1150, 30},
			{1200, 31},
		},
	},
	{
		.chip_id = 0x1a47c8,	/*GD25B512M*/
		.max_clk = 84,
		.tbl = {
			{950, 30},
			{1000, 30},
			{1100, 35},
			{1150, 35},
			{1200, 35},
		},
	},
	{
		.chip_id = 0x18405e,	/*ZB25VQ128DWJG*/
		.max_clk = CONFIG_SPI_FLASH_FREQ_MHZ,
		.tbl = {
			{950, 36},
			{1000, 38},
			{1100, 41},
			{1150, 43},
			{1200, 45},
		},
	},
	{
		.chip_id = 0x17405e,	/*ZB25VQ64CWJG*/
		.max_clk = CONFIG_SPI_FLASH_FREQ_MHZ,
		.tbl = {
			{950, 36},
			{1000, 38},
			{1100, 41},
			{1150, 43},
			{1200, 45},
		},
	},
	{
		.chip_id = 0x16405e,	/*ZB25VQ32DSJG*/
		.max_clk = CONFIG_SPI_FLASH_FREQ_MHZ,
		.tbl = {
			{950, 36},
			{1000, 38},
			{1100, 41},
			{1150, 43},
			{1200, 46},
		},
	},
};

