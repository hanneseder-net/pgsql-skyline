#!/bin/bash

BINDIR=/pgsql/bin

$BINDIR/pg_ctl start -w
$BINDIR/psql -d postgres < dumpall.dump

for dist in i c a; do
    for exp in `seq 1 6`; do
	for mantissa in 1 5; do
	    let n=mantissa*10**exp
	    if (( n <= 1000000 )); then
		for seed in `seq 0 9`; do
		    randdataset -$dist -d 15 -n $n -s $seed -I -R | $BINDIR/psql
		done
	    fi
	done
    done
done

$BINDIR/psql <<EOF
vacuum full verbose analyze;
EOF

$BINDIR/pg_ctl stop -w
