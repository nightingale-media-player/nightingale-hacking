#!/bin/sh

# The following search looks for Copyright recursively and then list the matching
# files. The gawk scripts filter out a bunch of directories we don't want to
# match in: dependencies, patches, documentation, tools/win32, installer; and doesn't
# match any Binary files. Also excluded are any subversion files or directories.
#
# This search gives a slew of matches that needed to be narrowed down by the
# specific sections below.
#
# 0.3pre timeline Jan 8, this returns 426 hits not all of which are POTI
# copyright statements

cd ../..
grep -n -R Copyright * | \
 gawk '! /check_copyright/ {print} ' | \
 gawk '! /sanity.diff/ {print} ' | \
 gawk '! /.svn/ {print} ' | \
 gawk '! /^Binary/ {print}' | \
 gawk '! /^dependencies/ {print}' | \
 gawk '! /^patches/ {print}' | \
 gawk '! /^documentation/ {print}' | \
 gawk '! /^tools\/win32/ {print}'  | \
 gawk '! /^installer/ {print}' | \
 gawk '! /2005-2007/ {print}'

# This filters the first column so we are left with JUST filenames
 #gawk -F: '{print $1}' | \

# These three replace the sections containg POTI copyright sections ~392 files
 #gawk '/POTI/ {print}' | \
 #gawk -F: '{print $1}' | \
 #xargs sed --in-place 's/\(^.*Copyright\).*/\1(c) 2005-2007 POTI, Inc./' 

# These three replace the sections containing Pioneers copyright sections - 12 files 
 #gawk '/Pioneers/ {print}' | \
 #gawk -F: '{print $1}' | \
 #xargs sed --in-place 's/\(^.*Copyright\)[:print:]*.*/\1(c) 2005-2007 POTI, Inc./' 

#
# Some of the files had a unicode copyright symbol that sed's regex couldn't match so
#   I had to go back over after doing the above replacements and do the following couple
#   of replacements to pick out the particular files and replace the entire line. I had
#   to seperate them out because they used different comment characters.
#

# For replacing the copyright line entirely in Makefile.in's
 # gawk '/POTI/ {print}' | \
 # gawk '/2006/ {print}' | \
 # gawk '/Makefile.in/ {print}' | \
 # gawk -F: '{print $1}' | \
 # xargs sed --in-place '/\(^.*\)Copyright.*/ c\# Copyright(c) 2005-2007 POTI, Inc.' 

# For replacing the copyright line entirely in c files
 # gawk '/POTI/ {print}' | \
 # gawk '/2006/ {print}' | \
 # gawk -F: '{print $1}' | \
 # xargs sed --in-place '/\(^.*\)Copyright.*/ c\// Copyright(c) 2005-2007 POTI, Inc.' 

