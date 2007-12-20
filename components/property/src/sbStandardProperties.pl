#!/usr/bin/perl -w
#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
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

# This is hard-coded to generate sbProperties.jsm from sbStandardProperties.h

use strict;

# file handles used:
# CPP        the C++ header to read the definitions from
# JSM_IN     the template for the JSM file
# JSM        handle to the output JSM
  
# open the files and print the headers
sub setup() {
  ($#ARGV < 0) and die "Usage: $0 \$(srcdir)";
  open(JSM_IN, "<", $ARGV[0] . "/sbProperties.jsm.in") or die "Failed to open sbProperties.jsm.in";
  open(CPP, "<", $ARGV[0] . "/sbStandardProperties.h") or die "Failed to open sbStandardProperties.h";
  open(JSM, ">", "sbProperties.jsm") or die "Failed to open sbProperties.jsm";

  while (<JSM_IN>) {
    m/^###/ and last; # marker for "insert contents here"
    print JSM;
  }
}

# print the trailers and clean up
sub finish() {
  while (<JSM_IN>) {
    print JSM;
  }

  close CPP;
  close JSM;
  close JSM_IN;
}

# print out one property in each file
sub emit($) {
  my $aProp = shift(@_);
  printf JSM "  get %-24s() { return this._base + \"%s\"; },\n", ($aProp, $aProp);

}

&setup();
while (<CPP>) {
  /SB_PROPERTY_/ or next;
  m/".*?#(.*?)"/ and emit $1;
}
&finish();
