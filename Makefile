CC = g++
FLAGS = -fopenmp -std=c++11 -O0 -g #-fno-threadsafe-statics
WORKSPACE_DIR = /g/g15/bolet1/workspace

APOLLO_INCLUDE = ../apollo/include
APOLLO_LIB = ${WORKSPACE_DIR}/apollo/build/src/

PAPI_BASE = ${WORKSPACE_DIR}/spack/opt/spack/linux-rhel7-broadwell/gcc-10.2.1/papi-6.0.0.1-42m6tpjegu2j7hxrz26ii5t23jcmtmca
PAPI_INCLUDE = ${PAPI_BASE}/include/
PAPI_LIB = ${PAPI_BASE}/lib/

CALIPER_BASE = ${WORKSPACE_DIR}/spack/opt/spack/linux-rhel7-broadwell/gcc-10.2.1/caliper-2.5.0-bqvkflizcmjjpbjtwnatgwjpikgajjgd
CALIPER_INCLUDE = ${CALIPER_BASE}/include/
CALIPER_LIB = ${CALIPER_BASE}/lib64/

all: main mainApollo #mainApolloPapi mainApolloCaliper

main: main.cpp
	${CC} ${FLAGS} $^ -o $@

mainApollo: main.cpp
	${CC} ${FLAGS} -I${APOLLO_INCLUDE} -L${APOLLO_LIB} \
				   -Wl,-rpath,${APOLLO_LIB} -DAPOLLO -lapollo \
				   $^ -o $@

mainApolloPapi: main.cpp PapiHelper.cpp
	${CC} ${FLAGS} -I${APOLLO_INCLUDE} -L${APOLLO_LIB} \
				   -I${PAPI_INCLUDE} -L${PAPI_LIB} \
				   -Wl,-rpath,${APOLLO_LIB} -DAPOLLO -lapollo \
				   -Wl,-rpath,${PAPI_LIB} -DPAPI_PERF_CNTRS -lpapi \
				   $^ -o $@


mainApolloCaliper: main.cpp
	${CC} ${FLAGS} -I${APOLLO_INCLUDE} -L${APOLLO_LIB} \
				   -I${CALIPER_INCLUDE} -L${CALIPER_LIB} \
				   -Wl,-rpath,${APOLLO_LIB} -DAPOLLO -lapollo \
				   -Wl,-rpath,${CALIPER_LIB} -DCALI_PERF_CNTRS -lcaliper \
				   $^ -o $@

testPapi: testPapi.c 
	${CC} ${FLAGS} -I${PAPI_INCLUDE} -L${PAPI_LIB} \
				   -Wl,-rpath,${PAPI_LIB} -lpapi \
				   $^ -o $@

.PHONY: clean
clean:
	rm -f *.o main *.core mainApollo mainApolloPapi mainApolloCaliper