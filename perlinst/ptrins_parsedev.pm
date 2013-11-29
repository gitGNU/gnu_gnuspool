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

# Routines for parsing .device files

package parsedev;

# For looking up device type and returning a new appropriate kind of object

our $Newtype = {PARALLEL => sub { parallel->new; },
                USB      => sub { usb->new; },
                SERIAL   => sub { serial->new; },
                LPDNET   => sub { lpdnet->new; },
                TELNET   => sub { telnet->new; },
                FTP      => sub { ftp->new; },
                XTLHP    => sub { xtlhp->new; }
};

#  Parse device file for a given printer

sub parse_device {
    my $ptr = shift;
    my $ptrname = $ptr->name;
    my $dir = "$main::SPOOLPT/$ptrname";

    unless (open(DVF, "$dir/.device"))  {
        print "***Warning no .device file for printer $ptrname, trying default\n";
        unless  (open(DVF, "$dir/default"))  {
            print "***Warning cannot find default or .device file for printer $ptrname, recommend abort\n";
            return;
        }
    }

    # Look for port type
    # This should come first

    my $pirout;

    while  (<DVF>)  {
        chomp;
        if  (/#\s*Porttype:\s*(\w+)/)  {
            my $porttype = uc $1;
            if  ($porttype eq 'CUSTOM')  {
                $ptr->custom(1);
                close DVF;
                return;
            }
            $pirout = $Newtype->{$porttype};
            unless  (defined $pirout)  {
                print "***Warning unknown port type $porttype in device file for $ptrname - treating as custom\n";
                $ptr->custom(1);
                close DVF;
                return;
            }
            last;
        }
    }

    # We expected to find the port description in the first few lines so we know what we are dealing with.

    unless  (defined $pirout)  {
        print "***Warning no port description in device file for $ptrname - treating as custom\n";
        $ptr->custom(1);
        close DVF;
        return;
    }

    # Run the routine to give us the right structure

    my $devstruct = &$pirout;
    $devstruct->resetbools;

    # Now read the rest of the file, interpreting keyword parameters and any network= command

    my @kwargs;
    my $nwcmd;

    while  (<DVF>)  {
        chomp;
        if  (/^(-?\w+)(?:\s+(\w+))?\s*$/)  {
            my @kw;
            push @kw, $1;
            push @kw, $2 if length($2) > 0;
            push @kwargs, \@kw;
        }
        elsif (/^network=(.*)/)  {
            $nwcmd = $1;
        }
    }
    close DVF;

    # Interpret keyword parameters

    my $lookup = $devstruct->{PARAMS};
    for my $kw (@kwargs)  {
        my $param = $kw->[0];
        next unless defined $lookup->{$param};
        my $ret = $lookup->{$param}->parse($kw);
        print "Device file for $ptrname - parse of $param went wrong\n" unless ref($ret);
    }

    # Parse network command

    if  (defined $nwcmd)  {
        unless  ($devstruct->{NETDEV})  {
            print "***Warning: device file for $ptrname given as $porttype has network command in - treating as custom\n";
            $ptr->custom(1);
            return;
        }
        my @breakdown = split(/\s+/, $nwcmd);
        $lookup = $devstruct->{OPT};
        while  (@breakdown)  {
            my $opt = $breakdown[0];
            last if length($opt) <= 1;
            unless  ((substr $opt, 0, 1) eq '-')  {
                shift @breakdown;
                next;
            }
            my $optletter = substr $opt, 1, 1;
            next unless defined $lookup->{$optletter};
            my $ret = $lookup->{$optletter}->parse(\@breakdown);
            last unless ref($ret);
            @breakdown = @$ret;
        }
    }
    elsif  ($devstruct->{NETDEV})  {
             print "***Warning: device file for $ptrname given as $porttype has no network command in - treating as custom\n";
            $ptr->custom(1);
            return;
    }

    $ptr->devtype($devstruct);
}

# Parse the devices file for the printer list.

sub parse_devices {
    my $ptrlist = shift;
    my @clones;             # Fix clones later

    for my $p (@{$ptrlist->getptrlist})  {
        my $ptr = $ptrlist->getptr($p);
        if ($ptr->isclone)  {
            push @clones, $ptr;
        }
        else  {
            parse_device($ptr);
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
        $c->devtype($cptr->devtype);
        $c->custom($cptr->custom);
    }
}

1;
