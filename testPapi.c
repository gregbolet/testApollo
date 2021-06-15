
#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <omp.h>

int main(int argc, char** argv){

    printf("Starting test!\n");
    int retval;

    for(int i = 0; i < 10; i++){
	    // Check correct versioning
	    retval = PAPI_library_init( PAPI_VER_CURRENT );
	    if ( retval != PAPI_VER_CURRENT ) {
		    fprintf(stderr, "PAPI_library_init error: code [%d] and version number [%d]\n", retval, PAPI_VER_CURRENT );
	    }

	    // Initialize the threading support
	    retval = PAPI_thread_init( ( unsigned long ( * )( void ) ) (omp_get_thread_num) );
	    if ( retval == PAPI_ECMP) {
	    fprintf(stderr, "PAPI thread init error: code [%d] PAPI_ECMP!\n", retval);
	    }
	    else if ( retval != PAPI_OK ) {
	    fprintf(stderr, "PAPI thread init error: code [%d]!\n", retval);
	    }

        PAPI_shutdown();
    }


    printf("Ending test!\n");
    return 0;
}