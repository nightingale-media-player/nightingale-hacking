#!/usr/bin/perl
#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2007 POTI, Inc.
# http://www.songbirdnest.com
# 
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the "GPL").
# 
# Software distributed under the License is distributed 
# on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
# express or implied. See the GPL for the specific language 
# governing rights and limitations.
#
# You should have received a copy of the GPL along with this 
# program. If not, go to http://www.gnu.org/licenses/gpl.html
# or write to the Free Software Foundation, Inc., 
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# 
# END SONGBIRD GPL
#


#
# Usage: expand-jar-mn.pl [sourcepath]
#
# Where sourcepath is the folder containing the soon-to-be-jarred content
#
# Fed a jar.mn file through STDIN, this script looks for extra 
# processing instructions.
#
# Supported instructions:
#
#   @include_all_to [destination]
#        Adds all files in the source folder to the jar manifest
#        using the given destination path.
#
#


$source_path = $ARGV[0];
if (!$source_path) {
  die "expand-jar-mn.pl requires source path arg\n";
}


# Read from stdin.  Expects to be fed a jar.mn file.
while (<STDIN>)
{  
  # Detect "@include_all_to" instruction and 
  # extract desired jar destination path 
  if (/\@include_all_to\s+(.*)$/)
  {    
    $jar_destination = $1;

    # Find all the files below the current folder
    # and filter out things that definitely
    # shouldn't go in the jar
    @files_list = split(/\n/,
       `find $source_path/* -type f ! -path "*.svn*" ! -name ".*" ! -name "Makefile.in" ! -name "jar.mn" ! -name "jar.mn.in" ! -name "*.vcproj" ! -name "_EXCLUDE" ! -name "*.user"  ! -name "Thumbs.db" ! -name "*.rej" ! -name "*.orig"`);
                            
    # Create a jar manifest line for each file 
    foreach $file (@files_list) {

      #
      # FILE EXTENSION WHITELIST
      # Make sure we only include whitelisted file extensions in the jar.
      # If you run into build problems you may need to add more extensions to
      # to the following regular expression.
      #
      if (!($file =~ /\.(css|js|ico|png|gif|jpg|jpeg|xul|xml|html|htm|dtd|properties|swf|swd|rdf|manifest|txt)$/)) {
        die <<END;
--- WARNING ---
expand-jar-mn.pl was aborted due to presence of file with non-whitelisted 
file extension.  

Offending file: $file

If this extension is valid, modify tools/scripts/expand-jar-mn.pl line 74.

Better yet, make Ben do it.  This was his idea. 
---------------
END
      }
      
      # Get the relative path of the file 
      $file =~ s/$source_path\///;
 
      print "  $jar_destination$file       ($file)\n";
    }
  }
  else
  {
    # No instruction found.  Just print the line as-is. 
    print;
  }
}