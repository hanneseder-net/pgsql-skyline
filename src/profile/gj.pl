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

my $seed = "s0";


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


##
## BNL/SFS append/prepend/entropy/random
##
@size = sizes(100000);
for my $windowpolicy ("append", "prepend", "entropy", "random") {
  for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
      my $jobfile = "bnl.sfs.${windowpolicy}.${dist}.${seed}.${dim}.sql";

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
--<query runid=bnl.${windowpolicy}.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=${windowpolicy};
--</query>
--<query runid=sfs.${windowpolicy}.${table}.${dim}>
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
## BNL/SFS+EF append/prepend/entropy/random
##
@size = sizes(100000);
for my $windowpolicy ("append", "prepend", "entropy", "random") {
  for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
      my $jobfile = "bnl.sfs.ef.${windowpolicy}.${windowpolicy}.${dist}.${seed}.${dim}.sql";

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
--<query runid=bnl.ef.${windowpolicy}.${windowpolicy}.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=${windowpolicy} ef efwindowpolicy=${windowpolicy};
--</query>
--<query runid=sfs.ef.${windowpolicy}.${windowpolicy}.${table}.${dim}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=${windowpolicy} ef efwindowpolicy=${windowpolicy};
--</query>
EOF

	print JOB "$job";
      }
      close (JOB);
    }
  }
}

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


##
## BNL/SFS append + EF (windowsize)
##
@size = ("1e4");
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	for my $size (@size) {
	    for my $wndsize (1,3,5,7,10,20,30,50,100) {
	my $jobfile = "bnl.sfs.ef.append.${dist}.${size}.${seed}.${dim}.ef${wndsize}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.ef.append.${table}.${dim}.ef${wndsize}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=append ef efslots=${wndsize};
--</query>
--<query runid=sfs.ef.append.${table}.${dim}.ef${wndsize}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=append ef efslots=${wndsize};
--</query>
EOF

print JOB "$job";
	close (JOB);
	}
    }
}
}



##
## BNL/SFS append + EF entropy (windowsize)
##
@size = ("1e4", "1e5");
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	for my $size (@size) {
	    for my $wndsize (1,3,5,7,10,20,30,50,100,200) {
	my $jobfile = "bnl.sfs.ef.append.entropy.${dist}.${size}.${seed}.${dim}.ef${wndsize}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.ef.append.entropy.${table}.${dim}.ef${wndsize}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=append ef efslots=${wndsize} efwindowpolicy=entropy;
--</query>
--<query runid=sfs.ef.append.entropy.${table}.${dim}.ef${wndsize}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=append ef efslots=${wndsize} efwindowpolicy=entropy;
--</query>
EOF

print JOB "$job";
	close (JOB);
	}
    }
}
}


##
## BNL/SFS prepend + EF entropy (windowsize)
##
@size = ("1e5");
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	for my $size (@size) {
	    for my $wndsize (1,3,5,7,10,20,30,50,100,200) {
	my $jobfile = "bnl.sfs.ef.prepend.entropy.${dist}.${size}.${seed}.${dim}.ef${wndsize}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.ef.prepend.entropy.${table}.${dim}.ef${wndsize}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=prepend ef efslots=${wndsize} efwindowpolicy=entropy;
--</query>
--<query runid=sfs.ef.prepend.entropy.${table}.${dim}.ef${wndsize}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=prepend ef efslots=${wndsize} efwindowpolicy=entropy;
--</query>
EOF

print JOB "$job";
	close (JOB);
	}
    }
}
}


##
## BNL/SFS entropy + EF entropy (windowsize)
##
@size = ("1e5");
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	for my $size (@size) {
	    for my $wndsize (1,3,5,7,10,20,30,50,100,200) {
	my $jobfile = "bnl.sfs.ef.entropy.entropy.${dist}.${size}.${seed}.${dim}.ef${wndsize}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.ef.entropy.entropy.${table}.${dim}.ef${wndsize}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=entropy ef efslots=${wndsize} efwindowpolicy=entropy;
--</query>
--<query runid=sfs.ef.entropy.entropy.${table}.${dim}.ef${wndsize}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=entropy ef efslots=${wndsize} efwindowpolicy=entropy;
--</query>
EOF

print JOB "$job";
	close (JOB);
	}
    }
}
}






exit;

##
## BNL/SFS append (windowsize)
##
@size = ("1e4", "5e4", "1e5");
for my $dist ("i", "c", "a") {
    for my $dim (2,3,4,5,6,7,8,9,10,11,12,13,14,15) {
	for my $size (@size) {
	    for my $wndsize (10,20,30,40,50,100) {
	my $jobfile = "bnl.sfs.append.${dist}.${size}.${seed}.${dim}.wnd${wndsize}.sql";

	next if (-e $jobfile);
	print "INFO: file ${jobfile}...\n";

	open (JOB, ">$jobfile");
	$job = "";

my $table = "${dist}15d${size}${seed}";
my $dims = "d1 min";
for (my $i=2; $i<=$dim; ++$i) {
    $dims = $dims . ", d${i} min";
}


$job = <<EOF;
--<comment>
explain analyze select * from ${table};
--</comment>
--<query runid=bnl.append.${table}.${dim}.wnd${wndsize}>
explain analyze select * from ${table} skyline of ${dims} with bnl windowpolicy=append slots=${wndsize};
--</query>
--<query runid=sfs.append.${table}.${dim}.wnd${wndsize}>
explain analyze select * from ${table} skyline of ${dims} with sfs windowpolicy=append slots=${wndsize};
--</query>
EOF

print JOB "$job";
	close (JOB);
	}
    }
}
}
