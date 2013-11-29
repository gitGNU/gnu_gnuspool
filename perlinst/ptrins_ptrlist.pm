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

# Maintain list of printers

package ptrlist;

sub new {
    my $that = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->_init();
    $self;
}

sub _init {
    my $self = shift;
    $self->{SORTEDLIST} = [];
    $self->{LOOKUP} = {};
}

#  Resort sorted list after changes

sub regenlist {
    my $self = shift;
    my @keys = sort keys %{$self->{LOOKUP}};
    $self->{SORTEDLIST} = \@keys;
}

# Look at prospective name and return 1 if it's OK, 0 if it's a duplicate name, -1 if invalid name

sub okadd {
    my $self = shift;
    my $name = shift;
    return  -1 unless $name =~ /^[a-zA-Z]\w*$/;
    return  0  if defined $self->{LOOKUP}->{lc $name};
    1;
}

# Add printer to list

sub add {
    my $self = shift;
    my $ptr = shift;
    my $name = $ptr->name;
    $self->{LOOKUP}->{lc $name} = $ptr;
    $self->regenlist();
}

# Check OK to delete (not cloned from)

sub okdel {
    my $self = shift;
    my $ptr = shift;
    my $name = $ptr->name;
    for my $p (keys %{$self->{LOOKUP}})  {
        my $np = $self->{LOOKUP}->{$p};
        return  0  if  $np->isclone eq $name;
    }
    1;
}

# Delete printer from list

sub del {
    my $self = shift;
    my $ptr = shift;
    my $name = $ptr->name;
    delete $self->{LOOKUP}->{lc $name};
    $self->regenlist();
}

# Get the printer structure for the name given

sub getptr {
    my $self = shift;
    my $name = shift;
    return $self->{LOOKUP}->{lc $name};
}

# Get names of printers

sub getptrlist {
    my $self = shift;
    $self->{SORTEDLIST};
}

# Get names of cloneable printers (i.e. not clones)

sub getcloneable {
    my $self = shift;
    my @result;
    for my $p (@{$self->{SORTEDLIST}})  {
        my $ptr = $self->{LOOKUP}->{lc $p};
        push @result, $p unless $ptr->isclone;
    }
    \@result;
}

# Get a list of devices (serial/parallel/USB) printers are installed on

sub getdevices {
    my $self = shift;
    my @result;
    for my $p (@{$self->{SORTEDLIST}})  {
        my $ptr = $self->{LOOKUP}->{lc $p};
        next unless $ptr->installed;
        my $pt = $ptr->devtype;
        next if length($pt->{NETDEV}) > 0;
        my $dev = $ptr->insdev;
        push @result, $dev if length($dev) > 0;
    }
    \@result;
}

# Generate vector of printer lists - as 2-element vector of installed and uninstalled.

sub display {
    my $self = shift;
    my @ires;
    my @ures;
    for my $p (@{$self->{SORTEDLIST}})  {
        my $ptr = $self->{LOOKUP}->{lc $p};
        if ($ptr->installed)  {
            push @ires, $ptr->display();
        }
        else {
            push @ures, $ptr->display();
        }
    }
    [ \@ires, \@ures ];
}

1;
