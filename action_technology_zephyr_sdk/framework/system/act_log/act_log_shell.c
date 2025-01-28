#include <init.h>
#include "act_log_inner.h"
#include <linker/linker-defs.h>

#ifdef CONFIG_DEBUG_ACTLOG_SHELL
#include <shell/shell.h>
#include <stdlib.h>

#define ACTLOG_PREFIX_STR	"#LOG:"
#define ACTLOG_BEGIN_STR	"BEGIN#"
#define ACTLOG_END_STR	    "END#"
#define ACTLOG_ERROR_STR	"ERROR CANNOT DUMP#"

#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)

void actlog_print_buffer(const uint8_t *addr, int width, int count, int linelen, unsigned long disp_addr)
{
    int i, thislinelen;
    const uint8_t *data;
    /* linebuf as a union causes proper alignment */
    union linebuf {
        u32_t ui[MAX_LINE_LENGTH_BYTES/sizeof(u32_t) + 1];
        u16_t us[MAX_LINE_LENGTH_BYTES/sizeof(u16_t) + 1];
        u8_t uc[MAX_LINE_LENGTH_BYTES/sizeof(u8_t) + 1];
    }lb;

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

int actlog_dump_callback(uint8_t *data, uint32_t len)
{
    uint32_t read_len = 0;
    uint32_t str_len;

    while(read_len < len) {
        if(*data) {
            str_len = actlog_strnlen(data, len - read_len);
            //flow output,do not use printk
            act_log_output_flow_write(NULL, data, str_len);
            read_len += str_len;
            data += str_len;
        } else {
            data++;
            read_len++;
        }
    }

    k_busy_wait(10);

    return 0;
}

int cmd_print_actlog(int file_id)
{
    int traverse_len;

    /* Print buffer */
    char print_buf[CONFIG_ACTLOG_LINEBUF_SIZE];

    traverse_len = act_log_backend_traverse(file_id, actlog_dump_callback, print_buf, sizeof(print_buf));

    return traverse_len;
}

static int cmd_print_stored_actlog(const struct shell *shell,
        size_t argc, char **argv)
{
    int traverse_len;

    int file_len;

    if (argc < 2) {
        goto err_ret;
    }

    uint32_t file_id = strtoul(argv[1], NULL, 16);

    shell_print(shell, "%s%s", ACTLOG_PREFIX_STR, ACTLOG_BEGIN_STR);

    traverse_len = cmd_print_actlog(file_id);

    shell_print(shell, "%s%s", ACTLOG_PREFIX_STR, ACTLOG_END_STR);

    file_len = act_log_backend_get_used_size(file_id);

    if (traverse_len) {
        shell_print(shell, "Stored actlog printed origin len %d print len %d.", file_len, traverse_len);
    } else {
        err_ret:
        shell_print(shell, "Stored actlog verification failed "
                "or there is no stored actlog.");
    }

    return 0;
}

static int cmd_erase_actlog_dump(const struct shell *shell,
        size_t argc, char **argv)
{
    int ret = 0;

    if (argc < 2) {
        goto err_ret;
    }

    uint32_t file_id = strtoul(argv[1], NULL, 16);

    ret = act_log_backend_clear(file_id);

    if (ret == 0) {
        shell_print(shell, "Stored actlog %d erased.", file_id);
    } else {
        err_ret:
        shell_print(shell, "Failed to perform erase command: %d", ret);
    }

    return 0;
}

static int cmd_query_stored_actlog(const struct shell *shell,
        size_t argc, char **argv)
{
    int ret = 0;

    if (argc < 2) {
        goto err_ret;
    }

    uint32_t file_id = strtoul(argv[1], NULL, 16);

    ret = act_log_backend_get_used_size(file_id);

    shell_print(shell, "Stored actlog found %d bytes.", ret);

    return 0;

    err_ret:
    shell_print(shell, "Failed to perform query: %d", ret);
    return 0;
}

static int cmd_set_level_actlog(const struct shell *shell,
        size_t argc, char **argv)
{
    int ret = 0;

    if (argc < 3) {
        goto err_ret;
    }

    uint32_t level = strtoul(argv[2], NULL, 16);

    ret = actlog_set_level_filter(argv[1], level);

    shell_print(shell, "Set actlog module %s level %d ret %d", argv[1], level, ret);

    return 0;

    err_ret:
    shell_print(shell, "Failed to set level: %d", ret);
    return 0;
}

static int cmd_set_num_actlog(const struct shell *shell,
        size_t argc, char **argv)
{
    int ret = 0;

    if (argc < 5) {
        goto err_ret;
    }

    uint8_t enable = strtoul(argv[2], NULL, 16);

    uint16_t log_num = strtoul(argv[3], NULL, 16);

    uint16_t limit_time = strtoul(argv[4], NULL, 16);

    ret = actlog_set_module_num_filter(argv[1], enable, log_num, limit_time);

    shell_print(shell, "Set actlog module %s enable %d max num %d time %d ret %d", argv[1], enable, log_num, limit_time, ret);

    return 0;

    err_ret:
    shell_print(shell, "Failed to set num: %d", ret);
    return 0;
}

static int cmd_get_info_actlog(const struct shell *shell,
        size_t argc, char **argv)
{
    actlog_dump_runtime_info();

    return 0;

}
void act_log_runtime_flush_enable(int enable);
static int cmd_runtime_log_enable(const struct shell *shell,
		size_t argc, char **argv)
{
	
	if (argc != 2) {
		printk("usage: rt_log  enable/disable\n");
		return -1;
	}
#ifdef CONFIG_ACTLOG_OUTPUT_BINARY
	if (!strcmp(argv[1], "enable")) {
		act_log_runtime_flush_enable(1);
	}else{
		act_log_runtime_flush_enable(0);
	}
#endif
	return 0;

}


#if 0
static int cmd_test_actlog(const struct shell *shell,
        size_t argc, char **argv)
{
    log_test();

    return 0;

}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_actlog,
        SHELL_CMD(find, NULL,
                "Query if there is a stored actlog",
                cmd_query_stored_actlog),
        SHELL_CMD(erase, NULL,
                "Erase stored actlog",
                cmd_erase_actlog_dump),
        SHELL_CMD(print, NULL,
                "Print stored actlog to shell",
                cmd_print_stored_actlog),
        SHELL_CMD(level, NULL,
                "Set level filter actlog to shell",
                cmd_set_level_actlog),
        SHELL_CMD(num, NULL,
                "Set num filter actlog to shell",
                cmd_set_num_actlog),
        SHELL_CMD(info, NULL,
                "get actlog info to shell",
                cmd_get_info_actlog),
		SHELL_CMD(rt_log, NULL,
			"runtime log enable/disable save log to flash",
			cmd_runtime_log_enable),

#if 0
        SHELL_CMD(test, NULL,
                "test actlog",
                cmd_test_actlog),
#endif
        SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(actlog, &sub_actlog,
        "actlog commands (flash partition backend)", NULL);

#endif /* CONFIG_DEBUG_COREDUMP_SHELL */
