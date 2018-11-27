/*
 * Copyright (c) $Date: 2017-08-21 11:33:11 -0500 (Mon, 21 Aug 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: msg.h 95502 2017-08-21 16:33:11Z bpeterse $
 *
 */

#ifndef INCLUDED_TIBEFTL_MSG_H
#define INCLUDED_TIBEFTL_MSG_H

#include "cJSON.h"

tibeftlMessage
tibeftlMessage_CreateWithJSON(
    _cJSON*                     json);

_cJSON*
tibeftlMessage_GetJSON(
    tibeftlMessage              message);

#endif /* INCLUDED_TIBEFTL_MSG_H */
