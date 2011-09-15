#!/bin/sh

# This script is not intended to be run manually, it is called by check_copyright.sh

# $1 = the behavior to use ("check"|"replace") 
# $2 = the working directory for the copyright checker (eg. "/c/Projects/Songbird/trunk/tools/scripts")
# $3 = the years to use in the replaced copyright statement (eg. "2005-2008")
# $4 = a line of result from the search performed by check_copyright.sh (eg. "<file_name>:<line_number>:<matching_line>")

 if test "$1" == ""; then
   echo "-- no behavior specified --"
   exit 1
 fi
 # check actually does nothing here, return success
 if test "$1" == "check"; then
   exit 0
 fi
 if test "$3" == ""; then
   echo "-- no copyright years specified --"
   exit 1
 fi
 if test "$2" == ""; then
   echo "-- no workdir specified --"
   exit 1
 fi
 file="`echo $4 | gawk -F: '{print $1}'`"
 if test "$file" == ""; then
   echo "-- no file specified --"
   exit 1
 fi

 echo -n "- Patching $file... "
 
 if test "$1" == "replace"; then
   sed -i 's/\(^.*Copyright.*\) 200.*POTI\, Inc\.\(.*\)/\1 '$3' POTI, Inc.\2/' $file
 fi
 echo "Ok."
 
 exit 0 
