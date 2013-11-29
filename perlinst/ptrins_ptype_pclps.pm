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

package printertype_pclps;
use strict;

our @ISA = qw(printertype);

sub _init {
    my $self = shift;
    $self->SUPER::_init();
    $self->addparam(new boolparam("linetermcr", 0, "Add sequence to add CR to LF in PCL mode", "noopt1"));
    $self->addparam(new boolparam("gsforps", 1, "Use ghostscript for PS", "noopt2"));
    $self->addparam(new condboolparam("colour", 1, "Printer is colour-capable", "gsforps", "noopt3"));
    $self->addparam(new condboolparam("colourdef", 1, "Colour by default", "colour", "noopt4"));
    # This next ought really to be conditional on gsforps
    $self->addparam(new optparam("papersize", "a4", "Default paper size", [ "a4", "11x7", "letter", "legal", "ledger", "note"], "noopt5"))
}

sub typedescr {
    my $self = shift;
    "PCLPS";
}

sub generate_setup {
    my $self = shift;
    my $fh = shift;

    # Remember parameters to save typing
    my $lineterm = $self->{PARAMS}->{linetermcr}->{VALUE};
    my $gsforps = $self->{PARAMS}->{gsforps}->{VALUE};
    my $colour = $self->{PARAMS}->{colour}->{VALUE};
    my $colourdef = $self->{PARAMS}->{colourdef}->{VALUE};
    my $papsize = $self->{PARAMS}->{papersize}->{VALUE};

    # Stick out standard stuff

    print $fh <<EOT;
# Setup file for "PS or PCL" printer handling
# This is for the kind of printer that does the right thing according to
# what is thrown at it.

# Add your own constructs here but please remember to change
# the type to "custom" at the top or this may get overwritten!

# Firstly, we give symbolic names to all the escape sequences used
# These apply only to PCL but are harmless for PS.

# Remember: These do not output anything or do anything.
# Codes such as \\e - escape and \\xXX character represented as hex
# are used.

# Define "reset printer" code (PCL)

RESET=\\eE

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

# This is for a printer which prints PostScript if it gets it or PCL
# if it gets that.
# We set up so form types form.ps are printed as PostScript with
# nothing changed, and form types form.anythingelse is treated as PCL

# For PCL printing We set up suffixes so that we can select
# portrait or landscape and a variety of pitches thus:
# form.p10  - portrait 10 pitch
# form.l12  - landscape 12 pitch etc.
# Firstly, we give symbolic names to all the escape sequences used
# Remember: These do not output anything or do anything so they are harmless for PS.
# Codes such as \\e - escape and \\xXX character represented as hex
# are used.

# Use shell script to generate banner pages - override later for PS

bannprog=$main::SPROGDIR/pclbanner

# Nothing has been output yet, but now we proceed to indicate situations
# when we output various codes

# When printer is completely halted, send out reset string (in PCL modes)
# We cancel all the strings in PS mode

halt    RESET

# If suffix name starts with "l", then output code to set landscape
# printing. When done, set it back to portrait mode
# We have a separate docstart for banners

{
        (l*)
# Uncomment the ""docstart" following to give a portrait banner despite
# landscape document
                banner
#               docstart LINE_TERM
                -banner
                docstart LINE_TERM LANDSCAPE
                docend  PORTRAIT
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
    unless  ($gsforps)  {

        # Case where we let printer worry about PS.
        # Insert code to trip out everything and make the empty string

        print $fh <<EOT;
# Cancel all we've done for PS documents and pass straight through

{
    (ps)
        halt=''
        sufstart=''
        sufend=''
        docstart=''
        docend=''
        bannprog=$main::SPROGDIR/psbanner
}
EOT
        return;
    }

    # Cases where we are using Ghostscript to convert PS to PCL

    my $gspath = filerouts::findonpath('gs');

    unless ($gspath)  {
        print "***Warning: GS is missing - please load it\n";
        $gspath = "/usr/bin/gs";
    }

    my $ijspath = filerouts::findonpath('hpijs');

    unless ($ijspath)  {
        print "***Warning: IJS is missing - please load it\n";
        $gspath = "/usr/bin/hpijs";
    }

    my $printertypeforgs = $colour? "HP Color LaserJet": "HP LaserJet";

    print $fh <<EOT;
# For PS documents, cancel all we've done and start setting up filter

{
    (*ps*)
        # Turn off headers as this probably confuses the issue
        nohdr
        halt=''
        sufstart=''
        sufend=''
        docstart=''
        docend=''
        filter '$gspath -q -sDEVICE=ijs -sstdout=%stderr -sIjsServer=$ijspath '
        filter '-dIjsUseOutputFD -sDeviceManufacturer=HEWLETT-PACKARD '
        filter '-sDeviceModel="$printertypeforgs" -sPAPERSIZE=$papsize '
}

# Now append to filter parameters according to suffix

EOT

    if  ($colour)  {
        if  ($colourdef)  {
            print $fh <<EOT;
# With colour the default, cater for various styles of PostScript
# Also have -dup to do duplex later
# ps/cps - default quality colour
# pps/cpps - higher quality colour
# hps/chps - higest quality colour
# mps - default quality no colour
# mpps - higher quality no colour
# mhps - hightest quality no colour
# dps/cdps - draft quality colour
# mdps - draft quality no colour
{
        (ps*)
                filter '-sIjsParams="Quality:Quality=0,Quality:ColorMode=2" '
        (cps*)
                filter '-sIjsParams="Quality:Quality=0,Quality:ColorMode=2" '
        (pps*)
                filter '-sIjsParams="Quality:Quality=2,Quality:ColorMode=2" '
        (cpps*)
                filter '-sIjsParams="Quality:Quality=2,Quality:ColorMode=2" '
        (hps*)
                filter '-sIjsParams="Quality:Quality=3,Quality:ColorMode=2" '
        (chps*)
                filter '-sIjsParams="Quality:Quality=3,Quality:ColorMode=2" '
        (mps*)
                filter '-sIjsParams="Quality:Quality=0,Quality:ColorMode=0" '
        (mpps*)
                filter '-sIjsParams="Quality:Quality=2,Quality:ColorMode=0" '
        (mhps*)
                filter '-sIjsParams="Quality:Quality=3,Quality:ColorMode=0" '
        (dps*)
                filter '-sIjsParams="Quality:Quality=1,Quality:ColorMode=2" '
        (cdps*)
                filter '-sIjsParams="Quality:Quality=1,Quality:ColorMode=2" '
        (mdps*)
                filter '-sIjsParams="Quality:Quality=1,Quality:ColorMode=0" '
}
EOT
       }
        else  {
            print $fh <<EOT;
# With colour available but the not the default, cater for various styles of PostScript
# Also have -dup to do duplex later
# ps/mps - default quality no colour
# pps/mpps - higher quality no colour
# hps/mhps - higest quality no colour
# cps - default quality colour
# cpps - higher quality colour
# chps - hightest quality colour
# dps/mdps - draft quality no colour
# cdps - draft quality colour
{
        (ps*)
                filter '-sIjsParams="Quality:Quality=0,Quality:ColorMode=0" '
        (cps*)
                filter '-sIjsParams="Quality:Quality=0,Quality:ColorMode=2" '
        (pps*)
                filter '-sIjsParams="Quality:Quality=2,Quality:ColorMode=0" '
        (cpps*)
                filter '-sIjsParams="Quality:Quality=2,Quality:ColorMode=2" '
        (hps*)
                filter '-sIjsParams="Quality:Quality=3,Quality:ColorMode=0" '
        (chps*)
                filter '-sIjsParams="Quality:Quality=3,Quality:ColorMode=2" '
        (mps*)
                filter '-sIjsParams="Quality:Quality=0,Quality:ColorMode=0" '
        (mpps*)
                filter '-sIjsParams="Quality:Quality=2,Quality:ColorMode=0" '
        (mhps*)
                filter '-sIjsParams="Quality:Quality=3,Quality:ColorMode=0" '
        (dps*)
                filter '-sIjsParams="Quality:Quality=1,Quality:ColorMode=0" '
        (cdps*)
                filter '-sIjsParams="Quality:Quality=1,Quality:ColorMode=2" '
        (dmps*)
                filter '-sIjsParams="Quality:Quality=1,Quality:ColorMode=0" '
}
EOT
        }
    }
    else  {
            print $fh <<EOT;
# With colour not available, cater for various styles of PostScript
# Also have -dup to do duplex later
# We support the same suffixes as printers with colour on for ease of transition
# ps/mps/cps - default quality no colour
# pps/mpps/cpps - higher quality no colour
# hps/mhps/chps - higest quality no colour
# dps/mdps - draft quality no colour
# cdps - draft quality colour
{
        (ps*)
                filter '-sIjsParams="Quality:Quality=0" '
        (cps*)
                filter '-sIjsParams="Quality:Quality=0" '
        (pps*)
                filter '-sIjsParams="Quality:Quality=2" '
        (cpps*)
                filter '-sIjsParams="Quality:Quality=2" '
        (hps*)
                filter '-sIjsParams="Quality:Quality=3" '
        (chps*)
                filter '-sIjsParams="Quality:Quality=3" '
        (mps*)
                filter '-sIjsParams="Quality:Quality=0" '
        (mpps*)
                filter '-sIjsParams="Quality:Quality=2" '
        (mhps*)
                filter '-sIjsParams="Quality:Quality=3" '
        (dps*)
                filter '-sIjsParams="Quality:Quality=1" '
        (cdps*)
                filter '-sIjsParams="Quality:Quality=1" '
        (dmps*)
                filter '-sIjsParams="Quality:Quality=1" '
}
EOT
    }

    # Add trailing stuff whether colour or not

    print $fh <<EOT;

# Cater for duplex mode if available

{
        (*ps-dup)
                filter ' -dDuplex=true'
}

# Finish off filter command for ps

{
        (*ps*)
                filter ' -dNOPAUSE -dSAFER -sOutputFile=- - -c quit'
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
                filter=''
}

EOT
}

sub cupspytypes {
    my $self = shift;
    my $gsforps = $self->{PARAMS}->{gsforps}->{VALUE};
    my $colour = $self->{PARAMS}->{colour}->{VALUE};
    my $colourdef = $self->{PARAMS}->{colourdef}->{VALUE};
    my @result;
    push @result, [ 'ps', ''];

     if ($gsforps)  {
        push @result, [ 'ps-dup', 'Double-sided'];
        if ($colour)  {
            if ($colourdef)  {
                push @result, [ 'mps', 'greyscale'];
                push @result, [ 'mps-dup', 'greyscale double-sided'];
            }
            else  {
                push @result, [ 'cps', 'colour'];
                push @result, [ 'cps-dup', 'colour double-sided'];
            }
        }
     }
     push @result, [ 'win', 'Unchanged for Windows clients'];
    \@result;
}

1;
