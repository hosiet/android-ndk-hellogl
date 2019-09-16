package com.android.gl2jni;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

public class DefaultActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Button startButton = findViewById(R.id.button1);
        Button stopButton = findViewById(R.id.button2);

        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent startIntent = new Intent(DefaultActivity.this, GLMonitoringService.class);
                startIntent.setAction(GLMonitoringService.START_ACTION);
                startService(startIntent);
            }
        });
        stopButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent stopIntent = new Intent(DefaultActivity.this, GLMonitoringService.class);
                stopIntent.setAction(GLMonitoringService.STOP_ACTION);
                startService(stopIntent);
            }
        });
    }
}
