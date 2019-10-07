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

/* for uint64_t printf support */
#define __STDC_FORMAT_MACROS
#include <cinttypes>


const char* LOG_TAG = "libgl2jni_monitor";
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define  LOGF(...)  fprintf(fp, __VA_ARGS__)

//#define LOGF(...) LOGE(__VA_ARGS__)

std::string OUTPUT_FILEPATH = "/sdcard/gl_output.txt";
static FILE * fp = nullptr;

/* List of counter names that we want to retrieve data from */
static const char* TARGET_PERF_COUNTER_NAME_LIST[] = {
        "PERF_SP_WAVE_IDLE_CYCLES",
        "PERF_SP_WAVE_CTRL_CYCLES",
        "PERF_SP_WAVE_EMIT_CYCLES",
        "PERF_SP_WAVE_NOP_CYCLES",
        "PERF_SP_WAVE_WAIT_CYCLES",
        "PERF_SP_WAVE_FETCH_CYCLES"
};

// OpenGL ES 2.0 code

static EGLConfig eglConf;
static EGLSurface eglSurface;
static EGLContext eglCtx;
static EGLDisplay eglDisp;
static GLenum err;

/* Some declarations about visible functions */
void setupEGL(int w, int h);
void shutdownEGL();
void doGLTests();
void doGLTestAllPerfCounters();
static void doTriggerMonitorDuringMeasurement();
static void doGLStartMonitorForMeasurement(GLuint, GLuint);


/* Real implementations */

const int PERF_MONITOR_LENGTH = 1;
const int PERF_COUNTER_LENGTH = 1;
static GLuint monitor_list[PERF_MONITOR_LENGTH] = { 0 };
static GLuint counterList[PERF_COUNTER_LENGTH] = {0};

// lets try with (oneplus7pro, 10, 22) PERF_SP_WAVE_IDLE_CYCLES)
// (oneplus7pro, 10, 7, NON_EXECUTION_CYCLES)
// Try with (oneplus7pro, 4, 4), PERF_HLSQ_UCHE_LATENCY_CYCLES
// working on (pixel2, 11, 22), PERF_SP_WAVE_IDLE_CYCLES

GLuint monitor_group_id = 11;
GLuint monitor_counter_id = 22;

class GroupCounterNameTriple {
public:
    GLuint group_id = 0;
    GLuint counter_id = 0;
    GLuint group_maxactive = 0;
    std::string group_name;
    std::string counter_name;
    std::string counter_type; // convert header magic number to string
    bool uninitialized = true;
    bool has_data = false;

    GroupCounterNameTriple() {
        uninitialized = true;
    }

    GroupCounterNameTriple(
            GLuint group_id_in,
            GLuint counter_id_in,
            GLuint group_maxactive_in,
            std::string &group_name_in,
            std::string &counter_name_in,
            std::string &counter_type_in) {
        group_id = group_id_in;
        counter_id = counter_id_in;
        group_name = group_name_in;
        group_maxactive = group_maxactive_in;
        counter_name = counter_name_in;
        counter_type = counter_type_in;
        uninitialized = false;
    }

    ~GroupCounterNameTriple() {
        uninitialized = true;
    }
};

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
    doGLTestAllPerfCounters();
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
    /*
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

    */
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
        /* This perf monitor have data output. Send such data to the file. */
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
void doGLTestAllPerfCounters() {
    const unsigned int TEST_TOTAL_MEASURE_SEC = 60;
    const unsigned int TEST_ALL_SLEEP_MILLISECONDS = 200;
    auto test_measure_times = \
            (unsigned int) ((TEST_TOTAL_MEASURE_SEC * 1000) / TEST_ALL_SLEEP_MILLISECONDS);
    const int PERF_OUTPUT_DATA_BUF_SIZE = 10240;
    GLuint output_data[PERF_OUTPUT_DATA_BUF_SIZE] = { 0 };
    GLsizei bytesWritten = 0;


    LOGE("doGLTestAllPerfCounters: Function is now starting...");

    for (int j = 0; j < PERF_MONITOR_LENGTH; j++) {
        monitor_list[j] = 0;
    }

    for (int j = 0; j < PERF_COUNTER_LENGTH; j++) {
        counterList[j] = 0;
    }
    // !! real fix for the issue
    counterList[0] = monitor_counter_id;

    glGenPerfMonitorsAMD(
            (GLsizei) PERF_MONITOR_LENGTH,
            monitor_list
    );
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("We got an OpenGL Error! The value is: %04x!", err);
    }
    glSelectPerfMonitorCountersAMD(
            monitor_list[0],
            GL_TRUE,
            monitor_group_id,
            1, //PERF_COUNTER_LENGTH,      // which is actually 1
            counterList
    );
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("We got an OpenGL Error! The value is: %04x!", err);
    }
    // Finish the full cycle about Monitoring dump data here
    for (int i = 0; i < test_measure_times; i++) {
    //for (auto &item : PerfCounterFullList) {

        LOGE("Note, our counter is: %d", i);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("1,We got an OpenGL Error! The value is: %04x!", err);
        }
        //LOGE("TestAllCounter(): starting, group %d, counter %d!\n",
                ///item.group_id, item.counter_id);
                //monitor_group_id, monitor_counter_id);

        glBeginPerfMonitorAMD(monitor_list[0]);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("2,We got an OpenGL Error! The value is: %04x!", err);
        }

        // sleep now
        std::this_thread::sleep_for(std::chrono::milliseconds(TEST_ALL_SLEEP_MILLISECONDS));

        // end measurement here
        glEndPerfMonitorAMD(monitor_list[0]);
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOGE("3,We got an OpenGL Error! The value is: %04x!", err);
        }

        // get the monitor information
        // first, get if the data is available
        bytesWritten = 0;
        /*
        for (int j = 0; j < PERF_OUTPUT_DATA_BUF_SIZE; j++) {
            output_data[j] = 0;
        }
         */
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

        //LOGF("COUNTER: (%d, %d) -- (%s, %s)\n",
        //       item.group_id, item.counter_id, item.group_name.c_str(),
        //        item.counter_name.c_str());
        //LOGE("Available or not Data collected, bytesWritten: %d, availability: --\n",
        //     bytesWritten);
        bytesWritten = 0;

        // first, get how many data has been collected
        /*
        for (int j = 0; j < PERF_OUTPUT_DATA_BUF_SIZE; j++) {
            output_data[j] = 0;
        }
         */
        glGetPerfMonitorCounterDataAMD(
                monitor_list[0],
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
                monitor_list[0],
                GL_PERFMON_RESULT_AMD,
                (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE, // dataSize
                output_data,
                &bytesWritten
        );


        //LOGE("Data collected, monitor_list is %d, bytesWritten is %d", monitor_list[0], bytesWritten);
        int max_nonzero = 0;
        if (bytesWritten > 0) {
            // This perf monitor have data output. Send such data to the file.
            // FIXME: most data is (GLuint groupid, GLuint counterid, uint64_t data),
            // we should use this way to output!
            //item.has_data = true;

            LOGE("==!!!== Data collected, bytesWritten is %d", bytesWritten);
            LOGF("==!!!== Data collected, No.%d\n", i);

            //LOGE("The written bytes:\n");
            // we assume it is of uint64_t
            LOGF("DATA: %d %" PRIu64 "\n", i, *((uint64_t *) (output_data + 2)));
            /* // This is not the correct way
            for (int j = 0; j < bytesWritten; j++) {
                LOGF("%d: %04x, \n", j, output_data[j]);
                if (output_data[j] != 0) {
                    max_nonzero = j;
                }
            }
             */

            // now, we only document about how many data was retrieved.
            //LOGF("count: %d, bytes: %d, max_nonzero: %d\n", i, bytesWritten, max_nonzero);
        }
        //LOGF("\n\n");
    }

    // delete the monitor
    glDeletePerfMonitorsAMD((GLsizei) PERF_MONITOR_LENGTH, monitor_list);
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("9,We got an OpenGL Error! The value is: %04x!", err);
        break;
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
    (void) fflush(fp);
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
        result = tryGLEnableGlobalMode();
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
    return std::make_tuple(999, 999);
}