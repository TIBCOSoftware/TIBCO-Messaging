/*
 * Copyright (c) $Date: 2020-05-26 13:01:55 -0700 (Tue, 26 May 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: thread.h 125253 2020-05-26 20:01:55Z $
 *
 */

#ifndef INCLUDED_TIBEFTL_THREAD_H
#define INCLUDED_TIBEFTL_THREAD_H

#include <inttypes.h>

#define WAIT_FOREVER -1

#if defined(_WIN32)

#include <windows.h>

typedef HANDLE thread_t;
#define INVALID_THREAD (NULL)
typedef CRITICAL_SECTION mutex_t;
typedef volatile struct {
    LONG lock;
    int state;
} thread_once_t;
#define THREAD_ONCE_INIT {0, 0}

typedef HANDLE semaphore_t;

#else

#include <pthread.h>

typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_once_t thread_once_t;
#define THREAD_ONCE_INIT PTHREAD_ONCE_INIT

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
typedef dispatch_semaphore_t semaphore_t;
#define INVALID_THREAD (NULL) 
#else
#include <semaphore.h>
typedef sem_t* semaphore_t;
#define INVALID_THREAD (0)
#endif

#endif

typedef void* (*thread_func_t)(void* data);

void thread_start(thread_t* thread, thread_func_t func, void* data);
void thread_destroy(thread_t thread);
void thread_join(thread_t thread);
void thread_detach(thread_t thread);

void mutex_init(mutex_t* mutex);
void mutex_destroy(mutex_t* mutex);
void mutex_lock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);

void thread_once(thread_once_t* once_control, void (*init_routine)(void));

semaphore_t semaphore_create(void);
void semaphore_destroy(semaphore_t semaphore);
int semaphore_wait(semaphore_t semaphore, int64_t timeout);
void semaphore_post(semaphore_t semaphore);

#endif /* INCLUDED_TIBEFTL_THREAD_H */
