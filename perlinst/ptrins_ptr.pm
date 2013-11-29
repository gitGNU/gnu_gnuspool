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

# Details of printer

package printer;

sub new {
    my $that = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->{INSTALLED} = 0;
    $self->{INSNET} = 0;
    $self->{CUSTOM} = 0;
    $self;
}

# This is the name of the printer in the spool queue

sub name {
    my $self = shift;
    my $n = shift;
    $self->{NAME} = $n if $n;
    $self->{NAME};
}

# Note printer is clone of another printer (name given)

sub cloneof {
    my $self = shift;
    my $cloneof = shift;
    $self->{CLONEOF} = $cloneof;
}

# Check if printer is clone

sub isclone {
    my $self = shift;
    $self->{CLONEOF};
}

# This is the user's description of the printer

sub descr {
    my $self = shift;
    my $d = shift;
    $self->{DESCR} = $d if $d;
    $self->{DESCR};
}

# This is the type of the printer as reported back (if possible)

sub printer_type {
    my $self = shift;
    my $pt = shift;
    $self->{PTRTYPE} = $pt if $pt;
    $self->{PTRTYPE};
}

# This is the device type - serial / USB / Parallel etc

sub devtype {
    my $self = shift;
    my $d = shift;
    $self->{DEVTYPE} = $d if $d;
    $self->{DEVTYPE};
}

# This is the printer emulation

sub ptremul {
    my $self = shift;
    my $p = shift;
    $self->{PTREMUL} = $p if $p;
    $self->{PTREMUL};
}

# This is the installed device name

sub insdev {
    my $self = shift;
    my $id = shift;
    $self->{INSDEV} = $id if $id;
    $self->{INSDEV};
}

sub insform {
    my $self = shift;
    my $ifm = shift;
    $self->{INSFORM} = $ifm if $ifm;
    $self->{INSFORM};
}

# Is printer installed

sub installed {
    my $self = shift;
    my $ins = shift;
    $self->{INSTALLED} = $ins if defined($ins);
    $self->{INSTALLED};
}

# Is it installed as a network device

sub insnet {
    my $self = shift;
    my $ins = shift;
    $self->{INSNET} = $ins if defined($ins);
    $self->{INSNET};
}

# Custom interface (not custom emulation type)

sub custom {
    my $self = shift;
    my $cust = shift;
    $self->{CUSTOM} = $cust if defined($cust);
    $self->{CUSTOM};
}

sub display {
    my $self = shift;
    my $descr = $self->descr;
    $descr = $self->printer_type if length($descr) == 0;
    $ins = $self->installed? $self->insdev: "Not inst";
    my $cl = $self->isclone;
    if ($cl)  {
        $cl = "(Clone of $cl)";
    }
    else  {
        $cl = "n/a";
    }
    [ $self->name, $descr, $ins, $cl ];
}

1;
