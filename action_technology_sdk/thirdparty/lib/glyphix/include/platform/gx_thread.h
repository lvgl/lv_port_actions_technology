/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "platform/gx_os.h"

namespace gx {
namespace os {
mutex_t mutex_create(const char *name);   // NOLINT(readability-*)
void mutex_delete(mutex_t mutex);         // NOLINT(readability-*)
bool mutex_take(mutex_t mutex, int time); // NOLINT(readability-*)
void mutex_release(mutex_t mutex);        // NOLINT(readability-*)

semaphore_t semaphore_create(int value, const char *name); // NOLINT(readability-*)
void semaphore_delete(semaphore_t semaphore);              // NOLINT(readability-*)
bool semaphore_wait(semaphore_t semaphore, int time);      // NOLINT(readability-*)
void semaphore_release(semaphore_t semaphore);             // NOLINT(readability-*)

/**
 * Create a timer.
 * @param time Timeout period, in milliseconds
 * @param name Timer name
 * @param flags Timer mode @see TimerFlag
 * @param callback Callback function
 * @param parameter Pass the parameters of a given timer callback
 * @return
 */
timer_t timer_create(int time, const char *name, int flags, // NOLINT(readability-*)
                     void (*callback)(void *), void *parameter);
void timer_delete(timer_t timer); // NOLINT(readability-*)
void timer_start(timer_t timer);  // NOLINT(readability-*)
void timer_cancel(timer_t timer); // NOLINT(readability-*)

void *this_thread(); // NOLINT(readability-*)
} // namespace os
} // namespace gx
