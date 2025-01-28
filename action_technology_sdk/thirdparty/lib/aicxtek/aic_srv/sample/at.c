/*****************************************************************************/
/*                                                                           */
/*    Copyright 2024 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.   */
/*                                                                           */
/*****************************************************************************/

/**
 * @file at_test.c
 *
 */
/*********************
 *      INCLUDES
 *********************/
#include "aic_type.h"
#include "aic_srv_at.h"
#include "aic_posix.h"

/*********************
 *      DEFINES
 *********************/
#define AT_CHANNEL_BUF_SIZE 400

static void *aic_srv_at_test_task(void* p)
{
    u32 buffer_len=0;
    static u8 recv_buffer[AT_CHANNEL_BUF_SIZE] = {0};

    while(1) {
        buffer_len = aic_srv_at_read(recv_buffer, AT_CHANNEL_BUF_SIZE);
        if (buffer_len) {
            alog_info("cmux cmux_read len = %d, recv_buffer = %s\n", buffer_len,recv_buffer);
        }
        sleep(1);
    }
    return (void *)0;
}
void aic_srv_at_test(void)
{
    pthread_attr_t attr;
    struct sched_param param;
    pthread_t cmux_at_read_task;
    int ret;

    /* create listen task. */
    pthread_attr_init(&attr);
    param.sched_priority = PTHREAD_TASK_20_PRI;
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setstacksize(&attr, BASE_STACK_SIZE);

    ret = pthread_create(&cmux_at_read_task, &attr, aic_srv_at_test_task, NULL);

}

void aic_srv_at_write_sample(void)
{
    char buffer[12] = "AT+CFUN=1\r\n";

    aic_srv_at_write(buffer);
}
