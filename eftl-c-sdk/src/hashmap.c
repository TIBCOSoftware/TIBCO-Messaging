/*
 * Copyright (c) $Date: 2020-03-31 10:23:10 -0700 (Tue, 31 Mar 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: hashmap.c 123342 2020-03-31 17:23:10Z $
 *
 */

#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

typedef struct entry_s entry_t;

struct entry_s
{
    size_t hash;
    char* key;
    void* value;
    entry_t* next;
};

struct hashmap_s
{
    entry_t** buckets;
    size_t bucketCount;
    size_t size;
};

static entry_t* entry_create(size_t hash, const char* key, void* value)
{
    entry_t* entry = calloc(1, sizeof(entry_t));
    if (!entry)
        return NULL;

    entry->hash = hash;
    entry->value = value;
    entry->next = NULL;
    if (key)
    {
        entry->key = strdup(key);
        if (!(entry->key))
        {
            free(entry);
            entry = NULL;
        }
    }

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

hashmap_t* hashmap_create(size_t initialSize)
{
    hashmap_t* map;
    size_t minBucketCount;

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
    size_t i;
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

static size_t hash_key(const char* key)
{
    size_t hash = strlen(key);
    unsigned char c;
    while ((c = (unsigned char)*key++))
    {
        hash = hash * 31 + c;
    }
    return hash;
}

static void hashmap_rehash(hashmap_t* map)
{
    size_t i;
    if (map->size > (map->bucketCount * 3 / 4))
    {
        size_t newBucketCount = map->bucketCount << 1;
        entry_t** newBuckets = calloc(newBucketCount, sizeof(entry_t*));
        if (newBuckets == NULL)
            return;

        for (i = 0; i < map->bucketCount; i++)
        {
            entry_t* entry = map->buckets[i];

            while (entry)
            {
                entry_t* next = entry->next;
                size_t index = entry->hash & newBucketCount;
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
    size_t hash, indx;

    hash = hash_key(key);
    indx = hash & (map->bucketCount - 1);

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
    size_t hash, indx;

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
    size_t hash, indx;

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

size_t hashmap_size(hashmap_t* map)
{
    return map->size;
}

size_t hashmap_collisions(hashmap_t* map)
{
    size_t i, collisions = 0;
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
    size_t i;
    for (i = 0; i < map->bucketCount; i++)
    {
        entry_t* entry;
        for (entry = map->buckets[i]; entry; entry = entry->next)
        {
            callback(entry->key, entry->value, context);
        }
    }
}

static void hashmap_merge(const char* key, void* value, void* context)
{
    hashmap_t* copy = (hashmap_t*)context;

    hashmap_put(copy, key, value);
}

hashmap_t* hashmap_copy(hashmap_t* map)
{
    hashmap_t* copy = hashmap_create(map->bucketCount);
    if (!copy)
        return NULL;

    hashmap_iterate(map, hashmap_merge, copy);

    return copy;
}
