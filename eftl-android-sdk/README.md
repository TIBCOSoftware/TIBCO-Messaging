Running the eFTL Android Sample Applications
--------------------------------------------

The Android sample applications demonstrate how to connect to an
eFTL channel, subscribe to messages, and publish messages.

Steps requried to build the eFTL Android sample applications:

  1. Using Android Studio, open the eFTL Android sample application
     as an existing Android Studio project.

  2. Add the eFTL client library by dragging the tibeftl.jar file
     to the project's app/libs folder.

  3. Build and run the eFTL Android sample application.

eFTL and GCM Push Notifications
-------------------------------

Configuring the application for Push Notifications

  The eFTL server can send push notifications to a disconnected application
  alerting the application that messages are available. The following steps
  can be used to configure an application for receiving push notifications
  from the Google Cloud Messaging service.

    1. Create a project on the Google Developer's Console website. The
       generated project number will be used by the Android application
       to request a registration token.

    2. Enable the project for Google Cloud Messaging.

    3. Get a server API authorization key for the project. The eFTL server
       will be configured with the server API authorization key in order to
       send push notifications to the Android application.

    4. Download the Google Play services SDK to Android Studio if not
       already installed. Open the SDK manager and check under SDK Tools
       for Google Play services.

    5. Add the Google Play services SDK to your application by adding the
       dependency to the build.gradle file inside your application module
       directory:

       dependencies {
           compile 'com.google.android.gms:play-services:8.4.0'
       }

    6. Update the application's manifest with the following permissions:

         android.permission.INTERNET
         android.permission.WAKE_LOCK
         com.google.android.c2dm.permission.RECEIVE
         <application package name>.permission.C2D_MESSAGE

       See the sample Subscriber project's  AndroidManifest.xml for an example.

    7. Update the application's manifest with the receiver:

         GcmReceiver

       And the two services:

         GcmListenerService
         InstanceIDListenerService

       See the sample Subscriber project's AndroidManifest.xml for an example.

    8. Implement the GcmListenerService to receive push notifications and
       InstanceIDListenerService to receive notification token refresh
       messages.

    9. Call InstanceID.getToken() to get an authorization token for Google Cloud
       Messaging to receive push notifications. Send the token to the eFTL server
       as a property when calling EFTL.connect().



