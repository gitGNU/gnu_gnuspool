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

# Routines for parsing default files

package parsedefault;
use strict;

# For looking up device type and returning a new appropriate kind of object

our $Newtype = {PLAIN => sub { printertype_plain->new; },
                IBM   => sub { printertype_ibm->new; },
                EPSON => sub { printertype_epson->new; },
                PCL   => sub { printertype_pcl->new; },
                PS    => sub { printertype_ps->new; },
                PCLPS => sub { printertype_pclps->new; },
                PJL   => sub { printertype_pjl->new; }
};

#  Parse device file for a given printer

sub parse_default {
    my $ptr = shift;
    my $ptrname = $ptr->name;
    my $dir = "$main::SPOOLPT/$ptrname";
    my ($emul, $model, $hasps, $usinggs, $papsize, $colour, $defcolour, %opts);

    unless (open(DFF, "$dir/default"))  {
        print "***Warning cannot find default file for printer $ptrname, recommend abort\n";
        return;
    }

    while  (<DFF>)  {
        chomp;

        # Recognise end of "options" bit and start of setup file proper by a line with
        # leading spaces in it or a keyword followed by =, neither of which are put in
        # by standard options
        last if /^(\s+\S|\{|\w+=)/;

        # Printer type and options are put in as comments

        if (/#\s*Ptrtype:\s*(.*)/)  {
            $emul = uc $1;
        }
        elsif (/#\s*Ptrname:\s*(.*)/)  {
            $model = $1;
        }
        elsif (/#\s*Postscript emulation/) {
            $hasps = 1;
        }
        elsif (/#\s*Using ghostscript/)  {
            $usinggs = 1;
        }
        elsif (/#\s*Paper:\s*(\w+)/)  {
            $papsize = lc $1;
        }
        elsif (/#\s*Colour:\s*(\w+)/) {
            $colour = (lc substr $1, 0, 1) eq 'y'? 1: 0;
        }
        elsif (/#\s*Def.*col.*:\s*(\w+)/) {
            $defcolour = (lc substr $1, 0, 1) eq 'y'? 1: 0;
        }

        elsif (/^(-?)(\w+)$/)  {
            $opts{$2} = length($1) != 0? 0: 1;
        }
    }
    close DFF;

    # Set on has PS if using Ghostscrip

    $hasps ||= $usinggs;

    # If we didn't read anything, must be some custom type

    my $perout = $Newtype->{$emul};

    $emul = "CUSTOM" unless $emul or defined $perout;

    # For fallover from old version, rephrase it if we have PCL printer with PS emulation

    $emul = "PCLPS" if $emul eq "PCL" and $hasps;

    # If custom, just put generic type and and don't do any more

    if  ($emul eq 'CUSTOM')  {
        $ptr->ptremul(printertype->new);
        return;
    }

    # Set actual type

    my $pestr = &$perout;
    $pestr->resetbools();
    $ptr->ptremul($pestr);
    my $peparams = $pestr->{PARAMS};
    $ptr->printer_type($model) if $model;

    # Set the options from the supplied ones

    for my $o (keys %opts)  {
        $peparams->{$o}->{VALUE} = $opts{$o} if defined $peparams->{$o};
    }

    # Set using Ghostscript paper type and colour separately if applicable

    $peparams->{gsforps}->{VALUE} = $usinggs if defined($peparams->{gsforps});
    $peparams->{papersize}->{VALUE} = $papsize if defined $papsize and defined($peparams->{papersize});
    $peparams->{colour}->{VALUE} = $colour if defined $colour and defined($peparams->{colour});
    $peparams->{colourdef}->{VALUE} = $defcolour = defined $defcolour and defined($peparams->{colourdef});
}

# Parse the devices file for the printer list.

sub parse_defaults {
    my $ptrlist = shift;
    my @clones;             # Fix clones later

    for my $p (@{$ptrlist->getptrlist})  {
        my $ptr = $ptrlist->getptr($p);
        if ($ptr->isclone)  {
            push @clones, $ptr;
        }
        else  {
            parse_default($ptr);
        }
    }

    # Go over the clones and copy over the device stuff and custom markers

    for my $c (@clones)  {
        my $cname = $c->isclone;
        my $cptr = $ptrlist->getptr($cname);
        # Better check for chains of symbolic links I suppose
        if  ($cptr->isclone)  {
            print "***Warning: Clones of clones - please note I cannot cope! (Abort suggested)\n";
            print $c->name," is a clone of $cname which is a clone of ", $cptr->isclone,"\n";
            return;
        }
        $c->ptremul($cptr->ptremul);
    }
}

1;
