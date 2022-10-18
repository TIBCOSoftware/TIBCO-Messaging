/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

#ifndef _INCLUDED_tibaux_h
#define _INCLUDED_tibaux_h

#include <stdio.h>
#include <string.h>
#include "tib/ftl.h"

#ifdef _WIN32
#  include <Windows.h>
#  include <process.h>
#  include <time.h>
#else
#  include <pthread.h>
#  include <inttypes.h>
#endif

#define CHECK(ex) do{if(tibEx_GetErrorCode(ex)) tibAux_ExitOnException(ex, __FILE__, __LINE__);} while (0)

#if !defined(PRId64)
#define auxPRId64      "lld"
#else
#define auxPRId64     PRId64 
#endif

#if defined(_WIN32)
#define tibAux_snprintf(str,sz,fmt, ...)  _snprintf_s(str,sz,_TRUNCATE,fmt, __VA_ARGS__)
#define tibAux_strtok_r(str, del, save) strtok_s(str, del, save)
#define tibAux_strdup(str) _strdup(str)
#else
#define tibAux_snprintf(str,sz,fmt, ...)  snprintf(str,sz,fmt, __VA_ARGS__)
#define tibAux_strtok_r(str, del, save) strtok_r(str, del, save)
#define tibAux_strdup(str) strdup(str)
#endif

#if defined(__GNUC__)
#  include <xmmintrin.h>
#endif
#define TIBAUX_PAUSE()  _mm_pause()

typedef void
(*tibAux_UsageCB) (
    int                 argc,
    char**              argv);

typedef struct {
    tibint32_t      N;
    tibint64_t      max_x;
    tibint64_t      min_x;
    tibdouble_t     mean_x;
    tibdouble_t     M2;
} tibAux_StatRecord;

typedef struct tibAux_Histogram_struct *tibAux_Histogram;

extern void
tibAux_ExitOnException(
    tibEx           ex,
    const char*     file,
    int             line);

extern void
tibAux_InitGetArgs(
    int                 argc,
    char**              argv,
    tibAux_UsageCB       callback);

extern tibbool_t
tibAux_GetFlag(
    int*                i,
    const char*         name,
    const char*         sname);

extern tibbool_t
tibAux_GetString(
    int*                i,
    const char*         name,
    const char*         sname,
    const char**        result);

extern tibbool_t
tibAux_GetOptionalString(
    int*                i,
    const char*         name,
    const char*         sname,
    const char**        result);

extern tibbool_t
tibAux_GetInt(
    int*                i,
    const char*         name,
    const char*         sname,
    tibint32_t*         result);

extern tibbool_t
tibAux_GetDouble(
    int*                i,
    const char*         name,
    const char*         sname,
    tibdouble_t*        result);

extern tibint64_t (*tibAux_getTimeFunc)(void);
#define tibAux_GetTime() ((*tibAux_getTimeFunc)())

extern tibdouble_t (*tibAux_getTimerScaleFunc)(void);
#define tibAux_GetTimerScale() ((*tibAux_getTimerScaleFunc)())

extern void
tibAux_CalibrateTimer(void);

extern void
tibAux_PrintNumber(
    double          n);

FILE*
tibAux_OpenCsvFile(
    const char*     csvFileName);

void
tibAux_StatUpdate(
    tibAux_StatRecord*  stats,
    tibint64_t          x);

tibdouble_t
tibAux_StdDeviation(
    tibAux_StatRecord*  stats);

tibAux_Histogram
tibAux_CreateHistogram(
    tibdouble_t         min,
    tibdouble_t         max,
    tibint32_t          steps);

void
tibAux_DestroyHistogram(
    tibAux_Histogram    h);

void
tibAux_AddPointToHistogram(
        tibAux_Histogram    h,
        tibdouble_t         x);

void
tibAux_DumpHistogramToFile(
    tibAux_Histogram    h,
    const char*         fname);

void
tibAux_PrintFractions(
    tibAux_Histogram    h,
    FILE*               fileDescriptor);

void
tibAux_PrintStats(
    tibAux_StatRecord*  stats,
    tibdouble_t         factor);

/*
 * The tibAux_StringCat* functions can be used as alternatives to 
 * the tibAux_Print* functios for formatting the output as a string 
 * instead of printing to stdout.
 */
void
tibAux_StringCat(
    char*           outbuf,
    size_t          outsize,
    const char*     str);

void
tibAux_StringCatNumber(
    char*           outbuf,
    size_t          outsize,
    double          n);

void
tibAux_StringCatStats(
    char*           outbuf,
    size_t          outsize,
    tibAux_StatRecord*     stats,
    tibdouble_t     factor);

void
tibAux_StringCopy(
    char*           outbuf,
    size_t          outsize,
    const char*     str);

void
tibAux_PrintException(
    tibEx           ex);

void
tibAux_PrintAdvisory(
    tibEx           ex,
    tibMessage      msg);

typedef void
(*tibAux_ThreadFunc)(void *closure);

#ifdef _WIN32
typedef HANDLE      tibAux_ThreadHandle;
#else
typedef pthread_t   tibAux_ThreadHandle;
#endif

tibAux_ThreadHandle
tibAux_LaunchThread(
    tibAux_ThreadFunc   runner,
    void*               closure);

void
tibAux_DetachThread(
    tibAux_ThreadHandle t);

void
tibAux_JoinThread(
    tibAux_ThreadHandle t);

void
tibAux_SleepMillis(
    tibint32_t      milliseconds);

#if defined(_WIN32)

typedef CRITICAL_SECTION        tibAux_Mutex;
#define tibAux_Mutex_Lock(m)    EnterCriticalSection(&(m))
#define tibAux_Mutex_Unlock(m)  LeaveCriticalSection(&(m))
#define tibAux_Mutex_TryLock(m) (TryEnterCriticalSection(&(m)) > 0 ? false : true)

#else

typedef pthread_mutex_t         tibAux_Mutex;
#define tibAux_Mutex_Lock(m)    pthread_mutex_lock(&(m))
#define tibAux_Mutex_Unlock(m)  pthread_mutex_unlock(&(m))
#define tibAux_Mutex_TryLock(m) pthread_mutex_trylock(&(m))

#endif

void
tibAux_InitializeMutex(
    tibAux_Mutex    *mutex);

void
tibAux_CleanupMutex(
    tibAux_Mutex    *mutex);

tibdouble_t
tibAux_Rand(void);

void
tibAux_StringCatLong(
    char*      outbuff,
    size_t     outsize,
    tibint64_t num);

void
tibAux_MillisecondsToStr(
    tibint64_t  millis,
    char        *buf,
    tibint32_t  size);

#endif /* _INCLUDED_tibaux_h */
