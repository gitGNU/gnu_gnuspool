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

package spool_ops;

# Is spooler running - check by seeing if gspl-plist gives an error code
# (maybe refine this?)

sub isrunning {
    system("$main::USERPATH/gspl-plist >/dev/null 2>&1") == 0;
}

# Is printer running?

sub isrunning_ptr {
    my $ptr = shift;
    my @args;
    push @args, "$main::USERPATH/gspl-pstat";
    push @args, $ptr->name;
    system(join(' ', @args)) == 0;
}

# Restart printer

sub restart_printer {
    my $ptr = shift;
    my @args;
    push @args, "$main::USERPATH/gspl-start";
    push @args, $ptr->name;
    system(join(' ', @args)) == 0;
}

# Stop printer

sub stop_printer {
    my $ptr = shift;
    my @args;
    push @args, "$main::USERPATH/gspl-pstop";
    push @args, $ptr->name;
    system(join(' ', @args)) == 0;
}

# Install a printer given in supplied printer structure

sub install_printer {
    my $ptr = shift;
    my @args;
    push @args, "$main::USERPATH/gspl-padd";
    push @args, "-l";
    push @args, $ptr->insdev;
    my $descr = $ptr->descr;
    $descr = $ptr->printer_type unless length($descr) > 0;
    $descr =~ y/-.,a-zA-Z0-9 //cd;
    unless (length($descr) == 0)  {
        $descr = '"' . $descr . '"';
        push @args, "-D";
        push @args, $descr;
    }
    push @args, "-N" if $ptr->insnet;
    push @args, $ptr->name;
    push @args, $ptr->insform;
    system(join(' ', @args)) == 0;
}

# Deinstall printer

sub deinstall_printer {
    my $ptr = shift;
    my @args;
    push @args, "$main::USERPATH/gspl-pdel";
    push @args, $ptr->name;
    system(join(' ', @args)) == 0;
}

# Get a list of defined printers as a "ptrlist"

sub list_defptrs {
    my $result = ptrlist->new;

    # If no printers main directory, just return null list, that's fine

    return $result unless opendir(PTDIR, $main::SPOOLPT);

    while  (my $de = readdir(PTDIR))  {
        my $sd = "$main::SPOOLPT/$de";

        # Ignore it if it isn't a directory and something which might be a printer

        next unless $de =~ /^[a-zA_Z]\w*$/  && -d $sd;

        # OK set up printer definition

        my $ptr = printer->new;
        $ptr->name($de);

        # Check if it's a clone but ignore whole thing if it's not on same level directory

        if (-l $sd) {       # We can't use -l _ after -f
            my $lc = readlink($sd);
            next if  $lc =~ m|/| || $lc eq $de;
            $ptr->cloneof($lc);
        }

        $result->add($ptr);
    }
    closedir PTDIR;
    $result;
}

# Update printer list with installed printers assuming spooler running

sub insptrs_online {
    my $ptrlist = shift;
    my $tempfile = "/tmp/tmppl$$";

    # Run gspl-plist with output to a temporary file (to avoid running it holding things up)

    my @args;

    push @args, "$main::USERPATH/gspl-plist";
    push @args, "-N";   # No header
    push @args, "-l";   # Local printers only
    push @args, "-C", "A-Pa-p";     # Set on all class code bits in case some are off
    push @args, "-F";   # Specify format
    push @args, "'%p:%d:%e:%f'";   # Format specifier, printer name, device, descr, form
    push @args, ">$tempfile", "2>/dev/null";

    unless (system(join(' ', @args)) == 0  &&  open(TF, $tempfile))  {
        unlink $tempfile;
        return;
    }

    while  (<TF>)  {
        chop;
        my ($ptr,$dev,$descr,$form) = split('\s*:\s*');
        next if length($ptr) == 0 or length($dev) == 0;
        my $pp = $ptrlist->getptr($ptr);
        next unless defined $pp;
        $pp->descr($descr);
        my $insn = 0;
        if  ($dev =~ /<(.*)>/)  {
            $dev = $1;
            $insn = 1;
        }
        $pp->insdev($dev);
        $pp->insnet($insn);
        $pp->insform($form);
        $pp->installed(1);
    }
    close TF;
    unlink $tempfile;
}

# This routine runs "xt-cplist" to get the installed printers if the spooler isn't running

sub insptrs_offline {
    my $ptrlist = shift;
    my $pfile = "$main::SPOOLDIR/spshed_pfile";
    my $tempfile = "/tmp/tmppl$$";

    # Can't do anything if we can't find it

    return unless -r $pfile;

    # OK run the command

    my @args;
    push @args, "$main::USERPATH/gspl-cplist";
    push @args, $pfile;
    push @args, $tempfile;

    unless  (system(join(' ', @args)) == 0  &&  open(TF, $tempfile))  {
        unlink $tempfile;
        return;
    }
    while  (<TF>)  {
        chop;
        next unless my ($net, $dev, $descr, $ptr, $form) = /^spadd\s+-\w\s+-(\w)\s+-\w\s+[-A-Pa-p]+\s+-l\s+'(.*?)'\s+-D\s+'(.*?)'\s+(\w+)\s+'(.*)'/;
        my $pp = $ptrlist->getptr($ptr);
        next unless defined $pp;
        $pp->descr($descr);
        my $insn = 0;
        $insn = 1 if $net eq 'N';
        $pp->insdev($dev);
        $pp->insnet($insn);
        $pp->insform($form);
        $pp->installed(1);
    }
    close  TF;
    unlink $tempfile;
    %result;
}

1;
