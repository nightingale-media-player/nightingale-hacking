#!/usr/bin/perl

use strict;

undef $/;
my $file = <STDIN>;

my ($x) = ($file =~ /$ARGV[0].*?{([^}]*)}/s);
for (my $i = 1; $i <= $#ARGV; $i++) {
  
  # this expression kills the properties named by additional arguments
  $x =~ s@
    (?!                   # make sure we are not starting in the middle of a comment
      (?:(?!/\*).)*         # anything not start of comment
      \*/                   # end of comment
    )
    (?:                   # comments before the identifier
      /\*                   # start of comment
      (?:(?!\*/).)*         # anything in the middle
      \*/                   # end of comment
    )*                    #... any number of comments (not just one)
    (?:(?!/\*).)*?        # anything that doesn't start a new comment
    \b${ARGV[$i]}\b       # the identifier we're interested in
    (?:(?!/\*).)*         # and the rest of the stuff to the next comment
    @@sxg;
}
print($x);
