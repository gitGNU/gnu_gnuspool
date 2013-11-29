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

package printertype_pcl;

our @ISA = qw(printertype);

sub _init {
    my $self = shift;
    $self->SUPER::_init();
    $self->addparam(new boolparam("linetermcr", 0, "Add sequence to add CR to LF"));
}

sub typedescr {
    my $self = shift;
    "PCL";
}

sub generate_setup {
    my $self = shift;
    my $fh = shift;
    print $fh <<EOT;
# Setup file for "Straight PCL" printer handling
# Add your own constructs here but please remember to change
# the type to "custom" at the top or this may get overwritten!

# We set up suffixes so that we can select portrait or landscape
# and a variety of pitches thus:
# form.p10  - portrait 10 pitch
# form.l12  - landscape 12 pitch etc.

# Firstly, we give symbolic names to all the escape sequences used
# Remember: These do not output anything or do anything.
# Codes such as \\e - escape and \\xXX character represented as hex
# are used.

# Use shell script to generate banner pages

bannprog=$main::SPROGDIR/pclbanner

# Define "reset printer" code

RESET=\\eE

# Printers may vary according to whether they want to insert
# CRs before line feeds themselves or expect the software to.
# You may need to make the next line have a "2" or a "3" in between
# "k" and "G".
# The "linetermcr" option sets this for you.

EOT

if ($self->{PARAMS}->{linetermcr}->{VALUE}) {
    print $fh "LINE_TERM=\\e&k3G\n";
}
else {
    print $fh "LINE_TERM=\\e&k2G\n";
}

print $fh <<EOT;
OFF_LINE_TERM=\\e&k0G

# Define codes to select portrait or landscape

PORTRAIT=\\e&l0O
LANDSCAPE=\\e&l1O

# Define codes for pitch selection

P8=\\e(s8.3H
P12=\\e(s12H
P16=\\e(s16.6H
P10=\\e(s10H
P15=\\e(s15H

SET6LPI=\\e&l6D
SET8LPI=\\e&l8D

ITALICS=\\e(s1S
CANCITAL=\\e(s0S

# Nothing has been output yet, but now we proceed to indicate situations
# when we output various codes

# When printer is completely halted, send out reset string

halt    RESET

# If suffix name starts with "l", then output code to set landscape
# printing. When done, set it back to portrait mode
# We have a separate docstart for banners which we can make portrait

{
    (l*)
# Uncomment the ""docstart" following to give a portrait banner despite
# landscape document
        banner
#       docstart LINE_TERM
        -banner
        docstart LINE_TERM LANDSCAPE
        docend PORTRAIT
    (*)
        docstart LINE_TERM
}

# Various pitches

{
    (*12)
        sufstart P12
        docstart LINE_TERM
        docend OFF_LINE_TERM
   (*8)
        sufstart P8
        docstart LINE_TERM
        docend OFF_LINE_TERM
   (*16)
        sufstart P16 SET8LPI
        sufend SET6LPI
        docstart LINE_TERM
        docend OFF_LINE_TERM
   (*15)
        sufstart P15 SET8LPI
        sufend SET6LPI
        docstart LINE_TERM
        docend OFF_LINE_TERM
   (*)
        sufstart P10
        docstart LINE_TERM
        docend OFF_LINE_TERM
}

# For modes with italics
{
    (*it*)
        docstart ITALICS
        sufend CANCITAL
}

EOT
}

1;
