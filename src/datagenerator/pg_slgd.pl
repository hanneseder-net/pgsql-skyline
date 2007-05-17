#!/bin/perl

#
# Globals
#
use vars qw/ %opt /;

my $ksldg = "ksldg";

#
# Command line options processing
#
sub init()
{
    use Getopt::Std;
    my $opt_string = 'hecad:n:rt:i';
    getopts( "$opt_string", \%opt ) or usage();
    usage() if $opt{h};
}

#
# Message about this program and how to use it
#
sub usage()
{
  print STDERR << "EOF";

Data Generator for Skyline for feeding PostgreSQL psql

usage: $0 [-hecadnrti] -- [OPTIONS FOR $ksldg]

 -h        : this (help) message

 -e|-c|-a  : distribution
 -d DIM    : dimensions
 -n NUMBER : number of vectors
 -r        : (re)create table
 -t TABLE  : override default table name [eca]$DIMd$NUMBER
 -i        : unique id for every vector

Options for $ksldg: see $ksldg -h

example: $0 -e -d 3 -n 100 | psql postgres

EOF
    exit;
}

init();

die "$0: error: you must spec. exactly one distribution, stopped at" if ($opt{e} + $opt{c} + $opt{a} != 1);

$header=1;
$id = 1;
$prefix = "";
$dist = $opt{e} ? "e" : $opt{c} ? "c" : $opt{a} ? "a" : "?";
$dim = $opt{d};
$n = $opt{n};
$ksldg_args = join(" ", "-$dist", "-d $dim", "-n $n", @ARGV);

$table = "$dist${dim}d$n";

if ($opt{t} ne "") {
  $table = $opt{t};
}

die "$0: you must spec. a table name" if ($table eq "");

open(KSLDG, "$ksldg $ksldg_args |") || die "can't fork: $!";
while ($line = <KSLDG>) {
  if ($header == 1) {
    @fields = split(/ /, $line);
    $dim = $#fields + 1;

    push @flds, "id" if ($opt{i});
    push @defs, "id int" if ($opt{i});

    for ($i=1; $i<=$dim; $i++) {
      push @flds, "d$i";
      push @defs, "d$i float";
    }

    if ($opt{r}) {
      print "drop table if exists $table;\n";
      print "create table $table (", join(", ", @defs), ");\n";
    }

    print "copy $table(", join(", ", @flds), ") from stdin delimiters ';';\n";
    $header = 0;
  }

  if ($opt{i}) {
    $prefix = "$id;";
    $id++;
  }

  $line =~ s/ /;/g;

  print "$prefix$line";
}
close KSLDG || die "bad $ksldg: $! $?";
print "\\.\n";
