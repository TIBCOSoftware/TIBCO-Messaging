package com.tibco.eftl.samples.subscriber;

import android.content.Intent;
import android.os.AsyncTask;
import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.google.android.gms.gcm.GoogleCloudMessaging;
import com.google.android.gms.iid.InstanceID;
import com.tibco.eftl.Connection;
import com.tibco.eftl.ConnectionListener;
import com.tibco.eftl.EFTL;
import com.tibco.eftl.Message;
import com.tibco.eftl.SubscriptionListener;

import java.util.Properties;

public class MainActivity extends AppCompatActivity implements ConnectionListener, SubscriptionListener {
    private static final String TAG = MainActivity.class.getSimpleName();

    private static final String EFTL_SERVER_URL = "ws://10.0.2.2:9191/channel";
    private static final String EFTL_USERNAME = "user";
    private static final String EFTL_PASSWORD = "password";
    private static final String EFTL_DURABLE_NAME = null;

    private Connection connection;
    private boolean isSubscribed;
    private String notificationToken;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Obtain the GCM push notification registration token
        // by calling InstanceID.getToken(). You must supply the
        // project number generated when the application is created
        // on the Google Developer's Console website.
        //
        // Supply the returned token as a notification token property
        // to the EFTL.connect() call in order to receive push
        // notifications.
/*
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                try {
                    InstanceID instanceID = InstanceID.getInstance(MainActivity.this);
                    notificationToken = instanceID.getToken("your project number",
                            GoogleCloudMessaging.INSTANCE_ID_SCOPE, null);
                    Log.d(TAG, "Got registration token: " + notificationToken);
                } catch (Exception e) {
                    Log.d(TAG, "Error getting registration token", e);
                }

                return null;
            }
        }.execute();
*/

        final Button button = (Button) findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // If not yet connected, start the connection to the
                // eFTL server. Once connected, the subscription will
                // be made. Otherwise, either subscribe or unsubscribe
                // depending on the current state.
                try {
                    if (connection == null) {
                        connect();
                    } else if (isSubscribed) {
                        unsubscribe();
                    } else {
                        subscribe();
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Error: " + e.getMessage(), e);
                    Toast.makeText(MainActivity.this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
                }
            }
        });

    }

    @Override
    protected void onPause() {
        super.onPause();

        // Close the connection to the eFTL server when the
        // application moves into the background.
        disconnect();
    }

    @Override
    protected void onResume() {
        super.onResume();

        // Restore the connection to the eFTL server when the
        // application move into the foreground.
        if (connection != null) {
            reconnect();
        }
    }

    private void connect() {
        Properties props = new Properties();
        props.setProperty(EFTL.PROPERTY_CLIENT_ID, Build.MODEL);
        props.setProperty(EFTL.PROPERTY_USERNAME, EFTL_USERNAME);
        props.setProperty(EFTL.PROPERTY_PASSWORD, EFTL_PASSWORD);

        if (notificationToken != null) {
            props.setProperty(EFTL.PROPERTY_NOTIFICATION_TOKEN, notificationToken);
        }

        // Asynchronously connect to the eFTL server.
        EFTL.connect(EFTL_SERVER_URL, props, this);
    }

    private void reconnect() {
        // Asynchronously reconnect to the eFTL server.
        if (connection != null) {
            connection.reconnect(null);
        }
    }

    private void disconnect() {
        // Asynchronously disconnect from the eFTL server.
        if (connection != null) {
            connection.disconnect();
        }
    }

    private void subscribe() {

        // Create a subscription matcher for messages containing a
        // destination field that matches the destination "sample".
        //
        // Specify a durable name to create a durable subscription.
        //
        // When connected to an FTL channel the content matcher
        // can be used to match any field in a published message.
        // Only matching messages will be received by the
        // subscription. The content matcher can match on string
        // and integer fields, or test for the presence or absence
        // of a field by setting it's value to the boolean true or
        // false.
        //
        // When connected to an EMS channel the content matcher
        // must only contain the destination field "_dest" set to
        // the EMS topic on which to subscribe.
        //
        String matcher = String.format("{\"%s\":\"%s\"}",
                Message.FIELD_NAME_DESTINATION, "sample");

        // Asynchronously subscribe to messages using a content matcher.
        if (connection != null) {
            connection.subscribe(matcher, EFTL_DURABLE_NAME, this);
            isSubscribed = true;
            ((Button) findViewById(R.id.button)).setText("Unsubscribe");
        }
    }

    private void unsubscribe() {
        // Asynchronously unsubscribe from all subscriptions.
        if (connection != null) {
            connection.unsubscribeAll();
            Log.d(TAG, "Unsubscribed");
            isSubscribed = false;
            ((Button) findViewById(R.id.button)).setText("Subscribe");
        }
    }

    @Override
    public void onConnect(Connection connection) {
        Log.d(TAG, "Connected to eFTL server");
        this.connection = connection;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                subscribe();
            }
        });
    }

    @Override
    public void onReconnect(Connection connection) {
        Log.d(TAG, "Reconnected to eFTL server");
    }

    @Override
    public void onDisconnect(Connection connection, int code, String reason) {
        Log.d(TAG, "Disconnected from eFTL server " + reason);
    }

    @Override
    public void onError(Connection connection, int code, String reason) {
        Log.d(TAG, "Error: " + reason);
    }

    @Override
    public void onMessages(Message[] messages) {
        for (Message message : messages) {
            Log.d(TAG, "Received message: " + message);
        }
    }

    @Override
    public void onSubscribe(String subscription) {
        Log.d(TAG, "Subscription succeeded");
    }

    @Override
    public void onError(String subscription, int code, String reason) {
        Log.d(TAG, "Subscription failed: " + reason);
    }
}
