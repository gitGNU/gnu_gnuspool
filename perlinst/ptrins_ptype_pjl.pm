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

package printertype_pjl;

# This wants testing!!!

our @ISA = qw(printertype);

sub _init {
    my $self = shift;
    $self->SUPER::_init();
    $self->addparam(new boolparam("linetermcr", 0, "Add sequence to add CR to LF in PCL mode"));
}

sub typedescr {
    my $self = shift;
    "PJL";
}

sub generate_setup {
    my $self = shift;
    my $fh = shift;

    # Remember parameters to save typing
    my $lineterm = $self->{PARAMS}->{linetermcr}->{VALUE};

    # Stick out standard stuff

    print $fh <<EOT;
# Setup file for "PJL" printer handling where you can set things up
# and then select the language

# Add your own constructs here but please remember to change
# the type to "custom" at the top or this may get overwritten!

# Firstly, we give symbolic names to all the escape sequences used

# Remember: These do not output anything or do anything.
# Codes such as \\e - escape and \\xXX character represented as hex
# are used.

# Use shell script to generate banner pages
# Override this for PostScript printing

bannprog=$main::SDATADIR/pclbanner

# Define "reset printer" code - PJL version

RESET=\\e%-12345X
DUPLEX=\@PJL SET DUPLEX=ON\\r\\n
NODUPLEX=\@PJL SET DUPLEX=OFF\\r\\n
SETPCL=\@PJL ENTER LANGUAGE = PCL\\r\\n
SETPS=\@PJL ENTER LANGUAGE = POSTSCRIPT\\r\\n
LANDSCAPE=\@PJL SET ORIENTATION = LANDSCAPE\\r\\n
PORTRAIT=\@PJL SET ORIENTATION = PORTRAIT\\r\\n

# Form suffixes (for PCL)
# Printers may vary according to whether they want to insert
# CRs before line feeds themselves or expect the software to.
# You may need to make the next line have a "2" or a "3" in between
# "k" and "G".
# The "linetermcr" option sets this for you.

EOT

    if ($lineterm) {
        print $fh "LINE_TERM=\\e&k3G\n";
    }
    else {
        print $fh "LINE_TERM=\\e&k2G\n";
    }

    print $fh <<EOT;
OFF_LINE_TERM=\\e&k0G

# Define codes to select portrait or landscape (in PCL)

PORTRAIT=\\e&l0O
LANDSCAPE=\\e&l1O

# Define codes for pitch selection (in PCL)

P8=\\e(s8.3H
P12=\\e(s12H
P16=\\e(s16.6H
P10=\\e(s10H
P15=\\e(s15H

SET6LPI=\\e&l6D
SET8LPI=\\e&l8D

ITALICS=\\e(s1S
CANCITAL=\\e(s0S

# For PCL printing We set up suffixes so that we can select portrait or landscape
# and a variety of pitches thus:
# form.p10  - portrait 10 pitch
# form.l12  - landscape 12 pitch etc.
# Firstly, we give symbolic names to all the escape sequences used
# Remember: These do not output anything or do anything so they are harmless for PS.
# Codes such as \\e - escape and \\xXX character represented as hex
# are used.

# Better to omit all headers

nohdr

# Nothing has been output yet, but now we proceed to indicate situations
# when we output various codes

# First we set what happens at the start of a document

docstart RESET
docend RESET

# If suffix ends with -dup we set duplex and remember to unset it at the end

{
    (*-dup)
            docstart DUPLEX
            docend NODUPLEX
}

# If suffix name starts with "l", then output code to set landscape
# printing. When done, set it back to portrait mode
# We have a separate docstart for banners

{
        (l*)
            docstart LANDSCAPE
            docend PORTRAIT
}

# Various pitches for PCL

{
    (*12)
        docstart SETPCL P12 LINE_TERM
    (*8)
        docstart SETPCL P8 LINE_TERM
    (*16)
        docstart SETPCL P16 SET8LPI LINE_TERM
    (*15)
        docstart SETPCL P15 SET8LPI LINE_TERM
    (*)
        docstart P10 LINE_TERM
        docend OFF_LINE_TERM
    (*ps*)
        docstart SETPS
}

# For modes with italics (PCL)
{
    (*it*)
        docstart ITALICS
}

# Printing from windows clients which have their own driver, turn everything off

{
        (win)
                nohdr
                docstart=''
                docend=''
                sufstart=''
                sufend=''
                halt=''
}

EOT
}

sub cupspytypes {
    my $self = shift;
    my @result;
    push @result, [ 'ps', ''];
    push @result, [ 'ps-dup', 'Double-sided'];
    push @result, [ 'win', 'Unchanged for Windows clients'];
    \@result;
}

1;
