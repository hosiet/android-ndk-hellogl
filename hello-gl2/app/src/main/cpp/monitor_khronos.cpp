//
// Created by hosiet on 19-9-19.
//

#include "monitor_khronos.h"

#include <jni.h>
#include <android/log.h>

#include <EGL/egl.h>

/* see https://stackoverflow.com/questions/17524794/defining-gl-glext-prototypes-vs-getting-function-pointers */
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cstring>
#include <cstdlib>

typedef struct
{
    GLuint       *counterList;
    int         numCounters;
    int         maxActiveCounters;
} CounterInfo;

void
getGroupAndCounterList(GLuint **groupsList, int *numGroups,
                       CounterInfo **counterInfo)
{
    GLint          n;
    GLuint        *groups;
    CounterInfo   *counters;

    glGetPerfMonitorGroupsAMD(&n, 0, NULL);
    groups = (GLuint*) malloc(n * sizeof(GLuint));
    glGetPerfMonitorGroupsAMD(NULL, n, groups);
    *numGroups = n;

    *groupsList = groups;
    counters = (CounterInfo*) malloc(sizeof(CounterInfo) * n);
    for (int i = 0 ; i < n; i++ )
    {
        glGetPerfMonitorCountersAMD(groups[i], &counters[i].numCounters,
                                    &counters[i].maxActiveCounters, 0, NULL);

        counters[i].counterList = (GLuint*)malloc(counters[i].numCounters *
                                                  sizeof(int));

        glGetPerfMonitorCountersAMD(groups[i], NULL, NULL,
                                    counters[i].numCounters,
                                    counters[i].counterList);
    }

    *counterInfo = counters;
}

static int  countersInitialized = 0;

int
getCounterByName(char *groupName, char *counterName, GLuint *groupID,
                 GLuint *counterID)
{
    int          numGroups;
    GLuint       *groups;
    CounterInfo  *counters;
    int          i = 0;

    if (!countersInitialized)
    {
        getGroupAndCounterList(&groups, &numGroups, &counters);
        countersInitialized = 1;
    }

    for ( i = 0; i < numGroups; i++ )
    {
        char curGroupName[256];
        glGetPerfMonitorGroupStringAMD(groups[i], 256, NULL, curGroupName);
        if (strcmp(groupName, curGroupName) == 0)
        {
            *groupID = groups[i];
            break;
        }
    }

    if ( i == numGroups )
        return -1;           // error - could not find the group name

    for ( int j = 0; j < counters[i].numCounters; j++ )
    {
        char curCounterName[256];

        glGetPerfMonitorCounterStringAMD(groups[i],
                                         counters[i].counterList[j],
                                         256, NULL, curCounterName);
        if (strcmp(counterName, curCounterName) == 0)
        {
            *counterID = counters[i].counterList[j];
            return 0;
        }
    }

    return -1;           // error - could not find the counter name
}
