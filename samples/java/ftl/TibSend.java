/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic java FTL publisher program which sends a single msg.
 */

package com.tibco.ftl.samples;

import com.tibco.ftl.*;
import com.tibco.ftl.exceptions.*;

public class TibSend 
{
    String          realmServer = "http://localhost:8080";
    Publisher       pub         = null;
    Message         msg         = null;
    Realm           realm       = null;

    String          usageString =
        "TibSend url\n" +
        "Default url is " + realmServer + "\n";

    public TibSend(String[] args)
    {
        System.out.printf("#\n# %s\n#\n# %s\n#\n",
                          this.getClass().getName(),
                          FTL.getVersionInformation());

        if (args.length > 0 && args[0] != null)
            realmServer = args[0];
    }

    public int send() throws FTLException
    {
        // connect to the realmserver.
        realm = FTL.connectToRealmServer(realmServer, null, null);

        // Create sender object and message to be sent
        pub = realm.createPublisher(null);

        msg = realm.createMessage(null);
        msg.setString("type", "hello");
        msg.setString("message", "hello world earth");

        System.out.printf("sending 'hello world' message\n");
        pub.send(msg);

        // Clean up
        msg.destroy();
        pub.close();
        realm.close();

        return 0;
    }
    
    public static void main(String[] args)
    {
        TibSend s  = new TibSend(args);
        try {
            s.send();
        }
        catch (Throwable e) {
            e.printStackTrace();
        }
    }
}
