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

const int PERF_MONITOR_LENGTH = 1;
const int PERF_COUNTER_LENGTH = 1;
static GLuint monitor_list[PERF_MONITOR_LENGTH] = { 0 };
static GLuint counterList[PERF_COUNTER_LENGTH] = {0};

class GroupCounterNameTriple {
public:
    GLuint group_id = 0;
    GLuint counter_id = 0;
    GLuint group_maxactive = 0;
    std::string group_name;
    std::string counter_name;
    std::string counter_type; // convert header magic number to string
    bool uninitialized = true;

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

/* // for static structure
struct ST_GC {
    const GLuint group_id = 1;
    const GLuint counter_id = 2;
} GCStruct_SP_ACTIVE_CYCLES_ALL;


const GLuint monitor_group_id = GCStruct_SP_ACTIVE_CYCLES_ALL.group_id;
const GLuint monitor_counter_id = GCStruct_SP_ACTIVE_CYCLES_ALL.counter_id;

*/

GLuint monitor_group_id;
GLuint monitor_counter_id;


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
}

void monitor_stop() {
    // end the monitor
    glEndPerfMonitorAMD(monitor_list[0]);
    LOGE("EndPerfMonitorAMD issued!");

    // get the monitor information
    // TODO FIXME
    const int PERF_OUTPUT_DATA_BUF_SIZE = 128;
    GLuint output_data[PERF_OUTPUT_DATA_BUF_SIZE] = { 0 };
    GLsizei bytesWritten = 0;


    // first, get if the data is available
    glGetPerfMonitorCounterDataAMD(
            monitor_list[0],
            GL_PERFMON_RESULT_AVAILABLE_AMD,
            (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE,
            output_data,
            &bytesWritten
    );
    LOGE("Available or not Data collected, bytesWritten is %d, availability data is %d\n",
            bytesWritten, ( *(uint8_t* ) output_data) );
    for (int i = 0; i < PERF_OUTPUT_DATA_BUF_SIZE; i++) {
        output_data[i] = 0;
    }
    bytesWritten = 0;

        // first, get how many data has been collected
    glGetPerfMonitorCounterDataAMD(
            monitor_list[0],
            GL_PERFMON_RESULT_SIZE_AMD,
            (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE,
            output_data,
            &bytesWritten
            );
    LOGE("Data collected, the size of bytesWritten is %d\n", bytesWritten);
    for (int i = 0; i < PERF_OUTPUT_DATA_BUF_SIZE; i++) {
        output_data[i] = 0;
    }


    glGetPerfMonitorCounterDataAMD(
            monitor_list[0],
            GL_PERFMON_RESULT_AMD,
            (GLsizei) PERF_OUTPUT_DATA_BUF_SIZE, // dataSize
            output_data,
            &bytesWritten
            );
    LOGF("Data collected, bytesWritten is %d", bytesWritten);
    LOGE("Data collected, monitor_list is %d, bytesWritten is %d", monitor_list[0], bytesWritten);
    if (bytesWritten > 0) {
        /* This perf monitor have data output. Send such data to the file. */

    }
    LOGE("The written bytes are: ");
    for (int i = 0; i < bytesWritten; i++) {
        LOGE("%d: %04x, ", i, output_data[i]);
    }


    // delete the monitor
    glDeletePerfMonitorsAMD((GLsizei) PERF_MONITOR_LENGTH, monitor_list);

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
    GLuint maxActiveCounters = 0;
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
                    groups[i], k, maxActiveCounters, groupString_l, counterString_l, data_type_str);
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



bool tryGLEnableGlobalMode() {
    GLboolean seeIfEnabled;

    LOGE("TryGlobalMode: before enable..");
    glEnable(GL_PERFMON_GLOBAL_MODE_QCOM);
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
        examineGLCapabilities();
        doGLStartDumpData();
        LOGE("doGLTests: All done (started!)!");
    }
}