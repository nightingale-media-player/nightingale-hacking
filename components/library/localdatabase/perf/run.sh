#!/bin/sh

LIBRARY_DIR=/home/steve/test_libraries/
RESULTS_FILE=/home/steve/results_memsort_orig.txt

LIBRARY_FILES="test_5000.db test_10000.db test_25000.db test_50000.db test_100000.db"
PERF_TESTS="guidarray guidarray_multisort guidarray_distinct guidarray_default_view guidarray_library_enumerate"

for library_file in $LIBRARY_FILES; do
  for perf_test in $PERF_TESTS; do
    for i in `seq 1 5`; do
      echo $LIBRARY_DIR$library_file
      export SB_PERF_LIBRARY=$LIBRARY_DIR$library_file
      export SB_PERF_RESULTS=$RESULTS_FILE
      ./songbird -test localdatabaselibraryperf:$perf_test
    done;
  done;
done
