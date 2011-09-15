#!/usr/bin/env python
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

from optparse import OptionParser
from datetime import datetime
import sys
import os

DEFAULT_KEY = 'SB_BUILD_ID'

def main(argv):                          
   o = OptionParser()
   o.add_option("-i", "--input-file", dest="inputFile",
                help="Input file to preprocess.")
   o.add_option("-o", "--output-file", dest="outputFile",
                help="File to dump output to; defaults to STDOUT")
   o.add_option("-k", "--key", action="append", dest="key",
                help="Key to substitute a value for; the value MUST be a " +
                     "declaration, i.e. --key variable=value")

   (options, args) = o.parse_args()

   if options.inputFile is None:
      o.print_help()
      return -1

   keys = options.key
   substitutions = {}

   #print "keys are %s" % keys

   if keys is not None:
      # Process all the substitutions
      for key in keys:
         if key.find('=') == -1:
            o.print_help()
            return -1
         (substKey, substValue) = key.split('=')
         substitutions[substKey] = substValue

   # Make sure the BuildID gets specified
   if (DEFAULT_KEY not in substitutions.keys()):
      substitutions[DEFAULT_KEY] = os.environ.get('SB_BUILDID_OVERRIDE',
                                                  datetime.now().strftime('%Y%m%d%H%M%S'))

   #print "substitions are %s" % substitutions

   lines = []

   for line in open(options.inputFile, 'rb'):
      for substKey in substitutions.keys():
         if line.startswith(substKey):
            line = substKey + '=' + substitutions[substKey] + "\n"
            break
      lines.append(line)

   if options.outputFile:
      output = open(options.outputFile, 'wb')
      output.writelines(lines)
      output.close()
   else:
      sys.stdout.writelines(lines)

   return 0

if __name__ == '__main__':
   sys.exit(main(sys.argv[1:]))
