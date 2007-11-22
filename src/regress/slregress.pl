#!/bin/perl -w
# -*- Perl -*- tell Emacs it's Perl File
# Author: Hannes Eder <Hannes@HannesEder.net>


my $program = "c:/pgsql/bin/psql.exe";

sub psql($$)
{
  my $sql = shift;
  my $outfile = shift;

  open(PSQL, "|$program >$outfile") || die "can't fork: $!";
  print PSQL "$sql\n";
  close(PSQL);
}


sub psql_immed
{
    my $sql = shift;
    my $args = shift;

    $args="" unless ($args);

    
my $res = `$program $args 2>&1 <<EOF
$sql
EOF`;

    return $res;
}


sub std_query()
{
  my @specs = split(/\s*,\s*/, $sl_attr);

  my @le = ();
  my @lt = ();
  my @eq = ();

  for $spec (@specs) {
    my ($attr, $dir) = split(/\s+/, $spec);
    if ($dir eq "min") {
      push @le, "r1.$attr <= r.$attr";
      push @lt, "r1.$attr < r.$attr";
    }
    elsif ($dir eq "max") {
      push @le, "r1.$attr >= r.$attr";
      push @lt, "r1.$attr > r.$attr";
    }
    elsif ($dir eq "diff") {
      push @eq, "r1.$attr = r.$attr";
    }
    else {
      die "$0: error skyline direction `$dir' is not supported, stopped at";
    }
  }


  my $where = join(" and ", (@le, @eq)) . " and (" . join(" or ", @lt) . ")";
  my $query = "select $tlist from ($basequery) r where not exists (select * from ($basequery) r1 where ($where)) $querysuffix;";

  return $query;
}


sub skyline_query
{
  my $with=shift;
  my @specs=split(/\s*,\s*/, $sl_attr);

  if ($with)
  {
      $with="with $with";
  }
  else
  {
      $with="";
  }

  my @sl = ();

  for $spec (@specs) {
    my ($attr, $dir) = split(/\s+/, $spec);
    if ($dir eq "min" || $dir eq "max" || $dir eq "diff") {
      push @sl, "(r.$attr) $dir";
    }
    else {
      die "$0: error skyline direction `$dir' is not supported, stopped at";
    }
  }

  my $skyline = join(", ", @sl);
  my $query = "select $tlist from ($basequery) r skyline by $skyline $with $querysuffix;";

  return $query;
}

sub setref($)
{
    my $query = shift;

    $refquery = $query;

    print "TEST[$testnr]: ", $basequery, " .";
    
    psql($query, "result.ref");
}

sub cmpref($)
{
    my $query = shift;

    print ".";
    psql($query, "result.cmp");

    system("diff", "-c", "result.ref", "result.cmp");

    if (($? >> 8) != 0)
    {
	print "REF: $refquery\nTHIS: $query\n";
	die "FAILED.\n\n";
    }
}

sub cmpdone()
{
    print " OK.\n";
}

sub runtest()
{
  setref(std_query());
  cmpref(skyline_query());
  cmpref(skyline_query("bnl slots=1"));
  cmpref(skyline_query("bnl entropy"));
  cmpref(skyline_query("sfs"));
  my @spec = split(/\s*,\s*/, $sl_attr);
  if ($#spec < 2) {
      cmpref(skyline_query("presort"));
  }
  cmpref(skyline_query("sfs entropy"));
  cmpref(skyline_query("sfs slots=1"));
  cmpref(skyline_query("sfs ef efslots=5"));
  cmpref(skyline_query("sfs ef efslots=5 efentropy"));
  cmpref(skyline_query("bnl ef efslots=5"));
  cmpref(skyline_query("bnl ef efslots=5 efentropy"));
  cmpdone();
}

##
## MAIN
##

sub selecttest($)
{

    my $select = shift;
    my $test = 1;

    $querysuffix = "";
    $tlist = "";
    $sl_attr = "";
    $basequery ="";

# 1
    $querysuffix = "order by r.id";
    $tlist = "id";
    $sl_attr = "d1 min";
    $basequery = "select * from rds2d('indep', 1000, 0)";
    return if ($select == $test++);

# 2
    $sl_attr = "d1 min, d2 min";
    $basequery = "select * from rds2d('indep', 1000, 0)";
    return if ($select == $test++);

# 3
    $basequery = "select * from a2d1000";
    return if ($select == $test++);

# 4    
    $sl_attr = "d1 min, d2 min, g1 diff";
    $basequery = "select *, floor(d1*10) as g1 from rds2d('indep', 1000, 0)";
    return if ($select == $test++);

# 5
    $sl_attr = "d1 min, d2 min, g1 diff, g2 diff";
    $basequery = "select *, floor(d1*2) as g1, floor(d2*3) as g2 from rds2d('indep', 1000, 0)";
    return if ($select == $test++);

# END

    $basequery = "";
    return;
}


$testnr = 1;
while (1)
{
    selecttest($testnr);
    last if ($basequery eq "");

    runtest();
    $testnr++;
}
