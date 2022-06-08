package com.tibco.eftl.samples.subscriber;

import android.media.MediaPlayer;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.iid.FirebaseInstanceId;
import com.google.firebase.iid.InstanceIdResult;
import com.tibco.eftl.Connection;
import com.tibco.eftl.ConnectionListener;
import com.tibco.eftl.EFTL;
import com.tibco.eftl.Message;
import com.tibco.eftl.SubscriptionListener;

import java.util.Properties;

public class MainActivity extends AppCompatActivity implements ConnectionListener, SubscriptionListener {
    private static final String TAG = MainActivity.class.getSimpleName();

    private static final String EFTL_SERVER_URL = "ws://10.0.2.2:8585/channel";
    private static final String EFTL_USERNAME = "";
    private static final String EFTL_PASSWORD = "";

    private Connection connection;
    private boolean isSubscribed;
    private String notificationToken;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Retrieves a notification token when the application is
        // registered with a Firebase project. Supply the token
        // as a property to the EFTL.connect() call in order to
        // receive push notifications.
        FirebaseInstanceId.getInstance().getInstanceId()
                .addOnCompleteListener(new OnCompleteListener<InstanceIdResult>() {
                    @Override
                    public void onComplete(@NonNull Task<InstanceIdResult> task) {
                        if (!task.isSuccessful()) {
                            Log.w(TAG, "getInstanceId failed", task.getException());
                            return;
                        }

                        // Get new Instance ID token
                        notificationToken = task.getResult().getToken();

                        Log.d(TAG, "Received notification token: " + notificationToken);
                    }
                });

        final Button button = findViewById(R.id.button);
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
        String id = Settings.Secure.getString(getContentResolver(), Settings.Secure.ANDROID_ID);

        Properties props = new Properties();
        props.setProperty(EFTL.PROPERTY_CLIENT_ID, "subscriber:" + id);
        props.setProperty(EFTL.PROPERTY_USERNAME, EFTL_USERNAME);
        props.setProperty(EFTL.PROPERTY_PASSWORD, EFTL_PASSWORD);

        // When a notification token is included the eFTL server will
        // send push notifications to the disconnected application as
        // messages are available to any of the eFTL subscriptions.
        if (notificationToken != null) {
            props.setProperty(EFTL.PROPERTY_NOTIFICATION_TOKEN, notificationToken);
        }

        // In a development environment where self-signed server certificates are being used
        // you may need to supply your own trusted server certificates by calling
        // EFTL.setSSLTrustStore(), or skip server certificate authentication all together.
        EFTL.setSSLTrustAll(false);

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
        // field named "type" with a value of "hello".
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
        String matcher = "{\"type\":\"hello\"}";

        // Asynchronously subscribe to messages using a content matcher.
        if (connection != null) {
            connection.subscribe(matcher, null, this);
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
