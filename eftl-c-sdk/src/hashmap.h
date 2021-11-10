/*
 * Copyright (c) 2021 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: hashmap.h 122649 2020-03-11 19:11:05Z $
 *
 */

#ifndef INCLUDED_TIBEFTL_HASHMAP_URL
#define INCLUDED_TIBEFTL_HASHMAP_URL

typedef struct hashmap_s hashmap_t;

hashmap_t* hashmap_create(size_t initialSize);

void hashmap_destroy(hashmap_t* map);

void* hashmap_put(hashmap_t* map, const char* key, void* entry);

void* hashmap_get(hashmap_t* map, const char* key);

void* hashmap_remove(hashmap_t* map, const char* key);

size_t hashmap_size(hashmap_t* map);

size_t hashmap_collisions(hashmap_t* map);

void hashmap_iterate(hashmap_t* map,
        void (*callback)(const char* key, void* value, void* context),
        void* context);

hashmap_t* hashmap_copy(hashmap_t* map);

#endif /* INCLUDED_TIBEFTL_HASHMAP_URL */
