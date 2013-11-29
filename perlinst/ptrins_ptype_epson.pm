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

package printertype_epson;

our @ISA = qw(printertype);

sub typedescr {
    my $self = shift;
    "Epson";
}

sub generate_setup {
    my $self = shift;
    my $fh = shift;
    print $fh <<EOT;
# Setup file for "EPSON" printer handling
# Add your own constructs here but please remember to change
# the type to "custom" at the top or this may get overwritten!

# We give symbolic names to the various strings recognised by
# Epson-style printers. Remember that \\e means escape \\xXX means
# Hex character etc

NLQMODE=\\ex1
DRAFTMODE=\\ex0

ELITE=\\eM
PICA=\\eP
P15=\\eg

SET6LPI=\\e2
SET8LPI=\\e0

ITALICS=\\e4
CANCITAL=\\e5

CONDENSED=\\e\\x0f
CANCCOND=\\x12

PROP=\\ep1
CANCPROP=\\ep0
NOPAPOUT=\\e8
PAPOUT=\\e9
RESET=\\e\@

# Turn detect paper out on at the start of each suffix, but avoid it if
# stopped properly

sufstart        PAPOUT
halt            NOPAPOUT

# Width of thing in columns

width 136

# If suffix has got "lq" in it somewhere, set nlq mode and charge double

{
        (*lq)   docstart        NLQMODE
                charge 2000
}

# Select various other modes according to beginning of suffix

{
        (g*)

# Graphics - select CR type mode for banner page but not for the main
# document.
                banner
                onlcr
                extabs
                -banner
                -onlcr
                -extabs

        (e*)    docstart        ELITE
                width 163

        (ec*)   docstart        ELITE CONDENSED
                width 272

        (c*)    docstart        CONDENSED
                width 233

        (p*)    docstart        PROP
                width 200

        (nr*)   docstart        PICA
}

# Select UK character set if suffix has got "uk" in it.

{
        (*uk*)  docstart        '\\eR\\x03'
                sufend          '\\eR\\x00'
}

# Select Italics if suffix has got "it" in it.

{
        (*it*)  docstart        ITALICS
                sufend          CANCITAL
}

# Select 8LPI if suffix has got "8ln" on the end

{
        (*8ln)  docstart        SET8LPI
                sufend          SET6LPI
}

# At the end of each document turn off any funny effects which the the job
# may have turned on.

docend  '\\f' RESET

EOT
}

1;
