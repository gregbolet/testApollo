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

int main(int argc, char** argv){



    // Let's allocate memory to start
    double* mat = (double*) malloc(sizeof(double)*MAX_ROWS*NCOLS);
    double* vec = (double*) malloc(sizeof(double)*NCOLS);
    double* out = (double*) malloc(sizeof(double)*MAX_ROWS);

#ifdef APOLLO
    // Setup Apollo
    Apollo *apollo = Apollo::instance();
    Apollo::Region *r = new Apollo::Region(NUM_FEATURES, "test-region", NUM_POLICIES);

#endif // end APOLLO 


#ifndef APOLLO
    fprintf(stdout, "numThreads,numRows,time(s)\n");
#endif

    // We want to re-do runs to average out our time readings
    for(int visitCount = 1; visitCount <= NUM_REVISITS; visitCount++){
        // Increase the row counts on each iteration
        for(int nrows = START_ROWS; nrows <= MAX_ROWS; nrows*=STEP_MULTIPLIER){

    #ifdef APOLLO
            resetMemory(mat, vec, out, nrows, NCOLS);

            int feature = nrows;
            r->begin();
            //r->setFeature(float(feature));
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

            //#ifdef PAPI_PERF_CNTRS
            //    PapiHelper* ph = mulMV(mat, vec, out, nrows, NCOLS);
            //#else
                // Perform the matrix-vector multiplication
                mulMV(mat, vec, out, nrows, NCOLS, r);
            //#endif

            // End the apollo region
            r->end();
            //goto prematureExit;
    #endif

    // Here is where we will sweep through all the thread counts
    // so that we can get a profile of how this program performs
    #ifndef APOLLO

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

    #endif

        } // end of problem-size loop

    #ifdef APOLLO
        // if we've explored all the problem-size and thread count combinations
        // let's create the policy decision trees
        // The integer input is meaningless, so let's fix it at 1
        if(visitCount == NUM_REVISITS){
            apollo->flushAllRegionMeasurements(1);
        }
    #endif

    } // end of revisit loop

prematureExit:

#ifdef APOLLO
    //delete r;
    //delete apollo; 
#endif 

    // Free our matrices
    free(mat);
    free(vec);
    free(out);


    return 0;
}