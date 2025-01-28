#include <drivers/flash.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <soc.h>
#include "spi_flash.h"
extern void xspi_nor_enable_status_qe(struct spinor_info *sni);

#define SPI_CTL_AHB_REQ              (1<<8)
#define SPI_STATUS_READY             (1<<8)
#define SPI_CTL_ADDR_MODE_MASK       (1<<27)
#define SPI_CTL_ADDR_MODE_2X4X       (0<<27)
#define SPI_CTL_ADDR_MODE_DUAL_QUAD  (1<<27)

static inline void spi_delay(void)
{
	volatile int i = 100000;

	while (i--)
		;
}

static inline void spi_setup_bus_width(struct spinor_info *sni, u8_t bus_width)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;

	spi->ctrl = (spi->ctrl & ~(0x3 << 10)) | (((bus_width & 0x7) / 2 + 1) << 10);

	spi_delay();
}

__ramfunc static void _nor_set_spi_read_mode(struct spinor_info *sni)
{
	struct spi_info *si = (struct spi_info *)&sni->spi;

	if(sni->spi.bus_width == 4) {
		printk("nor is 4 line mode\n");
		xspi_nor_enable_status_qe(sni);
		/* enable 4x mode */
		spi_setup_bus_width(sni, 4);
	} else if(sni->spi.bus_width == 2) {
		printk("nor is 2 line mode\n");
		/* enable 2x mode */
		spi_setup_bus_width(sni, 2);
	} else {
		sni->spi.bus_width = 1;
		printk("nor is 1 line mode\n");
		/* enable 1x mode */
		spi_setup_bus_width(sni, 1);
	}
}

#define COMPARE_BYTES   (32)
void _nor_read_mode_try(struct spinor_info *sni, unsigned char bus_width)
{
	u8_t temp_data[COMPARE_BYTES], src_data[COMPARE_BYTES];

	printk("_nor_read_mode_try:%d bit bus width\n", bus_width);
	sni->spi.bus_width = 1;
	sni->spi.flag &= ~SPI_FLAG_SPI_4XIO;
	memset(src_data, 0xff, COMPARE_BYTES);
	p_spinor_api->read(sni, 0, src_data, COMPARE_BYTES);
	/* try io mode */
	sni->spi.bus_width = bus_width;
	sni->spi.flag |= SPI_FLAG_SPI_4XIO;

	if (sni->spi.bus_width == 4) {
		xspi_nor_enable_status_qe(sni);
	}

	memset(temp_data, 0x00, COMPARE_BYTES);
	p_spinor_api->read(sni, 0, temp_data, COMPARE_BYTES);

	if (memcmp(temp_data, src_data, COMPARE_BYTES) == 0) {
		return ;
	}

	/* try output mode */
	sni->spi.flag &= ~SPI_FLAG_SPI_4XIO;

	memset(temp_data, 0x00, COMPARE_BYTES);

	p_spinor_api->read(sni, 0, temp_data, COMPARE_BYTES);
	if (memcmp(temp_data, src_data, COMPARE_BYTES) != 0) {
		sni->spi.bus_width = 1;
	}

}

void nor_dual_quad_read_mode_try(struct spinor_info *sni)
{
	if(sni->spi.bus_width == 4) {
		_nor_read_mode_try(sni, 4);
	} else if(sni->spi.bus_width == 2) {
		_nor_read_mode_try(sni, 2);
	}

	_nor_set_spi_read_mode(sni);
}

