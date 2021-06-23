#!/bin/bash

PREFIX=semi_optimized_papi

./mainApollo -r 5 -c PAPI_DP_OPS,PAPI_SP_OPS -m -n ${PREFIX}_2_cntrs_mtx
./mainApollo -r 5 -c PAPI_DP_OPS -m -n ${PREFIX}_1_cntr_mtx
./mainApollo -r 5 -c PAPI_DP_OPS -n ${PREFIX}_1_cntr_no_mtx
./mainApollo -r 5 -n vanilla_apollo