/*
 * Copyright (c) 2018: 2017-09-24 09:56:18 -0500 (Sun, 24 Sep 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: hashmap.c 96452 2017-09-24 14:56:18Z bpeterse $
 *
 */

#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

typedef struct entry_s entry_t;

struct entry_s
{
    int hash;
    char* key;
    void* value;
    entry_t* next;
};

struct hashmap_s
{
    entry_t** buckets;
    int bucketCount;
    int size;
};

static entry_t* entry_create(int hash, const char* key, void* value)
{
    entry_t* entry = calloc(1, sizeof(entry_t));
    if (!entry)
        return NULL;

    entry->hash = hash;
    if (key)
        entry->key = strdup(key);
    entry->value = value;
    entry->next = NULL;

    return entry;
}

static void entry_destroy(entry_t* entry)
{
    if (!entry)
        return;

    if (entry->key)
        free(entry->key);
    free(entry);
}

hashmap_t* hashmap_create(int initialSize)
{
    hashmap_t* map;
    int minBucketCount;

    map = malloc(sizeof(hashmap_t));
    if (!map)
        return NULL;

    map->bucketCount = 1;
    map->size = 0;

    // load factor of 0.75
    minBucketCount = initialSize * 4 / 3;
    while (map->bucketCount <= minBucketCount)
    {
        // power of 2
        map->bucketCount <<= 1;
    }

    map->buckets = calloc(map->bucketCount, sizeof(entry_t*));
    if (!map->buckets)
    {
        free(map);
        return NULL;
    }

    return map;
}

void hashmap_destroy(hashmap_t* map)
{
    int i;
    for (i = 0; i < map->bucketCount; i++)
    {
        entry_t* entry = map->buckets[i];
        while (entry)
        {
            entry_t* next = entry->next;
            entry_destroy(entry);
            entry = next;
        }
    }

    free(map->buckets);
    free(map);
}

static int hash_key(const char* key)
{
    int hash = strlen(key);
    char c;
    while ((c = *key++))
    {
        hash = hash * 31 + c;
    }
    return hash;
}

static void hashmap_rehash(hashmap_t* map)
{
    int i;
    if (map->size > (map->bucketCount * 3 / 4))
    {
        int newBucketCount = map->bucketCount << 1;
        entry_t** newBuckets = calloc(newBucketCount, sizeof(entry_t*));
        if (newBuckets == NULL)
            return;

        for (i = 0; i < map->bucketCount; i++)
        {
            entry_t* entry = map->buckets[i];

            while (entry)
            {
                entry_t* next = entry->next;
                int index = entry->hash & newBucketCount;
                entry->next = newBuckets[index];
                newBuckets[index] = entry;
                entry = next;
            }
        }

        free(map->buckets);
        map->buckets = newBuckets;
        map->bucketCount = newBucketCount;
    }
}

void* hashmap_put(hashmap_t* map, const char* key, void* value)
{
    entry_t** ptr;
    entry_t* entry;
    int hash, indx;

    hash = hash_key(key);
    indx = ((int)hash) & (map->bucketCount - 1);

    ptr = &(map->buckets[indx]);

    for (;;)
    {
        entry = *ptr;

        if (entry == NULL)
        {
            *ptr = entry_create(hash, key, value);
            if (*ptr == NULL) {
                return NULL;
            }
            map->size++;
            hashmap_rehash(map);
            return NULL;
        }

        if (strcmp(entry->key, key) == 0)
        {
            void* oldValue = entry->value;
            entry->value = value;
            return oldValue;
        }

        ptr = &entry->next;
    }
}

void* hashmap_get(hashmap_t* map, const char* key)
{
    entry_t* entry;
    int hash, indx;

    hash = hash_key(key);
    indx = hash & (map->bucketCount - 1);

    entry = map->buckets[indx];

    while (entry != NULL)
    {
        if (strcmp(key, entry->key) == 0)
            return entry->value;

        entry = entry->next;
    }

    return NULL;
}

void* hashmap_remove(hashmap_t* map, const char* key)
{
    entry_t** ptr;
    entry_t* entry;
    int hash, indx;

    hash = hash_key(key);
    indx = hash & (map->bucketCount - 1);

    ptr = &(map->buckets[indx]);

    while ((entry = *ptr))
    {
        if (strcmp(key, entry->key) == 0)
        {
            void* value = entry->value;
            *ptr = entry->next;
            entry_destroy(entry);
            map->size--;
            return value;
        }

        ptr = &entry->next;
    }

    return NULL;
}

int hashmap_size(hashmap_t* map)
{
    return map->size;
}

int hashmap_collisions(hashmap_t* map)
{
    int i, collisions = 0;
    for (i = 0; i < map->bucketCount; i++)
    {
        entry_t* entry = map->buckets[i];
        while (entry != NULL)
        {
            if (entry->next != NULL)
            {
                collisions++;
            }
            entry = entry->next;
        }
    }
    return collisions;
}

void hashmap_iterate(hashmap_t* map,
        void (*callback)(const char* key, void* value, void* context),
        void* context)
{
    int i;
    for (i = 0; i < map->bucketCount; i++)
    {
        entry_t* entry;
        for (entry = map->buckets[i]; entry; entry = entry->next)
        {
            callback(entry->key, entry->value, context);
        }
    }
}

