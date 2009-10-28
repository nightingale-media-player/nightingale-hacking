#!/usr/bin/env python

import os
import sys

from optparse import OptionParser

def main(argv):
   o = OptionParser()
   o.add_option('-d', '--srcDir', dest='srcDir', default=None,
                help='Source directory to start search in')

   o.add_option('-f', '--file', dest='file', default=None,
                help='File to search for')

   o.add_option('-v', '--verbose', dest='verbose',
                default=False, action="store_true",
                help="Be verbose about what we're doing")

   (options, args) = o.parse_args()

   if ((options.srcDir is None) or 
    not(os.path.isdir(options.srcDir)) or
    (options.file is None)):
      o.print_help()
      return -1 

   os.chdir(options.srcDir)
   checkDir = options.srcDir

   while True:
      curDir = checkDir
      possibleMakefile = os.path.join(checkDir, options.file)
      if options.verbose:
         print >>sys.stderr, ("Checking for existence: %s" % (possibleMakefile))

      if os.path.isfile(possibleMakefile):
         if options.verbose:
            print >>sys.stderr, ("Found makefile: %s; returning!\n" % 
             (possibleMakefile))
         
         print possibleMakefile
         return 0

      os.chdir("..")
      checkDir = os.getcwd()
      # Check if the |cd ..| didn't actually move us anywhere; if so,
      # we've reached the root; stop.
      if (curDir == checkDir):
         return -1

if __name__ == '__main__':
   sys.exit(main(sys.argv[1:]))
