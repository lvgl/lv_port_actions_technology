/*
 * Copyright (c) 2021 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: wuyufan<wuyufan@actions-semi.com>
 *
 * Change log:
 *	2021/2/17: Created by wuyufan.
 */

#include "att_pattern_test.h"

#define PRINT_BUF_SIZE STUB_ATT_RW_TEMP_BUFFER_LEN

#define _SIGN     1
#define _ZEROPAD  2
#define _LARGE    4

#define _putc(_str, _end, _ch) \
do                             \
{                              \
    *_str++ = _ch;             \
}                              \
while (0)

const char digits[16] = "0123456789abcdef";

/*!
 * \brief format output charactors with size and parameter list.
 */
static int _vsnprintf(char* buf, size_t size, const char* fmt, va_list args)
{
    char* str = buf;
    char* end = buf + size - 1;

    ARG_UNUSED(end);

    for (; *fmt != '\0'; fmt++)
    {
        uint32 flags;
        int width;

        uint32 number;
        uint32 base;

        char num_str[16];
        int num_len;
        int sign;
        uint8 ch;
        if (*fmt != '%')
        {
            _putc(str, end, *fmt);
            continue;
        }

        fmt++;

        flags = 0, width = 0, base = 10;

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

        switch (*fmt)
        {
            case 'c':
            {
                ch = (uint8)va_arg(args, int);

                _putc(str, end, ch);
                continue;
            }

            case 's':
            {
                char* s = va_arg(args, char*);

                while (*s != '\0')
                _putc(str, end, *s++);

                continue;
            }

            //case 'X':
            //    flags |= _LARGE;

            case 'x':
            //  case 'p':
            base = 16;
            break;

            case 'd':
            //  case 'i':
            flags |= _SIGN;

            //  case 'u':
            break;

            default:
            continue;
        }

        number = va_arg(args, uint32);

        sign = 0, num_len = 0;

        if (flags & _SIGN)
        {
            if ((int) number < 0)
            {
                number = -(int) number;

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

            while (number != 0)
            {
                char ch = digits[number % base];

                num_str[num_len++] = ch;
                number /= base;
            }
        }

        width -= num_len;

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

    *str = '\0';

    return (str - buf);
}


static void get_time_prefix(char *num_str)
{
    uint32_t number;
    uint8_t num_len, ch;

    number = k_uptime_get();

    num_str[13] = ' ';
    num_str[12] = ']';
    num_len = 11;
    while (number != 0) {
        ch = digits[number % 10];
        num_str[num_len--] = ch;
        number /= 10;
    }
    ch = num_len;
    while (ch > 0)
        num_str[ch--] = ' ';
    num_str[0] = '[';
    while (++num_len <= 8)
        num_str[num_len - 1] = num_str[num_len];
    num_str[8] = '.';
}

int att_buf_printf(const char * format, ...)
{
    int size, ret;
    va_list ap;
	char *p_buf;
    char att_log_buf[0x80];

	p_buf = att_log_buf;
	size = sizeof(att_log_buf);
	ret = 0;

#ifdef LOG_TO_PC_WITH_TIMESTAMP
	get_time_prefix(p_buf);
	p_buf += 14;
	size -= 14;
	ret += 14;
#endif

    va_start(ap, format);
	ret += _vsnprintf(p_buf, size, format, ap);
    va_end(ap);

	act_log_to_pc((uint8_t *)att_log_buf, ret);

	printf("%s", att_log_buf);

    return ret;
}

#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)

void print_buffer(const void *addr, int width, int count, int linelen, unsigned long disp_addr)
{
	int i, thislinelen;
	const void *data;
	/* linebuf as a union causes proper alignment */
	union linebuf {
		u32_t ui[MAX_LINE_LENGTH_BYTES/sizeof(u32_t) + 1];
		u16_t us[MAX_LINE_LENGTH_BYTES/sizeof(u16_t) + 1];
		u8_t  uc[MAX_LINE_LENGTH_BYTES/sizeof(u8_t) + 1];
	} lb;

	if (linelen * width > MAX_LINE_LENGTH_BYTES)
		linelen = MAX_LINE_LENGTH_BYTES / width;
	if (linelen < 1)
		linelen = DEFAULT_LINE_LENGTH_BYTES / width;

	if (disp_addr == -1)
		disp_addr = (unsigned long)addr;

	while (count) {
		thislinelen = linelen;
		data = (const void *)addr;

		printk("%08x:", (unsigned int)disp_addr);

		/* check for overflow condition */
		if (count < thislinelen)
			thislinelen = count;

		/* Copy from memory into linebuf and print hex values */
		for (i = 0; i < thislinelen; i++) {
			if (width == 4) {
				lb.ui[i] = *(volatile u32_t *)data;
				printk(" %08x", lb.ui[i]);
			} else if (width == 2) {
				lb.us[i] = *(volatile u16_t *)data;
				printk(" %04x", lb.us[i]);
			} else {
				lb.uc[i] = *(volatile u8_t *)data;
				printk(" %02x", lb.uc[i]);
			}
			data += width;
		}

		while (thislinelen < linelen) {
			/* fill line with whitespace for nice ASCII print */
			for (i = 0; i < width * 2 + 1; i++)
				printk(" ");
			linelen--;
		}

		/* Print data in ASCII characters */
		for (i = 0; i < thislinelen * width; i++) {
			if (lb.uc[i] < 0x20 || lb.uc[i] > 0x7e)
				lb.uc[i] = '.';
		}
		lb.uc[i] = '\0';
		printk("    %s\n", lb.uc);

		/* update references */
		addr += thislinelen * width;
		disp_addr += thislinelen * width;
		count -= thislinelen;
	}
}
