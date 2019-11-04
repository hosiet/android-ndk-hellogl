//
// Created by hosiet on 19-9-18.
//

#ifndef HELLO_GL2_MONITOR_CODE_H
#define HELLO_GL2_MONITOR_CODE_H

#include <string>
#include <GLES2/gl2.h>

void monitor_init();
void monitor_start();
void monitor_stop();
void monitor_trigger();

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

/* Some declarations about visible functions */
void setupEGL(int w, int h);
void shutdownEGL();
void doGLTests();
void doGLTestAllPerfCounters();
//static void doTriggerMonitorDuringMeasurement();
//static void doGLStartMonitorForMeasurement(GLuint, GLuint);
std::tuple<GLuint, GLuint> myGLGetCounterIDFromName(std::string const &);
void doGLTestAllPerfCounterWithDataOrNot();
std::string myGLGetCounterNameStringFromID(GLuint, GLuint);


/*
static std::string[][] monitor_counter_list = {
    {
        "PERF_CP_ALWAYS_COUNT",
        "PERF_CP_BUSY_GFX_CORE_IDLE",
        "PERF_CP_BUSY_CYCLES",
        "PERF_CP_PFP_IDLE",
        "PERF_CP_ME_BUSY_WORKING",
        "PERF_CP_ME_STARVE_CYCLES_ANY",
        "PERF_CP_ME_ICACHE_MISS",
        "PERF_CP_ME_ICACHE_HIT"
    },
    {
        "PERF_RBBM_ALWAYS_ON",
        "PERF_RBBM_STATUS_MASKED"
    },
    {
        "PERF_RAS_STARVE_CYCLES_TSE"
    },
    {
        "PERF_SP_WAVE_IDLE_CYCLES"
    },
    {
        "AXI_READ_REQUESTS_ID_0",
        "AXI_WRITE_REQUESTS_ID_0"
    },
    {
        "VBIF_CORE_STARVE_CYCLES_POWER"
    }
};
*/


const GLuint GROUP_CP = 0;
const GLuint GROUP_RBBM = 1;
const GLuint GROUP_RAS = 7;
const GLuint GROUP_SP = 11;
//const GLuint GROUP_AXI = 17;
const GLuint GROUP_VBIF = 18;

const GLuint GROUP_ID_LIST[] {
    GROUP_CP,
    GROUP_RBBM,
    GROUP_RAS,
    GROUP_SP,
    //GROUP_AXI,
    GROUP_VBIF
};


#endif //HELLO_GL2_MONITOR_CODE_H
