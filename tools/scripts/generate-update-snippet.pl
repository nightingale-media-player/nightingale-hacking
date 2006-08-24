#
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1998-2000
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either of the GNU General Public License Version 2 or later (the "GPL"),
# or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****
#

# This is a slightly modified excerpt from post-mozilla-rel.pl
#   (http://lxr.mozilla.org/mozilla/source/tools/tinderbox/post-mozilla-rel.pl)
#
# Specifically, it has been modified to only create third-gen update snippets
#   and it allows alternate tools to be used for computing hash values.

use strict;
use IO::File;

sub update_create_stats {
  my %args = @_;
  my $update = $args{'update'};
  my $type = $args{'type'};
  my $output_file_base = $args{'output_file'};
  my $url = $args{'url'};
  my $buildid = $args{'buildid'};
  my $appversion = $args{'appversion'};
  my $extversion = $args{'extversion'};
  my $hashfunction = $args{'hashfunction'};

  my($hashvalue, $size, $output, $output_file);

  ($size) = (stat($update))[7];

  my $hashTool;
  if ($hashfunction eq "sha1") {
    $hashTool = $ENV{'SHA1SUM'};
    $hashTool = 'sha1sum' if (not defined($hashTool));
  } else {
    $hashTool = $ENV{'MD5SUM'};
    $hashTool = "md5sum" if (not defined($hashTool));
  }
  $hashvalue = `$hashTool $update`;
  chomp($hashvalue);

  $hashvalue =~ s:^(\w+)\s.*$:$1:g;
  $hashfunction = uc($hashfunction);

  if ( defined($Settings::update_filehost) ) {
    $url =~ s|^([^:]*)://([^/:]*)(.*)$|$1://$Settings::update_filehost$3|g;
  }

  $output  = "$type\n";
  $output .= "$url\n";
  $output .= "$hashfunction\n";
  $output .= "$hashvalue\n";
  $output .= "$size\n";
  $output .= "$buildid\n";
  $output .= "$appversion\n";
  $output .= "$extversion\n";

  $output_file = "$output_file_base";
  if (defined($output_file)) {
    open(UPDATE_FILE, ">$output_file")
      or die "ERROR: Can't open '$output_file' for writing!";
    print UPDATE_FILE $output;
    close(OUTPUT_FILE);
  } else {
    printf($output);
  }
}

my $marfile = $ARGV[0];
my $type = $ARGV[1];
my $snippetfile = $ARGV[2];
my $updateurl = $ARGV[3];
my $buildid = $ARGV[4];
my $appversion = $ARGV[5];
my $hashfunction = $ARGV[6];

&update_create_stats(update => $marfile,
                     type => $type,
                     output_file => $snippetfile,
                     url => $updateurl,
                     buildid => $buildid,
                     appversion => $appversion,
                     extversion => $appversion,
                     hashfunction => $hashfunction,);
