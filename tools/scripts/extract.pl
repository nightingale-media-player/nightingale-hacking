#!/usr/bin/perl

use strict;

undef $/;
my $file = <STDIN>;

my ($x) = ($file =~ /$ARGV[0].*?{([^}]*)}/s);
print($x);
