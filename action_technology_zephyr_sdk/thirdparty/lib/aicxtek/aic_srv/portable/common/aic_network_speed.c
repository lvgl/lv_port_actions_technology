/**************************************************************************/
/*                                                                        */
/* Copyright 2024 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.   */
/*                                                                        */
/**************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "aic_type.h"
#include "aic_posix.h"
#include "aic_srv_tmr.h"
#include "aic_srv_net.h"
#include "aic_portable.h"
#include "aic_net.h"

#include <aic_init.h>
#include <errno.h>
#include "aic_at_socket.h"
#include "aic_network_speed.h"
#include <aic_time.h>

#define SPEED_THREAD_STACK_SIZE 2048
#define SPEED_TASK_PRIORITY    10
#define SPEED_TEST_SERVER_PORT 5678
#define SPEED_TEST_SERVER_ADDR "222.71.206.138"
static int sock_tcp_client = -1;

static pthread_t speed_test_task;
static u8 test_recv_buf[1024];
static u8 test_send_download_test_buf[] = "test 50K";
static u8 test_send_upload_test_buf[] = "test upload";

static int64_t first_time_point = 0;
static int64_t second_time_point = 0;

static aic_network_speed_callback speed_callback = NULL;

#define BUF_SIZE 1024
static char send_data[BUF_SIZE];


static void * speed_download_test_thread(void *thread_input)
{
    int status;
    struct sockaddr_in serv_addr;
    int dl_bytes = 0;
    int dlspeed = 0;
    struct timeval send_timeout;

    send_timeout.tv_sec = 60;
    send_timeout.tv_usec = 0;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SPEED_TEST_SERVER_PORT);
    inet_pton(AF_INET, SPEED_TEST_SERVER_ADDR, &serv_addr.sin_addr);

    sock_tcp_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_tcp_client == -1) {
        printf("[speed_test_thread] IPv4 TCP client socket create fail!\n");
        speed_callback(CREATE_SOCKET_ERROR,0);
        speed_callback = NULL;
        return NULL;
    }
    setsockopt(sock_tcp_client, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));

    /* Now connect this client to the server */
    status = connect(sock_tcp_client, (struct sockaddr *)&serv_addr,
                     sizeof(serv_addr));
    if (status != OK) {
        printf("[speed_test_thread] IPv4 TCP client socket connect fail 0x%x!\n",
               status);
        speed_callback(status,0);
        speed_callback = NULL;
        status = closesocket(sock_tcp_client);
        return NULL;
    }
    status = send(sock_tcp_client, test_send_download_test_buf, strlen(test_send_download_test_buf), 0);
    if (status == ERROR) {
        printf("[speed_test_thread] IPv4 TCP client socket %d send fail %d!\n",
               sock_tcp_client, errno);
    }

    while (1) {
        if (sock_tcp_client == -1) {
            break;
        }

        status = recvfrom(sock_tcp_client, (void *)test_recv_buf,1024, 0, NULL, NULL);
        if (status < 0) {
            if (errno == EAGAIN)
                continue;
            printf("[speed_test_thread] IPv4 TCP error on recv socket %d 0x%x 0x%x\n",
                   sock_tcp_client, status, errno);
            sleep(1);
            break;
        } else {
            printf("[speed_test_thread] IPv4 TCP client %d recv %d bytes\n",
                   sock_tcp_client, status);
            dl_bytes = dl_bytes + status;
        }
    }

    /* close this client socket */
    status = closesocket(sock_tcp_client);
    if (status != ERROR)
        printf("[speed_test_thread] IPv4 TCP client socket %d closed!\n",
                  sock_tcp_client);
    else
        printf("[speed_test_thread] IPv4 TCP client socket %d closed failed!\n",
                 sock_tcp_client);
    sock_tcp_client = -1;
    second_time_point = k_uptime_get();
    int mtime =  second_time_point - first_time_point;
    dlspeed = dl_bytes*1000/mtime;
    printf("[speed_test_thread] mtime = %d,dlspeed = %d\n",mtime,dlspeed);
    if(speed_callback != NULL){
        speed_callback(SPEED_NO_ERROR,dlspeed);
        speed_callback = NULL;
    }
    return NULL;
}

static void * speed_upload_test_thread(void *thread_input)
{
    int status;
    struct sockaddr_in serv_addr;
    int ul_bytes = 0;
    int ulspeed = 0;
    struct timeval send_timeout;

    send_timeout.tv_sec = 60;
    send_timeout.tv_usec = 0;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SPEED_TEST_SERVER_PORT);
    inet_pton(AF_INET, SPEED_TEST_SERVER_ADDR, &serv_addr.sin_addr);
    
    sock_tcp_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_tcp_client == -1) {
        printf("[speed_test_thread] IPv4 TCP client socket create fail!\n");
        speed_callback(CREATE_SOCKET_ERROR,0);
        speed_callback = NULL;
        return NULL;
    }

    setsockopt(sock_tcp_client, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));

    /* Now connect this client to the server */
    status = connect(sock_tcp_client, (struct sockaddr *)&serv_addr,
                     sizeof(serv_addr));
    if (status != OK) {
        printf("[speed_test_thread] IPv4 TCP client socket connect fail 0x%x!\n",
               status);
        speed_callback(status,0);
        speed_callback = NULL;
        status = closesocket(sock_tcp_client);
        return NULL;
    }
    status = send(sock_tcp_client, test_send_upload_test_buf, strlen(test_send_upload_test_buf), 0);
    if (status == ERROR) {
        printf("[speed_test_thread] IPv4 TCP client socket %d send fail %d!\n",
               sock_tcp_client, errno);
    }
    ul_bytes += strlen(test_send_upload_test_buf);
    memset(send_data, 'A', BUF_SIZE);

    int count = 0;

    while (1) {
        if (sock_tcp_client == -1) {
            break;
        }
        count++;
        if (count == 100) {
            break;
        }
        status = send(sock_tcp_client, send_data, BUF_SIZE, 0);
        if (status <= 0) {
            break;
        }
        ul_bytes += BUF_SIZE;
    }
    status = send(sock_tcp_client, "E", 1, 0);
    ul_bytes += 1;

    /* close this client socket */
    status = closesocket(sock_tcp_client);
    if (status != ERROR)
        printf("[speed_test_thread] IPv4 TCP client socket %d closed!\n",
                  sock_tcp_client);
    else
        printf("[speed_test_thread] IPv4 TCP client socket %d closed failed!\n",
                 sock_tcp_client);
    sock_tcp_client = -1;
    second_time_point = k_uptime_get();
    int mtime =  second_time_point - first_time_point;
    ulspeed = ul_bytes*1000/mtime;
    printf("[speed_test_thread] mtime = %d,ulspeed = %d\n",mtime,ulspeed);
    if(speed_callback != NULL){
        speed_callback(SPEED_NO_ERROR,ulspeed);
        speed_callback = NULL;
    }
    return NULL;
}

int aic_network_speed_download_test(aic_network_speed_callback callback)
{
    struct sched_param param;
    pthread_attr_t attr;
    int ret;
    if (speed_callback != NULL) {
       printf("speed test is doing");
       return -1;
    } else {
        speed_callback = callback;
    }
    /* create main task. */
    pthread_attr_init(&attr);
    param.sched_priority = SPEED_TASK_PRIORITY;
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setstacksize(&attr, SPEED_THREAD_STACK_SIZE);
    ret = pthread_create(&speed_test_task, &attr, speed_download_test_thread, NULL);
    AIC_ASSERT(!ret);
    first_time_point = k_uptime_get();

    return 0;
}

int aic_network_speed_upload_test(aic_network_speed_callback callback)
{
    struct sched_param param;
    pthread_attr_t attr;
    int ret;
    if (speed_callback != NULL) {
       printf("speed test is doing");
       return -1;
    } else {
        speed_callback = callback;
    }
    /* create main task. */
    pthread_attr_init(&attr);
    param.sched_priority = SPEED_TASK_PRIORITY;
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setstacksize(&attr, SPEED_THREAD_STACK_SIZE);
    ret = pthread_create(&speed_test_task, &attr, speed_upload_test_thread, NULL);
    AIC_ASSERT(!ret);
    first_time_point = k_uptime_get();

    return 0;
}


