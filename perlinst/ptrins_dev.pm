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

# Printer basic device parameters

package device;
use strict;

sub new {
    my $that = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->_init();
    $self;
}

sub typedescr {
    my $self = shift;
    "Custom";
}

sub addparam {
    my $self = shift;
    my $param = shift;
    my $name = $param->name();
    $self->{PARAMS}->{$name} = $param;
    my $l = $param->optlet();
    $self->{OPT}->{$l} = $param if $l;
    push @{$self->{PARAMLIST}}, $name;
}

sub _init {
    my $self = shift;
    $self->addparam(new intparam("open", 30, "Timeout on open before giving up" ));
    $self->addparam(new intparam("offline", 300, "Timeout on write before regarding device as offline" ));
    $self->addparam(new boolparam("canhang", 0, "Running processes attached to device hard to kill" ));
    $self->addparam(new intparam("outbuffer", 1024, "Size of output buffer in bytes" ));
    $self->addparam(new boolparam("reopen", 0, "Close and reopen device after each job"));
    $self->{NETDEV} = "";
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

sub netcmd {
    my $self = shift;
    undef;
}

package parallel;

our @ISA = qw(device);

sub typedescr {
    my $self = shift;
    "Parallel";
}

package usb;

our @ISA = qw(device);

sub typedescr {
    my $self = shift;
    "USB";
}

package serial;

our @ISA = qw(device);

sub _init {
    my $self = shift;
    $self->addparam(new optparam("baud", 9600, "Baud rate of serial device", [ 9600, 1200, 1800, 2400, 4800, 9600, 19200, 38400 ]));
    $self->addparam(new boolparam("ixon", 1, "Set xon/xoff flow control"));
    $self->addparam(new boolparam("ixany", 0, "Set xon/xoff with any character release")),
    $self->addparam(new optparam("csize", 8, "Character size", [ 8, 5, 6, 7]));
    $self->addparam(new optparam("stopbits", 1, "Stop bits", [1, 2]));
    $self->addparam(new boolparam("parenb", 0, "Parity enabled"));
    $self->addparam(new boolparam("parodd", 0, "Odd parity"));
    $self->addparam(new boolparam("clocal", 0, "No modem control"));
    $self->addparam(new boolparam("onlcr", 0, "Add CR before LF in driver"));
    $self->SUPER::_init();
}

sub typedescr {
    my $self = shift;
    "Serial";
}

package network;

our @ISA = qw(device);

sub _init {
    my $self = shift;
    $self->SUPER::_init();
    $self->addparam(new intparam("close", 10000, "Time to wait for close to complete"));
    $self->addparam(new intparam("postclose", 1, "Pause time after close"));
    $self->addparam(new boolparam("logerror", 1, "Log error messages to system log"));
    $self->addparam(new boolparam("fberror", 1, "Provide error indications on screen"));
    $self->{PARAMS}->{reopen}->{VALUE} = 1;
    $self->{NETDEV} = "Unknown netcmd";
}

sub netcmd {
    my $self = shift;
    my $result = [ "$main::SPROGDIR/" . $self->{NETDEV} ];
    for my $p (@{$self->{PARAMLIST}})  {
        my $next = $self->{PARAMS}->{$p}->cmdargs();
        push @$result, @$next;
    }
    $result;
}

package lpdnet;

our @ISA = qw(network);

sub _init {
    my $self = shift;
    $self->SUPER::_init();
    $self->{NETDEV} = "xtlpc";
    $self->addparam(new hostparam('H'));
    $self->addparam(new ctrlfile("ctrlfile", "xtlpc-ctrl", "Control file", "f"));
    $self->addparam(new optsender("outip", "Outgoing host name", "S"));
    $self->addparam(new optstringarg("lpdname", "", "Printer name for protocol", 'P'));
    $self->addparam(new boolparam("nonull", 1, "Do not send null jobs", "N"));
    $self->addparam(new boolparam("resp", 1, "Do not use reserved port", "U"));
    $self->addparam(new optintparam("loops", 3, "Attempts to connect", "l"));
    $self->addparam(new optintparam("loopwait", 1, "Seconds to wait between connect attempts", "L"));
    $self->addparam(new optintparam("itimeout", 5, "Input timeout (for response packets)", "I"));
    $self->addparam(new optintparam("otimeout", 5, "Output timeout (response receiving data)", "O"));
    $self->addparam(new optintparam("retries", 0, "Number of retries after timeouts", 'R'));
    $self->addparam(new optfloatparam("linger", 0.0, "Linger time (may be fractional)", 's'));
}

sub typedescr {
    my $self = shift;
    "LPDnet";
}

package telnet;

our @ISA = qw(network);

sub _init {
    my $self = shift;
    $self->{NETDEV} = "xtelnet";
    $self->SUPER::_init();
    $self->addparam(new hostparam('h'));
    $self->addparam(new portname("port", 9100, "Output port", 'p'));
    $self->addparam(new optintparam("loops", 3, "Attempts to connect", "l"));
    $self->addparam(new optintparam("loopwait", 1, "Seconds to wait between connect attempts", "L"));
    $self->addparam(new optintparam("endsleep", 0, "Time for process to sleep at end of each job", 't'));
    $self->addparam(new optfloatparam("linger", 0.0, "Linger time (may be fractional)", 's'));
}

package ftp;

our @ISA = qw(network);

sub _init {
    my $self = shift;
    $self->SUPER::_init();
    $self->{NETDEV} = "xtftp";
    $self->addparam(new hostparam('h'));
    $self->addparam(new optsender("myhost", "IP to send from", "A"));
    $self->addparam(new portname("port", "ftp", "Output port", 'p'));
    $self->addparam(new optstringarg("username", "",  "User Name", 'u'));
    $self->addparam(new optstringarg("password", "", "Password", 'w'));
    $self->addparam(new optstringarg("directory", "", "Directory name on server", 'D'));
    $self->addparam(new optstringarg("outfile", "", "Output file on server", 'o'));
    $self->addparam(new boolparam("textmode", 0, "Force text mode", "t"));
    $self->addparam(new optintparam("timeout", 750, "Timeout for select (ms)", "T"));
    $self->addparam(new optintparam("maintimeout", 30000, "Timeout for FTP (ms)", "R"));
}

sub typedescr {
    my $self = shift;
    "FTP";
}

package xtlhp;

our @ISA = qw(network);

sub _init {
    my $self = shift;
    $self->SUPER::_init();
    $self->{NETDEV} = "xtlhp";
    $self->addparam(new hostparam('h'));
    $self->addparam(new ctrlfile("configfile", "xtsnmpdef", "Configuration macro file", "f"));
    $self->addparam(new ctrlfile("ctrlfile", "xtlhp-ctrl", "Control file", "c"));
    $self->addparam(new portname("port", 9100, "Output port", 'p'));
    $self->addparam(new optsender("myhost", "Outgoing host or IP", "H"));
    $self->addparam(new optstringarg("commun", "public", "SNMP Community", 'C'));
    $self->addparam(new optfloatparam("timeout", 1.0, "UDP Timeout (may be fractional)", 't'));
    $self->addparam(new portname("snmpport", "snmp", "SNMP Port", 'S'));
    $self->addparam(new optintparam("blksize", 10240, "Block size", 'b'));
    $self->addparam(new boolparam("next", 0, "Get next on SNMP var fetches", 'N'));
}

sub typedescr {
    my $self = shift;
    "XTLHP";
}

1;
