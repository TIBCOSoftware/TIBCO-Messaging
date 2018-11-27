/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibemsUtilities.c 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

#include "tibemsUtilities.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
# include <time.h>
# include <sys/timeb.h>
#else
# include <sys/time.h>
#endif

/*-----------------------------------------------------------------------
 * common wrapper to wait for other threads
 *----------------------------------------------------------------------*/
void
ThreadJoin(
    int         threads,
    THREAD_OBJ* threadArray)
{
    int         i;

    for (i = 0; i < threads; i++)
    {
#if defined(_WIN32)
        WaitForSingleObject(threadArray[i], INFINITE);
#else
        pthread_join(threadArray[i], NULL);
#endif
    }
}

/*-----------------------------------------------------------------------
 * host name verifier
 *----------------------------------------------------------------------*/
static tibems_status
_verifyHostName(
    const char*     connected_hostname,
    const char*     expected_hostname,
    const char*     certificate_name,
    void*           closure)
{

    fprintf(stderr,"CUSTOM VERIFIER:\n"\
            "    connected: [%s]\n"\
            "    expected:  [%s]\n"\
            "    certCN:    [%s]\n",
        connected_hostname ? connected_hostname : "(null)",
        expected_hostname ? expected_hostname : "(null)",
        certificate_name ? certificate_name : "(null)");

    return TIBEMS_OK;
}

/*---------------------------------------------------------------------
 * current time in milliseconds
 *---------------------------------------------------------------------*/
tibems_long currentMillis(void)
{
    tibems_long         result;

#if defined(_WIN32)
    struct _timeb       current;

    _ftime(&current);
  
    result = ((tibems_long)current.time * 1000) +
        (tibems_long)current.millitm;

#else
    struct timeval      current;

    gettimeofday(&current, NULL);
    result = ((tibems_long)current.tv_sec * 1000) +
        ((tibems_long)current.tv_usec / 1000);
#endif

    return result;
}

/*---------------------------------------------------------------------
 * allocate memory and copy input string
 *---------------------------------------------------------------------*/
char* stringdup(const char* s)
{
#if defined(_WIN32)
    return _strdup(s);
#elif defined(__OS400__)
    char *new_s;
    new_s = (char *)malloc(strlen(s)); 
    return strcpy(new_s, s);   
#else
    return strdup(s);
#endif
}

void
sslUsage()
{
    printf("\n");
    printf("   where ssl options are:\n");
    printf("\n");
    printf("   -ssl_trusted\n");
    printf("   -ssl_issuer\n");
    printf("   -ssl_identity\n");
    printf("   -ssl_key\n");
    printf("   -ssl_password\n");
    printf("   -ssl_hostname\n");
    printf("   -ssl_custom\n");
    printf("   -ssl_noverifyhost\n");
    printf("   -ssl_noverifyhostname\n");
    printf("   -ssl_ciphers\n");
    printf("   -ssl_trace\n");
    printf("   -ssl_debug_trace\n");
    exit(0);
}
/*-----------------------------------------------------------------------
 * setSSLParams
 *----------------------------------------------------------------------*/
void setSSLParams(
    tibemsSSLParams ssl_params,
    int argc, 
    char* argv[],
    char* *pk_password)
{
    tibems_status status = TIBEMS_OK;
    tibems_bool certs = TIBEMS_FALSE;
    int i=1;
    

    while(i < argc)
    {
        if (strcmp(argv[i],"-ssl_trusted")==0) 
        {
            if ((i+1) >= argc) sslUsage();

            status = tibemsSSLParams_AddTrustedCertFile(ssl_params,argv[i+1],
                                    TIBEMS_SSL_ENCODING_AUTO);
            if (status != TIBEMS_OK)
            {
                fprintf(stderr,"Error: invalid trusted parameter '%s'\n",
                    argv[i+1]);
                sslUsage();
            }
            certs = TIBEMS_TRUE;
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ssl_issuer")==0) 
        {
            if ((i+1) >= argc) sslUsage();

            status = tibemsSSLParams_AddIssuerCertFile(ssl_params,argv[i+1],
                                TIBEMS_SSL_ENCODING_AUTO);
            if (status != TIBEMS_OK)
            {
                fprintf(stderr,"Error: invalid issuer parameter '%s'\n",
                    argv[i+1]);
                sslUsage();
            }
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ssl_identity")==0) 
        {
            if ((i+1) >= argc) sslUsage();

            status = tibemsSSLParams_SetIdentityFile(ssl_params,argv[i+1],
                                    TIBEMS_SSL_ENCODING_AUTO);
            if (status != TIBEMS_OK)
            {
                fprintf(stderr,"Error: invalid identity parameter '%s'\n",
                    argv[i+1]);
                sslUsage();
            }
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ssl_key")==0) 
        {
            if ((i+1) >= argc) sslUsage();

            status = tibemsSSLParams_SetPrivateKeyFile(ssl_params,argv[i+1],
                                    TIBEMS_SSL_ENCODING_AUTO);
            if (status != TIBEMS_OK)
            {
                fprintf(stderr,"Error: invalid private key parameter '%s'\n",
                    argv[i+1]);
                sslUsage();
            }
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ssl_hostname")==0) 
        {
            if ((i+1) >= argc) sslUsage();

            tibemsSSLParams_SetExpectedHostName(ssl_params,argv[i+1]);
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ssl_custom")==0) 
        {
            tibemsSSLParams_SetHostNameVerifier(ssl_params,_verifyHostName,NULL);
            argv[i] = NULL;
            i += 1;
        }
        else
        if (strcmp(argv[i],"-ssl_noverifyhost")==0) 
        {
            tibemsSSLParams_SetVerifyHost(ssl_params,TIBEMS_FALSE);
            argv[i] = NULL;
            i += 1;
        }
        else
        if (strcmp(argv[i],"-ssl_noverifyhostname")==0) 
        {
            tibemsSSLParams_SetVerifyHostName(ssl_params,TIBEMS_FALSE);
            argv[i] = NULL;
            i += 1;
        }
        else
        if (strcmp(argv[i],"-ssl_ciphers")==0) 
        {
        if ((i+1) >= argc) sslUsage();

            status = tibemsSSLParams_SetCiphers(ssl_params,argv[i+1]);
            if (status != TIBEMS_OK)
            {
                fprintf(stderr,"Error: invalid ciphers '%s'\n",
                    argv[i+1]);
                sslUsage();
            }
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ssl_trace")==0) 
        {
            tibemsSSL_SetTrace(TIBEMS_TRUE);
            argv[i] = NULL;
            i += 1;
        }
        else
        if (strcmp(argv[i],"-ssl_debug_trace")==0) 
        {
            tibemsSSL_SetDebugTrace(TIBEMS_TRUE);
            argv[i] = NULL;
            i += 1;
        }
        else
        if (strcmp(argv[i],"-ssl_password")==0) 
        {
            if ((i+1) >= argc) sslUsage();
            
            *pk_password = argv[i+1];
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else 
        {
            i += 1;
        }
    }

    if(!certs)
        tibemsSSLParams_SetVerifyHost(ssl_params,TIBEMS_FALSE);
}

