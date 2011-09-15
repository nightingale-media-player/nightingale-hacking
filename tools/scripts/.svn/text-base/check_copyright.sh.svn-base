#!/bin/sh

# This script is intended to be run once a year, when we need to change the date
# for the copyright notices throughout the source tree.
#
# To use it, perform the following steps :
#
# - Update the following variable:
    years="2005-2008"
# - Run this script with the 'check' parameter, to determine whether the search filters need updating
# - If needed, update the filters until the search matches only POTI statements
# - Run this script again, this time with the 'replace' parameter

add_result()
{
  if test "$1" != ""; then
    # discard it if it's already been processed
    msearch=`grep -n Copyright $1 | gawk '! /'$years'/ {print}'`
    if test "$msearch" != ""; then
      $dir/found_copyright.sh $dir "$1:x:Copyright POTI"
    fi
  fi
}

echo ""
echo -n "Songbird copyright updater script"

if test "$1" == "check"; then
  echo " - CHECK mode."
elif test "$1" == "replace"; then
  echo " - REPLACE mode."
  echo
  echo "Important note: This script is about to replace POTI copyright notices across the entire "
  echo "source code. If your source tree is in a diff state, you may want to press CTRL-C now,"
  echo "create a patch for your changes, and revert everything. This will help you get back"
  echo "on your feet if this script bulldozes overzealously."
  echo ""
  echo "Also note that running this script without getting any error does not mean that you"
  echo "automatically get a license to commit the changes it has made. You should still scrutinize"
  echo "a diff of the entire tree and look for copyright replacements occuring where they should not."
  echo ""
  echo -n "Press ENTER to continue."
  read
  echo ""
else
  echo ""
  echo ""
  echo "Usage: check_copyright.sh check|replace"
  exit 1
fi

echo ""
echo -n "Searching for 'Copyright'..."

dir="$PWD"

cd ../..

# reset result file, just in case the previous run didnt clean up properly
cat /dev/null > $dir/copyright.xxxxxa

# The following search looks for Copyright (potentially recursively) and then lists the 
# matching files. The gawk scripts filter out a bunch of directories we don't want to
# match in: dependencies, patches, documentation, tools/win32, installer; and doesn't
# match any binary files. Also excluded are any subversion files or directories, and
# a number of keywords that we want to exclude purposefully because they are either not
# one of our copyright statements, or are a special version of it handled as an
# exception in check_copyright.sh
#
# As of 0.4 RC1 (dec 17), this returns 1131 hits, all of which are POTI copyright statements.

if test "$rswitch" == 1; then rswitch="-r"; else rswitch=""; fi

grep -n -r Copyright * | \
 gawk '! /check_copyright/ {print} ' | \
 gawk '! /sanity.diff/ {print} ' | \
 gawk '! /.svn/ {print} ' | \
 gawk '! /^Binary/ {print}' | \
 gawk '! /^dependencies/ {print}' | \
 gawk '! /^patches/ {print}' | \
 gawk '! /^documentation/ {print}' | \
 gawk '! /^tools\/win32/ {print}'  | \
 gawk '! /^compiled/ {print}' | \
 gawk '! /Initial Developer/ {print}' | \
 gawk '! /Vectoreal/ {print}' | \
 gawk '! /Håkan Waara/ {print}' | \
 gawk '! /Free Software Foundation/ {print}' | \
 gawk '! /naturaldocs/ {print}' | \
 gawk '! /JSON.org/ {print}' | \
 gawk '! /smilScript/ {print}' | \
 gawk '! /\<year\>/ {print}' | \
 gawk '! /copyright_message/ {print}' | \
 gawk '! /copyright_url/ {print}' | \
 gawk '! /_COPYRIGHT/ {print}' | \
 gawk '! /LegalCopyright/ {print}' | \
 gawk '! /KEY_MAP_ENTRY/ {print}' | \
 gawk '! /_copyright/ {print}' | \
 gawk '! /patches/ {print}' | \
 gawk '! /1992, 1993/ {print}' | \
 gawk '! /'$years'/ {print}' | \
 gawk '! /.xxxxx/ {print}' | \
 xargs --max-lines=1 --delimiter=\\n $dir/found_copyright.sh $dir

# add a few files manually, these were not found because they are in directories that we discard,
# although we do have a few files of ours there.

add_result dependencies/Makefile.in
add_result dependencies/vendor/mp4/README_SONGBIRD
add_result dependencies/vendor/mozilla/Makefile.in
add_result documentation/Makefile.in

echo " Ok."

# Ideally, the above search does not match any non-POTI copyright statement,
# check that this is the case.

echo -n "Checking that all copyright notices found belong to POTI... "
cat $dir/copyright.xxxxxa | \
 gawk '! /POTI/ {print}' > $dir/copyright.xxxxxb

np="`wc -l $dir/copyright.xxxxxb | gawk '{print \$1}'`"
if test "$np" != "0"; then
  echo "FAILED!"
  echo
  echo "Not all the results are POTI copyright statements, please update the filters in this script to"
  echo "exclude the following hits:"
  echo ""
  cat $dir/copyright.xxxxxb 
  echo
  rm -f $dir/copyright.xxxxxa
  rm -f $dir/copyright.xxxxxb
  exit 1
fi
nc="`wc -l $dir/copyright.xxxxxa | gawk '{print \$1}'`"
echo "Ok ($nc occurences)"

if test "$nc" != "0"; then
  xargs --max-lines=1 --delimiter=\\n --arg-file=$dir/copyright.xxxxxa $dir/process_copyright.sh $1 $dir $years
fi

rm -f $dir/copyright.xxxxxa
rm -f $dir/copyright.xxxxxb

if test "$1" == "replace"; then

  echo "Processing exceptions... "
  
  # some files may need custon sed commands, add them here..

  echo "No exception."
  
  # file="<filename>"
  # echo -n "- $file... "
  # sed -i 's/from/to/' $file
  # echo "Ok."
  
  # file="<filename>"
  # echo -n "- $file... "
  # sed -i 's/from/to/' $file
  # echo "Ok."

fi

echo ""
echo "All done."
exit 0

