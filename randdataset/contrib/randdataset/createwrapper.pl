#!/bin/perl

$maxdim = 20;

@fields = ("id int");

print <<EOF
/*
 * drop all wrapper functions and data types
 */
EOF
    ;

for ($i=1; $i <= $maxdim; ++$i)
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

for ($i=1; $i <= $maxdim; ++$i)
{
    push @fields, (($i > 1 && (($i-1) % 5 == 0)) ? "\n\t" : "") . " d${i} float";

    # CREATE OR REPLACE TYPE not yet supported
    print "CREATE TYPE rand_dataset_${i}d AS\n\t(", join(",", @fields), ");\n\n";
}

print <<EOF
/*
 * wrapper functions for convenience
 */
EOF
    ;

@fields = ("id int");
for ($i=1; $i <= $maxdim; ++$i)
{
    push @fields, (($i > 1 && (($i-1) % 5 == 0)) ? "\n\t   " : "") . " d${i} float";

    $dt = "rds(" . join(",", @fields) . ")";

    print <<EOF
CREATE OR REPLACE FUNCTION rds${i}d(disttype text, rows int, seed int)
	RETURNS setof rand_dataset_${i}d
	AS \$\$
	SELECT *
	FROM pg_catalog.pg_rand_dataset(\$1, ${i}, \$2, \$3) AS
	${dt}
	\$\$ LANGUAGE SQL IMMUTABLE STRICT;

EOF
    ;
}
