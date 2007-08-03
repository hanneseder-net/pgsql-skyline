#!/bin/perl


my $psqlcmd = "/pgsql/bin/psql.exe";

sub psql {
  my $cmd = shift;
  my $outfile = shift;

  open(PSQL, "|$psqlcmd >$outfile") || die "can't fork: $!";
  print PSQL "$cmd\n";
  close(PSQL);
}

sub diff {
  my $file1 = shift;
  my $file2 = shift;

  system "diff $file1 $file2";
}

my $table = "a2d100000";

psql("select * from $table o where not exists (select * from $table i where (i.d1 <= o.d1 and i.d2 <= o.d2) and (i.d1 < o.d1 or i.d2 < o.d2))", "$table.ref");
psql("select * from $table skyline by (d1), (d2) with snl order by id;", "$table.snl");
psql("select * from $table skyline by (d1), (d2) with bnl slots=1 order by id;", "$table.bnl.slots1");
psql("select * from $table skyline by (d1), (d2) with bnl slots=100 order by id;", "$table.bnl.slots100");

diff("$table.ref", "$table.snl");
diff("$table.ref", "$table.bnl.slots1");
diff("$table.ref", "$table.bnl.slots100");
