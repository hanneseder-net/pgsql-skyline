#!/bin/perl -w

use strict;
use Fcntl ':flock'; # import LOCK_* constants

my $lockfile = "js.lock";
my $jobpattern = "*.sql";
#my $pid = "$ENV{HOSTNAME}.$$";
my $pid = $ENV{'HOSTNAME'};
my $delay = 5;

mkdir($pid) if (! -d $pid);
mkdir("done")  if (! -d ("done"));
mkdir("log") if (! -d "log");

sub lock {
    print "INFO: waiting for lock on \"$lockfile\"...";
    flock(LOCKFILE,LOCK_EX);
    print " [OK]\n";
}

sub unlock {
    flock(LOCKFILE,LOCK_UN);
    print "INFO: released lock on \"$lockfile\"\n";
}

print "INFO: open lockfile \"$lockfile\" non exclusive...\n";
open(LOCKFILE, ">$lockfile") or die "Can't open lockfile $!";

srand();

while (1) {
    print "INFO: sleep for a moment...\n";
    sleep(int(rand($delay)));

    lock();
    my @jobs = glob $jobpattern;
    if ($#jobs < 0) {
	print "INFO: no jobs found, sleeping for a moment...\n";
	sleep(int(rand($delay)));
	unlock();
    }
    else {
	my $job = shift @jobs;
	print "INFO: trying to dequeue job \"$job\"...\n";
	my $jobwip = $pid . "/" . $job;
	if (!rename($job, $jobwip)) {
	    print "WARNING: failed to rename \"$job\" to \"$jobwip\"...\n";
	    sleep(int(rand($delay)));
	    unlock();
	}
	else {
	    unlock();
	    print "INFO: fine, start working in job...\n";
	    my $joblog = "log/" . $job . "." . $pid . ".log";
	    system "echo \"--<pid pid=$pid/>\" | cat - $jobwip | c:/pgsql/bin/psql -a >$joblog 2>&1" || die "Couldn't execute job $!";
	    sleep(int(rand($delay)));
	    my $jobdone = "done/" . $job;
	    print "INFO: done working on job \"$jobwip\".\n";
	    if (-e $jobdone) {
		print "WARNING: unlinking \"$jobdone\".\n";
		unlink($jobdone) || die "Couldn't unlink file $!";
	    }
	    rename($jobwip, $jobdone) || die "Couldn't rename file $!";
	}
    }
}

close(LOCKFILE);

