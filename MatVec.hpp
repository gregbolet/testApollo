
#include <math.h>
#include <omp.h>
#include <stdlib.h>
#include <string>

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

// This will be the memory we go through to 
// fill the caches that isn't relevant to computation
#define THROWAWAY_MEM 16777216


class MatVec{
    public:
	    MatVec();
	    ~MatVec();
	    void setRows(int nrows);
        void resetMemory();

#ifdef APOLLO
        void mulMV(Apollo::Region* r);
#else
        void mulMV();
#endif

    private:
	    double* mat, *vec, *out, *throwaway;
	    int nrows, ncols;
};