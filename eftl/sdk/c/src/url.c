/*
 * Copyright (c) $Date: 2022-01-14 14:03:58 -0800 (Fri, 14 Jan 2022) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: url.c 138851 2022-01-14 22:03:58Z $
 *
 */

#include <stdlib.h>
#include <string.h>

#include "url.h"

#if defined(_WIN32)
#define strncasecmp _strnicmp
#endif

struct url_s
{
    char*       scheme;
    char*       username;
    char*       password;
    char*       host;
    char*       port;
    char*       path;

    int         secure;

    url_query_t *query;
};

struct url_query_s
{
    char*       key;
    char*       value;

    url_query_t *next;
};

static void
url_parse_query(
    url_t*      url,
    char*       src)
{
    int         i;
    char*       ptr;
    url_query_t *query = NULL, *head = NULL;

    if (!url)
        return;

    while (src)
    {
        query = calloc(1, sizeof(*query));

        i = strcspn(src, "&;");
        if (src[i] == '&' || src[i] == ';')
        {
            src[i] = '\0';
            ptr = &(src[i+1]);
        }
        else
        {
            ptr = NULL;
        }

        i = strcspn(src, "=");
        if (src[i] == '=')
        {
            src[i] = '\0';
            query->value = strdup(&(src[i+1]));
        }

        query->key = strdup(src);
        query->next = head;

        head = query;

        src = ptr;
    }

    url->query = query;
}

url_t*
url_parse(
    const char* src)
{
    url_t*      url;
    char*       cpy;
    char*       ptr;
    int         i;

    url = calloc(1, sizeof(*url));

    if (!src)
        return url;

    // make an editable copy of the source
    cpy = strdup(src);

    // pull out the query string and parse
    if ((ptr = strrchr(cpy, '?')))
    {
        *ptr = '\0';
        url_parse_query(url, ++ptr);
    }
   
    // parse protocol
    if ((ptr = strstr(cpy, "://")))
    {
        *ptr = '\0';
        url->scheme = strdup(cpy);
        ptr += 3;

        // check for secure protocol
        if (strncasecmp(url->scheme, "https", 5) == 0 ||
            strncasecmp(url->scheme, "wss", 3) == 0)
            url->secure = 1;
    }
    else
    {
        ptr = cpy;
    }

    // parse username and password
    i = strcspn(ptr, "@");
    if (ptr[i] == '@')
    {
        char* host = &(ptr[i+1]);

        ptr[i] = '\0';

        i = strcspn(ptr, ":");
        if (ptr[i] == ':')
        {
            ptr[i] = '\0';
            url->password = strdup(&(ptr[i+1]));
        }

        url->username = strdup(ptr);

        ptr = host;
    }

    // parse host:port/path
    if (strchr(ptr, '['))
    {
        i = strcspn(ptr, "]");
        if (ptr[i] == ']')
        {
            ptr[i] = '\0';
            url->host = strdup((ptr+1));
            ptr = &(ptr[i+1]);
        }
    }

    i = strcspn(ptr, "/");
    if (ptr[i] == '/')
    {
        url->path = strdup(ptr+i);
        ptr[i] = '\0';
    }

    i = strcspn(ptr, ":");
    if (ptr[i] == ':')
    {
        ptr[i] = '\0';
        url->port = strdup(&(ptr[i+1]));
    }

    if (*ptr != '\0')
        url->host = strdup(ptr);

    free(cpy);

    // supply defaults for missing elements
    if (!url->scheme) url->scheme = strdup((url->secure ? "https" : "http"));
    if (!url->host) url->host = strdup("localhost");
    if (!url->port) url->port = strdup((url->secure ? "443" : "80"));
    if (!url->path) url->path = strdup("/");

    return url;
}

void
url_destroy(
    url_t*      url)
{
    url_query_t *query;

    if (!url)
        return;

    free(url->scheme);
    free(url->username);
    free(url->password);
    free(url->host);
    free(url->port);
    free(url->path);

    query = url->query;

    while (query)
    {
        url_query_t *tmp = query;

        query = query->next;

        free(tmp->key);
        free(tmp->value);
        free(tmp);
    }

    free(url);
}

url_t*
url_copy(
    url_t*          url)
{
    url_t*          cpy;
    url_query_t*    query;
    url_query_t*    queryCpy;
    url_query_t*    prev;

    if (!url)
        return NULL;

    cpy = calloc(1, sizeof(*cpy));

    if (url->scheme)
        cpy->scheme = strdup(url->scheme);
    if (url->username)
        cpy->username = strdup(url->username);
    if (url->password)
        cpy->password = strdup(url->password);
    if (url->host)
        cpy->host = strdup(url->host);
    if (url->port)
        cpy->port = strdup(url->port);
    if (url->path)
        cpy->path = strdup(url->path);

    cpy->secure = url->secure;

    query = url->query;
    prev = NULL;

    while (query)
    {
        queryCpy = calloc(1, sizeof(*queryCpy));

        if (query->key)
            queryCpy->key = strdup(query->key);
        if (query->value)
            queryCpy->value = strdup(query->value);

        if (!prev)
            cpy->query = queryCpy;
        else
            prev->next = queryCpy;

        query = query->next;
        prev = queryCpy;
    }

    return cpy;
}

const char*
url_scheme(
    url_t*      url)
{
    return (url ? url->scheme : NULL);
}

const char*
url_host(
    url_t*      url)
{
    return (url ? url->host : NULL);
}

const char*
url_port(
    url_t*      url)
{
    return (url ? url->port : NULL);
}

const char*
url_path(
    url_t*      url)
{
    return (url ? url->path : NULL);
}

const char*
url_username(
    url_t*      url)
{
    return (url ? url->username : NULL);
}

const char*
url_password(
    url_t*      url)
{
    return (url ? url->password : NULL);
}

int
url_is_secure(
    url_t*      url)
{
    return (url ? url->secure : 0);
}

const char*
url_query(
    url_t*      url,
    const char* key)
{
    url_query_t *query;
    
    query = url->query;

    while (query)
    {
        if (strcmp(key, query->key) == 0)
            break;

        query = query->next;
    }

    return (query ? query->value : NULL);
}

url_query_t*
url_query_list(
    url_t*      url)
{
    return (url ? url->query : NULL);
}

const char*
url_query_key(
    url_query_t *query)
{
    return (query ? query->key : NULL);
}

const char*
url_query_value(
    url_query_t *query)
{
    return (query ? query->value : NULL);
}

url_query_t*
url_query_next(
    url_query_t *query)
{
    return (query ? query->next : NULL);
}

url_t**
url_list_parse(
    const char* str)
{
    int         size;
    url_t**     list;
    char*       copy;
    char*       ptr;
    int         i;

    if (!str)
        return NULL;

    size = 1;

    for (i = 0; str[i]; i++)
    {
        size += (str[i] == '|');
    }

    list = (url_t**)calloc(size + 1, sizeof(url_t*));

    copy = strdup(str);

    ptr = copy;

    for (i = 0; i < size && ptr; i++)
    {
        char* next = strchr(ptr, '|');
        if (next)
        {
            *next = '\0';
            ++next;
        }

        list[i] = url_parse(ptr);

        ptr = next;
    }

    free(copy);

    return list;
}

void
url_list_destroy(
    url_t**     list)
{
    url_t*      url;
    int         i = 0;

    if (!list)
        return;

    while ((url = list[i++]))
    {
        url_destroy(url);
    }

    free(list);
}

int
url_list_count(
    url_t**     list)
{
    int         i, cnt = 0;

    if (!list)
        return 0;

    for (i = 0; list[i]; i++)
        ++cnt;

    return cnt;
}

void
url_list_shuffle(
    url_t**     list)
{
    int         n, i, j;
    url_t*      u;

    if (!list)
        return;

    n = url_list_count(list);

    for (i = n - 1; i > 0; i--)
    {
        j = rand() % (i + 1);
        u = list[j];
        list[j] = list[i];
        list[i] = u;
    }
}
