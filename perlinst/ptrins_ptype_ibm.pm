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

package printertype_ibm;

our @ISA = qw(printertype);

sub typedescr {
    my $self = shift;
    "IBM";
}

sub generate_setup {
    my $self = shift;
    my $fh = shift;
    print $fh <<EOT;
# Setup file for "IBM Proprinter" printer handling
# Add your own constructs here but please remember to change
# the type to "custom" at the top or this may get overwritten!

# We give symbolic names to the various strings recognised by
# Epson-style printers. Remember that \\e means escape \\xXX means
# Hex character etc

NLQMODE=\\eG
DRAFTMODE=\\eH
SET6LPI=\\e2
SET8LPI=\\e0

# (settings for form length expressed in inches)

LEGALPAPER=\\eC 0 14
STDPAPER=\\eC 0 11

DBLWIDE=\\eW1
CANCDBLWIDE=\\eW0
ELITE=\\e:
PICA=\\x12
PROP=\\eP1
CANCPROP=\\eP0
FONTRESET=\\e I 0
RESET=\\x18
CONDENSED=\\e\\x0f
CANCCOND=\\x12

# reset fonts when completely halting printer

halt    FONTRESET

# If suffix ends in "lq", set nlq mode and charge double
{
        (*lq)   docstart        NLQMODE
                charge 2000
}

# If suffix ends "dbl", set pitch to double wide
{
        (*dbl)  docstart        DBLWIDE
}

# Select various other modes according to beginning of suffix
{
        (e*)    docstart        ELITE
                width 163
        (p*)    docstart        PROP
                width 200
        (nr*)   docstart        PICA
}

# If paper length included anywhere in suffix, set here
{
        (*leg*) docstart        LEGALPAPER
}

# If lines per inch included anywhere in suffix, set here
{
        (*8ln*) docstart        SET8LPI
}

# At the end of each document turn off any funny effects which the the job
# may have turned on.

docend  '\\f' FONTRESET
EOT
}

1;
