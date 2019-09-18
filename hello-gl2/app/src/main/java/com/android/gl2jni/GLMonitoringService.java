package com.android.gl2jni;

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import android.widget.RemoteViews;
import android.widget.Toast;

public class GLMonitoringService extends Service {

    private static final String TAG = GLMonitoringService.class.getSimpleName();
    private static final String FOREGROUND_CHANNEL_ID = "my_foreground_channel_id";

    public static final int NOTIFICATION_ID_FOREGROUND_SERVICE = 5462503;
    public static final String MAIN_ACTION = "test.action.main";
    public static final String START_ACTION = "test.action.start";
    public static final String STOP_ACTION = "test.action.stop";
    public static final int CONNECTED = 10;
    public static final int NOT_CONNECTED = 0;

    private NotificationManager mNotificationManager;
    private Handler handler;
    private int count = 0;
    private static int stateService = NOT_CONNECTED;

    private BackgroundGLThread mBackgroundGLThread = null;

    public GLMonitoringService() {
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        return null;
        //throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        stateService = NOT_CONNECTED;
        this.mBackgroundGLThread = new BackgroundGLThread();
    }

    @Override
    public void onDestroy() {
        stateService = NOT_CONNECTED;
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent == null) {
            stopForeground(true);
            stopOpenGLMonitoring();
            stopSelf();
            return START_NOT_STICKY;
        }

        // if user starts the service
        switch (intent.getAction()) {
            case START_ACTION:
                Log.e(TAG, "Received user start foreground intent");
                startForeground(NOTIFICATION_ID_FOREGROUND_SERVICE, prepareNotification());
                initOpenGLMonitoring();
                connect();
                break;
            case STOP_ACTION:
                stopForeground(true);
                stopOpenGLMonitoring();
                stopSelf();
                break;
            default:
                Log.e(TAG, "default action called!");
                stopForeground(true);
                stopOpenGLMonitoring();
                stopSelf();
        }
        return START_NOT_STICKY;
    }

    private void initOpenGLMonitoring() {
        this.mBackgroundGLThread.start();
        return;
    }

    private void stopOpenGLMonitoring() {
        this.mBackgroundGLThread.interrupt();
        return;
    }

    // after notification connection, change notification text
    private void connect() {
        // after 10 sec
        /*
        new android.os.Handler().postDelayed(
                new Runnable() {
                    public void run() {
                        Log.e(TAG, "Service/notification connected!");
                        Toast.makeText(getApplicationContext(), "Connected!", Toast.LENGTH_LONG)
                                .show();
                        stateService = CONNECTED;
                        startForeground(NOTIFICATION_ID_FOREGROUND_SERVICE, prepareNotification());
                    }
                }, 10000
        );

         */
    }

    private Notification prepareNotification() {
        // handle build version above android oreo
        if (Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O &&
                mNotificationManager.getNotificationChannel(FOREGROUND_CHANNEL_ID) == null) {
            CharSequence name = getString(R.string.text_name_notification);
            int importance = NotificationManager.IMPORTANCE_DEFAULT;
            NotificationChannel channel = new NotificationChannel(FOREGROUND_CHANNEL_ID,
                    name, importance);
            channel.enableVibration(false);
            mNotificationManager.createNotificationChannel(channel);
        }

        Intent notificationIntent = new Intent(this, DefaultActivity.class);
        notificationIntent.setAction(MAIN_ACTION);
        notificationIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);

        Intent stopIntent = new Intent(this, GLMonitoringService.class);
        stopIntent.setAction(STOP_ACTION);
        PendingIntent pendingStopIntent = PendingIntent.getService(
                this,
                0,
                stopIntent,
                PendingIntent.FLAG_UPDATE_CURRENT
        );
        RemoteViews remoteViews = new RemoteViews(getPackageName(), R.layout.notification);
        remoteViews.setOnClickPendingIntent(R.id.btn_stop, pendingStopIntent);

        // if it is connected
        switch (stateService) {
            case NOT_CONNECTED:
                remoteViews.setTextViewText(R.id.tv_state, "DISCONNECTED");
                break;
            case CONNECTED:
                remoteViews.setTextViewText(R.id.tv_state, "CONNECTED");
                break;
        }

        // notification builder
        Notification.Builder notificationBuilder;
        if (Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            notificationBuilder = new Notification.Builder(this, FOREGROUND_CHANNEL_ID);
        } else {
            notificationBuilder = new Notification.Builder(this);
        }
        notificationBuilder
                .setContent(remoteViews)
                .setSmallIcon(R.mipmap.ic_launcher)
                .setCategory(Notification.CATEGORY_SERVICE)
                .setOnlyAlertOnce(true)
                .setOngoing(true)
                .setAutoCancel(true)
                .setContentIntent(pendingStopIntent);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            notificationBuilder.setVisibility(Notification.VISIBILITY_PUBLIC);
        }

        return notificationBuilder.build();
    }

}