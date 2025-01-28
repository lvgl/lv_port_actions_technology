#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "fw_version.h"
#include <crc.h>
#include <drivers/nvram_config.h>

static struct fw_version fw_ver;

static int fw_version_get_factory_ver(struct fw_version *ver)
{
	int rlen;
	struct code_res_version code_res;

	memset(&code_res, 0x0, sizeof(struct code_res_version));
	rlen = nvram_config_get_factory(FIRMWARE_VERSION, &code_res, sizeof(struct code_res_version));
	if (rlen != sizeof(struct code_res_version)) {
		printk("cannot found FIRMWARE_VERSION\n");
		code_res.version_code = ver->version_code;
		code_res.version_res = ver->version_code;
		nvram_config_set_factory(FIRMWARE_VERSION, &code_res, sizeof(struct code_res_version));
	}else{
		ver->version_code = code_res.version_code;
		ver->version_res = code_res.version_res;
	}
	return 0;
}

const struct fw_version *fw_version_get_current(void)
{
	const struct fw_version *ver =
		(struct fw_version *)soc_boot_get_fw_ver_addr();

	memcpy(&fw_ver, ver, sizeof(struct fw_version));
	fw_version_get_factory_ver(&fw_ver);

	return &fw_ver;
}

void fw_version_dump(const struct fw_version *ver)
{
	printk("***  Current Firmware Version  ***\n");
	printk(" Firmware Version: 0x%08x\n", ver->version_code);
	printk(" res Version: 0x%08x\n", ver->version_res);
	printk("   System Version: 0x%08x\n", ver->system_version_code);
	printk("     Version Name: %s\n", ver->version_name);
	printk("       Board Name: %s\n", ver->board_name);
}

int fw_version_check(const struct fw_version *ver)
{
	uint32_t checksum;

	if (ver->magic != FIRMWARE_VERSION_MAGIC)
		return -1;

	checksum = utils_crc32(0, (const u8_t *)ver, sizeof(struct fw_version) - 4);

	if (ver->checksum != checksum)
		return -1;

	return 0;
}

static int fw_version_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	const struct fw_version *ver = fw_version_get_current();

	if (fw_version_check((struct fw_version *)soc_boot_get_fw_ver_addr())) {
		printk("BAD firmware version !!!\n");
		return -1;
	}

	fw_version_dump(ver);

	return 0;
}

SYS_INIT(fw_version_init, APPLICATION, 1);
