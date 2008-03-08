#!/bin/perl -w

use strict;


my $job = undef;

my @size;
for my $exp (1,2,3,4,5,6) {
    for my $mant (1,5) {
	my $n = $mant*10**$exp;
	if ($n <= 1000000) {
	    push @size, "${mant}e${exp}";
	}
    }
}

my $seed = "s0";

for my $dist ("i", "c", "a") {
    for my $dim (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	my $jobfile = "sort.${dist}.${seed}.${dim}.sql";
	print "INFO: file ${jobfile}...\n";
	next if (-e $jobfile);

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}";
my $dims = "d1";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i}";
}


$job = <<EOF;
--<comment>
select * from ${table};
--</comment>
--<query runid=select.${table}>
select * from ${table};
--</query>
--<query runid=sort.${seed}.${dim}>
select * from ${table} order by ${dims};
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}

