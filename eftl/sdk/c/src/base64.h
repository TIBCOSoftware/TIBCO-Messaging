/* 
 * Copyright (c) 2009-2017 Cloud Software Group, Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * 
 * $Id$
 * 
 */

#ifndef INCLUDED_TIBEFTL_BASE64_H
#define INCLUDED_TIBEFTL_BASE64_H

#define BASE64_ENC_MAX_LEN(slen) (((slen)*4)/3+4+1)
#define BASE64_DEC_MAX_LEN(slen) ((slen)*3/4+3)

int
base64_encode(
    const unsigned char*    binaryData,
    int                     binaryDataSize,
    char*                   text,
    int                     textSize);

int
base64_decode(
    unsigned char*          binaryData,
    int                     binaryDataSize,
    const char*             text,
    int                     textSize);

#endif /* INCLUDED_TIBEFTL_BASE64_H */
