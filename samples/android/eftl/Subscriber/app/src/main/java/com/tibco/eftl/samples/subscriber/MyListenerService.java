package com.tibco.eftl.samples.subscriber;

import android.os.Bundle;
import android.util.Log;

import com.google.android.gms.gcm.GcmListenerService;

public class MyListenerService extends GcmListenerService {
    private static final String TAG = MyListenerService.class.getSimpleName();

    @Override
    public void onMessageReceived(String from, Bundle data) {
        Log.d(TAG, "Received GCM push notification");
    }
}
