#!/bin/perl -w

use strict;


my $job = undef;

sub sizes {
    my $max = shift;
    my @size;
    for my $exp (1,2,3,4,5,6) {
	for my $mant (1,5) {
	    my $n = $mant*10**$exp;
	    if ($n <= $max) {
		push @size, "${mant}e${exp}";
	    }
	}
    }
    return @size;
}

my @size = sizes(1000000);

my $seed = "s4";

##
## SORT
##
for my $dist ("i", "c", "a") {
    for my $dim (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	my $jobfile = "sort.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

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
explain analyze select * from ${table};
--</comment>
--<query runid=select.${table}>
explain analyze select * from ${table};
--</query>
--<query runid=sort.${table}.${dim}>
explain analyze select * from ${table} order by ${dims};
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}


##
## BNL/SFS append
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	my $jobfile = "bnl.sfs.append.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.append.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=append;
--</query>
--<query runid=sfs.append.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=append;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}

##
## BNL/SFS prepend
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	my $jobfile = "bnl.sfs.prepend.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.prepend.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=prepend;
--</query>
--<query runid=sfs.prepend.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=prepend;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}


##
## BNL/SFS ranked
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	my $jobfile = "bnl.sfs.ranked.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.ranked.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=ranked;
--</query>
--<query runid=sfs.ranked.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=ranked;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}



##
## BNL/SFS+EF append
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	my $jobfile = "bnl.sfs.ef.append.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.ef.append.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=append ef efwindowpolicy=append;
--</query>
--<query runid=sfs.ef.append.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=append ef efwindowpolicy=append;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}

##
## BNL/SFS+EF prepend
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	my $jobfile = "bnl.sfs.ef.prepend.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.ef.prepend.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=prepend ef efwindowpolicy=prepend;
--</query>
--<query runid=sfs.ef.prepend.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=prepend ef efwindowpolicy=prepend;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}


##
## BNL/SFS+EF ranked
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	my $jobfile = "bnl.sfs.ef.ranked.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.ef.ranked.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=ranked ef efwindowpolicy=ranked;
--</query>
--<query runid=sfs.ef.ranked.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=ranked ef efwindowpolicy=ranked;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}


##
## SFS+INDEX append
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6) {
	my $jobfile = "sfs.index.append.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}idx";
my $dims = "d1 min";
my $sort = "d1";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
    $sort = $sort . ", d${i}";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table} order by ${sort};
--</comment>
--<query runid=sfs.index.append.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=append;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}

##
## SFS+INDEX prepend
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6) {
	my $jobfile = "sfs.index.prepend.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}idx";
my $dims = "d1 min";
my $sort = "d1";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
    $sort = $sort . ", d${i}";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table} order by ${sort};
--</comment>
--<query runid=sfs.index.prepend.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=prepend;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}


##
## SFS+INDEX ranked
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6) {
	my $jobfile = "sfs.index.ranked.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}idx";
my $dims = "d1 min";
my $sort = "d1";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
    $sort = $sort . ", d${i}";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table} order by ${sort};
--</comment>
--<query runid=sfs.index.ranked.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=ranked;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}



##
## SFS+INDEX+EF append
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6) {
	my $jobfile = "sfs.index.ef.append.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}idx";
my $dims = "d1 min";
my $sort = "d1";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
    $sort = $sort . ", d${i}";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table} order by ${sort};
--</comment>
--<query runid=sfs.index.ef.append.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=append ef efwindowpolicy=append;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}

##
## SFS+INDEX+EF prepend
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6) {
	my $jobfile = "sfs.index.ef.prepend.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}idx";
my $dims = "d1 min";
my $sort = "d1";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
    $sort = $sort . ", d${i}";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table} order by ${sort};
--</comment>
--<query runid=sfs.index.ef.prepend.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=prepend ef efwindowpolicy=prepend;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}


##
## SFS+INDEX+EF ranked
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6) {
	my $jobfile = "sfs.index.ef.ranked.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}idx";
my $dims = "d1 min";
my $sort = "d1";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
    $sort = $sort . ", d${i}";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table} order by ${sort};
--</comment>
--<query runid=sfs.index.ef.prepend.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=ranked ef efwindowpolicy=ranked;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}


##
## PRESORT
##
@size = sizes(1000000);
for my $dist ("i", "c", "a") {
    for my $dim (2) {
	my $jobfile = "presort.${dist}.${seed}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=presort.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with presort;
--</query>
--<query runid=presort.idx.${table}.${dim}>
explain analyze select * from ${table}idx skyline of ${dims} with presort;
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}

##
## SQL
##
@size = sizes(100000);
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	my $jobfile = "sql.${dist}.${seed}.${dim}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";
	for my $size (@size) {

my $table = "${dist}15d${size}${seed}";
my $and = "i.d1 <= o.d1";
my $or = "i.d1 < o.d1";
for (my $i=2; $i<=$dim; ++$i) {
    $and = $and . " and i.d${i} <= o.d${i}";
    $or = $or . " or i.d${i} < o.d${i}";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=sql.${table}.${dim}>
explain analyze select o.* from ${table} as o where not exists (select * from ${table} as i where (${and}) and (${or}));
--</query>
EOF

print JOB "$job";
	}
	close (JOB);
    }
}
