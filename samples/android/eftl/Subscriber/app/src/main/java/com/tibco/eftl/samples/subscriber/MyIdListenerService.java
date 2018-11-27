package com.tibco.eftl.samples.subscriber;

import android.util.Log;

import com.google.android.gms.iid.InstanceIDListenerService;

/**
 * Created by bpeterse on 4/27/16.
 */
public class MyIdListenerService extends InstanceIDListenerService {
    private static final String TAG = MyIdListenerService.class.getSimpleName();

    @Override
    public void onTokenRefresh() {
        Log.d(TAG, "Token refresh");
    }
}
