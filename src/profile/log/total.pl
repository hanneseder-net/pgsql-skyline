#!/bin/perl -w

use strict;

my $line;
my $n;
my $total = 0.0;

while (defined($line = <>)) {
    if ($line =~ m/^ Total runtime: (\d+\.\d+) ms/) {
	++$n;
	$total += $1;
    }
}

print "n=${n}, total=${total}\n";

