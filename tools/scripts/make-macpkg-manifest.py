#!/usr/bin/env python
# vim: set sw=2 :
#
#=BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2010 POTI, Inc.
# http://www.songbirdnest.com
#
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the ``GPL'').
#
# Software distributed under the License is distributed
# on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
# express or implied. See the GPL for the specific language
# governing rights and limitations.
#
# You should have received a copy of the GPL along with this
# program. If not, go to http://www.gnu.org/licenses/gpl.html
# or write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#=END SONGBIRD GPL
#

"""
Create a Mac OSX PackageMaker.app file list

Example:
  %prog Songbird.app -o 01songbird-contents.xml
"""

from optparse import OptionParser
import os
import sys

DEFAULT_INDENT = '   '

def sweepDir(dir, out, indent):
   """ Generate the contents of one directory
      Note: Does not create the DOM node for the directory itself
      @param dir The path to the directory
      @param out The output stream to write to
      @param indent The indent string to use; will be longer for subdirectories
      @return None
   """
   for item in os.listdir(dir):
      if os.path.isdir(os.path.join(dir, item)):
         # this is a subdirectory; recurse, with longer indent
         # permission is 040775, rwxrwxr-x
         out.write(indent + '<f n="%s" o="root" g="admin" p="16893">\n' % item)
         sweepDir(os.path.join(dir, item), out, indent + DEFAULT_INDENT)
         out.write(indent + DEFAULT_INDENT + '<mod>mode</mod>\n')
         out.write(indent + '</f>\n')
      else:
         # this is a file
         perm = 0100664 # rw-rw-r--
         if os.access(os.path.join(dir, item), os.X_OK):
            perm = 0100775 # rwxrwxr-x
         
         out.write(indent)
         out.write('<f n="%(name)s" o="root" g="admin" p="%(perm)s">\n' %
                    { "name": item,
                      "perm": perm
                    })
         out.write(indent + DEFAULT_INDENT + '<mod>mode</mod>\n')
         out.write(indent + '</f>\n')

def main(argv):
   o = OptionParser(usage="%prog <directory to pack> -o <output file name>",
                           description="Create a Mac OSX PackageMaker.app file list")
   o.add_option("-o", "--out",
                type="string", dest="outname", default=None)

   (opts, topdirs) = o.parse_args(args=argv[1:])

   if opts.outname is None:
      o.print_usage()
      return -1

   for d in topdirs:
      if not os.path.isdir(d):
         print >> sys.stderr, "Invalid directory: " + d
         o.print_usage()
         return -1

   out = None
   try:
      if (opts.outname == '-'):
         out = sys.stdout
      else:
         out = open(opts.outname, 'w')

      out.write('<pkg-contents spec="1.12">\n')
      for topdir in topdirs:
         out.write(DEFAULT_INDENT + 
                   '<f n="%(name)s" o="root" g="admin" p="16893" '
                          'pt="%(path)s" m="true" t="file">\n' %
                   { "name": os.path.basename(topdir),
                     "path": topdir
                   })
         sweepDir(topdir, out, DEFAULT_INDENT)
         out.write(DEFAULT_INDENT + DEFAULT_INDENT + '<mod>mode</mod>\n')
         out.write(DEFAULT_INDENT + '</f>\n')
         out.write('</pkg-contents>\n')
   except Exception, ex:
      print >> sys.stderr, "Error: " + str(ex)
      return 1
   finally:
      if out is not None:
         out.close()

   return 0

if __name__ == "__main__":
   sys.exit(main(sys.argv))
