#!/bin/perl -w

use strict;
use Getopt::Long;

my $n = 1;
my $m = 1;

my $result = GetOptions("n=i" => \$n,
			"m=i" => \$m);

my $table = "k${n}n$m";

my $x = 0.0;
my $y = 0.0;

print <<EOM
DROP TABLE IF EXISTS "$table";
CREATE TABLE "$table" (id varchar, d1 float, d2 float);
COPY "$table" (id, d1, d2) FROM STDIN DELIMITERS ',';
EOM
    ;

for (my $i=1; $i <= $n; $i++) {
    print "\"a$i\",$x,$y\n";
    $x++;
    $y--;
}

for (my $i=1; $i <= $m; $i++) {
    print "\"b$i\",$x,$y\n";
    $x++;
    $y--;
}

$x = -0.5;
$y = -0.5;

for (my $i=1; $i <= $n; $i++) {
    print "\"c$i\",$x,$y\n";
    $x++;
    $y--;
}

print "\\.\n";
