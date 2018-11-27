/*
 * Copyright (c) 2001-$Date: 2018-05-14 16:33:28 -0500 (Mon, 14 May 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: RequestListener.java 101286 2018-05-14 21:33:28Z bpeterse $
 *
 */
package com.tibco.eftl.impl;

import com.tibco.eftl.*;

public interface RequestListener {
    public void onSuccess(Message response);

    public void onError(String reason);

    public void onError(int code, String reason);
}
