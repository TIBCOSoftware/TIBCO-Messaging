package com.tibco.eftl.samples.publisher;

import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.tibco.eftl.CompletionListener;
import com.tibco.eftl.Connection;
import com.tibco.eftl.ConnectionListener;
import com.tibco.eftl.EFTL;
import com.tibco.eftl.Message;

import java.util.Properties;

public class MainActivity extends AppCompatActivity implements ConnectionListener, CompletionListener {
    private static final String TAG = MainActivity.class.getSimpleName();

    private static final String EFTL_SERVER_URL = "ws://10.0.2.2:9191/channel";
    private static final String EFTL_USERNAME = "user";
    private static final String EFTL_PASSWORD = "password";

    private Connection connection;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final Button button = (Button) findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // If not yet connected, start the connection to the
                // eFTL server. Once connected a message will be
                // published. Otherwise, publish a message.
                try {
                    if (connection == null) {
                        connect();
                    } else {
                        publish();
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

    private void publish() {
        Message message = connection.createMessage();

        // Publish messages with a destination field.
        //
        // When connected to an EMS channel the destination field must
        // be present and set to the EMS topic on which to publish the
        // message.
        //
        message.setString(Message.FIELD_NAME_DESTINATION, "sample");

        // Add additional fields to the message.
        message.setString("text", "This is a sample eFTL message");

        // Asynchronously publish the message.
        connection.publish(message, this);
    }

    @Override
    public void onConnect(Connection connection) {
        Log.d(TAG, "Connected to eFTL server");
        MainActivity.this.connection = connection;
        publish();
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
    public void onError(Connection connection, int i, String reason) {
        Log.d(TAG, "Error: " + reason);
    }

    @Override
    public void onCompletion(Message message) {
        Log.d(TAG, "Published message: " + message);
    }

    @Override
    public void onError(Message message, int code, String reason) {
        Log.d(TAG, "Publisher error: " + reason);
    }
}
