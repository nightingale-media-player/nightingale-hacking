#!/bin/env python
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

from optparse import OptionParser
from datetime import datetime
import sys
import os

defaultKey = "BuildID"

o = OptionParser()
o.add_option("-i", "--input-file", dest="inputFile")
o.add_option("-o", "--output-file", dest="outputFile")
o.add_option("-k", "--key", dest="key")

(options, args) = o.parse_args()

buildID = os.environ.get("SB_BUILDID_OVERRIDE",
                         datetime.now().strftime('%Y%m%d%H%M%S'))

key = options.key or defaultKey

lines = []

for line in open(options.inputFile, 'rb'):
  if line.startswith(key):
    line = key + "=%s\n" % buildID
  lines.append(line)

if options.outputFile:
  output = open(options.outputFile, 'wb')
  output.writelines(lines)
  output.close()
else:
  sys.stdout.writelines(lines)
