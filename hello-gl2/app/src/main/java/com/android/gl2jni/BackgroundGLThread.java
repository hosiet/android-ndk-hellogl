package com.android.gl2jni;

import android.util.Log;

/**
 * The class to run OpenGL measurement forever
 */
public class BackgroundGLThread extends Thread {
    @Override
    public void run() {
        super.run();
        Log.e("Thread...", "start running!");
        while (true) {
            try {
                Thread.sleep(10000);
            } catch (java.lang.InterruptedException e) {
                //e.printStackTrace();
                /* no longer execute */
                Log.e("Thread...", "thread stopped!!!!!");
                break;
            }
        }
    }
}