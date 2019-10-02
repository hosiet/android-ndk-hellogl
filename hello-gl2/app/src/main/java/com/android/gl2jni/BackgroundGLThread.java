package com.android.gl2jni;

import android.util.Log;

/**
 * The class to run OpenGL measurement forever
 */
public class BackgroundGLThread extends Thread {
    @Override
    public void run() {
        super.run();
        Log.e("BG_Thread (java)", "start running!");
        GL2JNILib.initMonitor();
        Log.e("BG_Thread (java)", "starting dump all data");
        GL2JNILib.startMonitor();
        int counter = 0;
        while (true) {
            try {
                Log.e("BG_Thread (java)", "counter: " + counter);
                Thread.sleep(1000);
                /* Instead of sleeping, try with running the GL code
                   to collect the data
                 */
                GL2JNILib.triggerMonitor();
                counter += 1;
            } catch (java.lang.InterruptedException e) {
                //e.printStackTrace();
                /* no longer execute */
                Log.e("BG_Thread (java)", "Break due to exception!");
                break;
            }
            if (counter >= 60) {
                break;
            }
        }
        Log.e("BG_Thread (java)", "Finishing, calling stopMonitor()...");
        GL2JNILib.stopMonitor();
    }
}