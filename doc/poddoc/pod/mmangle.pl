#! /usr/bin/perl

while (my $arg = shift @ARGV) {
    unless  (open(INF, $arg))  {
	print STDERR "Cannot open $arg\n";
	next;
    }
    open(TMPF, ">MTemp") or die "Cannot open temp file";
    while  (<INF>)  {
	chop;
	next if /^'\"/; # '
	if  (/^\./)  {
	    unless  (/\.SH/)  {
		print TMPF "\n";
		next;
	    }
	    s/^\.SH\s+(.*)/=head1 $1/;
	    s/SYNOPSYS/SYNOPSIS/;
	}
	else  {
	    s/\\f([BI])([^\\]*)\\fR/$1<$2>/g;
	}
	print TMPF "$_\n";
    }
    print TMPF <<EOM;
=head1 AUTHOR

John M Collins, Xi Software Ltd.

=cut

## Local Variables:
## mode: nroff
## End:
EOM
    close INF;
    close TMPF;
    unlink $arg;
    rename "MTemp", $arg;
}

