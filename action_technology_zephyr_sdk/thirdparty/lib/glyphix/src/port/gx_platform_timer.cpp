/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "kernel.h"
#include "sys/util.h"

#include "platform/gx_os.h"
#include "platform/gx_thread.h"

namespace gx {
namespace os {

struct k_gx_work{
    struct k_work worker;
    void (*user_callback)(void *);
    void *user_data;
};

struct k_gx_timer{
    struct k_timer timer;
    k_gx_work *worker;
    int time;
    int flag;
};

void timer_adaptor_callback(struct k_work *work) {
    struct k_gx_work *data = CONTAINER_OF(work, struct k_gx_work, worker);
    data->user_callback(data->user_data);
}

void my_timer_handler(struct k_timer *timer)
{
    struct k_gx_timer *data = CONTAINER_OF(timer, struct k_gx_timer, timer);
    k_work_submit(&data->worker->worker);
}

static k_gx_timer *timer_cast(timer_t timer) { return reinterpret_cast<k_gx_timer *>(timer); }

timer_t timer_create(int time, const char *, int flags, void (*callback)(void *), void *parameter) {
    struct k_gx_work *recv_work = new k_gx_work;
    recv_work->user_callback = callback;
    recv_work->user_data = parameter;
    recv_work->worker = Z_WORK_INITIALIZER(timer_adaptor_callback);

    struct k_gx_timer *recv_timer = new k_gx_timer;
    recv_timer->time = time;
    recv_timer->worker = recv_work;
    recv_timer->flag = flags;
    k_timer_init(&recv_timer->timer, my_timer_handler, nullptr);
    return timer_t(recv_timer);
}

void timer_delete(timer_t timer) {
    auto _timer = timer_cast(timer);
    delete _timer->worker;
    delete _timer;
 }

void timer_start(timer_t timer) {
   auto _timer = timer_cast(timer);
   if(_timer->flag == TimerFlagOneShot) {
       k_timer_start(&_timer->timer, K_MSEC(_timer->time), K_NO_WAIT);
   }
   else if (_timer->flag == TimerFlagPeriodic) {
       k_timer_start(&_timer->timer, K_MSEC(_timer->time), K_MSEC(_timer->time));
   }
}

void timer_cancel(timer_t timer) {
   auto _timer = timer_cast(timer);
   k_timer_stop(&_timer->timer);
}
} // namespace os
} // namespace gx
