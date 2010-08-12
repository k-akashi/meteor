#!/usr/bin/perl -w
# Created by Ben Okopnik on Wed Oct 31 10:45:14 EST 2001

# Sleep for a delay expressed in ms; callable from shell scripts

die "Usage: ", $0 =~ m{([^/]*)$}, " <delay_in_ms>\n" unless @ARGV;
select undef, undef, undef, 0.001 * $ARGV[0]
