#
#   Copyright 2013 Free Software Foundation, Inc.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Various routines to do useful things with the spooler
# Note that virtually everything requires $main::USERPATH to be set to the User Path where binaries are stuffed

package printertype_ps;

our @ISA = qw(printertype);

sub typedescr {
    my $self = shift;
    "PS";
}

sub generate_setup {
    my $self = shift;
    my $fh = shift;
    print $fh <<EOT;
# Setup file for "PS" printer handling
# We assume everything is set up in whatever produced the PS.

# Just say we don't want any formfeeds or anything put at the
# and of the print

# Use shell script to generate banner pages

bannprog=$main::SPROGDIR/psbanner

docstart ''
docend ''
EOT
}

sub cupspytypes {
    my $self = shift;
    my @result;
    # We don't need these but we like to be consistent
    push @result, [ 'ps', ''];
    push @result, [ 'win', 'Unchanged for Windows clients'];
    \@result;
}

1;
