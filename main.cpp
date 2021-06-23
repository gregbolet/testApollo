#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <vector>
#include <string>
#include <omp.h>
#include "MatVec.hpp"

// Defaulted input arguments to the program
int usePapi = 0;
std::vector<std::string> counters;
int isMultiplexed = 0;
int numRuns = 1;
std::string regionName = "test-region";

#ifdef APOLLO
void tryAllPolicies(MatVec* mv, Apollo::Region* r, 
                    int nrows, int doPredict=0){

    // Set the problem size
    mv->setRows(nrows);

    // Try each policy out
    for(int j = 0; j < NUM_POLICIES; ++j){
            
        mv->resetMemory();

        //int feature = nrows;
        r->begin();
        // Include these features for timing tests only
        r->setFeature(float(nrows));

        // If we're predicting, the getPolicyIndex won't work, so we need will just try all policies
        int policy = (doPredict) ? j : r->getPolicyIndex();
        //printf("Feature %d Policy %d\n", feature, policy);

        int num_threads;

        switch(policy){
            case 0: num_threads = 1; break;
            case 1: num_threads = 2; break;
            case 2: num_threads = 4; break;
            case 3: num_threads = 8; break;
            case 4: num_threads = MAX_THREADS; break;
            default: num_threads = 1;
        }

        // Set the thread count according to policy
        omp_set_num_threads(num_threads);

        // Perform the matrix-vector multiplication
        mv->mulMV(r);

        // End the apollo region
        r->end();

        if(doPredict){
            // Get the predicted best policy from this run
            int predictedBestPolicy = r->queryPolicyModel(r->lastFeats);
            printf("predicted best policy: [%d], executed with: policy [%d], nrows: %d, DP_OPS: %g\n", 
                    predictedBestPolicy, policy, nrows, r->lastFeats[0]);
        }
    }
}

void trainRegion(MatVec* mv, Apollo::Region* r){

    int trainSet[9] = {1, 2, 4, 8, 32, 128, 262144, 524288, MAX_ROWS};

    // Go through each training scenario
    for(int i = 0; i < 9; ++i){

        int nrows = trainSet[i];
        tryAllPolicies(mv, r, nrows);
    }
}

void testRegion(MatVec* mv, Apollo::Region* r){

    int testSet[3] = {4, 64, 524288};

    // Go through each training scenario
    for(int i = 0; i < 3; ++i){

        int nrows = testSet[i];
        tryAllPolicies(mv, r, nrows, 1);
    }
}
#endif // end trainRegion and testRegion definitions


void parseInputs(int argc, char** argv){

    if (argc == 1){
        fprintf(stderr, "Usage: %s [-c CNTR1,CNTR2] [-m] [-r numRuns] [-n regionName] \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int opt;
    while ((opt = getopt(argc, argv, "c:mr:n:")) != -1) {
        // Comma-separated counters list
        if(opt == 'c'){
            usePapi = 1;
            char* cntr;
            const char sep[2] = ",";
            cntr = strtok(optarg, sep);

            while(cntr != nullptr){
                counters.push_back(cntr);
                printf("requested counter: [%s]\n", cntr);
                cntr = strtok(nullptr, sep);
            }
        }
        else if(opt == 'm'){
            isMultiplexed = 1;
        }
        else if(opt == 'r'){
            numRuns = atoi(optarg);
        }
        else if(opt == 'n'){
            regionName = optarg;
        }
        else{
            fprintf(stderr, "Usage: %s [-c CNTR1,CNTR2] [-m] [-r numRuns] [-n regionName] \n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    printf("is multiplexed: %d, use papi: %d, num runs: %d num cntrs: %d \n", 
            isMultiplexed, usePapi, numRuns, counters.size());

    for(int i = 0; i < counters.size(); ++i){
        printf("using counter: [%s]\n", counters[i].c_str());
    }
}


int main(int argc, char** argv){

    parseInputs(argc, argv);

    MatVec mv;

#ifdef APOLLO
    // Setup Apollo
    Apollo *apollo = Apollo::instance();
    Apollo::Region *r;

    if(usePapi){
        r = new Apollo::Region( NUM_FEATURES, "test-region", 
                                NUM_POLICIES, 
                                counters, isMultiplexed);
    }
    else{
        r = new Apollo::Region( NUM_FEATURES, "test-region", 
                                NUM_POLICIES);
    }

    // Repeat the experiements to make sure we are getting 
    // a decent sample of measurements
    for(int i = 0; i < numRuns; ++i){
        // Gather measurements
        trainRegion(&mv, r); 
    }

    // Build a tree from the measurements
    apollo->flushAllRegionMeasurements(1);

    //printf("last feats size: %d\n", r->lastFeats.size());
    //testRegion(&mv, r); 

#endif

// Here is where we will sweep through all the thread counts
// so that we can get a profile of how this program performs
#ifndef APOLLO
    fprintf(stdout, "numThreads,numRows,time(s)\n");

    // We want to re-do runs to average out our time readings
    for(int visitCount = 1; visitCount <= numRuns; visitCount++){
        // Increase the row counts on each iteration
        for(int nrows = START_ROWS; nrows <= MAX_ROWS; nrows*=STEP_MULTIPLIER){
            mv.setRows(nrows);

            for(int num_threads = 1; num_threads <= 16; num_threads*=2){
                mv.resetMemory();

                // Set the thread count
                omp_set_num_threads(num_threads);

                auto exec_time_begin = std::chrono::steady_clock::now();

                // Perform the matrix-vector multiplication
                mv.mulMV();

                auto exec_time_end = std::chrono::steady_clock::now();

                double duration = std::chrono::duration<double>(
                                exec_time_end - exec_time_begin)
                                .count();

                fprintf(stdout, "%d, %d, %g\n", num_threads, nrows, duration);
            }
        } // end of problem-size loop
    } // end of revisit loop
#endif

    return 0;
}

