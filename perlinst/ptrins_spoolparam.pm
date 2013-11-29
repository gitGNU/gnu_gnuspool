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

# Miscellaneous spooling parameters

package spoolparam;

sub new {
    my $that = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->_init();
    $self;
}

sub addparam {
    my $self = shift;
    my $param = shift;
    my $name = $param->name();
    $self->{PARAMS}->{$name} = $param;
    push @{$self->{PARAMLIST}}, $name;
}

sub _init {
    my $self = shift;
    $self->addparam(new boolparam("addcr", 0, "Add CR before each newline (text only!)" ));
    $self->addparam(new boolparam("retain", 0, "Retain all jobs on queue after printing" ));
    $self->addparam(new boolparam("norange", 0, "Ignore page ranges" ));
    $self->addparam(new boolparam("inclpage1", 0, "Always print page 1 when printing ranges" ));
    $self->addparam(new intparam("windback", 0, "Wind back pages if job halted"));
    $self->addparam(new boolparam("single", 0, "Single-job mode"));
    $self->addparam(new boolparam("onecopy", 0, "Limit to one copy - handled elsewhere"));
    $self->addparam(new boolparam("nohdr", 0, "Turn off all header pages"));
    $self->addparam(new condboolparam("forcehdr", 0, "Force on header pages always", "!nohdr"));
    $self->addparam(new condboolparam("hdrpercopy", 0, "Put header page in front of every copy", "!nohdr"));;
}

sub resetbools {
    my $self = shift;
    my $plist = $self->{PARAMS};
    for my $p (@{$self->{PARAMLIST}})  {
        $plist->{$p}->resetbools();
    }
}

sub ask {
    my $self = shift;
    my $chenv = shift;
    my $plist = $self->{PARAMS};
    for my $p (@{$self->{PARAMLIST}})  {
        $plist->{$p}->ask($chenv, $plist);
    }
}

sub output {
    my $self = shift;
    my $fh = shift;
    for my $p (@{$self->{PARAMLIST}})  {
        $self->{PARAMS}->{$p}->output($fh);
    }
}

sub display {
    my $self = shift;
    my $res = [];
    for my $p (@{$self->{PARAMLIST}})  {
        push @$res, $self->{PARAMS}->{$p}->display();
    }
    $res;
}

1;
