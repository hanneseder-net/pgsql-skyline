#!/bin/perl -w
# -*- Perl -*- tell emacs that it is a perl file
#
# Author: Hannes Eder <Hannes@HannesEder.net>
#
# Description: used to generate parts of randataset.sql.in
#

use strict;

my $maxdim = 20;
my @fields = ("id int");

print <<EOF
/*
 * drop all wrapper functions and data types
 */
EOF
    ;

for (my $i=1; $i <= $maxdim; ++$i)
{
    print "DROP FUNCTION IF EXISTS rds${i}d(text,int,int);\n";
    print "DROP TYPE IF EXISTS rand_dataset_${i}d;\n\n";
}

print <<EOF
/*
 * data types for wrapper functions
 */
EOF
    ;

for (my $i=1; $i <= $maxdim; ++$i)
{
    push @fields, (($i > 1 && (($i-1) % 5 == 0)) ? "\n\t" : "") . " d${i} float";

    # CREATE OR REPLACE TYPE not supported
    print "CREATE TYPE rand_dataset_${i}d AS\n\t(", join(",", @fields), ");\n\n";
}

print <<EOF
/*
 * wrapper functions for convenience
 */
EOF
    ;

@fields = ("id int");
for (my $i=1; $i <= $maxdim; ++$i)
{
    push @fields, (($i > 1 && (($i-1) % 5 == 0)) ? "\n\t   " : "") . " d${i} float";

    my $dt = "rds(" . join(",", @fields) . ")";

    print <<EOF
CREATE OR REPLACE FUNCTION rds${i}d(disttype text, rows int, seed int)
	RETURNS setof rand_dataset_${i}d
	AS \$\$
	SELECT *
	FROM pg_rand_dataset(\$1, ${i}, \$2, \$3) AS
	${dt}
	\$\$ LANGUAGE SQL IMMUTABLE STRICT;

EOF
    ;
}
