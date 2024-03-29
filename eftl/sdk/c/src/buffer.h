/*
 * Copyright (c) $Date: 2017-05-25 10:22:38 -0700 (Thu, 25 May 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: buffer.h 93627 2017-05-25 17:22:38Z $
 *
 */

#ifndef INCLUDED_TIBEFTL_BUFFER_H
#define INCLUDED_TIBEFTL_BUFFER_H

typedef struct buffer_s
{
    unsigned char*    data;

    unsigned char*    position;
    unsigned char*    limit;

    int               size;

} buffer_t;

#define buffer_data(buf)        ((buf)->data)
#define buffer_size(buf)        ((buf)->size)
#define buffer_position(buf)    ((buf)->position)
#define buffer_limit(buf)       ((buf)->limit)
#define buffer_remaining(buf)   ((buf)->limit - (buf)->position)

#define buffer_ready(buf, size) \
    { buffer_limit(buf) = buffer_data(buf) + size; \
      buffer_position(buf) = buffer_data(buf); }

static inline buffer_t* buffer_create(int size)
{
    buffer_t*         buf;

    buf = malloc(sizeof(*buf));

    if (buf)
    {
        buf->size = (size < 32 ? 32 : size);
        buf->data = malloc(buf->size);
        buf->position  = buf->data;
        buf->limit = buf->data + buf->size;
    }

    return buf;
}

static inline void buffer_destroy(buffer_t* buf)
{
    if (!buf)
        return;

    free(buf->data);
    free(buf);
}

#endif /* INCLUDED_TIBEFTL_BUFFER_H */
