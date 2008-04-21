#!/bin/perl -w

use strict;

my $pmin=1;
my $pmax=1;

my $qmin;
my $qmax;

my $dropped = 0;
my $flushed = 0;
my $improved = 0;
my $n = 0;

my $verbose = 0;

while (<>) {
    $n++;

    s/[ \n\r]//g;

    my @values = split /,/;

    

    $qmin = 1;
    $qmax = 0;

    for my $value (@values) {
	if ($value < $qmin) {
	    $qmin = $value;
	}
	if ($value > $qmax) {
	    $qmax = $value;
	}
    }

    print $_, " [", $qmin, ",", $qmax, "]\n" if ($verbose >= 2);

    if ($pmax < $qmin) {
	$dropped++;
	print "$n: dropped\n" if ($verbose);
    }

    if ($qmax < $pmin) {
	$flushed++;
	print "$n: flushed\n" if ($verbose);
	$pmax = $qmax;
	$pmin = $qmin;
    }

    if ($qmax < $pmax) {
	$improved++;
	print "$n: improved\n" if ($verbose);
	$pmax = $qmax;
    }
}

print <<EOF
  minmax: [$pmin, $pmax]
  tuples: $n
 flushed: $flushed
improved: $improved
 dropped: $dropped
EOF

