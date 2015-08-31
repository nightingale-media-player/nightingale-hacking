#!/bin/sh

set -x

# Clean up
rm -rf api

# Make output folder
mkdir api

# Run doxygen
doxygen nightingale.doxyfile
