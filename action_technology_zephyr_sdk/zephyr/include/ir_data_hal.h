#ifndef __IR_DATA_HAL_H__
#define __IR_DATA_HAL_H__

#include <errno.h>
#include <zephyr/types.h>
#include <device.h>

#define CAPTURE_DEVICE_NAME  "capture"

/* IR mode */
#define IR_REPEAT_MODE			(0 << 8)
#define IR_CYCLE_MODE			(1 << 8)

#define IR_MASK					(0xff)
#define incode_mask				(0xff)

typedef void (*capture_notify_cb)(struct device *dev, struct input_value *val);

typedef struct {
	u8_t ucode_offset;
	u32_t ucode_mask;
	u8_t dcode_offset;
	u32_t dcode_mask;
	u8_t inucode_offset;
	u8_t indcode_offset;
	u32_t user_code;
	u8_t code_endian;
}ir_data_format_t;

enum protocol_kind {
	IR_uPD6121,
	IR_uPD6121_repeat,
	IR_9012,
	IR_9012_repeat,
	IR_RC6_dat1,
	IR_RC6_dat2,
	IR_50462,
	IR_M50560_dat1,
	IR_M50560_dat2,
	IR_RC5,
	IR_7461,
	IR_7461_repeat,
	IR_3004,
	IR_RCA,
	IR_sharp,
};

struct ir_tranfer_param {
	u32_t data;
	u16_t protocol;
	u32_t key_on_off;
};

struct capture_handle
{
	struct device *input_dev;
	capture_notify_cb capture_notify;
};

struct capture_data_handle
{
	struct capture_handle * handle;
	capture_notify_cb capture_notify;
};

struct capture_protocol
{
	uint16_t mode;/* protocol kind */
	uint32_t *data;/*!< ir data */
	uint16_t carry_rate;/* carry rate */
};

typedef struct /* remove user code and leave data */
{
	uint32_t data1_mask;
	uint32_t data0_mask;
	uint8_t data_offset;
	uint32_t addr1_mask;
	uint32_t addr0_mask;
	uint8_t addr_offset;
	uint8_t endian;
}capture_data_mask;

enum PWM_IR_KEY_VAL {
	IR_KEY_OFF,
	IR_KEY_ON,
};

enum endian_define {
	Big_Endian = 0,
	Little_Endian,
};

#ifdef capture_param_init

ir_data_format_t d_format[] = {

	{
	.ucode_offset = 16,
	.ucode_mask = 0xffff,
	.dcode_offset = 8,
	.dcode_mask = 0xff,
	.inucode_offset = incode_mask,
	.indcode_offset = 0,
	.user_code = 0x55aa,
	.code_endian = Little_Endian,
	},//nec

	{
	.ucode_offset = incode_mask,
	.ucode_mask = 0xffff,
	.dcode_offset = incode_mask,
	.dcode_mask = 0xff,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x55aa,
	.code_endian = Little_Endian,
	},//nec_repeat

	{
	.ucode_offset = 16,
	.ucode_mask = 0xffff,
	.dcode_offset = 8,
	.dcode_mask = 0xff,
	.inucode_offset = incode_mask,
	.indcode_offset = 0,
	.user_code = 0x55aa,
	.code_endian = Little_Endian,
	},//9012

	{
	.ucode_offset = incode_mask,
	.ucode_mask = 0xffff,
	.dcode_offset = incode_mask,
	.dcode_mask = 0xff,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x55aa,
	.code_endian = Little_Endian,
	},//9012_repeat

	{
	.ucode_offset = 0,
	.ucode_mask = 0xf,
	.dcode_offset = incode_mask,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x55,
	.code_endian = Big_Endian,
	},//RC-6_dat1
	
	{
	.ucode_offset = 8,
	.ucode_mask = 0xff,
	.dcode_offset = 0,
	.dcode_mask = 0xff,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x55,
	.code_endian = Big_Endian,
	},//RC-6_dat2

	{
	.ucode_offset = 8,
	.ucode_mask = 0xff,
	.dcode_offset = 0,
	.dcode_mask = 0xff,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x15,
	.code_endian = Little_Endian,
	},//50642

	{
	.ucode_offset = 0,
	.ucode_mask = 0xff,
	.dcode_offset = incode_mask,
	.dcode_mask = 0xff,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x55,
	.code_endian = Little_Endian,
	},//50560_dat1

	{
	.ucode_offset = incode_mask,
	.ucode_mask = 0xff,
	.dcode_offset = 0,
	.dcode_mask = 0xff,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x55,
	.code_endian = Little_Endian,
	},//50560_dat2

	{
	.ucode_offset = 6,
	.ucode_mask = 0x3f,
	.dcode_offset = 0,
	.dcode_mask = 0x3f,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x15,
	.code_endian = Big_Endian,
	},//RC-5

	{
	.ucode_offset = 29,
	.ucode_mask = 0x1fff,
	.dcode_offset = 8,
	.dcode_mask = 0xff,
	.inucode_offset = 16,
	.indcode_offset = 0,
	.user_code = 0x15ff,
	.code_endian = Little_Endian,
	},//7461

	{
	.ucode_offset = incode_mask,
	.ucode_mask = 0x1fff,
	.dcode_offset = incode_mask,
	.dcode_mask = 0xff,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x15,
	.code_endian = Little_Endian,
	},//7461_repeat

	{
	.ucode_offset = 6,
	.ucode_mask = 0xf,
	.dcode_offset = 0,
	.dcode_mask = 0x3f,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x6,
	.code_endian = Big_Endian,
	},//3004

	{
	.ucode_offset = 20,
	.ucode_mask = 0xf,
	.dcode_offset = 12,
	.dcode_mask = 0xff,
	.inucode_offset = 8,
	.indcode_offset = 0,
	.user_code = 0x55,
	.code_endian = Big_Endian,
	},//rca

	{
	.ucode_offset = 10,
	.ucode_mask = 0x1f,
	.dcode_offset = 0,
	.dcode_mask = 0x3ff,
	.inucode_offset = incode_mask,
	.indcode_offset = incode_mask,
	.user_code = 0x6,
	.code_endian = Little_Endian,
	},//sharp

};


#else
extern ir_data_format_t d_format[];
#endif

void capture_ir_init(capture_notify_cb cb);

int ir_dev_init();

int ir_dev_send(struct ir_tranfer_param *param);


#endif	//__IR_DATA_HAL_H__

