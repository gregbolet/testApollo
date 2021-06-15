
#ifndef PAPI_HELPER_H
#define PAPI_HELPER_H

#include "papi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <omp.h>
#include <iostream>
#include <fstream>

class PapiHelper{
    public:
        PapiHelper(int isMultiplexed, int numEvents, std::string* eventNames, int problemSize);
        ~PapiHelper();

        int setupThread(void);
        int startThreadRead(void);
        int stopThreadRead(void);
        int destroyThread(void);

    private:
        int numThreads;
        int isMultiplexed;
        int numEvents;
        int problemSize;
        
        // Keep our counter values in here
        // Map the omp_get_thread_num to its counter values
        long long** all_cntr_values;
        int* event_sets;

        // Keep our events in here
        std::string* events_to_track;
};

#endif