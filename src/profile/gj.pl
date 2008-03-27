#!/bin/perl -w

use strict;

die "usage: gj.pl SEED" if ($#ARGV != 0);

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

my $seed = $ARGV[0];


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
## SORT.INDEX
##
for my $dist ("i", "c", "a") {
  for my $dim (1,2,3,4,5,6) {
    my $jobfile = "sort.index.${dist}.${seed}.${dim}.sql";

    next if (-e $jobfile);
    print "INFO: file ${jobfile}...\n";

    open (JOB, ">$jobfile");
    $job = "";
    for my $size (@size) {

      my $table = "${dist}15d${size}${seed}idx";
      my $dims = "d1";
      for (my $i=2; $i<=$dim; ++$i) {
	$dims = $dims . ", d${i}";
      }


      $job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<comment>
explain analyze select * from ${table} order by ${dims};
--</comment>
--<query runid=sort.index.${table}.${dim}>
explain analyze select * from ${table} order by ${dims};
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
explain analyze select * from ${table} skyline of ${dims} with presort;
--</comment>
--<query runid=presort.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with presort;
--</query>
--<comment>
explain analyze select * from ${table}idx skyline of ${dims} with presort;
--</comment>
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

my @dims = (2,3,4,5,6,7,8,9,10,15);
#my @windowsizes = (16,32,64,128,256,512,1024,2048,4096,8192,16384);
my @windowsizes = (1024);
my $windowsize_default = 1024;

my @efwindowsizes = (1,2,4,8,16,32,64,128);
my $efwindowsize_default = 8;

for my $windowsize (@windowsizes) {

    my $wssuffix = ($windowsize != $windowsize_default ? ".ws${windowsize}k" : "");
    my $windowsize_sql = ($windowsize != $windowsize_default ? "windowsize=${windowsize}" : "");

##
## BNL/SFS append/prepend/entropy/random
##
@size = sizes(100000);
for my $windowpolicy ("append", "prepend", "entropy", "random") {
  for my $dist ("i", "c", "a") {
    for my $dim (@dims) {

	

      my $jobfile = "bnl.sfs.${windowpolicy}.${dist}.${seed}.${dim}${wssuffix}.sql";

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
--<query runid=bnl.${windowpolicy}.${table}.${dim}${wssuffix}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=${windowpolicy} $windowsize_sql;
--</query>
--<query runid=sfs.${windowpolicy}.${table}.${dim}${wssuffix}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=${windowpolicy} $windowsize_sql;
--</query>
EOF

	print JOB "$job";
      }

      close (JOB);
    }
  }
}



for my $efwindowsize (@efwindowsizes) {

    my $efwssuffix = ($efwindowsize != $efwindowsize_default ? ".efws${efwindowsize}k" : "");
    my $efwindowsize_sql = ($efwindowsize != $efwindowsize_default ? "efwindowsize=${efwindowsize}" : "");


##
## BNL/SFS+EF append/prepend/entropy/random
##
@size = sizes(100000);
for my $windowpolicy ("append", "prepend", "entropy", "random") {
    for my $efwindowpolicy ("append", "prepend", "entropy", "random") {
	
	next if ($windowpolicy ne $efwindowpolicy);

	for my $dist ("i", "c", "a") {
	    for my $dim (@dims) {
		my $jobfile = "bnl.sfs.ef.${windowpolicy}.${efwindowpolicy}.${dist}.${seed}.${dim}${wssuffix}${efwssuffix}.sql";

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
--<query runid=bnl.ef.${windowpolicy}.${efwindowpolicy}.${table}.${dim}${wssuffix}${efwssuffix}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=${windowpolicy} ef efwindowpolicy=${efwindowpolicy} $windowsize_sql $efwindowsize_sql;
--</query>
--<query runid=sfs.ef.${windowpolicy}.${efwindowpolicy}.${table}.${dim}${wssuffix}${efwssuffix}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=${windowpolicy} ef efwindowpolicy=${efwindowpolicy} $windowsize_sql $efwindowsize_sql;
--</query>
EOF

                    print JOB "$job";
		}
		close (JOB);
	    }
	}
    }
}

} # for efwindowsizes
} # for windowsizes

##
## SFS+INDEX append/prepend/entropy/random
##
@size = sizes(100000);
for my $windowpolicy ("append", "prepend", "entropy", "random") {
  for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6) {
      my $jobfile = "sfs.index.${windowpolicy}.${dist}.${seed}.${dim}.sql";

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
--<query runid=sfs.index.${windowpolicy}.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=${windowpolicy};
--</query>
EOF

	print JOB "$job";
      }
      close (JOB);
    }
  }
}


##
## SFS+INDEX+EF append/prepend/entropy/random
##
@size = sizes(100000);
for my $windowpolicy ("append", "prepend", "entropy", "random") {
  for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6) {
      my $jobfile = "sfs.index.ef.${windowpolicy}.${windowpolicy}.${dist}.${seed}.${dim}.sql";

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
--<query runid=sfs.index.ef.${windowpolicy}.${windowpolicy}.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=${windowpolicy} ef efwindowpolicy=${windowpolicy};
--</query>
EOF

	print JOB "$job";
      }
      close (JOB);
    }
  }
}

