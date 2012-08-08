#!/usr/bin/perl
use strict;
use Getopt::Long;
use File::Copy;

my %opt;
GetOptions(\%opt, 'r') || die "Illegal command line"

if($#ARGV == 0) {die "No file specified";}

my $file = $ARGV[0];

# See if this a .a versus .so file
if($file ~= m/
