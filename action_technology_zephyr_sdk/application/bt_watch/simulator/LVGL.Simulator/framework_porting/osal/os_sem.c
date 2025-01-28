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
#include "stdint.h"
#include "string.h"

#ifdef CONFIG_SIMULATOR
#include <Windows.h>
#endif

os_sem os_sem_create(uint32_t initial_count, uint32_t limit)
{
#ifdef CONFIG_SIMULATOR
	return (os_sem)CreateSemaphore(
        NULL,           // default security attributes
        initial_count,  // initial count
        limit,          // maximum count
        NULL);          // unnamed semaphore
#else
	return 0;
#endif	
}

int os_sem_init(os_sem *sem, uint32_t initial_count, uint32_t limit)
{
#ifdef CONFIG_SIMULATOR
    if (!sem)
        return -1;

    *sem = (os_sem)CreateSemaphore(
        NULL,           // default security attributes
        initial_count,  // initial count
        limit,          // maximum count
        NULL);          // unnamed semaphore

    return 0;
#else
	return k_sem_init(sem, initial_count, limit);
#endif
}

#ifdef CONFIG_SIMULATOR
HANDLE sem_handle_get(os_sem* sem)
{
    if (!sem)
        return NULL;

    HANDLE handle = (HANDLE)(*sem);

    if (!handle) // if handle is not created
        *sem = os_sem_create(0, 1);

    return (HANDLE)(*sem);
}
#endif

int os_sem_take(os_sem *sem, int32_t timeout)
{
#ifdef CONFIG_SIMULATOR
    HANDLE handle = sem_handle_get(sem);

    DWORD dw = WaitForSingleObject(
        handle,             // handle to semaphore
        timeout);           

    if (dw == WAIT_FAILED)
    {
        printf("os_sem_take: handle is not valid\n");
    }
    return dw == WAIT_FAILED ? 0 : -1;
#else
	return os_sem_take(sem, SYS_TIMEOUT_MS(timeout));
#endif
}

void os_sem_give(os_sem *sem)
{
#ifdef CONFIG_SIMULATOR
    HANDLE handle = sem_handle_get(sem);

	BOOL res = ReleaseSemaphore( 
                    handle,       // handle to semaphore
                    1,            // increase count by one
                    NULL);        // not interested in previous count

    if (!res)
    {
        //printf("os_sem_give: handle is not valid\n");
    }

#else
	return  k_sem_give(sem);
#endif
}

void os_sem_release(os_sem *sem)
{
#ifdef CONFIG_SIMULATOR
    HANDLE handle = sem_handle_get(sem);
    BOOL res = CloseHandle(handle);

    if (!res)
    {
        printf("os_sem_release: handle is not valid\n");
    }

#else

#endif
}

int os_sem_reset(os_sem *sem)
{
#ifdef CONFIG_SIMULATOR  
    int count = 0;

    HANDLE handle = sem_handle_get(sem);
    BOOL res = ReleaseSemaphore(
                handle,       // handle to semaphore
                1,            // increase count by one
                &count);      // previous count

    if (!res)
    {
        printf("os_sem_reset: handle is not valid\n");
    }
    else
    {
        for (int i = 0; i < count + 1; i++)
        {
            // only sets the count of semaphore to zero.
            // so waiting time set to 1
            DWORD dw = WaitForSingleObject(handle, 1);
        }
    }

	return res;
#else
	return os_sem_reset(sem);
#endif
}
