
#include "PapiHelper.h"

PapiHelper::PapiHelper(int isMultiplexed, 
                       int numEvents,
                       std::string* eventNames,
					   int problemSize) 
      : isMultiplexed(isMultiplexed), 
      numEvents(numEvents),
	  problemSize(problemSize),
      events_to_track(eventNames){
                    

    // Take the max possible number of threads to work with
    this->numThreads = omp_get_max_threads();

    // Setup the memory to collect data 
    this->all_cntr_values = (long long**) malloc(sizeof(long long*) * this->numThreads);

    // Setup the eventset mapping
    this->event_sets = (int *) malloc(sizeof(int) * this->numThreads);

    // Let's set up PAPI from the main thread
	int retval;

	// Check correct versioning
	retval = PAPI_library_init( PAPI_VER_CURRENT );
	if ( retval != PAPI_VER_CURRENT ) {
		fprintf(stderr, "PAPI_library_init error: code [%d]\n", retval );
	}

    if(isMultiplexed){
	    // Initialize the multiplexing
	    retval = PAPI_multiplex_init();
	    if ( retval == PAPI_ENOSUPP) {
	    fprintf(stderr, "Multiplexing not supported!\n");
	    }
	    else if ( retval != PAPI_OK ) {
		    fprintf(stderr, "PAPI multiplexing error: code [%d]\n", retval );
	    }
    }

	// Initialize the threading support
	retval = PAPI_thread_init( ( unsigned long ( * )( void ) ) (omp_get_thread_num) );
	if ( retval == PAPI_ECMP) {
	   fprintf(stderr, "PAPI thread init error: code [%d] PAPI_ECMP!\n", retval);
	}
	else if ( retval != PAPI_OK ) {
	   fprintf(stderr, "PAPI thread init error: code [%d]!\n", retval);
	}
}

PapiHelper::~PapiHelper(){

	int i,j;

	// Write data to a file
	std::ofstream outfile;
	outfile.open("counterData.csv", std::ios_base::app);

	// Write the header for all of these runs
	//for(i=0; i < this->numEvents; ++i){
		//outfile << this->events_to_track[i] << ",";
	//}

	//outfile << "thread_idx,num_threads,problem_size\n";

	// Write out our counter data for each thread
	for(i=0; i < this->numThreads; ++i){
		for(j=0; j < this->numEvents; ++j){
			outfile << this->all_cntr_values[i][j] << ",";
		}

		outfile << i << "," << this->numThreads << "," << this->problemSize << "\n";
	}

	// Write the counter names as headers
	//for(i = 0; i < this->numThreads; ++i){
		//for(j = 0; j < this->numEvents; ++j){
			//outfile << this->events_to_track[j] << "_t" << i << ",";
		//}
	//}

	//outfile << "num_threads" << "\n";

	//// Write the counter values now
	//for(i = 0; i < this->numThreads; ++i){
		//for(j = 0; j < this->numEvents; ++j){
			//outfile << this->all_cntr_values[i][j] << ",";
		//}
	//}

	//outfile << this->numThreads << "\n";

	outfile.close();

	// Free up counter memory at the end of the helper run 
	for(i=0; i < this->numThreads; i++){
		//printf("Freeing: [%d]\n", i);
	    free(this->all_cntr_values[i]);
	}

	// Free up the parent cntr values pointer 
	//printf("Freeing all cntr vals\n");
	free(this->all_cntr_values);
	//printf("Freeing event_sets\n");
	free(this->event_sets);
}


int PapiHelper::setupThread(void){
    // Keep track of the eventset identifier for this thread
 	int EventSet = PAPI_NULL;
 	int retval;

	// Setup the event counter memory where we will store
	// the counter results for this thread
	// Let's initialize the whole array to 0
 	long long* cntr_values = (long long*) calloc(this->numEvents, sizeof(long long));


	// Register this thread with PAPI
	if ( ( retval = PAPI_register_thread() ) != PAPI_OK ) {
		fprintf(stderr, "PAPI thread registration error!\n");
	}

	/* Create the Event Set for this thread */
	if (PAPI_create_eventset(&EventSet) != PAPI_OK){
		fprintf(stderr, "PAPI eventset creation error!\n");
	}

	/* In Component PAPI, EventSets must be assigned a component index
	   before you can fiddle with their internals. 0 is always the cpu component */
	retval = PAPI_assign_eventset_component( EventSet, 0 );
	if ( retval != PAPI_OK ) {
		fprintf(stderr, "PAPI assign eventset component error!\n");
	}

    if(this->isMultiplexed){
	    /* Convert our EventSet to a multiplexed EventSet*/
	    if ( ( retval = PAPI_set_multiplex( EventSet ) ) != PAPI_OK ) {
		    if ( retval == PAPI_ENOSUPP) {
			    fprintf(stderr, "PAPI Multiplexing not supported!\n");
	   	    }
		    fprintf(stderr, "PAPI error setting up multiplexing!\n");
	    }
    }

	// Add each of the events by getting their event code from the string
	int eventCode = PAPI_NULL;
	for(int i = 0; i < this->numEvents; i++){

        char* event = &((this->events_to_track[i])[0]);

		if((retval = PAPI_event_name_to_code(event, &eventCode)) != PAPI_OK){
			fprintf(stderr, "Event code for [%s] does not exist! errcode:[%d]\n", event, retval);
		}

		/* Add the events to track to out EventSet */
		if ((retval=PAPI_add_event(EventSet, eventCode)) != PAPI_OK){
			fprintf(stderr, "Add event [%s] error! errcode:[%d]\n", event, retval);
		}
	}

    int threadId = omp_get_thread_num();

    // Add this counter pointer to the counter data set
    this->all_cntr_values[threadId] = cntr_values;
    this->event_sets[threadId] = EventSet;

    // Need to do an OMP global barrier here to make sure everyone is ready
    // to advance 
    #pragma omp barrier
    return retval;
}

int PapiHelper::startThreadRead(){

	int threadId = omp_get_thread_num();
	int EventSet = this->event_sets[threadId];

	// Zero-out all the counters in the eventset
	if (PAPI_reset(EventSet) != PAPI_OK){
		fprintf(stderr, "Could NOT reset eventset!\n");
	}

	/* Start counting events in the Event Set */
	if (PAPI_start(EventSet) != PAPI_OK){
		fprintf(stderr, "Could NOT start eventset counting!\n");
	}

    return 0;
}

int PapiHelper::stopThreadRead(){

	int threadId = omp_get_thread_num();
	int EventSet = this->event_sets[threadId];

	/* stop counting events in the Event Set */
	// Store the resulting values into our counter values array
	if (PAPI_stop( EventSet, (this->all_cntr_values[threadId]) ) != PAPI_OK){
		fprintf(stderr, "Could NOT stop eventset counting!\n");
	}

    return 0;
}

int PapiHelper::destroyThread(){

	int threadId = omp_get_thread_num();
	int EventSet = this->event_sets[threadId];
	int retval = 0;

	// Remove all events from the eventset
	if ( ( retval = PAPI_cleanup_eventset( EventSet ) ) != PAPI_OK ) {
		fprintf(stderr, "PAPI could not cleanup eventset!\n");
	}

	// Deallocate the empty eventset from memory
	if ( ( retval = PAPI_destroy_eventset( &EventSet) ) != PAPI_OK ) {
		fprintf(stderr, "PAPI could not destroy eventset!\n");
	}

	// Shut down this thread and free the thread ID
	if ( ( retval = PAPI_unregister_thread(  ) ) != PAPI_OK ) {
		fprintf(stderr, "PAPI could not unregister thread!\n");
	}

	return retval;
}