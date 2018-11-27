/*
 * Copyright (c) 2018: 2017-06-02 19:29:54 -0500 (Fri, 02 Jun 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: hashmap.h 93815 2017-06-03 00:29:54Z bpeterse $
 *
 */

#ifndef INCLUDED_TIBEFTL_HASHMAP_URL
#define INCLUDED_TIBEFTL_HASHMAP_URL

typedef struct hashmap_s hashmap_t;

hashmap_t* hashmap_create(int initialSize);

void hashmap_destroy(hashmap_t* map);

void* hashmap_put(hashmap_t* map, const char* key, void* entry);

void* hashmap_get(hashmap_t* map, const char* key);

void* hashmap_remove(hashmap_t* map, const char* key);

int hashmap_size(hashmap_t* map);

int hashmap_collisions(hashmap_t* map);

void hashmap_iterate(hashmap_t* map,
        void (*callback)(const char* key, void* value, void* context),
        void* context);

#endif /* INCLUDED_TIBEFTL_HASHMAP_URL */
