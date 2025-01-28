/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file os common interface
 */

#ifndef __OS_COMMON_API_H__
#define __OS_COMMON_API_H__

typedef void (*os_work_handler_t)(struct os_work_t* work);

#ifndef CONFIG_SIMULATOR
#include <zephyr.h>
#define LOG_MODULE_NAME os_api
#include <logging/log.h>
LOG_MODULE_DECLARE(os, LOG_LEVEL_INF);

#define LOG_PRINT_FUNC_NAME 1
#include <tracing/tracing.h>
#else
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <simulator_config.h>
#include "sys/slist.h"

#define LOG_PRINT_FUNC_NAME 1

#define __in_section_unique(session)

#define __aligned(align_value)

#define min(a, b) ((a) < (b)) ? (a) : (b)

#define _builtin_return_address(a)

#define __ramfunc
#define __unused

#endif


/**
 * @file
 *
 * @brief Public OS COMMON APIs.
 */

/**
 * @brief os common APIs
 * @defgroup os_common_apis OS Common APIs
 * @{
 * @}
 */

#define OS_FOREVER 				(-1)
#define OS_ANY 				NULL
#define OS_END				NULL
#define OS_NO_WAIT (0)


#ifdef CONFIG_SIMULATOR
#define OS_ORI_FOREVER K_FOREVER

#define OS_MSEC(ms) ms
#define OS_SECONDS(s) OS_MSEC(s * 1000)
#define OS_MINUTES(m) OS_SECONDS(m * 60)
#define OS_HOURS(h) OS_MINUTES(h * 60)

#if defined(_WIN64)
typedef unsigned __int64 OS_UINT_PTR;
#else
typedef _W64 unsigned int OS_UINT_PTR;
#endif // DEBUG

typedef char *os_thread_stack_t;
typedef OS_UINT_PTR os_tid_t;
typedef OS_UINT_PTR os_mutex;
typedef OS_UINT_PTR os_sem;
typedef OS_UINT_PTR os_timer;
typedef OS_UINT_PTR os_msgq;
typedef OS_UINT_PTR os_mbox;
typedef OS_UINT_PTR os_pipe;
typedef OS_UINT_PTR os_fifo;
typedef OS_UINT_PTR os_lifo;
typedef OS_UINT_PTR os_mbox_msg;
typedef OS_UINT_PTR os_timeout;
typedef unsigned long os_thread;

struct os_work_q_t
{
    /* The thread that animates the work. */
    os_thread tread_id;

    /* List of k_work items to be worked. */
    sys_slist_t pending;

    os_mutex mutex;
};

struct os_work_t
{
    /* Node to link into k_work_q pending list. */
    sys_snode_t node;

    /* The function to be invoked by the work queue thread. */
    os_work_handler_t handler;

    /* The queue on which the work item was last submitted. */
    struct os_work_q_t* queue;

    /*Currently, only delaywork is valid. */
    /*0 Normal. */
    /*1 Cancel. */
    int state;

    os_mutex mutex;
};

struct os_delayed_work_t
{
    /* The work item. */
    struct os_work_t work;

    /* Timeout used to submit work after a delay. */
    os_timeout timeout;
};

struct os_work_queue_config_t
{
    /** The name to be given to the work queue thread.
     *
     * If left null the thread will not have a name.
     */
    const char* name;
};

typedef struct os_work_q_t            os_work_q;
typedef struct os_work_t              os_work;
typedef struct os_delayed_work_t      os_delayed_work;
typedef struct os_work_queue_config_t os_work_queue_config;

/*
    void* Unified storage with int
*/
#define OS_MUTEX_DEFINE(name) os_mutex name;
#define OS_SEM_DEFINE(name, initial_count, count_limit) os_sem name;

#else
#define OS_ORI_FOREVER K_FOREVER
#define OS_MSEC(ms) K_MSEC(ms)
#define OS_SECONDS(s) K_SECONDS(s)
#define OS_MINUTES(m) K_MINUTES(m)
#define OS_HOURS(h) K_HOURS(h)
typedef k_thread_stack_t	os_thread_stack_t;
typedef k_tid_t 		os_tid_t;
typedef struct k_mutex		os_mutex;
typedef struct k_sem		os_sem;
typedef struct k_timer		os_timer;
typedef struct k_delayed_work	os_delayed_work;
typedef struct k_work		os_work;
typedef struct k_work_q		os_work_q;
typedef struct k_msgq		os_msgq;
typedef struct k_mbox		os_mbox;
typedef struct k_pipe		os_pipe;
typedef struct k_fifo		os_fifo;
typedef struct k_lifo		os_lifo;
typedef struct k_mbox_msg	os_mbox_msg;
typedef struct k_thread		os_thread;
typedef struct k_work_queue_config os_work_queue_config;
#define OS_MUTEX_DEFINE(name) K_MUTEX_DEFINE(name)
#define OS_FIFO_DEFINE(name) K_FIFO_DEFINE(name)
#define OS_SEM_DEFINE(name, initial_count, count_limit)                        \
	K_SEM_DEFINE(name, initial_count, count_limit)
#define OS_WORK_DEFINE(work, work_handler) K_WORK_DEFINE(work, work_handler)

#define OS_DELAY_WORK_DEFINE(work, work_handler)                               \
	K_DELAYED_WORK_DEFINE(work, work_handler)

#define OS_THREAD_STACK_DEFINE(name, size) K_THREAD_STACK_DEFINE(name, size)
#define OS_TIMER_DEFINE(name, expiry_fn, stop_fn)                              \
	K_TIMER_DEFINE(name, expiry_fn, stop_fn)

#define OS_MSGQ_DEFINE(q_name, q_msg_size, q_max_msgs, q_align)                \
	K_MSGQ_DEFINE(q_name, q_msg_size, q_max_msgs, q_align)
#endif

#ifdef CONFIG_USER_WORK_Q
extern os_work_q user_work_q;
#endif


/**
 * @defgroup os_mutex_apis Os Mutex APIs
 * @ingroup os_common_apis
 * @{
 */

 /**
  * @brief Create a mutex.
  *
  * @return N/A
  */
os_mutex os_mutex_create();

/**
 * @brief Initialize a mutex.
 *
 * This routine initializes a mutex object, prior to its first use.
 *
 * Upon completion, the mutex is available and does not have an owner.
 *
 * @param mutex Address of the mutex.
 *
 * @return N/A
 */
int os_mutex_init(os_mutex *mutex);

/**
 * @brief Lock a mutex.
 *
 * This routine locks @a mutex. If the mutex is locked by another thread,
 * the calling thread waits until the mutex becomes available or until
 * a timeout occurs.
 *
 * A thread is permitted to lock a mutex it has already locked. The operation
 * completes immediately and the lock count is increased by 1.
 *
 * @param mutex Address of the mutex.
 * @param timeout Waiting period to lock the mutex (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Mutex locked.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
//#define os_mutex_lock(mutex,timeout) k_mutex_lock(mutex,timeout)
int os_mutex_lock(os_mutex * mutex, int32_t timeout);
/**
 * @brief Unlock a mutex.
 *
 * This routine unlocks @a mutex. The mutex must already be locked by the
 * calling thread.
 *
 * The mutex cannot be claimed by another thread until it has been unlocked by
 * the calling thread as many times as it was previously locked by that
 * thread.
 *
 * @param mutex Address of the mutex.
 *
 * @return N/A
 */
int os_mutex_unlock(os_mutex* mutex);

/**
 * @brief Release a mutex.
 *
 * @param mutex Address of the mutex.
 *
 * @return N/A
 */
int os_mutex_release(os_mutex* mutex);


/**
 * @brief Initialize a semaphore.
 *
 * This routine initializes a semaphore object, prior to its first use.
 *
 * @param sem Address of the semaphore.
 * @param initial_count Initial semaphore count.
 * @param limit Maximum permitted semaphore count.
 *
 * @return N/A
 */

/**
 * @} end defgroup os_mutex_apis
 */

/**
 * @defgroup os_sem_apis Os Semaphore APIs
 * @ingroup os_common_apis
 * @{
 */

int os_sem_init(os_sem * sem, uint32_t initial_count, uint32_t limit);
/**
 * @brief Take a semaphore.
 *
 * This routine takes @a sem.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param sem Address of the semaphore.
 * @param timeout Waiting period to take the semaphore (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @note When porting code from the nanokernel legacy API to the new API, be
 * careful with the return value of this function. The return value is the
 * reverse of the one of nano_sem_take family of APIs: 0 means success, and
 * non-zero means failure, while the nano_sem_take family returns 1 for success
 * and 0 for failure.
 *
 * @retval 0 Semaphore taken.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
//#define os_sem_take(sem, timeout) os_sem_take(sem, timeout)
int os_sem_take(os_sem *sem, int32_t timeout);

/**
 * @brief Give a semaphore.
 *
 * This routine gives @a sem, unless the semaphore is already at its maximum
 * permitted count.
 *
 * @note Can be called by ISRs.
 *
 * @param sem Address of the semaphore.
 *
 * @return N/A
 */
void os_sem_give(os_sem *sem);

/**
 * @brief Release a semaphore.
 *
 * @param sem Address of the semaphore.
 *
 * @return N/A
 */
void os_sem_release(os_sem* sem);

/**
 * @brief Reset a semaphore's count to zero.
 *
 * This routine sets the count of @a sem to zero.
 *
 * @param sem Address of the semaphore.
 *
 * @return N/A
 */
//#define os_sem_reset(sem) k_sem_reset(sem)
int os_sem_reset(os_sem *sem);

/**
 * @brief Get a semaphore's count.
 *
 * This routine returns the current count of @a sem.
 *
 * @param sem Address of the semaphore.
 *
 * @return Current semaphore count.
 */
#define os_sem_count_get(sem) k_sem_count_get(sem)
/**
 * @} end defgroup os_sem_apis
 */

/**
 * @defgroup os_threads_apis Threads APIs
 * @ingroup os_common_apis
 * @{
 */

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration
 * milliseconds.
 *
 * @param duration Number of milliseconds to sleep.
 *
 * @return N/A
 */
//#define os_sleep(duration) k_sleep(duration)
int32_t os_sleep(int timeout);
/**
 * @brief Cause the current thread to busy wait.
 *
 * This routine causes the current thread to execute a "do nothing" loop for
 * @a usec_to_wait microseconds.
 *
 * @return N/A
 */
#define os_delay(usec_to_wait) k_busy_wait(usec_to_wait)

/**
 * @brief Yield the current thread.
 *
 * This routine causes the current thread to yield execution to another
 * thread of the same or higher priority. If there are no other ready threads
 * of the same or higher priority, the routine returns immediately.
 *
 * @return N/A
 */
void os_yield();
/**
 * @brief Lock the scheduler.
 *
 * This routine prevents the current thread from being preempted by another
 * thread by instructing the scheduler to treat it as a cooperative thread.
 * If the thread subsequently performs an operation that makes it unready,
 * it will be context switched out in the normal manner. When the thread
 * again becomes the current thread, its non-preemptible status is maintained.
 *
 * This routine can be called recursively.
 *
 * @note os_sched_lock() and os_sched_unlock() should normally be used
 * when the operation being performed can be safely interrupted by ISRs.
 * However, if the amount of processing involved is very small, better
 * performance may be obtained by using irq_lock() and irq_unlock().
 *
 * @return N/A
 */
void os_sched_lock();

int os_irq_lock(void);

void os_irq_unlock(int key);

/**
 * @brief Unlock the scheduler.
 *
 * This routine reverses the effect of a previous call to os_sched_lock().
 * A thread must call the routine once for each time it called os_sched_lock()
 * before the thread becomes preemptible.
 *
 * @return N/A
 */
void os_sched_unlock();

/**
 * @brief Start a work queue
 *
 * @param work_q Address of workqueue.
 * @param stack Pointer to the stack space.
 * @param stack_size Stack size in bytes.
 * @param prio  workqueue thread priority.
 * @param cfg  workqueue config.
 *
 * @return N/A
 */
void os_work_queue_start(os_work_q* queue, os_thread_stack_t* stack,
    size_t stack_size, int prio, const os_work_queue_config* cfg);

/**
 * @brief Initialize a work item.
 *
 * This routine initializes a workqueue work item, prior to its first use.
 *
 * @param work Address of work item.
 * @param handler Function to invoke each time work item is processed.
 *
 * @return N/A
 */
void os_work_init(os_work *work, os_work_handler_t handler);

/**
 * @brief Submit a work item.
 *
 * This routine submits work item @a work to be processed by workqueue
 * @a work_q. If the work item is already pending in the workqueue's queue
 * as a result of an earlier submission, this routine has no effect on the
 * work item. If the work item has already been processed, or is currently
 * being processed, its work is considered complete and the work item can be
 * resubmitted.
 *
 * @warning
 * A submitted work item must not be modified until it has been processed
 * by the workqueue.
 *
 * @note Can be called by ISRs.
 *
 * @param work_q Address of workqueue.
 * @param work Address of work item.
 *
 * @return N/A
 */
void os_work_submit_to_queue(os_work_q *work_q, os_work *work);

/**
 * @brief Check if a work item is pending.
 *
 * This routine indicates if work item @a work is pending in a workqueue's
 * queue.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of work item.
 *
 * @return 1 if work item is pending, or 0 if it is not pending.
 */
#define os_work_pending(work) k_work_pending(work)

/**
 * @brief Start a workqueue.
 *
 * This routine starts workqueue @a work_q. The workqueue spawns its work
 * processing thread, which runs forever.
 *
 * @param work_q Address of workqueue.
 * @param stack Pointer to work queue thread's stack space.
 * @param stack_size Size of the work queue thread's stack (in bytes).
 * @param prio Priority of the work queue's thread.
 *
 * @return N/A
 */
#define os_work_q_start(work_q, stack, stack_size, prio) \
			k_work_q_start(work_q, stack, stack_size, prio)

/**
 * @brief Initialize a delayed work item.
 *
 * This routine initializes a workqueue delayed work item, prior to
 * its first use.
 *
 * @param work Address of delayed work item.
 * @param handler Function to invoke each time work item is processed.
 *
 * @return N/A
 */

void os_delayed_work_init(os_delayed_work *dwork, os_work_handler_t handler);
/**
 * @brief Submit a delayed work item.
 *
 * This routine schedules work item @a work to be processed by workqueue
 * @a work_q after a delay of @a delay milliseconds. The routine initiates
 * an asychronous countdown for the work item and then returns to the caller.
 * Only when the countdown completes is the work item actually submitted to
 * the workqueue and becomes pending.
 *
 * Submitting a previously submitted delayed work item that is still
 * counting down cancels the existing submission and restarts the countdown
 * using the new delay. If the work item is currently pending on the
 * workqueue's queue because the countdown has completed it is too late to
 * resubmit the item, and resubmission fails without impacting the work item.
 * If the work item has already been processed, or is currently being processed,
 * its work is considered complete and the work item can be resubmitted.
 *
 * @warning
 * A delayed work item must not be modified until it has been processed
 * by the workqueue.
 *
 * @note Can be called by ISRs.
 *
 * @param work_q Address of workqueue.
 * @param work Address of delayed work item.
 * @param delay Delay before submitting the work item (in milliseconds).
 *
 * @retval 0 Work item countdown started.
 * @retval -EINPROGRESS Work item is already pending.
 * @retval -EINVAL Work item is being processed or has completed its work.
 * @retval -EADDRINUSE Work item is pending on a different workqueue.
 */
int os_delayed_work_submit_to_queue(os_work_q *work_q, os_delayed_work *dwork, int32_t delay);
/**
 * @brief Cancel a delayed work item.
 *
 * This routine cancels the submission of delayed work item @a work.
 * A delayed work item can only be cancelled while its countdown is still
 * underway.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of delayed work item.
 *
 * @retval 0 Work item countdown cancelled.
 * @retval -EINPROGRESS Work item is already pending.
 * @retval -EINVAL Work item is being processed or has completed its work.
 */

int os_delayed_work_cancel(os_delayed_work *dwork);
/**
 * @brief Submit a work item to the system workqueue.
 *
 * This routine submits work item @a work to be processed by the system
 * workqueue. If the work item is already pending in the workqueue's queue
 * as a result of an earlier submission, this routine has no effect on the
 * work item. If the work item has already been processed, or is currently
 * being processed, its work is considered complete and the work item can be
 * resubmitted.
 *
 * @warning
 * Work items submitted to the system workqueue should avoid using handlers
 * that block or yield since this may prevent the system workqueue from
 * processing other work items in a timely manner.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of work item.
 *
 * @return N/A
 */
//#define os_work_submit(work) k_work_submit(work)
void os_work_submit(os_work *work);
/**
 * @brief Submit a delayed work item to the system workqueue.
 *
 * This routine schedules work item @a work to be processed by the system
 * workqueue after a delay of @a delay milliseconds. The routine initiates
 * an asychronous countdown for the work item and then returns to the caller.
 * Only when the countdown completes is the work item actually submitted to
 * the workqueue and becomes pending.
 *
 * Submitting a previously submitted delayed work item that is still
 * counting down cancels the existing submission and restarts the countdown
 * using the new delay. If the work item is currently pending on the
 * workqueue's queue because the countdown has completed it is too late to
 * resubmit the item, and resubmission fails without impacting the work item.
 * If the work item has already been processed, or is currently being processed,
 * its work is considered complete and the work item can be resubmitted.
 *
 * @warning
 * Work items submitted to the system workqueue should avoid using handlers
 * that block or yield since this may prevent the system workqueue from
 * processing other work items in a timely manner.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of delayed work item.
 * @param delay Delay before submitting the work item (in milliseconds).
 *
 * @retval 0 Work item countdown started.
 * @retval -EINPROGRESS Work item is already pending.
 * @retval -EINVAL Work item is being processed or has completed its work.
 * @retval -EADDRINUSE Work item is pending on a different workqueue.
 */

int os_delayed_work_submit(os_delayed_work *dwork, int32_t delay);

/**
 * @brief Get time remaining before a delayed work gets scheduled.
 *
 * This routine computes the (approximate) time remaining before a
 * delayed work gets executed. If the delayed work is not waiting to be
 * schedules, it returns zero.
 *
 * @param work Delayed work item.
 *
 * @return Remaining time (in milliseconds).
 */
#define os_delayed_work_remaining_get(work) k_delayed_work_remaining_get(work)

/**
 * @brief Get a thread's priority.
 *
 * This routine gets the priority of @a thread.
 *
 * @param osthread ID of thread whose priority is needed.
 *
 * @return Priority of @a thread, always return -15 ~ 15, not windows define.
 *
 */
int os_thread_priority_get(int thread_id);
/**
 * @brief Set a thread's priority.
 *
 * This routine immediately changes the priority of @a thread.
 *
 * Rescheduling can occur immediately depending on the priority @a thread is
 * set to:
 *
 * - If its priority is raised above the priority of the caller of this
 * function, and the caller is preemptible, @a thread will be scheduled in.
 *
 * - If the caller operates on itself, it lowers its priority below that of
 * other threads in the system, and the caller is preemptible, the thread of
 * highest priority will be scheduled in.
 *
 * Priority can be assigned in the range of -CONFIG_NUM_COOP_PRIORITIES to
 * CONFIG_NUM_PREEMPT_PRIORITIES-1, where -CONFIG_NUM_COOP_PRIORITIES is the
 * highest priority.
 *
 * @param osthread ID of thread whose priority is to be set.
 * @param prio New priority, -15 ~ 15, not windows define.
 *
 * @warning Changing the priority of a thread currently involved in mutex
 * priority inheritance may result in undefined behavior.
 *
 * @return N/A
 */
int os_thread_priority_set(int thread_id, int prio);

const char *os_thread_get_name_by_prio(int prio);

/**
 * @brief Suspend a thread.
 *
 * This routine prevents the kernel scheduler from making @a thread the
 * current thread. All other internal operations on @a thread are still
 * performed; for example, any timeout it is waiting on keeps ticking,
 * kernel objects it is waiting on are still handed to it, etc.
 *
 * If @a thread is already suspended, the routine has no effect.
 *
 * @param osthread ID of thread to suspend.
 *
 * @return N/A
 */
#define os_thread_suspend(osthread) k_thread_suspend(osthread)


/**
 * @brief Resume a suspended thread.
 *
 * This routine allows the kernel scheduler to make @a thread the current
 * thread, when it is next eligible for that role.
 *
 * If @a osthread is not currently suspended, the routine has no effect.
 *
 * @param osthread ID of thread to resume.
 *
 * @return N/A
 */
#define os_thread_resume(osthread)	k_thread_resume(osthread)

/**
 * @brief Get thread ID of the current thread.
 *
 * @return ID of current thread.
 */
//#define os_current_get() k_current_get()
uint32_t os_current_get(void);
/**
 * @brief Cancel thread performing a delayed start.
 *
 * This routine prevents @a thread from executing if it has not yet started
 * execution. The thread must be re-spawned before it will execute.
 *
 * @param osthread ID of thread to cancel.
 *
 * @retval 0 osthread spawning cancelled.
 * @retval -EINVAL Thread has already started executing.
 */

void os_thread_cancel(int thread_id);
/**
 * @brief Abort a thread.
 *
 * This routine permanently stops execution of @a thread. The thread is taken
 * off all kernel queues it is part of (i.e. the ready queue, the timeout
 * queue, or a kernel object wait queue). However, any kernel resources the
 * thread might currently own (such as mutexes or memory blocks) are not
 * released. It is the responsibility of the caller of this routine to ensure
 * all necessary cleanup is performed.
 *
 * @param osthread ID of thread to abort.
 *
 * @return N/A
 */
void os_thread_abort(int thread_id);

/**
 * @brief Set current thread name
 *
 * Set the name of the thread to be used when @option{CONFIG_THREAD_MONITOR}
 * is enabled for tracing and debugging.
 *
 * @param thread_id Thread to set name, or NULL to set the current thread
 * @param value Name string
 * @retval 0 on success
 * @retval -EFAULT Memory access error with supplied string
 * @retval -ENOSYS Thread name configuration option not enabled
 * @retval -EINVAL Thread name too long
 */
int os_thread_name_set(int thread_id, const char * value);

/**
 * @brief Create a thread.
 *
 * This routine initializes a thread, then schedules it for execution.
 *
 * The new thread may be scheduled for immediate execution or a delayed start.
 * If the newly spawned thread does not have a delayed start the kernel
 * scheduler may preempt the current thread to allow the new thread to
 * execute.
 *
 * Thread options are architecture-specific, and can include K_ESSENTIAL,
 * K_FP_REGS, and K_SSE_REGS. Multiple options may be specified by separating
 * them using "|" (the logical OR operator).
 *
 * Historically, users often would use the beginning of the stack memory region
 * to store the struct k_thread data, although corruption will occur if the
 * stack overflows this region and stack protection features may not detect this
 * situation.
 *
 * @param new_thread Pointer to uninitialized struct k_thread
 * @param stack Pointer to the stack space.
 * @param stack_size Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread priority, -15 ~ 15, not windows define.
 * @param options Thread options.
 * @param delay Scheduling delay (in milliseconds), or K_NO_WAIT (for no delay).
 *
 * @return ID of new thread.
 */

int os_thread_create(char* stack, size_t stack_size,
    void (*entry)(void*, void*, void*),
    void* p1, void* p2, void* p3,
    int prio, uint32_t options, int delay);

/**
 * @brief prepare thread terminal.
 *
 * This routine prepare thread terminal.
 *
 * @param tid Pointer to terminal struct k_thread
 *
 * @return 0 ok others failed.
 */
int os_thread_prepare_terminal(int tid);

/**
 * @brief wait thread terminal.
 *
 * This routine wait thread terminal.
 *
 * @param tid Pointer to terminal struct k_thread
 *
 * @return 0 ok others failed.
 */
int os_thread_wait_terminal(int tid);

/**
 * @} end defgroup os_threads_apis
 */
/**
 * @defgroup os_pipe_apis Fipe APIs
 * @ingroup os_common_apis
 * @{
 */

/**
 * @brief Initialize a pipe.
 *
 * This routine initializes a pipe object, prior to its first use.
 *
 * @param pipe Address of the pipe.
 * @param buffer Address of the pipe's ring buffer, or NULL if no ring buffer
 *               is used.
 * @param size Size of the pipe's ring buffer (in bytes), or zero if no ring
 *             buffer is used.
 *
 * @return N/A
 */
#define os_pipe_init(pipe, buffer, size) k_pipe_init(pipe, buffer, size)

/**
 * @brief Write data to a pipe.
 *
 * This routine writes up to @a bytes_to_write bytes of data to @a pipe.
 *
 * @param pipe Address of the pipe.
 * @param data Address of data to write.
 * @param bytes_to_write Size of data (in bytes).
 * @param bytes_written Address of area to hold the number of bytes written.
 * @param min_xfer Minimum number of bytes to write.
 * @param timeout Waiting period to wait for the data to be written (in
 *                milliseconds), or one of the special values K_NO_WAIT
 *                and K_FOREVER.
 *
 * @retval 0 At least @a min_xfer bytes of data were written.
 * @retval -EIO Returned without waiting; zero data bytes were written.
 * @retval -EAGAIN Waiting period timed out; between zero and @a min_xfer
 *                 minus one data bytes were written.
 */
#define os_pipe_put(pipe, data, bytes_to_write, bytes_written, min_xfer, timeout) \
			k_pipe_put(pipe, data, bytes_to_write, bytes_written, min_xfer, timeout)

/**
 * @brief Read data from a pipe.
 *
 * This routine reads up to @a bytes_to_read bytes of data from @a pipe.
 *
 * @param pipe Address of the pipe.
 * @param data Address to place the data read from pipe.
 * @param bytes_to_read Maximum number of data bytes to read.
 * @param bytes_read Address of area to hold the number of bytes read.
 * @param min_xfer Minimum number of data bytes to read.
 * @param timeout Waiting period to wait for the data to be read (in
 *                milliseconds), or one of the special values K_NO_WAIT
 *                and K_FOREVER.
 *
 * @retval 0 At least @a min_xfer bytes of data were read.
 * @retval -EIO Returned without waiting; zero data bytes were read.
 * @retval -EAGAIN Waiting period timed out; between zero and @a min_xfer
 *                 minus one data bytes were read.
 */
#define os_pipe_get(pipe, data, bytes_to_read, bytes_read, min_xfer, timeout) \
			k_pipe_get(pipe, data, bytes_to_read, bytes_read, min_xfer, timeout)

/**
 * @brief Write memory block to a pipe.
 *
 * This routine writes the data contained in a memory block to @a pipe.
 * Once all of the data in the block has been written to the pipe, it will
 * free the memory block @a block and give the semaphore @a sem (if specified).
 *
 * @param pipe Address of the pipe.
 * @param block Memory block containing data to send
 * @param size Number of data bytes in memory block to send
 * @param sem Semaphore to signal upon completion (else NULL)
 *
 * @return N/A
 */
#define os_pipe_block_put(pipe, block, size, sem) \
           k_pipe_block_put(pipe, block, size, sem)

/**
 * @} end defgroup os_pipe_apis
 */
/**
 * @defgroup os_fifo_apis Fifo APIs
 * @ingroup os_common_apis
 * @{
 */

/**
 * @brief Initialize a fifo.
 *
 * This routine initializes a fifo object, prior to its first use.
 *
 * @param fifo Address of the fifo.
 *
 * @return N/A
 */
#define os_fifo_init(fifo) k_fifo_init(fifo)

/**
 * @brief Add an element to a fifo.
 *
 * This routine adds a data item to @a fifo. A fifo data item must be
 * aligned on a 4-byte boundary, and the first 32 bits of the item are
 * reserved for the kernel's use.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the fifo.
 * @param data Address of the data item.
 *
 * @return N/A
 */
#define os_fifo_put(fifo, data) k_fifo_put(fifo, data)

/**
 * @brief Calculate element in a fifo.
 */
#define os_fifo_cnt_sum(fifo) k_fifo_cnt_sum(fifo)

/**
 * @brief Get an element from a fifo.
 *
 * This routine removes a data item from @a fifo in a "first in, first out"
 * manner. The first 32 bits of the data item are reserved for the kernel's use.
 *
 * @note Can be called by ISRs, but @a timeout must be set to OS_NO_WAIT.
 *
 * @param fifo Address of the fifo.
 * @param timeout Waiting period to obtain a data item (in milliseconds),
 *                or one of the special values OS_NO_WAIT and OS_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
//#define os_fifo_get(fifo, timeout) k_fifo_get(fifo, timeout)
void * os_fifo_get(os_fifo *fifo, int32_t timeout);

/**
 * @brief Query a fifo to see if it has data available.
 *
 * Note that the data might be already gone by the time this function returns
 * if other threads is also trying to read from the fifo.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the fifo.
 *
 * @return Non-zero if the fifo is empty.
 * @return 0 if data is available.
 */
#define os_fifo_is_empty(fifo) k_fifo_is_empty(fifo)

/**
 * @} end defgroup os_fifo_apis
 */

/**
 * @defgroup os_mailbox_apis Mailbox APIs
 * @ingroup os_common_apis
 * @{
 */
/**
 * @brief Initialize a mailbox.
 *
 * This routine initializes a mailbox object, prior to its first use.
 *
 * @param mbox Address of the mailbox.
 *
 * @return N/A
 */
#define os_mbox_init(mbox) k_mbox_init(mbox)

/**
 * @brief Send a mailbox message in a synchronous manner.
 *
 * This routine sends a message to @a mbox and waits for a receiver to both
 * receive and process it. The message data may be in a buffer, in a memory
 * pool block, or non-existent (i.e. an empty message).
 *
 * @param mbox Address of the mailbox.
 * @param tx_msg Address of the transmit message descriptor.
 * @param timeout Waiting period for the message to be received (in
 *                milliseconds), or one of the special values OS_NO_WAIT
 *                and OS_FOREVER. Once the message has been received,
 *                this routine waits as long as necessary for the message
 *                to be completely processed.
 *
 * @retval 0 Message sent.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
#define os_mbox_put(mbox, tx_msg, timeout) k_mbox_put(mbox, tx_msg, timeout)

/**
 * @brief Send a mailbox message in an asynchronous manner.
 *
 * This routine sends a message to @a mbox without waiting for a receiver
 * to process it. The message data may be in a buffer, in a memory pool block,
 * or non-existent (i.e. an empty message). Optionally, the semaphore @a sem
 * will be given when the message has been both received and completely
 * processed by the receiver.
 *
 * @param mbox Address of the mailbox.
 * @param tx_msg Address of the transmit message descriptor.
 * @param sem Address of a semaphore, or NULL if none is needed.
 *
 * @return N/A
 */
#define os_mbox_async_put(mbox, tx_msg, sem) k_mbox_async_put(mbox, tx_msg, sem)

/**
 * @brief Receive a mailbox message.
 *
 * This routine receives a message from @a mbox, then optionally retrieves
 * its data and disposes of the message.
 *
 * @param mbox Address of the mailbox.
 * @param rx_msg Address of the receive message descriptor.
 * @param buffer Address of the buffer to receive data, or NULL to defer data
 *               retrieval and message disposal until later.
 * @param timeout Waiting period for a message to be received (in
 *                milliseconds), or one of the special values OS_NO_WAIT
 *                and OS_FOREVER.
 *
 * @retval 0 Message received.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
#define os_mbox_get(mbox, rx_msg, buffer, timeout) \
			k_mbox_get(mbox, rx_msg, buffer, timeout)

/**
 * @brief clear all message in mbox.
 *
 * This routine clear all message in mbox , if async message is send to many than
 * max async message num ,drop all message send before.
 *
 * @param mbox Address of the mailbox.
 *
 * @retval void
 */

#define os_mbox_clear_msg(mbox) \
			k_mbox_clear_msg(mbox)

/**
 * @} end defgroup os_mailbox_apis
 */

/**
 * @defgroup os_msgq_apis Msgq APIs
 * @ingroup os_common_apis
 * @{
 */

/**
 * @brief Initialize a message queue.
 *
 * This routine initializes a message queue object, prior to its first use.
 *
 * The message queue's ring buffer must contain space for @a max_msgs messages,
 * each of which is @a msg_size bytes long. The buffer must be aligned to an
 * N-byte boundary, where N is a power of 2 (i.e. 1, 2, 4, ...). To ensure
 * that each message is similarly aligned to this boundary, @a q_msg_size
 * must also be a multiple of N.
 *
 * @param q Address of the message queue.
 * @param buffer Pointer to ring buffer that holds queued messages.
 * @param msg_size Message size (in bytes).
 * @param max_msgs Maximum number of messages that can be queued.
 *
 * @return N/A
 */
static inline void os_msgq_init(os_msgq *q, char *buffer, size_t msg_size,
				 uint32_t max_msgs)
{
	//k_msgq_init(q, buffer, msg_size, max_msgs);
}

/**
 * @brief Release allocated buffer for a queue
 *
 * Releases memory allocated for the ring buffer.
 *
 * @param msgq message queue to cleanup
 *
 * @retval 0 on success
 * @retval -EBUSY Queue not empty
 */
static inline int os_msgq_cleanup(os_msgq *msgq)
{
	//return k_msgq_cleanup(msgq);
}

/**
 * @brief Send a message to a message queue.
 *
 * This routine sends a message to message queue @a q.
 *
 * @note Can be called by ISRs.
 * @note The message content is copied from @a data into @a msgq and the @a data
 * pointer is not retained, so the message content will not be modified
 * by this function.
 *
 * @param msgq Address of the message queue.
 * @param data Pointer to the message.
 * @param timeout Non-negative waiting period to add the message (in milliseconds),
 *                or one of the special values OS_NO_WAIT and
 *                OS_FOREVER.
 *
 * @retval 0 Message sent.
 * @retval -ENOMSG Returned without waiting or queue purged.
 * @retval -EAGAIN Waiting period timed out.
 */
int os_msgq_put(os_msgq *msgq, const void *data, int32_t timeout);

/**
 * @brief Receive a message from a message queue.
 *
 * This routine receives a message from message queue @a q in a "first in,
 * first out" manner.
 *
 * @note Can be called by ISRs, but @a timeout must be set to OS_NO_WAIT.
 *
 * @param msgq Address of the message queue.
 * @param data Address of area to hold the received message.
 * @param timeout Waiting period to receive the message (in milliseconds),
 *                or one of the special values OS_NO_WAIT and
 *                OS_FOREVER.
 *
 * @retval 0 Message received.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
int os_msgq_get(os_msgq *msgq, void *data, int32_t timeout);

/**
 * @brief Peek/read a message from a message queue.
 *
 * This routine reads a message from message queue @a q in a "first in,
 * first out" manner and leaves the message in the queue.
 *
 * @note Can be called by ISRs.
 *
 * @param msgq Address of the message queue.
 * @param data Address of area to hold the message read from the queue.
 *
 * @retval 0 Message read.
 * @retval -ENOMSG Returned when the queue has no message.
 */
static inline int os_msgq_peek(os_msgq *msgq, void *data)
{
	return k_msgq_peek(msgq, data);
}

/**
 * @brief Purge a message queue.
 *
 * This routine discards all unreceived messages in a message queue's ring
 * buffer. Any threads that are blocked waiting to send a message to the
 * message queue are unblocked and see an -ENOMSG error code.
 *
 * @param msgq Address of the message queue.
 *
 * @return N/A
 */
static inline void os_msgq_purge(os_msgq *msgq)
{
	k_msgq_purge(msgq);
}

/**
 * @brief Get the amount of free space in a message queue.
 *
 * This routine returns the number of unused entries in a message queue's
 * ring buffer.
 *
 * @param msgq Address of the message queue.
 *
 * @return Number of unused ring buffer entries.
 */
static inline uint32_t os_msgq_num_free_get(os_msgq *msgq)
{
	return k_msgq_num_free_get(msgq);
}

/**
 * @brief Get the number of messages in a message queue.
 *
 * This routine returns the number of messages in a message queue's ring buffer.
 *
 * @param msgq Address of the message queue.
 *
 * @return Number of messages.
 */
static inline uint32_t os_msgq_num_used_get(os_msgq *msgq)
{
	return k_msgq_num_used_get(msgq);
}

/**
 * @} end defgroup os_msgq_apis
 */

/**
 * @defgroup os_timer_apis Timer APIs
 * @ingroup os_common_apis
 * @{
 */


/**
 * @brief Initialize a timer.
 *
 * This routine initializes a timer, prior to its first use.
 *
 * @param timer     Address of timer.
 * @param expiry_fn Function to invoke each time the timer expires.
 * @param stop_fn   Function to invoke if the timer is stopped while running.
 *
 * @return N/A
 */
#define  os_timer_init(timer, expiry_fn, stop_fn) \
			k_timer_init(timer, expiry_fn, stop_fn)

/**
 * @brief Start a timer.
 *
 * This routine starts a timer, and resets its status to zero. The timer
 * begins counting down using the specified duration and period values.
 *
 * Attempting to start a timer that is already running is permitted.
 * The timer's status is reset to zero and the timer begins counting down
 * using the new duration and period values.
 *
 * @param timer     Address of timer.
 * @param duration  Initial timer duration (in milliseconds).
 * @param period    Timer period (in milliseconds).
 *
 * @return N/A
 */
#define os_timer_start(timer, duration, period) \
			k_timer_start(timer, duration, period)

/**
 * @brief Stop a timer.
 *
 * This routine stops a running timer prematurely. The timer's stop function,
 * if one exists, is invoked by the caller.
 *
 * Attempting to stop a timer that is not running is permitted, but has no
 * effect on the timer.
 *
 * @note Can be called by ISRs.  The stop handler has to be callable from ISRs
 * if @a os_timer_stop is to be called from ISRs.
 *
 * @param timer     Address of timer.
 *
 * @return N/A
 */
#define os_timer_stop(timer) k_timer_stop(timer)

/**
 * @brief Get system uptime.
 *
 * This routine returns the elapsed time since the system booted,
 * in milliseconds.
 *
 * @return Current uptime.
 */

int64_t os_uptime_get(void);

/**
 * @brief Get system uptime (32-bit version).
 *
 * This routine returns the lower 32-bits of the elapsed time since the system
 * booted, in milliseconds.
 *
 * This routine can be more efficient than k_uptime_get(), as it reduces the
 * need for interrupt locking and 64-bit math. However, the 32-bit result
 * cannot hold a system uptime time larger than approximately 50 days, so the
 * caller must handle possible rollovers.
 *
 * @return Current uptime.
 */
uint32_t os_uptime_get_32(void);

/**
 * @brief Get elapsed time.
 *
 * This routine computes the elapsed time between the current system uptime
 * and an earlier reference time, in milliseconds.
 *
 * @param reftime Pointer to a reference time, which is updated to the current
 *                uptime upon return.
 *
 * @return Elapsed time.
 */
#define os_uptime_delta(reftime) k_uptime_delta(reftime)

/**
 * @brief Get elapsed time (32-bit version).
 *
 * This routine computes the elapsed time between the current system uptime
 * and an earlier reference time, in milliseconds.
 *
 * This routine can be more efficient than k_uptime_delta(), as it reduces the
 * need for interrupt locking and 64-bit math. However, the 32-bit result
 * cannot hold an elapsed time larger than approximately 50 days, so the
 * caller must handle possible rollovers.
 *
 * @param reftime Pointer to a reference time, which is updated to the current
 *                uptime upon return.
 *
 * @return Elapsed time.
 */
#define os_uptime_delta_32(reftime) k_uptime_delta_32(reftime)

/**
 * @brief Read the hardware clock.
 *
 * This routine returns the current time, as measured by the system's hardware
 * clock.
 *
 * @return Current hardware clock up-counter (in cycles).
 */
uint32_t os_cycle_get_32(void);


/**
 * @brief Writes an ERROR level message to the log.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param ... A string optionally containing os_printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#ifdef CONFIG_SIMULATOR
#define SYS_LOG_ERR(...)	\
		do {	\
			if (LOG_PRINT_FUNC_NAME) \
				printf("E: %s(%d): ", __func__, __LINE__);	\
			else \
				printf("E: ");	\
			printf(__VA_ARGS__);	\
			printf("\n");	\
		} while (0)
#else
#define SYS_LOG_ERR(...)	\
		do {	\
			if (LOG_PRINT_FUNC_NAME) \
				os_printk("%s ", __func__);	\
			Z_LOG(LOG_LEVEL_ERR, __VA_ARGS__);	\
			os_printk("\n");	\
		} while (0)
#endif
/**
 * @brief Writes a WARNING level message to the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param ... A string optionally containing os_printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#ifdef CONFIG_SIMULATOR
#define SYS_LOG_WRN(...)	\
		do {	\
			if (LOG_PRINT_FUNC_NAME) \
				printf("W: %s(%d): ", __func__, __LINE__);	\
			else \
				printf("W: ");	\
			printf(__VA_ARGS__);	\
			printf("\n");	\
		} while (0)
#else
#define SYS_LOG_WRN(...)	\
		do {	\
			if (LOG_PRINT_FUNC_NAME) \
				os_printk("%s ", __func__);	\
			Z_LOG(LOG_LEVEL_WRN, __VA_ARGS__);	\
			os_printk("\n");	\
		} while (0)
#endif
/**
 * @brief Writes an INFO level message to the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param ... A string optionally containing os_printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#ifdef CONFIG_SIMULATOR
#define SYS_LOG_INF(...)	\
		do {	\
			if (LOG_PRINT_FUNC_NAME) \
				printf("I: %s(%d): ", __func__, __LINE__);	\
			else \
				printf("I: ");	\
			printf(__VA_ARGS__);	\
			printf("\n");	\
		} while (0)
#else
#define SYS_LOG_INF(...)	\
		do {	\
			if (LOG_PRINT_FUNC_NAME) \
				os_printk("%s ", __func__);	\
			Z_LOG(LOG_LEVEL_INF, __VA_ARGS__);	\
			os_printk("\n");	\
		} while (0)
#endif

/**
 * @brief Writes a DEBUG level message to the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param ... A string optionally containing os_printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#ifdef CONFIG_SIMULATOR
#define SYS_LOG_DBG //printf
#else
#define SYS_LOG_DBG(...)	\
		do {	\
			if (LOG_PRINT_FUNC_NAME) \
			Z_LOG(LOG_LEVEL_DBG, __VA_ARGS__);	\
		} while (0)
#endif
/**
 * @brief Output a string
 *
 * Output a string on output installed by platform at init time. Some
 * printf-like formatting is available.
 *
 * Available formatting:
 * - %x/%X:  outputs a number in hexadecimal format
 * - %s:     outputs a null-terminated string
 * - %p:     pointer, same as %x with a 0x prefix
 * - %u:     outputs a number in unsigned decimal format
 * - %d/%i:  outputs a number in signed decimal format
 *
 * Field width (with or without leading zeroes) is supported.
 * Length attributes h, hh, l, ll and z are supported. However, integral
 * values with %lld and %lli are only printed if they fit in a long
 * otherwise 'ERR' is printed. Full 64-bit values may be printed with %llx.
 *
 * @param fmt formatted string to output
 *
 * @return N/A
 */
void os_printk(const char *fmt, ...);

/**
 * @brief Allocate memory from system mem heap .
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param num_bytes Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void* mem_malloc_debug(size_t size, const char* func);
#define mem_malloc(size) mem_malloc_debug(size, __func__)

#define app_mem_malloc mem_malloc

/**
 * @brief Free memory allocated  system mem heap.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated from the heap memory pool.
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @return N/A
 */
void mem_free(void *ptr);

#define app_mem_free mem_free

/**
 * @} end defgroup os_timer_apis
 */

/**
 * @defgroup os_msg_apis message APIs
 * @ingroup os_common_apis
 * @{
 */
int system_check_low_latencey_mode(void);

void system_set_low_latencey_mode(bool low_latencey);

#define MSG_MEM_SIZE (5 * 4)

int msg_pool_get_free_msg_num(void);
void msg_pool_dump(void(*dump_fn)(os_tid_t sender, os_tid_t receiver,
		const char *content, int max_size));
int os_send_sync_msg(void *receiver, void *msg, int msg_size);
int os_send_async_msg(void *receiver, void *msg, int msg_size);
int os_receive_msg(void *msg, int msg_size,int timeout);
void os_msg_clean(void);
void os_msg_init(void);

bool os_is_in_isr(void);

os_work_q *os_get_user_work_queue(void);
os_work_q *os_get_display_work_queue(void);

/**
 * @} end defgroup os_msg_apis
 */

/**
 * @defgroup os_tracing_apis tracing APIs
 * @ingroup os_common_apis
 * @{
 */

#ifndef CONFIG_SIMULATOR
#define os_strace_void(id) \
	sys_trace_void(id)
#define os_strace_end_call(id) \
	sys_trace_end_call(id)
#define os_strace_end_call_u32(id, retv) \
	sys_trace_end_call_u32(id, retv)

#define os_strace_u32(id, p1) \
	sys_trace_u32(id, p1)
#define os_strace_u32x2(id, p1, p2) \
	sys_trace_u32x2(id, p1, p2)
#define os_strace_u32x3(id, p1, p2, p3) \
	sys_trace_u32x3(id, p1, p2, p3)
#define os_strace_u32x4(id, p1, p2, p3, p4) \
	sys_trace_u32x4(id, p1, p2, p3, p4)
#define os_strace_u32x5(id, p1, p2, p3, p4, p5) \
	sys_trace_u32x5(id, p1, p2, p3, p4, p5)
#define os_strace_u32x6(id, p1, p2, p3, p4, p5, p6) \
	sys_trace_u32x6(id, p1, p2, p3, p4, p5, p6)
#define os_strace_u32x7(id, p1, p2, p3, p4, p5, p6, p7) \
	sys_trace_u32x7(id, p1, p2, p3, p4, p5, p6, p7)
#define os_strace_u32x8(id, p1, p2, p3, p4, p5, p6, p7, p8) \
	sys_trace_u32x8(id, p1, p2, p3, p4, p5, p6, p7, p8)
#define os_strace_u32x9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9) \
	sys_trace_u32x9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define os_strace_u32x10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10) \
	sys_trace_u32x10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)

#define os_strace_string(id, string) \
	sys_trace_string(id, string)
#define os_strace_string_u32x5(id, string, p1, p2, p3, p4, p5) \
	sys_trace_string_u32x5(id, string, p1, p2, p3, p4, p5)
#else
#define os_strace_void(id)
#define os_strace_end_call(id)
#define os_strace_end_call_u32(id, retv)

#define os_strace_u32(id, p1)
#define os_strace_u32x2(id, p1, p2)
#define os_strace_u32x3(id, p1, p2, p3)
#define os_strace_u32x4(id, p1, p2, p3, p4)
#define os_strace_u32x5(id, p1, p2, p3, p4, p5)
#define os_strace_u32x6(id, p1, p2, p3, p4, p5, p6)
#define os_strace_u32x7(id, p1, p2, p3, p4, p5, p6, p7)
#define os_strace_u32x8(id, p1, p2, p3, p4, p5, p6, p7, p8)
#define os_strace_u32x9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define os_strace_u32x10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)

#define os_strace_string(id, string)
#define os_strace_string_u32x5(id, string, p1, p2, p3, p4, p5)
#endif /* CONFIG_SIMULATOR */

/**
 * @} end defgroup os_tracing_apis
 */

#endif /* __OS_COMMON_API_H__ */
