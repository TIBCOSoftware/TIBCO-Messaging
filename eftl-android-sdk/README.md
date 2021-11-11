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


eFTL and FCM Push Notifications
-------------------------------

Configuring the sample Subscriber application for Push Notifications

  The eFTL server can send push notifications to a disconnected application
  alerting the application that messages are available. The following steps
  can be used to configure the sample Subscrier application for receiving 
  push notifications from the Firebase Cloud Messaging service.

    1. Create a project on the Firebase Console website. 

    2. Copy the Cloud Messaging Server key for the project. The eFTL server
       channel will be configured with this Server key in order to send push
       notifications to the sample Subscriber Android application.

    3. Download the google-services.json for your application and copy into
       the app/ directory of the Android Studio project.

    4. Uncomment this line to the app-level build.gradle:

           apply plugin: 'com.google.gms.google-services'

    5. Remove the resource value "google_app_id" from strings.xml as we'll
       use the value found in google-services.json.

  Run the sample Subscriber Android appliation. Once connected, send the
  application to the background causing the connection to the eFTL server
  to close. When a message is published to the sample Subscrier Android 
  application, from the sample Publisher Android application for example, 
  a log message will appear in the Run console of Android Studio indictating 
  that a remote notification has been received.
