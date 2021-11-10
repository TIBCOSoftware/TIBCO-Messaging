/* 
 * Copyright (c) 2009-2021 TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: base64.h 93970 2017-06-09 21:20:05Z $
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
