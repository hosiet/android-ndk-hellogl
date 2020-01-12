package com.android.gl2jni;

import android.accessibilityservice.AccessibilityService;
import android.util.Log;
import android.view.accessibility.AccessibilityEvent;

public class MyKeyAccessibilityService extends AccessibilityService {

    private static final String TAG = "AccService";

    @Override
    public void onAccessibilityEvent(final AccessibilityEvent event) {

        Log.d(TAG, "onAccessibilityEvent: " + event.toString());
        /*
        int eventType = event.getEventType();
        if (eventType == AccessibilityEvent.TYPE_VIEW_CLICKED) {
            Log.e("KeyEvent", "Click happened!");
        } else {
            Log.e("Event", "some event happened");
        }
        */

    }

    @Override
    public void onInterrupt() {
    }
}