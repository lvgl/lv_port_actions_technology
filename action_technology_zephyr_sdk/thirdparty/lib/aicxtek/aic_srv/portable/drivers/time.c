#include <drivers/rtc.h>
#include <aic_portable.h>
#include <aic_time.h>
#include <aic_timer.h>
#include <soc.h>
#include <sys_clock.h>
#include <posix/time.h>

void aic_msec_to_abstime(long msec, struct timespec *abstime)
{
    struct timespec tp;
    int ret;

    AIC_ASSERT((msec >= 0) && (abstime != NULL));

    ret = clock_gettime(CLOCK_MONOTONIC, &tp);
    AIC_ASSERT(ret == 0);

    abstime->tv_sec = tp.tv_sec + msec / MSEC_PER_SEC;
    abstime->tv_nsec = tp.tv_nsec + (msec % MSEC_PER_SEC) * NSEC_PER_MSEC;
    if (abstime->tv_nsec >= NSEC_PER_SEC) {
        abstime->tv_nsec %= NSEC_PER_SEC;
        abstime->tv_sec += 1;
    }
}

void aic_msec_to_time(long msec, struct timespec *timeout)
{
    AIC_ASSERT((msec >= 0) && (timeout != NULL));

    timeout->tv_sec = msec / MSEC_PER_SEC;
    timeout->tv_nsec = (msec % MSEC_PER_SEC) * NSEC_PER_MSEC;
}

void aic_udelay(int usec)
{
    k_busy_wait(usec);
}

void aic_msleep(int time_ms)
{
    k_msleep(time_ms);
}
