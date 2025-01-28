/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
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

 * Author: wh<wanghui@actions-semi.com>
 *
 * Change log:
 *	2021/12/9: Created by wh.
 */

#include "os_common_api.h"
#include "stdbool.h"
#include "string.h"

#ifdef CONFIG_SIMULATOR
#include <Windows.h>
#include "TlHelp32.h"
#endif

#define MAX_THREAD_TERMINAL_NUM 3

struct thread_terminal_info_t {
	os_thread *wait_terminal_thread;
	os_sem terminal_sem;
};

static struct thread_terminal_info_t thread_terminal_info[MAX_THREAD_TERMINAL_NUM] = {0};

static void os_thread_abort_callback(os_thread *aborted)
{
	os_sched_lock();
	for (int i = 0; i < MAX_THREAD_TERMINAL_NUM; i++){
		if(thread_terminal_info[i].wait_terminal_thread == aborted) {
			os_sem_give(&thread_terminal_info[i].terminal_sem);
			SYS_LOG_INF(" %p \n",aborted);
		}
	}
	os_sched_unlock();
}

#ifdef CONFIG_SIMULATOR
int os_thread_prio_covert_to_win(int prio)
{
    int prio_win = THREAD_PRIORITY_NORMAL;

    if (prio >= -15 && prio < -10)
        prio_win = THREAD_PRIORITY_HIGHEST;

    else if (prio >= -10 && prio < -5)
        prio_win = THREAD_PRIORITY_ABOVE_NORMAL;

    else if (prio >= -5 && prio < 0)
        prio_win = THREAD_PRIORITY_NORMAL;

    else if (prio >=0 && prio < 5)
        prio_win = THREAD_PRIORITY_BELOW_NORMAL;

    else if (prio >= 5 && prio < 10)
        prio_win = THREAD_PRIORITY_LOWEST;

    else if (prio >= 10 && prio <= 15)
        prio_win = THREAD_PRIORITY_IDLE;

    return prio_win;
}

int os_thread_prio_covert_win(int prio_win)
{
    int prio = 0;

    if (prio_win == THREAD_PRIORITY_HIGHEST)
        prio = -15;

    else if (prio_win == THREAD_PRIORITY_ABOVE_NORMAL)
        prio = -10;

    else if (prio_win == THREAD_PRIORITY_NORMAL)
        prio = -5;

    else if (prio_win == THREAD_PRIORITY_BELOW_NORMAL)
        prio = 0;

    else if (prio_win == THREAD_PRIORITY_LOWEST)
        prio = 5;

    else if (prio_win == THREAD_PRIORITY_IDLE)
        prio = 10;

    return prio;
}
#endif

/**thread function */
int os_thread_create(char* stack, size_t stack_size,
    void (*entry)(void*, void*, void*),
    void* p1, void* p2, void* p3,
    int prio, uint32_t options, int delay)
{
#ifdef CONFIG_SIMULATOR

    /* Only p1 is valid. */
	DWORD ThreadID = (DWORD)0xFFFFFFFF;
	HANDLE handle = CreateThread( 
                     NULL,       // default security attributes
                     0,          // default stack size
                    (LPTHREAD_START_ROUTINE)entry,
                     p1,       // no thread function arguments
                     0,          // default creation flags
                     &ThreadID); // receive thread identifier

    SetThreadPriority(handle, os_thread_prio_covert_to_win(prio));

    CloseHandle(handle);

	return ThreadID;
#else
	k_tid_t tid = NULL;

	os_thread *thread = NULL;

	thread = (os_thread *)stack;

	tid = k_thread_create(thread, (os_thread_stack_t *)&stack[sizeof(os_thread)],
							stack_size - sizeof(os_thread),
							entry,
							p1, p2, p3,
							prio,
							options,
							SYS_TIMEOUT_MS(delay));

	thread->fn_abort = os_thread_abort_callback;

	return (int)tid;
#endif
}

int os_thread_prepare_terminal(int tid)
{
#ifdef CONFIG_SIMULATOR
    printf(("preparing terminal thread, thread is: %i\n"), tid);
	int ret = 0;
#else
	struct thread_terminal_info_t *terminal_info = NULL;

	os_sched_lock();

	for (int i = 0; i < MAX_THREAD_TERMINAL_NUM; i++){
		if(!thread_terminal_info[i].wait_terminal_thread) {
			terminal_info = &thread_terminal_info[i];
			break;
		}
	}

	if (!terminal_info) {
		SYS_LOG_ERR("%p busy\n", tid);
		ret = -EBUSY;
		goto exit;
	}

	terminal_info->wait_terminal_thread = tid;
	os_sem_init(&terminal_info->terminal_sem, 0, 1);

	SYS_LOG_INF(" 0x%x ok\n",tid);
exit:
	os_sched_unlock();
#endif
	return ret;
}

int os_thread_wait_terminal(int tid)
{
#ifdef CONFIG_SIMULATOR
    int ret = 0;
    HANDLE handle = OpenThread(THREAD_ALL_ACCESS, TRUE, tid);
    DWORD dw = WaitForSingleObject(handle, 5 * 1000);
    if (dw == WAIT_TIMEOUT)
    {
        printf(("time out,  thread id: %i\n"), tid);
        ret = -1;
    }
    else if (dw == WAIT_FAILED)
    {
        printf(("terminal tid %i not found\n"), tid);
        ret = -1;
    }
    else
    {
        printf(("ok, thread id: %i\n"), tid);
        ret = 0;
    }
    CloseHandle(handle);
#else
	struct thread_terminal_info_t *terminal_info = NULL;

	os_sched_lock();
	for (int i = 0; i < MAX_THREAD_TERMINAL_NUM; i++){

		if(thread_terminal_info[i].wait_terminal_thread == tid) {
			terminal_info = &thread_terminal_info[i];
		}
	}
	os_sched_unlock();

	if (!terminal_info) {
		SYS_LOG_ERR("terminal tid %p not found\n",tid);
		ret = -EBUSY;
	}

	if (os_sem_take(&terminal_info->terminal_sem, 5*1000)) {
		SYS_LOG_ERR("timeout \n");
		ret = -EBUSY;
	}

	os_sched_lock();
	terminal_info->wait_terminal_thread = NULL;
	os_sched_unlock();

	SYS_LOG_INF(" 0x%x ok\n",tid);
#endif
	return ret;
}

const char *os_thread_get_name_by_prio(int prio)
{
#ifdef CONFIG_SIMULATOR
    //THREADENTRY32 te32 = { 0 };
    //te32.dwSize = sizeof(THREADENTRY32);
    //// 获取全部线程快照
    //HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    //if (INVALID_HANDLE_VALUE == hThreadSnap)
    //    return "NULL";

    //BOOL bRet = Thread32First(hThreadSnap, &te32);
    //while (bRet)
    //{
    //    int cur_prio = os_thread_priority_get(te32.th32ThreadID);

    //    if (prio == cur_prio)
    //        return os_thread_name_get(te32.th32ThreadID);

    //    // 获取快照中下一条信息
    //    bRet = Thread32Next(hThreadSnap, &te32);
    //}

    return "NULL";
#else
	struct k_thread *thread_list = (struct k_thread *)(_kernel.threads);
	unsigned int key = irq_lock();	

	while (thread_list != NULL) {
		int thread_prio = os_thread_priority_get(thread_list);
		if (prio == thread_prio) {
			break;
		}

		thread_list = (struct k_thread *)thread_list->next_thread;
	}
	irq_unlock(key);

	if (thread_list) {
		return k_thread_name_get(thread_list);
	}
#endif

}

int os_thread_priority_get(int thread_id)
{
#ifdef CONFIG_SIMULATOR
    HANDLE handle = OpenThread(THREAD_ALL_ACCESS, TRUE, thread_id);
    int prio = os_thread_prio_covert_win(GetThreadPriority(handle));

    CloseHandle(handle);

    return prio;
#else

#endif

}

int os_thread_priority_set(int thread_id, int prio)
{
#ifdef CONFIG_SIMULATOR
    HANDLE handle = OpenThread(THREAD_ALL_ACCESS, TRUE, thread_id);
    BOOL res =  SetThreadPriority(handle, os_thread_prio_covert_to_win(prio));

    CloseHandle(handle);

    if (!res)
        printf("os_thread_priority_set: set priority failed!");

    return res;
#else

#endif
}

#ifdef CONFIG_SIMULATOR
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // must be 0x1000
    LPCSTR szName; // pointer to name (in user addr space)
    DWORD dwThreadID; // thread ID (-1=caller thread)
    DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;
#endif

int os_thread_name_set(int thread_id, const char * value)
{
#ifdef CONFIG_SIMULATOR

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = value;
    info.dwThreadID = thread_id;
    info.dwFlags = 0;

    __try
    {
        RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (DWORD*)&info);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
    return 0;
#else
    return -1;
#endif
}

void os_thread_abort(int thread_id)
{
#ifdef CONFIG_SIMULATOR
    /*
        This method carries risks when exiting a thread.
        May cause some resources in the thread to be unable to be reclaimed.
    */
    HANDLE handle = OpenThread(THREAD_ALL_ACCESS, TRUE, thread_id);
    TerminateThread(handle, -1);
    CloseHandle(handle);
#else

#endif
}
void os_thread_cancel(int thread_id)
{
#ifdef CONFIG_SIMULATOR
    os_thread_abort(thread_id);
#else

#endif
}
