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
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <tuple>

/* for uint64_t printf support */
#define __STDC_FORMAT_MACROS
#include <cinttypes>
#include <ctime>


//#define NO_LOG_FILE


const char* LOG_TAG = "libgl2jni_monitor";
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)



#if defined(NO_LOG_FILE)
#define LOGF(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define  LOGF(...)  fprintf(fp, __VA_ARGS__)
#endif


std::string OUTPUT_FILEPATH = "/sdcard/gl_output.txt";
static FILE * fp = nullptr;



// OpenGL ES 2.0 code

static EGLConfig eglConf;
static EGLSurface eglSurface;
static EGLContext eglCtx;
static EGLDisplay eglDisp;
static GLenum err;


/* Real implementations */

const bool ENABLE_GLOBAL_MODE = false;



const int PERF_MONITOR_LENGTH = 5;
//const int PERF_COUNTER_LENGTH = sizeof(TARGET_PERF_COUNTER_NAME_LIST) / sizeof(char *);
static GLuint monitor_list[PERF_MONITOR_LENGTH] = { 0 };
static GLuint counterList[66] = { 0 };

// lets try with (oneplus7pro, 10, 22) PERF_SP_WAVE_IDLE_CYCLES)
// (oneplus7pro, 10, 7, NON_EXECUTION_CYCLES)
// Try with (oneplus7pro, 4, 4), PERF_HLSQ_UCHE_LATENCY_CYCLES
// working on (pixel2, 11, 22), PERF_SP_WAVE_IDLE_CYCLES

//GLuint monitor_group_id = 11;
//GLuint monitor_counter_id = 22;

static const unsigned int TEST_ALL_SLEEP_MILLISECONDS = 100;


/*  STATIC list!  ***********************************  */


static GLuint counter_list_id_0_0_cp[] = {
    0, 1, 2, 3, 10, 12, 18, 19
};

static GLuint counter_list_id_1_1_rbbm[] = {
    1, 6, 9
};

static GLuint counter_list_id_2_7_ras[] = {
    3
};

static GLuint counter_list_id_3_11_sp[] = {
    22
};

/*
static GLuint counter_list_id_4_17_axi[] = {
    0
};
*/

static GLuint counter_list_id_5_18_vbif[] = {
    2
};

/* ENDOF STATIC  counter lists!  *************************  */




static std::vector<GroupCounterNameTriple> PerfCounterFullList;


/**
 * Open a file and allow appending later.
 *
 * The file would be overwritten.
 */
FILE *_gl_output_open_file(char *filepath_p) {
    FILE* fp = nullptr;
    int _my_errno = 0;
    if (filepath_p == nullptr) {
        fp = fopen(OUTPUT_FILEPATH.c_str(), "w");
        _my_errno = errno;
    } else {
        fp = fopen(filepath_p, "w");
        _my_errno = errno;
    }
    if (fp == nullptr) {
        /* some error may happen? */
        LOGE("Error opening file! err: %s", strerror(_my_errno));
    } else {
        LOGI("open_file(): Open log file succeeded!");
    }
    return fp;
 }

 bool _gl_output_close_file(FILE *fp) {
    if (fp == nullptr) {
        LOGE("close_file(): Cannot close null file pointer!");
        return false;
    } else {
        fclose(fp);
        LOGI("close_file(): file close finished.");
        return true;
    }
}


void monitor_init() {
    fp = _gl_output_open_file(nullptr);
    setupEGL(80, 60);
    doGLTests();
}

void monitor_start() {
    if (false) {
        doGLTestAllPerfCounters(true, 30);                     //  normal data collection
    } else {
        doGLTestAllPerfCounters(true, 30);
    }
    //doGLTestAllPerfCounterWithDataOrNot();           //  transverse; test whether with data or not
    //doGLStartMonitorForMeasurement(monitor_group_id, monitor_counter_id);
}

void monitor_trigger() {
    //doTriggerMonitorDuringMeasurement();
}

void monitor_stop() {
    // stop the monitor and discard data?
    //glEndPerfMonitorAMD(monitor_list[0]);

    // disable global mode
    LOGE("TryGlobalMode: before disable..");
    glDisable(GL_PERFMON_GLOBAL_MODE_QCOM);
    /* also test whether it is enabled */

    bool seeIfEnabled = glIsEnabled(GL_PERFMON_GLOBAL_MODE_QCOM);
    if (seeIfEnabled) {
        LOGE("QCOM_Global_mode: now enabled!");
    } else {
        LOGE("QCOM_Global_mode: now NOT enabled!");
    }

    shutdownEGL();

    // close output file
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

extern "C" JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_triggerMonitor(JNIEnv * env, jclass obj) {
    monitor_trigger();
}

void monitor_init_oneshot() {
    monitor_init();
    /* Also reset the OUTPUT_FILEPATH */
    std::time_t result = std::time(nullptr);
    auto t = static_cast<long int>(result);
    OUTPUT_FILEPATH = std::string("/sdcard/gl_output_") + std::to_string(t) + \
            std::string(".txt");
}

/* Start. Actually will generate the monitor. */
void monitor_start_oneshot() {
    for (int j = 0; j < PERF_MONITOR_LENGTH; j++) {
        monitor_list[j] = GROUP_ID_LIST[j];
    }
    glGenPerfMonitorsAMD(
            (GLsizei) PERF_MONITOR_LENGTH,
            monitor_list
            );
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("glGenPerfMonitor: We got an OpenGL Error! The value is: %04x!", err);
    }
    {
        // Now, try to enable certain monitor counters.
        // For Pixel 2, try the following:
        // groups: 0, 1, 7, 11, 17, 18
        // group 0
        glSelectPerfMonitorCountersAMD(
                monitor_list[0],
                GL_TRUE, GROUP_ID_LIST[0], 8, counter_list_id_0_0_cp);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("glSelect0: We got an OpenGL Error! The value is: %04x!", err);
        }

        // group 1
        glSelectPerfMonitorCountersAMD(
                monitor_list[1],
                GL_TRUE, GROUP_ID_LIST[1], 3, counter_list_id_1_1_rbbm);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("glSelect1: We got an OpenGL Error! The value is: %04x!", err);
        }

        // group 2
        glSelectPerfMonitorCountersAMD(
                monitor_list[2],
                GL_TRUE, GROUP_ID_LIST[2], 1, counter_list_id_2_7_ras);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("glSelect2: We got an OpenGL Error! The value is: %04x!", err);
        }

        // group 3
        glSelectPerfMonitorCountersAMD(
                monitor_list[3],
                GL_TRUE, GROUP_ID_LIST[3], 1, counter_list_id_3_11_sp);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("glSelect3: We got an OpenGL Error! The value is: %04x!", err);
        }

        // group 4
        glSelectPerfMonitorCountersAMD(
                monitor_list[4],
                GL_TRUE, GROUP_ID_LIST[4], 1, counter_list_id_5_18_vbif);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("glSelect5: We got an OpenGL Error! The value is: %04x!", err);
        }
    }
}

void monitor_trigger_oneshot(int event_type) {
    int tmp_turn = 0;
    GLsizei bytesWritten = 0;
    const int PERF_OUTPUT_DATA_BUF_SIZE = 4096;
    static GLuint output_data[PERF_OUTPUT_DATA_BUF_SIZE] = { 0 }; // TODO: ensure correct

    for (int i = 0; i < PERF_MONITOR_LENGTH; i++) {
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("1,We got an OpenGL Error! The value is: %04x!", err);
        }
        glBeginPerfMonitorAMD(monitor_list[tmp_turn]);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("2,We got an OpenGL Error! The value is: %04x!", err);
        }
        // sleep now
        std::this_thread::sleep_for(std::chrono::milliseconds(TEST_ALL_SLEEP_MILLISECONDS));

        // end measurement here
        glEndPerfMonitorAMD(monitor_list[tmp_turn]);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("3.2, %d: We got an OpenGL Error! The value is: %04x!", tmp_turn, err);
        }

        // get the monitor information
        glGetPerfMonitorCounterDataAMD(
                monitor_list[tmp_turn],
                GL_PERFMON_RESULT_AVAILABLE_AMD,
                (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE,
                output_data,
                &bytesWritten
        );
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("4,We got an OpenGL Error! The value is: %04x!", err);
            break;
        }
        bytesWritten = 0;
        glGetPerfMonitorCounterDataAMD(
                monitor_list[tmp_turn],
                GL_PERFMON_RESULT_SIZE_AMD,
                (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE,
                output_data,
                &bytesWritten
        );
        LOGE("Data size collected, bytesWritten: %d\n", bytesWritten);
        // also output the data size real data
        if (bytesWritten == 4) {
            uint64_t real_size = *((uint64_t *) output_data);
            LOGE("Data size is: %" PRIu64 "!", real_size);
        }
        bytesWritten = 0;

        glGetPerfMonitorCounterDataAMD(
                monitor_list[tmp_turn],
                GL_PERFMON_RESULT_AMD,
                (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE, // dataSize
                output_data,
                &bytesWritten
        );

        //LOGE("Data collected, monitor_list is %d, bytesWritten is %d", monitor_list[0], bytesWritten);
        //int max_nonzero = 0;
        if (bytesWritten > 0) {
            // This perf monitor have data output. Send such data to the file.
            // FIXME: most data is (GLuint groupid, GLuint counterid, uint64_t data),
            // we should use this way to output!
            //item.has_data = true;

            LOGE("==!!!== Data collected, bytesWritten is %d", bytesWritten);
            LOGF("==!!!== Data collected, No.%d, EVENT: %d, TIMESTAMP: %" PRIu64 "\n",
                 i,
                 event_type,
                 std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch()).count());

            //LOGE("The written bytes:\n");
            // we assume it is of uint64_t
            int k = 0;
            while (k < bytesWritten) {
                LOGF("Data: type: %d, time: %d, group: %d, counter: %d, data: %" PRIu64 "\n",
                     tmp_turn,
                     i,
                     *((uint32_t *) (output_data + k)),
                     *((uint32_t *) (output_data + k + 1)),
                     *((uint64_t *) (output_data + k + 2))
                );
                k += 4;
            }
        }
        tmp_turn += 1;
        (void) fflush(fp);
    }

}

/* Stop measurement.
 *
 * This should also be the event handler(?) for SIGINT and other abnormal quit.
 * Oh well... Event handler may not really hold the OpenGL context. Let's just
 * use a global variable to set that. TODO Properly end measurement
 *
 * Note: disabling GLOBAL_MODE was not included here.
 * */
void monitor_stop_oneshot() {
    glDeletePerfMonitorsAMD(
            (GLsizei) PERF_MONITOR_LENGTH,
            monitor_list
            );
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("We got an OpenGL Error! The value is: %04x!", err);
    }
    shutdownEGL();
    _gl_output_close_file(fp);     // close output file
}



/** ***************************************************************************************** */

/**
 * Get the full list of performance counter, return a list.
 *
 * @return the std::vector of certain class about counter info.
 */
std::vector<GroupCounterNameTriple> getPerfCounterFullList() {
    std::vector<GroupCounterNameTriple> newPerfList;
    const int PERF_GROUP_NUM = 128;
    GLint numGroups = 0;
    GLsizei groupsSize = PERF_GROUP_NUM;
    GLuint groups[PERF_GROUP_NUM] = { 0 };
    const int PERF_CHAR_BUFFER_NUM = 512;
    const int PERF_COUNTER_NUM = 128;
    const int PERF_VOID_DATA_LEN = 64;
    GLsizei countersSize = PERF_COUNTER_NUM;
    GLchar groupString[PERF_CHAR_BUFFER_NUM] = {0};
    GLsizei bufSize = PERF_CHAR_BUFFER_NUM;
    GLuint counters[PERF_COUNTER_NUM] = { 0 };
    GLchar counterString[PERF_CHAR_BUFFER_NUM] = { 0 };
    GLsizei length = 0;
    GLint numCounters = 0;
    GLint maxActiveCounters = 0;
    GLchar data[PERF_VOID_DATA_LEN] = { 0 };
    GLvoid *data_p = &data;

    glGetPerfMonitorGroupsAMD(&numGroups, groupsSize, groups);
    //LOGE("examineGLCap: beginning...");
    //LOGF("AMD_perf_mon: we have monitor group size: %d!\n", numGroups);
    for (int i = 0; i < numGroups; i++) {
        //LOGE("AMD_perf_mon: GROUP %d: %d!\n", i, groups[i]); // should be 0 to 15
        ;

        for (int j = 0; j < bufSize; j++) {
            groupString[j] = '\0';
        }
        glGetPerfMonitorGroupStringAMD((GLuint) i, bufSize, &length, groupString);
        //LOGF("AMD_perf_mon: group #%d, length is %d, string is '%s'!\n", i, length, groupString);


        for (int j = 0; j < PERF_COUNTER_NUM; j++) {
            counters[j] = 0;
        }
        glGetPerfMonitorCountersAMD((GLuint) i, &numCounters, &maxActiveCounters,
                                    countersSize, counters);
        //LOGF("AMD_perf_mon: group #%d, counter number is %d, max active is %d!\n",
        //     i, numCounters, maxActiveCounters);
        for (unsigned int k = 0; k < numCounters; k++) {
            glGetPerfMonitorCounterStringAMD(
                    (GLuint) i,
                    (GLuint) k,
                    bufSize,
                    &length,
                    counterString
            );
            //LOGF("AMD_perf_mon: counterStr: %s\n", counterString);
            glGetPerfMonitorCounterInfoAMD(
                    (GLuint) i,
                    (GLuint) k,
                    GL_COUNTER_TYPE_AMD,
                    data_p
            );
            std::string data_type_str;
            uint32_t data_matcher = * ((uint32_t *) data_p);
            uint64_t data_range[2] = { 0 };
            for (int j = 0; j < PERF_VOID_DATA_LEN; j++) {
                data[j] = '\0';
            }
            glGetPerfMonitorCounterInfoAMD(
                    (GLuint) i,
                    (GLuint) k,
                    GL_COUNTER_RANGE_AMD,
                    data_p
            );
            bool do_output_data_range = false;
            switch (data_matcher) {
                case GL_UNSIGNED_INT:
                    data_type_str = "GL_UNSIGNED_INT";
                    break;
                case GL_UNSIGNED_INT64_AMD:
                    data_type_str = "GL_UNSIGNED_INT64_AMD";
                    data_range[0] = * ((uint64_t *) data_p);
                    data_range[1] = * (((uint64_t *) data_p) + 1);
                    do_output_data_range = true;
                    break;
                case GL_PERCENTAGE_AMD:
                    data_type_str = "GL_PERCENTAGE_AMD";
                    break;
                case GL_FLOAT:
                    data_type_str = "GL_FLOAT";
                    break;
                default:
                    data_type_str = "ERROR_OTHER_TYPE";
                    break;
            }

            /* YES! We add the counter info to the array here. */
            std::string groupString_l = std::string(groupString);
            std::string counterString_l = std::string(counterString);
            GroupCounterNameTriple newTriple(
                    groups[i], k, (GLuint) maxActiveCounters, groupString_l, counterString_l, data_type_str);
            newPerfList.push_back(newTriple);

            //LOGF("AMD_perf_mon: the counter type is %s.\n", data_type_str.c_str());
            if (do_output_data_range) {
                //LOGF("AMD_perf_mon: the counter range: No1: %" PRIu64 ", No2: %" PRIu64 "\n\n",
                //     data_range[0], data_range[1]);
            }
            // 281474976710655 is actually 2^48 - 1
        }
    }
    return newPerfList;
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


/**
 * Dump all GL_AMD_performance_monitor information into external file.
 */
void examineGLCapabilities() {
    /* Let us try the AMD_performance_monitor extension */
    LOGE("examineGLCap(): very start here");

    /* We have encapsulated the GroupCounterNameTriple, use that! */
    for (const auto &item : PerfCounterFullList) {
        LOGF("examineGLCap(): Group(%d, %s, max_active %d), Counter(%d, %s, %s).\n",
                item.group_id,
                item.group_name.c_str(),
                item.group_maxactive,
                item.counter_id,
                item.counter_name.c_str(),
                item.counter_type.c_str());
    }
    LOGE("examineGLCap(): finished");
}


/**
 * Trigger monitor to actually collect and have the data written
 * into the output file.
 */
 /*
static void doTriggerMonitorDuringMeasurement() {
    glEndPerfMonitorAMD(monitor_list[0]);
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("We got an OpenGL Error! The value is: %04x!", err);
    }

    // now retrieve the data
    // TODO FIXME
    const int PERF_OUTPUT_DATA_BUF_SIZE = 10240;
    const int PERF_OUTPUT_SIZE_DATA_BUF_SIZE = 128;
    static GLuint output_data[PERF_OUTPUT_DATA_BUF_SIZE] = { 0 };
    static GLuint output_size_data[PERF_OUTPUT_SIZE_DATA_BUF_SIZE] = { 0 };
    GLsizei bytesWritten = 0;
    GLsizei bytesWrittenSize = 0;

    // first, get if the data is available
    for (int i = 0; i < PERF_OUTPUT_DATA_BUF_SIZE; i++) {
        output_data[i] = 0;
    }
    for (int i = 0; i < PERF_OUTPUT_SIZE_DATA_BUF_SIZE; i++) {
        output_size_data[i] = 0;
    }

    glGetPerfMonitorCounterDataAMD(
            monitor_list[0],
            GL_PERFMON_RESULT_AVAILABLE_AMD,
            (GLsizei) PERF_OUTPUT_SIZE_DATA_BUF_SIZE,
            output_size_data,
            &bytesWrittenSize
    );
    LOGE("Available or not Data collected, bytesWritten: %d, availability: %d\n",
         bytesWrittenSize, ( *(GLuint *) output_size_data) );
    bytesWrittenSize = 0;

    // first, get how many data has been collected
    for (int i = 0; i < PERF_OUTPUT_SIZE_DATA_BUF_SIZE; i++) {
        output_size_data[i] = 0;
    }
    glGetPerfMonitorCounterDataAMD(
            monitor_list[0],
            GL_PERFMON_RESULT_SIZE_AMD,
            (GLsizei) PERF_OUTPUT_SIZE_DATA_BUF_SIZE,
            output_size_data,
            &bytesWrittenSize
    );
    LOGE("Data size collected, bytesWritten: %d\n", bytesWrittenSize);
    // also output the data size real data
    if (bytesWrittenSize == 4) {
        LOGE("Data size should be: %d\n", *((GLuint *) output_size_data));
    }
    for (int i = 0; i < PERF_OUTPUT_SIZE_DATA_BUF_SIZE; i++) {
        output_size_data[i] = 0;
    }


    bytesWritten = 0;
    glGetPerfMonitorCounterDataAMD(
            monitor_list[0],
            GL_PERFMON_RESULT_AMD,
            (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE, // dataSize
            output_data,
            &bytesWritten
    );
    // catch if there is any opengl error.
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("We got an OpenGL Error! The value is: %04x!", err);
    }
    //LOGF("Data collected, bytesWritten is %d", bytesWritten);
    LOGE("Data collected, monitor_list is %d, bytesWritten is %d", monitor_list[0], bytesWritten);
    if (bytesWritten > 0) {
        // This perf monitor have data output. Send such data to the file.
        // FIXME: most data is (GLuint groupid, GLuint counterid, uint64_t data),
        // we should use this way to output!
        LOGE("The written bytes are: ");
        for (int i = 0; i < bytesWritten; i++) {
            LOGE("%d: %04x, ", i, output_data[i]);
        }

    }

    // now restart the monitor
    glSelectPerfMonitorCountersAMD(
            monitor_list[0],
            GL_TRUE,
            (GLuint) monitor_group_id,
            (GLint) monitor_counter_id,
            counterList
    );
    glBeginPerfMonitorAMD(monitor_list[0]);
}
  */


 /*
static void doGLStartMonitorForMeasurement(GLuint group_id, GLuint counter_id) {
    GLuint my_group_id = group_id;
    GLuint my_counter_id = counter_id;
    LOGE("monitor_start(): doGLStartMonitorForMeasurement(): starting, group %d, counter %d!\n",
         monitor_group_id, monitor_counter_id);
    for (int i = 0; i < PERF_MONITOR_LENGTH; i++) {
        monitor_list[i] = 0;
    }
    for (int i = 0; i < PERF_COUNTER_LENGTH; i++) {
        counterList[i] = 0;
    }
    glGenPerfMonitorsAMD(
            (GLsizei) PERF_MONITOR_LENGTH,
            monitor_list
    );
    LOGE("doGLStartMonitor(): after glGenPerfMonitors...");
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("We got an OpenGL Error! The value is: %04x!", err);
    }
    glSelectPerfMonitorCountersAMD(
            monitor_list[0],
            GL_TRUE,
            (GLuint) monitor_group_id,
            (GLint) monitor_counter_id,
            counterList
    );
    LOGE("doGLStartMonitor(): after glSelectPerfMonitorCounters...");
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("We got an OpenGL Error! The value is: %04x!", err);
    }
    glBeginPerfMonitorAMD(monitor_list[0]);
    LOGE("doGLStartMonitor(): after glBeginPerfMonitors...");
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("We got an OpenGL Error! The value is: %04x!", err);
    }
}
*/

/*
 * Used only to test whether the perf counter
 * is providing data or not.
 *
 */
void doGLTestAllPerfCounterWithDataOrNot() {
    GLuint monitor_group_id = 0;
    GLuint monitor_counter_id = 0;
    const unsigned int TEST_CYCLE_SLEEP_MILLISECONDS = 200;
    const unsigned int TEST_CYCLE = 5;
    const int PERF_OUTPUT_DATA_BUF_SIZE = 10240;
    GLuint output_data[PERF_OUTPUT_DATA_BUF_SIZE] = { 0 };
    GLsizei bytesWritten = 0;

    monitor_list[0] = 0;

    for (auto &i : PerfCounterFullList) {
        monitor_group_id = i.group_id;
        monitor_counter_id = i.counter_id;
        counterList[0] = i.counter_id;
        glGenPerfMonitorsAMD(1, monitor_list);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("We got an OpenGL Error! The value is: %04x!", err);
        }
        glSelectPerfMonitorCountersAMD(
                monitor_list[0],
                GL_TRUE,
                monitor_group_id,
                1, //PERF_COUNTER_LENGTH,
                counterList
        );
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("We got an OpenGL Error! The value is: %04x!", err);
        }
        LOGE("Testing: (%d, %d)", monitor_group_id, monitor_counter_id);
        for (int j = 0; j < TEST_CYCLE; j++) {
            glBeginPerfMonitorAMD(monitor_list[0]);
            while ((err = glGetError()) != GL_NO_ERROR) {
                LOGE("2,We got an OpenGL Error! The value is: %04x!", err);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(TEST_CYCLE_SLEEP_MILLISECONDS));
            // end measurement here
            glEndPerfMonitorAMD(monitor_list[0]);
            while ((err = glGetError()) != GL_NO_ERROR) {
                LOGE("3,We got an OpenGL Error! The value is: %04x!", err);
            }
            // get the monitor information
            bytesWritten = 0;
            glGetPerfMonitorCounterDataAMD(
                    monitor_list[0],
                    GL_PERFMON_RESULT_AVAILABLE_AMD,
                    (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE,
                    output_data,
                    &bytesWritten
            );
            while ((err = glGetError()) != GL_NO_ERROR) {
                LOGE("4,We got an OpenGL Error! The value is: %04x!", err);
                break;
            }
            bytesWritten = 0;
            glGetPerfMonitorCounterDataAMD(
                    monitor_list[0],
                    GL_PERFMON_RESULT_SIZE_AMD,
                    (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE,
                    output_data,
                    &bytesWritten
            );
            LOGE("Data size collected, bytesWritten: %d\n", bytesWritten);
            if (bytesWritten == 4) {
                uint64_t real_size = *((uint64_t *) output_data);
                LOGE("Data size is: %" PRIu64 "!", real_size);
            }
            bytesWritten = 0;

            glGetPerfMonitorCounterDataAMD(
                    monitor_list[0],
                    GL_PERFMON_RESULT_AMD,
                    (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE, // dataSize
                    output_data,
                    &bytesWritten
            );
            //LOGE("Data collected, monitor_list is %d, bytesWritten is %d", monitor_list[0], bytesWritten);
            //int max_nonzero = 0;
            if (bytesWritten > 0) {
                // This perf monitor have data output. Send such data to the file.
                // FIXME: most data is (GLuint groupid, GLuint counterid, uint64_t data),
                // we should use this way to output!
                //item.has_data = true;

                LOGE("==!!!== Data collected, bytesWritten is %d", bytesWritten);
                LOGF("==!!!== Data collected, No.%d\n", j);

                //LOGE("The written bytes:\n");
                // we assume it is of uint64_t
                int k = 0;
                while (k < bytesWritten) {
                    LOGF("Data: cycle_counter: %d, group: %d, counter: %d, name: %s, data: %"
                                 PRIu64
                                 "\n",
                         j,
                         *((uint32_t * )(output_data + k)),
                         *((uint32_t * )(output_data + k + 1)),
                         myGLGetCounterNameStringFromID(monitor_group_id, monitor_counter_id).c_str(),
                         *((uint64_t * )(output_data + k + 2))
                    );
                    k += 4;
                }
            }
        }
        glDeletePerfMonitorsAMD((GLsizei) PERF_MONITOR_LENGTH, monitor_list);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("9,We got an OpenGL Error! The value is: %04x!", err);
            break;
        }
        fflush(fp);
    }
    LOGE("All done!");
}


/**
 * Used only for testing all perf counters.
 *
 * Currently the code will run background for 60s/120s without
 * switching back to the Java environment. Seems that this
 * no-reentry could solve the issue.
 *
 * If we are going to create a monitor and not cancelling it
 * in the future, the buffer must be large enough?
 */
void doGLTestAllPerfCounters(bool if_spec_total_time, unsigned int max_sec) {
    // TODO FIXME
    //const GLuint monitor_group_id = 0; // for pixel 2, 0: CP

    unsigned int TEST_TOTAL_MEASURE_SEC = 30;

    if (if_spec_total_time) {
        TEST_TOTAL_MEASURE_SEC = max_sec;
    }
    auto test_measure_times = \
            (unsigned int) ((TEST_TOTAL_MEASURE_SEC * 1000) / TEST_ALL_SLEEP_MILLISECONDS);
    const int PERF_OUTPUT_DATA_BUF_SIZE = 10240;
    GLuint output_data[PERF_OUTPUT_DATA_BUF_SIZE] = { 0 };
    GLsizei bytesWritten = 0;


    LOGE("doGLTestAllPerfCounters: Function is now starting...");

    for (int j = 0; j < PERF_MONITOR_LENGTH; j++) {
        monitor_list[j] = GROUP_ID_LIST[j];
    }

//    LOGE("checkpoint 0, PERF_COUNTER_LENGTH is %d", PERF_COUNTER_LENGTH);

    glGenPerfMonitorsAMD(
            (GLsizei) PERF_MONITOR_LENGTH,
            monitor_list
    );
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("glGenPerfMonitor: We got an OpenGL Error! The value is: %04x!", err);
    }


    {
    // Now, try to enable certain monitor counters.
    // For Pixel 2, try the following:
    // groups: 0, 1, 7, 11, 17, 18
        // group 0
        glSelectPerfMonitorCountersAMD(
                monitor_list[0],
                GL_TRUE, GROUP_ID_LIST[0], 8, counter_list_id_0_0_cp);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("glSelect0: We got an OpenGL Error! The value is: %04x!", err);
        }

        // group 1
        glSelectPerfMonitorCountersAMD(
                monitor_list[1],
                GL_TRUE, GROUP_ID_LIST[1], 3, counter_list_id_1_1_rbbm);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("glSelect1: We got an OpenGL Error! The value is: %04x!", err);
        }

        // group 2
        glSelectPerfMonitorCountersAMD(
                monitor_list[2],
                GL_TRUE, GROUP_ID_LIST[2], 1, counter_list_id_2_7_ras);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("glSelect2: We got an OpenGL Error! The value is: %04x!", err);
        }

        // group 3
        glSelectPerfMonitorCountersAMD(
                monitor_list[3],
                GL_TRUE, GROUP_ID_LIST[3], 1, counter_list_id_3_11_sp);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("glSelect3: We got an OpenGL Error! The value is: %04x!", err);
        }

        // group 4
        glSelectPerfMonitorCountersAMD(
                monitor_list[4],
                GL_TRUE, GROUP_ID_LIST[4], 1, counter_list_id_5_18_vbif);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("glSelect5: We got an OpenGL Error! The value is: %04x!", err);
        }
    }


    //LOGE("checkpoint 2s");
    // Finish the full cycle about Monitoring dump data here
    // altering performance monitor length!
    int tmp_turn = 0;
    for (int i = 0; i < test_measure_times * PERF_MONITOR_LENGTH; i++) {

        LOGE("Note, time is: %d", i);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("1,We got an OpenGL Error! The value is: %04x!", err);
        }
        //LOGE("TestAllCounter(): starting, group %d, counter %d!\n",
                ///item.group_id, item.counter_id);
                //monitor_group_id, monitor_counter_id);

        glBeginPerfMonitorAMD(monitor_list[tmp_turn]);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("2,We got an OpenGL Error! The value is: %04x!", err);
        }

        // sleep now
        std::this_thread::sleep_for(std::chrono::milliseconds(TEST_ALL_SLEEP_MILLISECONDS));

        // end measurement here
        glEndPerfMonitorAMD(monitor_list[tmp_turn]);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("3.2, %d: We got an OpenGL Error! The value is: %04x!", tmp_turn, err);
        }

        // get the monitor information
        // first, get if the data is available
        bytesWritten = 0;
        /*
        for (int j = 0; j < PERF_OUTPUT_DATA_BUF_SIZE; j++) {
            output_data[j] = 0;
        }
        */

        // Handle all data in a cycle
        bytesWritten = 0;
        glGetPerfMonitorCounterDataAMD(
                monitor_list[tmp_turn],
                GL_PERFMON_RESULT_AVAILABLE_AMD,
                (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE,
                output_data,
                &bytesWritten
        );
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("4,We got an OpenGL Error! The value is: %04x!", err);
            break;
        }

        bytesWritten = 0;
        glGetPerfMonitorCounterDataAMD(
                monitor_list[tmp_turn],
                GL_PERFMON_RESULT_SIZE_AMD,
                (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE,
                output_data,
                &bytesWritten
        );
        LOGE("Data size collected, bytesWritten: %d\n", bytesWritten);
        // also output the data size real data
        if (bytesWritten == 4) {
            uint64_t real_size = *((uint64_t *) output_data);
            LOGE("Data size is: %" PRIu64 "!", real_size);
        }
        bytesWritten = 0;

        glGetPerfMonitorCounterDataAMD(
                monitor_list[tmp_turn],
                GL_PERFMON_RESULT_AMD,
                (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE, // dataSize
                output_data,
                &bytesWritten
        );


        //LOGE("Data collected, monitor_list is %d, bytesWritten is %d", monitor_list[0], bytesWritten);
        //int max_nonzero = 0;
        if (bytesWritten > 0) {
            // This perf monitor have data output. Send such data to the file.
            // FIXME: most data is (GLuint groupid, GLuint counterid, uint64_t data),
            // we should use this way to output!
            //item.has_data = true;

            LOGE("==!!!== Data collected, bytesWritten is %d", bytesWritten);
            LOGF("==!!!== Data collected, No.%d, TIMESTAMP: %" PRIu64 "\n",
                    i,
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count());

            //LOGE("The written bytes:\n");
            // we assume it is of uint64_t
            int k = 0;
            while (k < bytesWritten) {
                LOGF("Data: type: %d, time: %d, group: %d, counter: %d, data: %" PRIu64 "\n",
                     tmp_turn,
                     i,
                     *((uint32_t *) (output_data + k)),
                     *((uint32_t *) (output_data + k + 1)),
                     *((uint64_t *) (output_data + k + 2))
                     );
                k += 4;
            }
        }
        tmp_turn += 1;
        if (tmp_turn >= PERF_MONITOR_LENGTH) {
            tmp_turn = 0;
        }
        (void) fflush(fp);
    }    /* END for all things within the cycle */

        // now, we only document about how many data was retrieved.
        //LOGF("count: %d, bytes: %d, max_nonzero: %d\n", i, bytesWritten, max_nonzero);


        //LOGF("\n\n");

    // delete the monitor
    glDeletePerfMonitorsAMD(
            (GLsizei) PERF_MONITOR_LENGTH,
            monitor_list
    );
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("We got an OpenGL Error! The value is: %04x!", err);
    }

    // now, output another data about the availability of data
    /*
    LOGF("\n\nRESULT\n");
    for (const auto &item : PerfCounterFullList) {
        LOGF("testAllCounters(): Group(%d, %s, max_active %d), Counter(%d, %s, %s), data: %s.\n",
             item.group_id,
             item.group_name.c_str(),
             item.group_maxactive,
             item.counter_id,
             item.counter_name.c_str(),
             item.counter_type.c_str(),
             item.has_data ? "true" : "false");
    }
    LOGE("TestAllCounter(): Finished!\n");
     */
}

/*
void doGLStartDumpData() {
    LOGE("examineCap: Before BenPerfMonit...");
    // Now try with GenPerfMonitorsAMD and actually monitor one!
    for (int i = 0; i < PERF_MONITOR_LENGTH; i++) {
        monitor_list[i] = 0;
    }
    glGenPerfMonitorsAMD((GLsizei) PERF_MONITOR_LENGTH, monitor_list);
    counterList[0] = monitor_counter_id;
    glSelectPerfMonitorCountersAMD(
            monitor_list[0],
            GL_TRUE,
            (GLuint) monitor_group_id, // group 0->10, "SP"
            (GLint) monitor_counter_id,  // No. 0->37, "SP_ACTIVE_CYCLES_ALL"
            counterList
    );
    glBeginPerfMonitorAMD(monitor_list[0]);
    LOGE("examineCap(): glBeginPerfMonitorAMD issued, Finished ok!");
}
 */



bool tryGLEnableGlobalMode() {
    GLboolean seeIfEnabled;

    LOGE("TryGlobalMode: before enable..");
    glEnable(GL_PERFMON_GLOBAL_MODE_QCOM);
    LOGE("tryGLEnableGlobalMode(): after glEnable...");
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("We got an OpenGL Error! The value is: %04x!", err);
    }
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
    int do_test_extension = 1;
    bool result;

    if (do_test_extension) {
        if (ENABLE_GLOBAL_MODE) {
            result = tryGLEnableGlobalMode();
        }
        PerfCounterFullList = getPerfCounterFullList();
        examineGLCapabilities();
        //doGLStartDumpData();
        LOGE("doGLTests: All done (started!)!");
    }
}

std::string myGLGetCounterNameStringFromID(GLuint group, GLuint counter) {
    for (auto &i : PerfCounterFullList) {
        if (i.group_id == group && i.counter_id == counter) {
            return i.counter_name;
        }
    }
    // by default return not found
    return std::string("ERR_NOT_FOUND");
}

std::tuple<GLuint, GLuint> myGLGetCounterIDFromName(std::string const &counterName) {
    for (auto &i : PerfCounterFullList) {
        if (i.counter_name == counterName) {
            return std::make_tuple(i.group_id, i.counter_id);
        }
    }
    // by default return not found
    return std::make_tuple((GLuint) 999, (GLuint) 999);
}