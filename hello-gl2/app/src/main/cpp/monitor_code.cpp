//
// Created by hosiet on 19-9-18.
//

#include "monitor_code.h"

#include <jni.h>
#include <android/log.h>

#include <EGL/egl.h>

/* see https://stackoverflow.com/questions/17524794/defining-gl-glext-prototypes-vs-getting-function-pointers */
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>
#include <cerrno>

const char* LOG_TAG = "libgl2jni_monitor";
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

const std::string OUTPUT_FILEPATH = "/sdcard/gl_output.txt";
static FILE * fp = nullptr;

// OpenGL ES 2.0 code

static EGLConfig eglConf;
static EGLSurface eglSurface;
static EGLContext eglCtx;
static EGLDisplay eglDisp;

/* Some declarations about visible functions */
void setupEGL(int w, int h);
void shutdownEGL();
void doGLTests();


/* Real implementations */

/**
 * Open a file and allow appending later.
 *
 * The file would be overwritten.
 */
FILE *_gl_output_open_file(char *filepath_p) {
    FILE* fp = nullptr;
    int _my_errno = 0;
    if (filepath_p == nullptr) {
        fp = fopen(OUTPUT_FILEPATH.c_str(), "we");
        _my_errno = errno;
    } else {
        fp = fopen(filepath_p, "we");
        _my_errno = errno;
    }
    if (fp == nullptr) {
        /* some error may happen? */
        LOGE(LOG_TAG, "Error opening file! err: %s", strerror(_my_errno));
    } else {
        LOGI(LOG_TAG, "open_file(): Open log file succeeded!");
    }
    return fp;
 }

 bool _gl_output_close_file(FILE *fp) {
    if (fp == nullptr) {
        LOGE(LOG_TAG, "close_file(): Cannot close null file pointer!");
        return false;
    } else {
        fclose(fp);
        LOGI(LOG_TAG, "close_file(): file close finished.");
        return true;
    }
}


void monitor_init() {
    fp = _gl_output_open_file(nullptr);
    setupEGL(80, 60);
    doGLTests();
}

void monitor_start() {
}

void monitor_stop() {
    shutdownEGL();
    _gl_output_close_file(fp);
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



void setupEGL(int w, int h) {
    // EGL config attributes
    const EGLint confAttr[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,    // very important!
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,          // we will create a pixelbuffer surface
            EGL_RED_SIZE,   8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE,  8,
            EGL_ALPHA_SIZE, 8,     // if you need the alpha channel
            EGL_DEPTH_SIZE, 16,    // if you need the depth buffer
            EGL_NONE
    };

    // EGL context attributes
    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,              // very important!
            EGL_NONE
    };

    // surface attributes
    // the surface size is set to the input frame size
    const EGLint surfaceAttr[] = {
            EGL_WIDTH, w,
            EGL_HEIGHT, h,
            EGL_NONE
    };

    EGLint eglMajVers;
    EGLint eglMinVers;
    EGLint numConfigs;

    eglDisp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(eglDisp, &eglMajVers, &eglMinVers);

    LOGI("EGL init with version %d.%d", eglMajVers, eglMinVers);

    // choose the first config, i.e. best config
    eglChooseConfig(eglDisp, confAttr, &eglConf, 1, &numConfigs);

    eglCtx = eglCreateContext(eglDisp, eglConf, EGL_NO_CONTEXT, ctxAttr);

    // create a pixelbuffer surface
    eglSurface = eglCreatePbufferSurface(eglDisp, eglConf, surfaceAttr);

    eglMakeCurrent(eglDisp, eglSurface, eglSurface, eglCtx);
}

void shutdownEGL() {
    eglMakeCurrent(eglDisp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(eglDisp, eglCtx);
    eglDestroySurface(eglDisp, eglSurface);
    eglTerminate(eglDisp);

    eglDisp = EGL_NO_DISPLAY;
    eglSurface = EGL_NO_SURFACE;
    eglCtx = EGL_NO_CONTEXT;

    LOGI("EGL now shutdown!!!");
}


void examineGLCapabilities() {
    /* Let us try the AMD_performance_monitor extension */
    const int PERF_GROUP_NUM = 128;
    GLint numGroups = 0;
    GLsizei groupsSize = PERF_GROUP_NUM;
    GLuint groups[PERF_GROUP_NUM] = { 0 };
    glGetPerfMonitorGroupsAMD(&numGroups, groupsSize, groups);
    //int counter = 0;
    LOGE("AMD_perf_mon: we have monitor group size: %d!\n", numGroups);
    for (int i = 0; i < numGroups; i++) {
        //LOGE("AMD_perf_mon: GROUP %d: %d!\n", i, groups[i]); // should be 0 to 15
        ;
    }

    // Now try the GetPerfMonitorGroupStringAMD
    const int PERF_CHAR_BUFFER_NUM = 512;
    GLchar groupString[PERF_CHAR_BUFFER_NUM] = {0};
    GLsizei bufSize = PERF_CHAR_BUFFER_NUM;
    GLsizei length = 0;
    for (int i = 0; i < numGroups; i++) {
        for (int j = 0; j < bufSize; j++) {
            groupString[j] = '\0';
        }
        glGetPerfMonitorGroupStringAMD((GLuint) i, bufSize, &length, groupString);
        LOGE("AMD_perf_mon: group #%d, length is %d, string is '%s'!\n", i, length, groupString);
    }

    // Now try the GetPerfMonitorCountersAMD
    // Now try the GetPerfMonitorCounterStringAMD
    const int PERF_COUNTER_NUM = 128;
    GLint numCounters = 0;
    GLint maxActiveCounters = 0;
    GLsizei countersSize = PERF_COUNTER_NUM;
    GLuint counters[PERF_COUNTER_NUM] = { 0 };
    GLchar counterString[PERF_CHAR_BUFFER_NUM] = { 0 };
    for (int i = 0; i < numGroups; i++) {
        for (int j = 0; j < PERF_COUNTER_NUM; j++) {
            counters[j] = 0;
        }
        glGetPerfMonitorCountersAMD((GLuint) i, &numCounters, &maxActiveCounters,
                                    countersSize, counters);
        LOGE("AMD_perf_mon: group #%d, counter number is %d, max active is %d!\n",
             i, numCounters, maxActiveCounters);
        for (int k = 0; k < numCounters; k++) {
            glGetPerfMonitorCounterStringAMD(
                    (GLuint) i,
                    (GLuint) k,
                    bufSize,
                    &length,
                    counterString
            );
            LOGE("AMD_perf_mon: counterStr: %s\n", counterString);
        }
    }
}

bool tryGLEnableGlobalMode() {
    GLboolean seeIfEnabled;

    LOGE("TryGlobalMode: before enable..");
    glEnable(GL_PERFMON_GLOBAL_MODE_QCOM);
    /* also test whether it is enabled */

    LOGE("TryGlobalMode: before try..");
    seeIfEnabled = glIsEnabled(GL_PERFMON_GLOBAL_MODE_QCOM);
    if (seeIfEnabled) {
        LOGE("QCOM_Global_mode: enabled!");
    } else {
        LOGE("QCOM_Global_mode: NOT enabled!");
    }

    LOGE("TryGlobalMode: before disable..");
    glDisable(GL_PERFMON_GLOBAL_MODE_QCOM);
    /* also test whether it is enabled */

    LOGE("TryGlobalMode: before try..");
    seeIfEnabled = glIsEnabled(GL_PERFMON_GLOBAL_MODE_QCOM);
    if (seeIfEnabled) {
        LOGE("QCOM_Global_mode: enabled!");
        return true;
    } else {
        LOGE("QCOM_Global_mode: NOT enabled!");
        return false;
    }
}

void doGLTests() {

    int do_test_extention = 1;
    bool result;

    if (do_test_extention) {
        examineGLCapabilities();
        result = tryGLEnableGlobalMode();
        LOGE("doGLTests: All done!");
    }
}