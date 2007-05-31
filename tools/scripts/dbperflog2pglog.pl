#!/usr/bin/perl
#
# This script will convert the output of the sbDatabaseEnginePerformanceLogger
# log to something that resembles a postgresql log file so you can use report
# generation tools such as pgFouine (http://pgfouine.projects.postgresql.org)
#
# Usage:
#
# First, enable the sbDatabaseEnginePerformanceLogger log and capture the
# output into a file
#
# $ export NSPR_LOG_MODULES=sbDatabaseEnginePerformanceLogger:5
# $ export NSPR_LOG_FILE=nsprlog.txt
#
# Then run songbird to generate some output.  Once you've quit songbird, filter
# the log file to throw away any extra stuff that was captured
#
# $ grep sbDatabaseEnginePerformanceLogger nsprlog.log > dbperf.log
#
# Use this script to convert the log file to the postgresql style log file
#
# $ ./dbperflog2pglog.pl < dbperf.log > pg.log
#
# Finally, run the report tool over the log
#
# $ ./pgfouine.php -file pg.log -durationunit ms > report.html
#

my $line = '';
my $count = 0;

while(<>) {
  chop;

  # remove everything up to sbDatabaseEnginePerformanceLogger
  s/.*sbDatabaseEnginePerformanceLogger //g;

  if (/^\+/) {
    s/^\+//g;
    $line .= $_;
  }
  else {
    if ($line ne '') {
      print "Jan  1 00:00:00 hostname postgres[1]: [$count-1] $line\n";
      $count++;
    }
    my @parts = split("\t");
    $line = "LOG:  duration: " . ($parts[1] / 1000) . " ms statement: " . $parts[2];
  }
}

