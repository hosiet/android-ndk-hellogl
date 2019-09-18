//
// Created by hosiet on 19-9-18.
//

#include "monitor_code.h"

#include <jni.h>
#include <android/log.h>

/* see https://stackoverflow.com/questions/17524794/defining-gl-glext-prototypes-vs-getting-function-pointers */
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

const char* LOG_TAG = "libgl2jni_monitor";
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// OpenGL ES 2.0 code

void monitor_init() {
    return;
}

void monitor_start() {
    return;
}

void monitor_stop() {
    return;
}

extern "C" JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_initMonitor(JNIEnv * env, jclass obj) {
    monitor_init();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_startMonitor(JNIEnv * env, jclass obj) {
    monitor_start();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_stopMonitor(JNIEnv * env, jclass obj) {
    monitor_stop();
}