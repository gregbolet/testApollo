
#include "MatVec.hpp"

MatVec::MatVec(){

    // Let's allocate memory to start
    this->mat       = (double*) malloc(sizeof(double)*MAX_ROWS*NCOLS);
    this->vec       = (double*) malloc(sizeof(double)*NCOLS);
    this->out       = (double*) malloc(sizeof(double)*MAX_ROWS);
    this->throwaway = (double*) malloc(sizeof(double)*THROWAWAY_MEM);
    this->nrows = nrows;
    this->ncols = NCOLS;
}

MatVec::~MatVec(){
    free(this->mat);
    free(this->vec);
    free(this->out);
    free(this->throwaway);
}

void MatVec::setRows(int nrows){
    this->nrows = nrows;
}

void MatVec::resetMemory(){
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


// Let's write an OpenMP matrix-vector multiplication kernel
// We assume an MxN matrix and Nx1 vector as inputs, and an Mx1 vector as output
// We vary the value of M from 1 to a large power of two 
#ifdef APOLLO
extern int usePapi;
void MatVec::mulMV(Apollo::Region* r){
#else
void MatVec::mulMV(){
#endif

    
    int i,j;

    // Make our parallel region
    #pragma omp parallel 
    { 

    #ifdef APOLLO 
        if(usePapi){
            r->apolloThreadBegin();
        }
    #endif

        // Run our for loop
        #pragma omp for private(j)
        for(i=0; i < nrows; i++){
            for(j=0; j < ncols; j++){
                out[i] += mat[ncols*i + j] * vec[j]; 
            }
        }

    #ifdef APOLLO 
        if(usePapi){
            r->apolloThreadEnd();
        }
    #endif

    } // end of omp parallel region

    //fprintf(stdout, "Completed one MULMV call!\n");
}