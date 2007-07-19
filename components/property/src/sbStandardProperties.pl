#!/usr/bin/perl -w
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

# This is hard-coded to generate sbStandardProperties.h from
# $(topsrcdir)/app/content/scripts/songbird_interfaces.js

use strict;

sub setup() {
  if ($#ARGV < 0) {
    die "Usage: $0 \$(topsrcdir)/app/content/scripts/songbird_interfaces.js";
  }
  my $SRC = $ARGV[0];
  open(INPUT, "<", "$SRC") or die "Failed to open input file $SRC";
  open(OUTPUT, ">", "sbStandardProperties.h") or die "Failed to open output file";
  
  print OUTPUT <<EOF
#ifndef __SB_STANDARD_PROPERTIES_H__
#define __SB_STANDARD_PROPERTIES_H__
EOF
}

sub read() {
  while(<INPUT>) {
    m/const \s+ SB_PROPERTY_ (\w+) \s* = \s*
      SB_PROPERTY_PREFACE \s* \+ \s* "([^"]*?)"/x or next;
    print OUTPUT "#define SB_PROPERTY_$1 \"http://songbirdnest.com/data/1.0#$2\"\n";
  }
}

&setup();
&read();

print OUTPUT <<EOF
#endif /* __SB_STANDARD_PROPERTIES_H__ */
EOF
;
close INPUT;
close OUTPUT;
