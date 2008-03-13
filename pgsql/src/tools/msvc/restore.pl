#!/bin/perl -w

use strict;

my $BINDIR="/pgsql/bin";

#`$BINDIR/pg_ctl start -w`;
`$BINDIR/psql -d postgres < dumpall.dump`;

my $dim=15;

for my $dist ("i","c", "a") {
    for (my $exp=1; $exp <= 6; ++$exp) {
	for my $mantissa (1, 5) {
	    my $n=$mantissa*10**$exp;
	    if ( $n <= 1000000 ) {
		# tables without index
		for my $seed (0,1,2,3,4) {
		    my $tablename="${dist}${dim}d${mantissa}e${exp}s${seed}";
		    `randdataset -$dist -d $dim -n $n -s $seed -I -R -T $tablename | $BINDIR/psql`;
		}

		# drop some old tables
		#for my $seed (5,6,7,8,9) {
		#    my $tablename="${dist}${dim}d${mantissa}e${exp}s${seed}";
		#    `$BINDIR/psql -c \"drop table if exists $tablename;\"`;
		#}

		# same tables as above, but this time we do create indizes on them
		for my $seed (0,1,2,3,4) {
		    my $tablename="${dist}${dim}d${mantissa}e${exp}s${seed}idx";
		    `randdataset -$dist -d $dim -n $n -s $seed -I -R -T $tablename | $BINDIR/psql`;

		    for my $idx (1,2,3,4,5,6) {
			my $idxname = "ix_${tablename}_${idx}d";
			my @idxfields;
			for (my $i=1; $i<=$idx; ++$i) {
			    push @idxfields, "d$i";
			}

			my $sql = "create index $idxname on $tablename (" . join(", ", @idxfields) . ");";
			`$BINDIR/psql -c \"$sql\"`;
		    }
		}
	    }
	}
    }
}

`$BINDIR/psql -c \"vacuum full verbose analyze;\"`;

#`$BINDIR/pg_ctl stop -w`;
