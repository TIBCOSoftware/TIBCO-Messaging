/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

#if defined(_WIN32)
#define _CRT_RAND_S
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>

#if defined(_WIN32)
#include <Windows.h>
#else
#include <time.h>
#if !defined(CLOCK_MONOTONIC)
#include <sys/time.h>
#endif
#include <unistd.h>
#include <sys/select.h>
#endif

#include "tibaux.h"

static  int             savedArgc;
static  char**          savedArgv;
static  tibAux_UsageCB   savedCallback;

tibint64_t
tibAux_GetTimeSystem(void);

double
tibAux_GetTimerScaleSystem(void);

static tibdouble_t      clockScale;
tibint64_t (*tibAux_getTimeFunc)(void) = tibAux_GetTimeSystem;
tibdouble_t (*tibAux_getTimerScaleFunc)(void) = tibAux_GetTimerScaleSystem;

void
tibAux_InitGetArgs(
    int                 argc,
    char**              argv,
    tibAux_UsageCB       callback)
{
    savedArgc = argc;
    savedArgv = argv;
    savedCallback = callback;
}

tibbool_t
tibAux_GetFlag(
    int*                i,
    const char*         name,
    const char*         sname)
{
    if (strcmp(savedArgv[*i], name)==0 || strcmp(savedArgv[*i], sname)==0)
        return tibtrue;

    return tibfalse;
}

tibbool_t
tibAux_GetOptionalString(
    int*                i,
    const char*         name,
    const char*         sname,
    const char**        result)
{
    if (tibAux_GetFlag(i, name, sname))
    {
        if ((*i+1) < savedArgc)
        {
            if (strchr(savedArgv[*i+1],'-') != savedArgv[*i+1])
            {
                *i = *i+1;
                *result = savedArgv[*i];
                return tibtrue;
            }
        }
        else
        {
            return tibfalse;
        }
    }

    return tibfalse;
}

tibbool_t
tibAux_GetString(
    int*                i,
    const char*         name,
    const char*         sname,
    const char**        result)
{
    if (tibAux_GetFlag(i, name, sname))
    {
        if ((*i+1) < savedArgc)
        {
            *i = *i+1;
            *result = savedArgv[*i];
            return tibtrue;
        }
        else
        {
            printf("missing value for %s\n", savedArgv[*i]);
            savedCallback(savedArgc, savedArgv);
        }
    }

    return tibfalse;
}

tibbool_t
tibAux_GetInt(
    int*                i,
    const char*         name,
    const char*         sname,
    tibint32_t*         result)
{
    const char*         strVal;
    char*               p      = NULL;

    if (tibAux_GetString(i, name, sname, &strVal))
    {
        *result = strtol(strVal, &p, 10);
        if (p && *p)
        {
            printf("invalid value for %s\n", savedArgv[*i-1]);
            savedCallback(savedArgc, savedArgv);
        }
        return tibtrue;
    }

    return tibfalse;
}

extern tibbool_t
tibAux_GetDouble(
    int*                i,
    const char*         name,
    const char*         sname,
    tibdouble_t*        result)
{
    const char*         strVal;
    char*               p      = NULL;
    
    if (tibAux_GetString(i, name, sname, &strVal))
    {
        *result = strtod(strVal, &p);
        if (p && *p)
        {
            printf("invalid value for %s\n", savedArgv[*i-1]);
            savedCallback(savedArgc, savedArgv);
        }
        return tibtrue;
    }
    
    return tibfalse;
}

#if !defined(_WIN32)
static void
cpuID(int data[4], unsigned selector)
{
    asm("cpuid"
        : "=a" (data[0]),
          "=b" (data[1]),
          "=c" (data[2]),
          "=d" (data[3])
        : "a"(selector));
}

#else

#   define cpuID(x,y)  __cpuid((x), (y))

#endif

static
tibint64_t
cpuTSC(void)
{
#if !defined(_WIN32)
    unsigned a, d;
    __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));
    return ((unsigned long long)a) | (((unsigned long long)d) << 32);

#else
    return __rdtsc();

#endif
}

static
tibdouble_t
cpuTSCScale(void)
{
    return clockScale;
}

void
tibAux_CalibrateTimer(void)
{
    int         cpu_info[4];
    tibbool_t   tscGood = tibtrue;

    cpuID(cpu_info, 0x80000001);
    if ((cpu_info[3] & 0x8000000) == 0)
    {
        // rdtsc is not available
        tscGood = tibfalse;
    }

    cpuID(cpu_info, 0x80000007);
    if ((cpu_info[3] & 0x100) == 0)
    {
        // tsc register counting rate is not constant
        tscGood = tibfalse;
    }

    if (tscGood)
    {
        tibint64_t  start;
        tibint64_t  stop;
        tibint64_t  start_cycles;
        tibint64_t  stop_cycles;

        printf("Calibrating tsc timer... ");
        fflush(stdout);

        start = tibAux_GetTimeSystem();
        start_cycles = cpuTSC();

        tibAux_SleepMillis(1000);

        stop = tibAux_GetTimeSystem();
        stop_cycles = cpuTSC();
        printf("done.\n");

        clockScale = ((stop-start)*tibAux_GetTimerScaleSystem())/(stop_cycles-start_cycles);
        tibAux_getTimeFunc = cpuTSC;
        tibAux_getTimerScaleFunc = cpuTSCScale;

        printf("CPU speed estimated to be ");
        tibAux_PrintNumber(1.0/clockScale);
        printf(" Hz\n");
    }
    else
    {
        printf("Using system timers.\n");
    }
}

tibint64_t
tibAux_GetTimeSystem(void)
{
#if defined(_WIN32)
    
    LARGE_INTEGER now;
    
    QueryPerformanceCounter(&now);
    return (tibint64_t)now.QuadPart;
    
#elif defined(CLOCK_MONOTONIC)
    struct timespec tspec;
    
    clock_gettime(CLOCK_MONOTONIC, &tspec);
    
    return (tibint64_t)tspec.tv_nsec + 1000000000ULL * (tibint64_t)tspec.tv_sec;
    
#else
    struct timeval tspec;
    
    gettimeofday(&tspec, NULL);
    
    return 1000ULL * (tibint64_t)tspec.tv_usec + 1000000000ULL * (tibint64_t)tspec.tv_sec;
    
#endif
}

double
tibAux_GetTimerScaleSystem(void)
{
#if defined(_WIN32)
    
    LARGE_INTEGER resolution;
    
    QueryPerformanceFrequency(&resolution);
    return 1.0 / resolution.QuadPart;
    
#else
    
    return 1E-9;
    
#endif
}

void
tibAux_ExitOnException(
    tibEx           ex,
    const char*     file,
    int             line)
{
    tibint32_t      len = tibEx_ToString(ex, NULL, 0);
    char            *exStr = malloc(len);

    fprintf(stderr, "%s: %d\n", file, line);
    if (exStr)
    {
        tibEx_ToString(ex, exStr, len);
        printf("%s\n", exStr);
        free(exStr);
    }

    tib_Close(ex);
    exit(-1);
}

void
tibAux_StringCatLong(
    char*      outbuff,
    size_t     outsize,
    tibint64_t num)
{
    char   buff[16];
    char*  next;
    char*  end;

    if (outbuff == NULL || outsize < strlen(outbuff)) {
        return;
    }

    next = outbuff;
    end = next+outsize;
    next += strlen(next);

    tibAux_snprintf(buff, sizeof(buff), "%"auxPRId64, num);

    tibAux_StringCopy(next,(end-next),buff);
}

void
tibAux_StringCatNumber(
    char*           outbuf,
    size_t          outsize,
    double          n)
{
    tibint32_t  power;
    tibdouble_t factor;
    tibdouble_t sign;
    char*  next;
    char*  end;

    
    if (outbuf == NULL || outsize < 12 + strlen(outbuf)) {
        return;
    }

    next = outbuf;
    end = next+outsize;
    next += strlen(next);

    if (n == 0.0)  {
        tibAux_StringCat(outbuf,outsize,"   0.00E+00");
        return;
    }
    
    if (n < 0.0)  {
        sign = -1;
        n = -n;
    } else {
        sign = 1;
    }
    
    power = (tibint32_t) (floor(log10(n)/3.0)*3.0);
    factor = exp(power * log(10));

#if defined(_WIN32)
    _snprintf_s(next, end-next, _TRUNCATE, "% 7.2fE%+03d", sign * n / factor, power);
#else
    snprintf(next, end-next, "% 7.2fE%+03d", sign * n / factor, power);
#endif
}

void
tibAux_PrintNumber(
    double          n)
{
    char            buff[16];

    buff[0] = 0;
    tibAux_StringCatNumber(buff,sizeof(buff),n);
    printf("%s", buff);
}

void
tibAux_StringCat(
    char*           outbuf,
    size_t          outsize,
    const char*     str)
{

    if( outbuf && outsize > (strlen(outbuf)+strlen(str)))
    {
#if defined(_WIN32)
        strcat_s(outbuf,outsize,str);
#else
        strncat(outbuf, str, strlen(str));
#endif
    }
}

void
tibAux_StringCopy(
    char*           outbuf,
    size_t          outsize,
    const char*     str)
{

    if(outbuf && outsize > 1)
    {
        *outbuf='\0';
        tibAux_StringCat(outbuf,outsize,str);
    }
}

FILE*
tibAux_OpenCsvFile(
    const char*     csvFileName)
{
    FILE*           csvFile                 = NULL;
#if defined(_WIN32)
    errno_t                 err;
    char                    buff[256];
    
    err = fopen_s(&csvFile, csvFileName, "w");
    if (err || !csvFile) {
        strerror_s(buff, sizeof(buff), err);
        printf("Unable to open %s for writing: %s\n", csvFileName, buff);
        exit(1);
    }
#else
    csvFile = fopen(csvFileName, "w");

    if (!csvFile) {
        printf("Unable to open %s for writing: %s\n", csvFileName, strerror(errno));
        exit(1);
    }
#endif
    return csvFile;
}

void
tibAux_StatUpdate(
    tibAux_StatRecord*   stats,
    tibint64_t          x)
{
    tibdouble_t delta;

    if (stats->N == 0) {
        stats->min_x = stats->max_x = x;
        stats->mean_x = stats->M2 = 0.0;
    } else if (x < stats->min_x) {
        stats->min_x = x;
    } else if (x > stats->max_x) {
        stats->max_x = x;
    }
    
    stats->N++;
    delta = (double)x - stats->mean_x;
    stats->mean_x += delta / (double)stats->N;
    stats->M2 += delta * ((double)x - stats->mean_x);
}

tibdouble_t
tibAux_StdDeviation(
    tibAux_StatRecord*     stats)
{
    if (stats->N && stats->M2 >= 0.0)
        return sqrt(stats->M2 / (tibdouble_t)stats->N);
    else
        return 0.0;
}

void
tibAux_StringCatStats(
    char*           outbuf,
    size_t          outsize,
    tibAux_StatRecord*     stats,
    tibdouble_t     factor)
{

    tibAux_StringCat(outbuf,outsize,"min/max/avg/dev: ");
    tibAux_StringCatNumber(outbuf,outsize, stats->min_x * factor); 
    tibAux_StringCat(outbuf,outsize," / ");
    tibAux_StringCatNumber(outbuf,outsize, stats->max_x * factor);
    tibAux_StringCat(outbuf,outsize," / ");
    tibAux_StringCatNumber(outbuf,outsize, stats->mean_x * factor);
    tibAux_StringCat(outbuf,outsize," / ");
    tibAux_StringCatNumber(outbuf,outsize, tibAux_StdDeviation(stats) * factor);
}

void
tibAux_PrintStats(
    tibAux_StatRecord*     stats,
    tibdouble_t     factor)
{
    char            buff[512];

    buff[0] = 0;
    tibAux_StringCatStats(buff,sizeof(buff),stats,factor);
    printf("%s", buff);
}

void
tibAux_PrintException(
    tibEx               ex)
{
    tibint32_t      len = tibEx_ToString(ex, NULL, 0);
    char            *exStr = malloc(len);
    
    if (exStr)
    {
        tibEx_ToString(ex, exStr, len);
        printf("%s\n", exStr);
        free(exStr);
    }
    
    tibEx_Clear(ex);
}

void
tibAux_PrintAdvisory(
    tibEx               ex,
    tibMessage          msg)
{
    const char          *svalue = NULL;
    tibDateTime         *tvalue = NULL;
    tibint64_t          lvalue = 0;
    tibdouble_t         dvalue = 0.0;
    const char          **avalue = NULL;
    tibint32_t          asize = 0;
    tibint32_t          i;

    printf("advisory:\n");

    svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_SEVERITY);
    printf("  %s: %s\n", TIB_ADVISORY_FIELD_SEVERITY, svalue);

    svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_MODULE);
    printf("  %s: %s\n", TIB_ADVISORY_FIELD_MODULE, svalue);

    svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_NAME);
    printf("  %s: %s\n", TIB_ADVISORY_FIELD_NAME, svalue);

    svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_REASON);
    printf("  %s: %s\n", TIB_ADVISORY_FIELD_REASON, svalue);

    tvalue = tibMessage_GetDateTime(ex, msg, TIB_ADVISORY_FIELD_TIMESTAMP);
    printf("  %s: sec=%"auxPRId64" nsec=%"auxPRId64"\n", 
           TIB_ADVISORY_FIELD_TIMESTAMP, tvalue->sec, tvalue->nsec);

    lvalue = tibMessage_GetLong(ex, msg, TIB_ADVISORY_FIELD_AGGREGATION_COUNT);
    printf("  %s: %"auxPRId64"\n", TIB_ADVISORY_FIELD_AGGREGATION_COUNT, lvalue);

    dvalue = tibMessage_GetDouble(ex, msg, TIB_ADVISORY_FIELD_AGGREGATION_TIME);
    printf("  %s: %f\n", TIB_ADVISORY_FIELD_AGGREGATION_TIME, dvalue);

    if (tibMessage_IsFieldSet(ex, msg, TIB_ADVISORY_FIELD_ENDPOINTS))
    {
        avalue = (const char**)tibMessage_GetArray(ex, msg, 
                                                   TIB_FIELD_TYPE_STRING_ARRAY,
                                                   TIB_ADVISORY_FIELD_ENDPOINTS,
                                                   &asize);
        printf("  %s:\n", TIB_ADVISORY_FIELD_ENDPOINTS);
        for (i = 0; i < asize; i++)
            printf("    %s\n", avalue[i]);
    }
    
    if (tibMessage_IsFieldSet(ex, msg, TIB_ADVISORY_FIELD_SUBSCRIBER_NAME))
    {
        svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_SUBSCRIBER_NAME);
        printf("  %s: %s\n", TIB_ADVISORY_FIELD_SUBSCRIBER_NAME, svalue);
    }

    fflush(stdout);
}



#if !defined(_WIN32)

typedef struct {
    tibAux_ThreadFunc      func;
    void*           arg;
} pthread_arguments;

static void*
pthread_runner(
    void*           arg)
{
    pthread_arguments* pt = (pthread_arguments*)arg;
    
    pt->func(pt->arg);
    
    free(pt);
    return NULL;
}
#endif

tibAux_ThreadHandle
tibAux_LaunchThread(
    tibAux_ThreadFunc      runner,
    void*           closure)
{
    tibAux_ThreadHandle    returnThread;
#if defined(_WIN32)
    returnThread = (HANDLE) _beginthread(runner, 0, closure);
    if (returnThread == (HANDLE) (-1))
    {
        printf("Unable to start thread.\n");
        exit(1);
    }
#else
    pthread_arguments*  pt = malloc(sizeof(pthread_arguments));
    
    if (!pt)
    {
        printf("Unable to allocate thread storage.\n");
        exit(1);
    }

    pt->func    = runner;
    pt->arg     = closure;
    if (pthread_create(&returnThread, NULL, pthread_runner, pt))
    {
        printf("Unable to start thread.\n");
        exit(1);
    }
#endif
    return returnThread;
}

void
tibAux_DetachThread(
    tibAux_ThreadHandle    t)
{
#if !defined(_WIN32)
    int err;
    
    err = pthread_detach(t);
    
    if (err)
    {
        printf("Unable to detach thread.\n");
        exit(1);
    }
#endif
}

void
tibAux_JoinThread(
    tibAux_ThreadHandle    t)
{
#if defined(_WIN32)
    (void) WaitForSingleObject(t, INFINITE);
#else
    int                         err;
    
    err = pthread_join(t, NULL);
    
    if (err)
    {
        printf("Unable to join thread.\n");
        exit(1);
    }
#endif
}

void
tibAux_SleepMillis(
    tibint32_t      milliseconds)
{
    if (milliseconds <= 0)
        return;
    
#if defined(_WIN32)
    Sleep((int)(milliseconds));
#else
    struct timeval      sleeprec;
    
    sleeprec.tv_sec  = (time_t)(milliseconds / 1000);
    sleeprec.tv_usec = (suseconds_t)(milliseconds % 1000) * 1000;    
    
    select(0, NULL, NULL, NULL, &sleeprec);
#endif
}

void
tibAux_InitializeMutex(
    tibAux_Mutex    *mutex)
{
#if defined(_WIN32)
    InitializeCriticalSection(mutex);
#else

    pthread_mutexattr_t     attr;
    int                     err;
    
    err = pthread_mutexattr_init(&attr);
    if (err) {
        printf("Unable to initialize mutex attributes.\n");
        exit(1);
    }
    
#if defined(PTHREAD_PROCESS_PRIVATE)
    if (err == 0) {
        err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
        if (err) {
            printf("Unable to set mutex attributes.\n");
            exit(1);
        }
    }
#endif
    
#if defined(PTHREAD_MUTEX_RECURSIVE)
    if (err == 0)
    {
        err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        if (err) {
            printf("Unable to set mutex attributes.\n");
            exit(1);
        }
    }
#elif defined(PTHREAD_MUTEX_RECURSIVE_NP)
    if (err == 0)
    {
        err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        if (err) {
            printf("Unable to set mutex attributes.\n");
            exit(1);
        }
    }
#endif
    
    if (err == 0)
    {
        err = pthread_mutex_init(mutex, &attr);
        if (err)
        {
            printf("Unable to create mutex.\n");
            exit(1);
        }
    }
    (void) pthread_mutexattr_destroy(&attr);
#endif
}

void
tibAux_CleanupMutex(
    tibAux_Mutex    *mutex)
{
#ifdef _WIN32
    DeleteCriticalSection(mutex);
#else
    pthread_mutex_destroy(mutex);
#endif
}

struct tibAux_Histogram_struct
{
    tibdouble_t         min;
    tibdouble_t         lmin;
    tibdouble_t         max;
    tibdouble_t         lsize;
    tibint64_t          total;
    tibint32_t          n;
    tibbool_t           underflow;
    tibbool_t           overflow;
    tibint32_t          *bins;
};

tibAux_Histogram
tibAux_CreateHistogram(
    tibdouble_t         min,
    tibdouble_t         max,
    tibint32_t          steps)
{
    tibAux_Histogram    h;

    h = calloc(1, sizeof(*h));
    if (!h)
        return NULL;

    h->min = min;
    h->lmin = log10(min);
    h->max = max;
    h->lsize = log10(max) - h->lmin;
    h->n = steps;

    h->bins = calloc(steps, sizeof(*h->bins));
    if (!h->bins)
    {
        free(h);
        return NULL;
    }

    return h;
}

void
tibAux_DestroyHistogram(
    tibAux_Histogram    h)
{
    if (h)
    {
        if (h->bins)
            free(h->bins);

        free(h);
    }
}

static tibint32_t
binForPoint(tibAux_Histogram h, tibdouble_t x)
{
    return (tibint32_t)(h->n * (log10(x) - h->lmin) / h->lsize);
}

static tibdouble_t
pointForBin(tibAux_Histogram h, tibint32_t i)
{
    return pow(10, ((tibdouble_t)i / h->n) * h->lsize + h->lmin);
}

void
tibAux_AddPointToHistogram(
        tibAux_Histogram    h,
        tibdouble_t         x)
{
    tibint32_t  bin;

    bin = binForPoint(h, x);

    if (bin < 0)
    {
        bin = 0;
        h->underflow = tibtrue;
    }
    else if (bin >= h->n)
    {
        bin = h->n - 1;
        h->overflow = tibtrue;
    }

    h->bins[bin] += 1;
    h->total += 1;
}

void
tibAux_DumpHistogramToFile(
    tibAux_Histogram    h,
    const char*         fname)
{
    tibint32_t          i;
    tibint64_t          frac = 0;
    FILE*   f = tibAux_OpenCsvFile(fname);

    if (!f)
        return;

    for (i = 0; i < h->n;  i++)
    {
        if (h->bins[i])
        {
            frac += h->bins[i];
            fprintf(f, "%d,%.12e,%d,%.12e,%.12e\n",
                    i,
                    pointForBin(h, i),
                    h->bins[i],
                    ((tibdouble_t)h->bins[i])/((tibdouble_t)h->total),
                    ((tibdouble_t)frac)/((tibdouble_t)h->total));
        }
    }

    fflush(f);
    fclose(f);
}


static tibint32_t
findFraction(tibAux_Histogram h, tibdouble_t fraction)
{
    tibint32_t      i;
    tibint32_t      count = 0;
    tibint32_t      target = (tibint32_t)((1.0 - fraction) * h->total);

    for (i = h->n-1;  i >= 0;  i--)
    {
        count += h->bins[i];
        if (count >= target)
        {
            return i;
        }
    }

    return 0;
}

static void
printFraction(FILE* fileDescriptor, tibAux_Histogram h, tibdouble_t fraction)
{
    tibint32_t      i;
    char            buff[16];

    i = findFraction(h, fraction);
    fprintf(fileDescriptor, "%2.3f%% is between ", fraction * 100.0);

    if (i == 0 && h->underflow)
    {
        fprintf(fileDescriptor, "0 and ");
    }
    else
    {
        buff[0] = 0;
        tibAux_StringCatNumber(buff,sizeof(buff),pointForBin(h, i));
        printf("%s and ", buff);
    }

    if (i == h->n-1 && h->overflow)
    {
        fprintf(fileDescriptor, "more\n");
    }
    else
    {
        buff[0] = 0;
        tibAux_StringCatNumber(buff,sizeof(buff),pointForBin(h, i+1));
        printf("%s\n", buff);
    }
}

void
tibAux_PrintFractions(
    tibAux_Histogram    h,
    FILE*               fileDescriptor)
{
    printFraction(fileDescriptor, h, 0.5);
    printFraction(fileDescriptor, h, 0.9);
    printFraction(fileDescriptor, h, 0.99);
    printFraction(fileDescriptor, h, 0.999);
    printFraction(fileDescriptor, h, 0.9999);
    printFraction(fileDescriptor, h, 0.99999);
}

void
tibAux_MillisecondsToStr(
    tibint64_t  millis,
    char        *buf,
    tibint32_t  size)
{
    time_t seconds = 0;
    struct tm *tm;

    seconds = (time_t)(millis/1000);
    tm = localtime(&seconds); 
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm);
}

#if defined(_WIN32)
tibdouble_t
tibAux_Rand(void)
{
    unsigned int randomValue;
    rand_s(&randomValue);
    return (tibdouble_t)randomValue / (tibdouble_t)INT_MAX;
}
#else
tibdouble_t
tibAux_Rand(void)
{
    return (tibdouble_t)random() / (tibdouble_t)INT_MAX;
}
#endif
