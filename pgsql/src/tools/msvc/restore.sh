#!/bin/bash

BINDIR=/pgsql/bin

$BINDIR/pg_ctl start -w
$BINDIR/psql -d postgres < dumpall.dump

for dist in a c i; do
    for dim in 2 3 4; do
	for n in 100 1000 10000 100000 1000000; do
	    pg_sldg -$dist -d $dim -n $n -r -i | $BINDIR/psql
	done
    done
done

dist=i
dim=1

for n in 100 1000 10000 100000 1000000; do
    pg_sldg -$dist -d $dim -n $n -r -i | $BINDIR/psql
done


$BINDIR/pg_ctl stop -w

