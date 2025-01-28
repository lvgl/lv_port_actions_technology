#ifndef _AIC_POSIX_H
#define _AIC_POSIX_H

// app task priority of macro: begin  priority is higher if value is smaller.
#define PTHREAD_TASK_LOWESTPRI                  31
#define PTHREAD_TASK_1_PRI                      30
#define PTHREAD_TASK_2_PRI                      29
#define PTHREAD_TASK_3_PRI                      28
#define PTHREAD_TASK_4_PRI                      27
#define PTHREAD_TASK_5_PRI                      26
#define PTHREAD_TASK_6_PRI                      25
#define PTHREAD_TASK_7_PRI                      24
#define PTHREAD_TASK_8_PRI                      23
#define PTHREAD_TASK_9_PRI                      22
#define PTHREAD_TASK_10_PRI                     21
#define PTHREAD_TASK_11_PRI                     20
#define PTHREAD_TASK_12_PRI                     19
#define PTHREAD_TASK_13_PRI                     18
#define PTHREAD_TASK_14_PRI                     17
#define PTHREAD_TASK_15_PRI                     16
#define PTHREAD_TASK_16_PRI                     15
#define PTHREAD_TASK_17_PRI                     14
#define PTHREAD_TASK_18_PRI                     13
#define PTHREAD_TASK_19_PRI                     12
#define PTHREAD_TASK_20_PRI                     11
#define PTHREAD_TASK_21_PRI                     10
#define PTHREAD_TASK_22_PRI                     9
#define PTHREAD_TASK_23_PRI                     8
#define PTHREAD_TASK_24_PRI                     7
#define PTHREAD_TASK_25_PRI                     6
#define PTHREAD_TASK_26_PRI                     5
#define PTHREAD_TASK_27_PRI                     4
#define PTHREAD_TASK_28_PRI                     3
#define PTHREAD_TASK_29_PRI                     2
#define PTHREAD_TASK_30_PRI                     1
#define PTHREAD_TASK_HIGHEST_PRI                0

#include <posix/pthread.h>
#include <posix/mqueue.h>

#define PTHREAD_SET_DEFAULT

#define BASE_STACK_SIZE     (1024)

#endif /* _AIC_POSIX_H */
