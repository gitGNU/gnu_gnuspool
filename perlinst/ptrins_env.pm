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

# Get system parameters and setup

package ptrins_env;

our @ISA = qw(charops);

sub isrunning {
    my $self = shift;
    my $ir = shift;
    $self->{ISRUNNING} = $ir if defined($ir);
    $self->{ISRUNNING};
}

sub printerlist {
    my $self = shift;
    my $pl = shift;
    $self->{PRINTERLIST} = $pl if defined($pl);
    $self->{PRINTERLIST};
}

# When we start up discover what our standard form type and offer to
# replace it if it's "standard" in other users' profiles

sub stdform {
    my $self = shift;
    return $self->{STDFORM} if defined $Self->{STDFORM};
    # Get root's standard form type
    my $ft=`$main::USERPATH/spulist -s -F %f root 2>/dev/null`;
    if (length($ft) == 0)  {
        $ft = "a4";
    }
    else {
        chop $ft;
    }
    return $self->{STDFORM} = $ft if  $ft ne 'standard';

    outformat::underline(\*STDOUT, "Standard form type");
    print <<EOT;
We think this is probably the first time you have run this.

When GNUspool is installed the users are set up with a "standard" as
the default form type.

You probably don't want this, but probably some standard name for the
size, such as "a4" or "letter", or some other name like "plain" of
"80gsm".

You probably want to set all other users to use this by default.
EOT
    return $self->{STDFORM} = $ft unless $self->askyorn("Do you want to change this", 1, "Change default form from standard") or
                              !$self->askyorn("Are you sure", 0, "Are you sure you do not want to change the form type");
    my $newft = editptr::askform($self, "New default form", "a4", "userdefform");
    return $self->{STDFORM} = $ft unless defined $newft;
    # OK now reset users to new form type
    system("$main::USERPATH/spuchange -A -D -f '$newft' -F '$newft'");
    $self->{STDFORM} = $newft;
}

sub parseargs {
    my $that = shift;
    my $sysu = shift;
    my $argv = shift;
    my $Systemtype = `uname -s`;
    chop $Systemtype;
    $that->{SYSTYPE} = $Systemtype;
    my @spubits = getpwnam($sysu);
    die "Cannot find system user id $sysu" unless @spubits;
    $that->{UID} = $spubits[2];
    $that->{GID} = $spubits[3];
    my $Havecupspy = 'U';              # Don't know
    if ($#$argv >= 0) {
        my $arg = shift @$argv;
        if ($arg eq '--cupspy')  {
            $Havecupspy = 1;
            $arg = shift @$argv;
        }
        elsif ($arg eq '--no-cupspy')  {
            $Havecupspy = 0;
            $arg = shift @$argv;
        }
    }

    # Try to work out if we are using CUPSPY

    $Havecupspy = -d '/etc/cups/cupspy'? 1: 0 if $Havecupspy eq 'U';

    $that->{CUPSPY} = $Havecupspy;
    $that->{LIBREOFF} = -d "/etc/libreoffice"? 1: 0;

    # Get standard form

    $that->stdform;
}

sub islinux {
    my $self = shift;
    $self->{SYSTYPE} eq 'Linux';
}

1;
