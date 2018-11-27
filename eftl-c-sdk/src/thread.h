/*
 * Copyright (c) $Date: 2018-03-08 14:04:14 -0600 (Thu, 08 Mar 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: thread.h 100048 2018-03-08 20:04:14Z bpeterse $
 *
 */

#ifndef INCLUDED_TIBEFTL_THREAD_H
#define INCLUDED_TIBEFTL_THREAD_H

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
void mutex_destroy(mutex_t* m);
void mutex_lock(mutex_t* m);
void mutex_unlock(mutex_t* m);

void thread_once(thread_once_t* once_control, void (*init_routine)(void));

semaphore_t semaphore_create(void);
void semaphore_destroy(semaphore_t semaphore);
int semaphore_wait(semaphore_t semaphore, int timeout);
void semaphore_post(semaphore_t semaphore);

#endif /* INCLUDED_TIBEFTL_THREAD_H */
