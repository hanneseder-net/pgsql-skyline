#!/bin/perl -w

use strict;
use Fcntl ':flock'; # import LOCK_* constants

my $lockfile = "js.lock";
my $jobpattern = "*.sql";
my $pid = "$ENV{HOSTNAME}.$$";

sub lock {
    print "INFO: waiting for lock on \"$lockfile\"...\n";
    flock(LOCKFILE,LOCK_EX);
    print "INFO: got lock...\n";
}

sub unlock {
    flock(LOCKFILE,LOCK_UN);
    print "INFO: released lock on \"$lockfile\"...\n";
}

print "INFO: open lockfile \"$lockfile\" non exclusive...\n";
open(LOCKFILE, ">$lockfile") or die "Can't open lockfile $!";

srand();

while (1) {
    lock();
    my @jobs = glob $jobpattern;
    if ($#jobs < 0) {
	print "INFO: no jobs found sleeping for a moment...\n";
	sleep(int(rand(5)));	
    }
    else {
	my $job = shift @jobs;
	print "INFO: working on job $job...\n";
	my $jobwip = $job . "." . $pid;
	if (!rename($job, $jobwip)) {
	    print "WARNING: failed to rename \"$job\" to \"$jobwip\"...\n";
	    sleep(int(rand(5)));
	}
	else {
	    `cat $jobwip`;
	    sleep(int(rand(5)));
	    my $jobdone = $jobwip . ".done";
	    rename($jobwip, $jobdone) || die "Can't rename file $!";
	}
    }
    unlock();
}

close(LOCKFILE);

