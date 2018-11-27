/*
 * Copyright (c) 2013-$Date: 2017-09-06 13:06:54 -0500 (Wed, 06 Sep 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

Running the eFTL iOS Sample Applications
----------------------------------------

The iOS sample applications demonstrate how to connect to an
eFTL channel, subscribe to messages, and publish messages.

Steps required to build the eFTL iOS sample applications:

  1. Open the eFTL iOS sample Xcode project.

  2. Add the eFTL Objective-C framework to the project by either
     dragging the eFTL framework  bundle from a Finder window into
     the project in the project navigator, or by choosing the menu
     options File > Add Files to "<App_Name>" and selecting the
     eFTL framework bundle.

  3. Build and run the eFTL iOS sample application.


Creating a new eFTL iOS Application
-----------------------------------

When creating a new eFTL iOS application, along with the inclusion of the
eFTL framework bundle, the following steps are required to set up the proper 
dependencies and configuration for your project. These steps are already
a part of the eFTL iOS sample Xcode projects: 

  1. Add the required built-in frameworks to the project.

      a. In the project navigator, select the project.

      b. Click "Build Phases" at the top of the project editor.

      c. Open the "Link Binary With Libraries" section.

      d. Click the add (+) button to add a library or framework.

      e. Enter the following library and frameworks in the search field, 
         select and "Add".

            libicucore.tbd
            CFNetwork.framework
            Security.framework

  2. Add the -ObjC value to the "Other Linker Flags" build setting.

      a. In the project navigator, select the target.

      b. Click the "Build Setting" tab and scroll down to the "Linking"
         section.

      c. In "Other Linker Flags" add the value "-ObjC".


eFTL and Apple Push Notifications
---------------------------------

Configuring the application for Push Notifications

  The eFTL server can send push notifications to a disconnected application
  alerting the application that messages are available. The following steps
  can be used to configure an application for receiving push notifications
  from the Apple Push Notification service.

    1. In the project navigator, select the project.

    2. Click "Capabilities" at the top of the project editor.

    3. Enable "Push Notifications".

    4. Enable "Background Modes" and check the "Remote notification" checkbox.


Requesting a Device Token from the Apple Push Notification service

  Once configured for push notifications, the application must retrieve
  a device token from the Apple Push Notification service and send the
  device token to the eFTL server at the time of connection.

  The device token request is called from  the app delegate's 
  didFinishLaunchingWithOptions: method: 

    [[UIApplication sharedApplication] registerForRemoteNotifications]

  The device token is received in the app delegate's 
  didRegisterForRemoteNotificationsWithDeviceToken: method. 

  Any errors resulting from the request are received in the app
  delegate's didFailToRegisterForRemoteNotificationsWithError: method.

  The returned device token is then sent as a property to the eFTL server
  when calling the eFTL connect:properties:listener: method.


Creating a Push Notification SSL Certificate

  In order for the eFTL server to send push notifications to an 
  application the eFTL server channel must be configured with an SSL
  certificate generated from the Apple Member Center. Each application
  requires its own certificate. 

    1. From the Apple Member Center, select "Certificates, Identifiers & 
       Profiles".

    2. Select "Certificates" and click the Add (+) button in the upper-right.

    3. Under Production, select "Apple Push Notification service SSL 
       (Sandbox & Production)".

    4. Click Continue.

    5. Choose the App ID that matches your application, and click Continue.

    6. Follow the instructions on the next page to create a certificate 
       request, and click Continue.

    7. Click Choose File, select the certificate request, and click Open.

    8. Click Continue.

    9. Click Download to download your certificate.

  To verify the application settings:

    1. Select "App IDs" under Identifiers.

    2. Choose the App ID that matches your application, and verify that
       Push Notifications are enabled (a green circle followed by "Enabled").

  The downloaded certificate must be converted to PEM format in order
  to configure the eFTL server channel.

    1. Open the downloaded certificate in Keychain Access (this should happen
       automatically if you double click the certificate).

    2. Select "login" and "My Certificates" in the left side of Keychain
       access, and export certificate by right clicking and selecting
       "Export ...".

    3. Save the certifiate in .p12 format, for example my_cert.p12.

    4. Convert the certificate to PEM format with the following command:

        openssl pkcs12 -in my_cert.p12 -out my_cert.pem -nodes -clcerts

  The PEM formated certificate can now be added to the eFTL server channel on
  which the application will connect.

