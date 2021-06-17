#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <string>
#include <chrono>

#ifdef APOLLO
    #include "apollo/Config.h"
    #include "apollo/Apollo.h"
    #include "apollo/Region.h"

    // The only feature we're tracking is number of rows
    // for a run.
    #define NUM_FEATURES 1

    // We will have 5 execution policies
    // 1, 2, 4, 8, 16 threads
    #define NUM_POLICIES 5
    #define MAX_THREADS 16

#endif //end APOLLO


// Keep the number of columns constant
#define NCOLS 100

// Going to change the number of rows
// and see if the policy switches between
// serial and parallel
// Stopping at 2^20 rows (i.e: 21 steps)
#define MAX_ROWS 16777216/16
#define START_ROWS 1
#define STEP_MULTIPLIER 2

// How many times to repeat the runs
// This means we will have (51-1)/NUM_POLICIES = 10 measurements
// for each feature/policy combination
#define NUM_REVISITS 5

// This will be the memory we go through to 
// fill the caches that isn't relevant to computation
#define THROWAWAY_MEM 16777216
double* throwaway;

// Let's write an OpenMP matrix-vector multiplication kernel
// We assume an MxN matrix and Nx1 vector as inputs, and an Mx1 vector as output
// We vary the value of M from 1 to a large power of two 
#ifdef APOLLO
void mulMV(double* mat, double* vec, double* out, const int m, const int n, Apollo::Region* r){
#else
void mulMV(double* mat, double* vec, double* out, const int m, const int n){
#endif

    
    int i,j;

    // Make our parallel region
    #pragma omp parallel 
    { 

    #ifdef APOLLO
        r->apolloThreadBegin();
    #endif

        // Run our for loop
        #pragma omp for private(j)
        for(i=0; i < m; i++){
            for(j=0; j < n; j++){
                out[i] += mat[n*i + j] * vec[j]; 
            }
        }

    #ifdef APOLLO
        r->apolloThreadEnd();
    #endif

    } // end of omp parallel region

    //fprintf(stdout, "Completed one MULMV call!\n");
}

void resetMemory(double* mat, double* vec, double* out, int nrows, int ncols){

    // Let's fill up each bit of memory with ones
    int i,j;
    for(i = 0; i < nrows; i++){
        for(j = 0; j < ncols; j++){
            mat[i*ncols + j] = 1;
        }
    }
    for(j = 0; j < ncols; j++){
        vec[j] = 2;
    }
    for(i = 0; i < nrows; i++){
        out[i] = 0;
    }

    // Fill up the caches with this throwaway memory
    for(i = 0; i < THROWAWAY_MEM; ++i){
        throwaway[i] = 0;
    }

    return;
}

void showResult(double* out, int nrows){

    int j;

    fprintf(stdout, "\n[");
    for(j = 0; j < nrows-1; j++){
        fprintf(stdout, "%g\t", out[j]);
    }
    fprintf(stdout, "%g]\n", out[j]);
}

#ifdef APOLLO
void trainRegion(double* mat, double* vec, double* out, Apollo::Region* r){

    int trainSet[6] = {2, 8, 32, 128, 262144, MAX_ROWS};

    // Go through each training scenario
    for(int i = 0; i < 6; ++i){

        int nrows = trainSet[i];

        // Try each policy out
        for(int j = 0; j < NUM_POLICIES; ++j){
            
            resetMemory(mat, vec, out, nrows, NCOLS);

            //int feature = nrows;
            r->begin();
            //r->setFeature(float(feature));
            // MUST BE USING THE ROUNDROBIN POLICY FOR THIS TO WORK
            int policy = r->getPolicyIndex();
            //printf("Feature %d Policy %d\n", feature, policy);

            int num_threads;

            switch(policy){
                case 0:
                    num_threads = 1;
                    break;
                case 1:
                    num_threads = 2;
                    break;
                case 2:
                    num_threads = 4;
                    break;
                case 3:
                    num_threads = 8;
                    break;
                case 4:
                    num_threads = MAX_THREADS;
                    break;
                default:
                    num_threads = 1;
            }

            // Set the thread count according to policy
            omp_set_num_threads(num_threads);

            // Perform the matrix-vector multiplication
            mulMV(mat, vec, out, nrows, NCOLS, r);

            // End the apollo region
            r->end();

        }
    }
}

void testRegion(double* mat, double* vec, double* out, Apollo::Region* r){

    int testSet[3] = {4, 64, 524288};

    // Go through each training scenario
    for(int i = 0; i < 3; ++i){

        int nrows = testSet[i];

        // Try each policy out
        for(int j = 0; j < NUM_POLICIES; ++j){
            
            resetMemory(mat, vec, out, nrows, NCOLS);

            //int feature = nrows;
            r->begin();
            //r->setFeature(float(feature));
            // MUST BE USING THE ROUNDROBIN POLICY FOR THIS TO WORK
            int policy = j; //r->getPolicyIndex();
            //printf("Feature %d Policy %d\n", feature, policy);

            int num_threads;

            switch(policy){
                case 0:
                    num_threads = 1;
                    break;
                case 1:
                    num_threads = 2;
                    break;
                case 2:
                    num_threads = 4;
                    break;
                case 3:
                    num_threads = 8;
                    break;
                case 4:
                    num_threads = MAX_THREADS;
                    break;
                default:
                    num_threads = 1;
            }

            // Set the thread count according to policy
            omp_set_num_threads(num_threads);

            // Perform the matrix-vector multiplication
            mulMV(mat, vec, out, nrows, NCOLS, r);

            // End the apollo region
            r->end();

            // Get the predicted best policy from this run
            int predictedBestPolicy = r->queryPolicyModel(r->lastFeats);
            printf("predicted best policy: [%d], executed with: policy %d, nrows: %d, DP_OPS: %g\n", 
                   predictedBestPolicy, policy, nrows, r->lastFeats[0]);

        }
    }
}
#endif // end trainRegion and testRegion definitions

int main(int argc, char** argv){

    // Let's allocate memory to start
    double* mat = (double*) malloc(sizeof(double)*MAX_ROWS*NCOLS);
    double* vec = (double*) malloc(sizeof(double)*NCOLS);
    double* out = (double*) malloc(sizeof(double)*MAX_ROWS);
    throwaway = (double*) malloc(sizeof(double)*THROWAWAY_MEM);

#ifdef APOLLO
    // Setup Apollo
    Apollo *apollo = Apollo::instance();
    Apollo::Region *r = new Apollo::Region(NUM_FEATURES, "test-region", NUM_POLICIES);

    // Repeat the experiements to make sure we are getting 
    // a decent sample of measurements
    for(int i = 0; i < 1; ++i){
        // Gather measurements
        trainRegion(mat, vec, out, r); 
    }

    // Build a tree from the measurements
    apollo->flushAllRegionMeasurements(1);

    // Run the testing set and query the model after each run
    // to see if the predicted best policy is correct
    testRegion(mat, vec, out, r); 

    //goto prematureExit;
#endif

// Here is where we will sweep through all the thread counts
// so that we can get a profile of how this program performs
#ifndef APOLLO
    fprintf(stdout, "numThreads,numRows,time(s)\n");

    // We want to re-do runs to average out our time readings
    for(int visitCount = 1; visitCount <= NUM_REVISITS; visitCount++){
        // Increase the row counts on each iteration
        for(int nrows = START_ROWS; nrows <= MAX_ROWS; nrows*=STEP_MULTIPLIER){

            for(int num_threads = 1; num_threads <= 16; num_threads*=2){
                resetMemory(mat, vec, out, nrows, NCOLS);

                // Set the thread count
                omp_set_num_threads(num_threads);


                auto exec_time_begin = std::chrono::steady_clock::now();

                // Perform the matrix-vector multiplication
                mulMV(mat, vec, out, nrows, NCOLS);

                auto exec_time_end = std::chrono::steady_clock::now();

                double duration = std::chrono::duration<double>(
                                exec_time_end - exec_time_begin)
                                .count();

                fprintf(stdout, "%d, %d, %g\n", num_threads, nrows, duration);
            }
        } // end of problem-size loop
    } // end of revisit loop
#endif

prematureExit:

#ifdef APOLLO
    //delete r;
    //delete apollo; 
#endif 

    // Free our matrices
    free(mat);
    free(vec);
    free(out);
    free(throwaway);

    return 0;
}