#ifndef __PORTABLE_INCLUDE_AIC_TIME_H__
#define __PORTABLE_INCLUDE_AIC_TIME_H__

#include <time.h>
#include <sys/time.h>

void aic_msec_to_abstime(long msec, struct timespec *abstime);
void aic_msec_to_time(long msec, struct timespec *timeout);

#endif /* __PORTABLE_INCLUDE_AIC_TIME_H__ */
