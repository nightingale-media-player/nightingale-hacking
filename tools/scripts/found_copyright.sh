#!/bin/sh

# This script is not intended to be run manually, it is called by check_copyright.sh

# $1 = the working directory for the copyright checker (eg. "/c/Projects/Songbird/trunk/tools/scripts")
# $2 = a line of result from the search performed by search_copyright.sh (eg. "<file_name>:<line_number>:<matching_line>")

if test "$1" == ""; then
  echo "-- no workdir --"
  exit 1
fi
if test "$2" == ""; then
  exit 0 # this is normal if we didnt find any hit, we'll still be called once with no result
fi

echo -n "."
echo "$2" >> $1/copyright.xxxxxa
exit 0
  
