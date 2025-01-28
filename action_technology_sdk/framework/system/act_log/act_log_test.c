#define LOG_MODULE_CUSTOMER
#include "act_log_inner.h"

LOG_MODULE_REGISTER( actlog, CONFIG_ACTLOG_LOG_LEVEL);

void log_test_compile(void)
{
    printk("log test:%d\n", __log_level);
    printk("check module %s level LOG_LEVEL_INF:%d\n", log_name_get(ACTLOG_ID_GET()),
            ACTLOG_CONST_LEVEL_CHECK(LOG_LEVEL_INF));
    SYS_LOG_INF("check test\n");
}

static const char *test_str = "0123456789abcdefghijklmnopqrstuvwxyz";

extern int cmd_print_actlog(int file_id);

void log_test(void)
{
    int i;
    char buffer[4];
    os_sleep(100);

    act_log_backend_clear(0);
    log_test_compile();
    SYS_LOG_ERR("log start\n");

#if 0
    SYS_LOG_INF("%d", 1);
    SYS_LOG_INF("%d %d", 1, 2);
    SYS_LOG_INF("%d %d %d", 1, 2, 3);
    SYS_LOG_INF("%d %d %d %d", 1, 2, 3, 4);
    SYS_LOG_INF("%d %d %d %d %d", 1, 2, 3, 4, 5);
    SYS_LOG_INF("%d %d %d %d %d %d", 1, 2, 3, 4, 5, 6);
    SYS_LOG_INF("%d %d %d %d %d %d %d\n", 1, 2, 3, 4, 5, 6, 7);
    SYS_LOG_INF("%s", "test0");
    SYS_LOG_INF("%d %s", 1, "test1");
    SYS_LOG_INF("%d %d %s", 1, 2, "test2");
    SYS_LOG_INF("%d %d %d %s", 1, 2, 3, "test3");
    SYS_LOG_INF("%d %d %d %d %s", 1, 2, 3, 4, "test4");
    SYS_LOG_INF("%d %d %d %d %d %s", 1, 2, 3, 4, 5, "test5");
    SYS_LOG_INF("%d %d %d %d %d %d %s", 1, 2, 3, 4, 5, 6, "test6");
#endif
    LOG_DBG("%s %s", "test1", "test2");
    LOG_INF("%s %s %s", "test1", "test2", "test3");
    LOG_WRN("%s %s %s %s", "test1", "test2", "test3", "test4");
    LOG_ERR("%s %s %s %s %s", "test1", "test2", "test3", "test4", "test5");
    LOG_INF("%s %s %s %s %s %s", "test1", "test2", "test3", "test4", "test5", "test6");
    LOG_INF("%s %s %s %s %s %s %s", "test1", "test2", "test3", "test4", "test5", "test6", "test7");
    buffer[0] = 't';
    buffer[1] = 'e';
    buffer[2] = 's';
    buffer[3] = '\0';
    SYS_LOG_INF("%s", buffer);

    for (i = 0; i < 1024; i++) {
        SYS_LOG_INF("test %d %s\n", i, test_str);

        os_sleep(10);
    }

    printk("dump test\n");
    cmd_print_actlog(0);
    printk("dump test end\n");
}
