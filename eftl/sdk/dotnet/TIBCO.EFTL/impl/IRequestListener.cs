/*
 * Copyright (c) 2001-$Date: 2015-10-26 22:07:01 -0700 (Mon, 26 Oct 2015) $ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id: ICompletionListener.cs 82627 2015-10-27 05:07:01Z $
 *
 */
using System;

namespace TIBCO.EFTL 
{
    public interface IRequestListener 
    {
        void OnSuccess(IMessage response);
        void OnError(String reason);
        void OnError(int code, String reason);
    }
}
