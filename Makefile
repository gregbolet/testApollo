CC = g++
FLAGS = -fopenmp -std=c++11 #-O0 -g #-fno-threadsafe-statics
WORKSPACE_DIR = /g/g15/bolet1/workspace

APOLLO_INCLUDE = ../apollo/include
APOLLO_LIB = ${WORKSPACE_DIR}/apollo/build/src/

all: main mainApollo 

main: main.cpp MatVec.cpp
	${CC} ${FLAGS} $^ -o $@

mainApollo: main.cpp MatVec.cpp
	${CC} ${FLAGS} -I${APOLLO_INCLUDE} -L${APOLLO_LIB} \
				   -Wl,-rpath,${APOLLO_LIB} -DAPOLLO -lapollo \
				   $^ -o $@

.PHONY: clean
clean:
	rm -f *.o main *.core mainApollo 