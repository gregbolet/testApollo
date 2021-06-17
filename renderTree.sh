#!/bin/bash

~/workspace/apollo/src/python/analysis/rtree2dot.py -i ~/workspace/testApollo/dtree-latest-rank-0-test-region.yaml -o ~/workspace/testApollo/treefile.dot
dot -Tpdf treefile.dot >> treefile.pdf

