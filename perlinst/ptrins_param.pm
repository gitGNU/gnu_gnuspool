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

# Package for setting up parameter types for printer features

############################################################################################################
#
#  Basic parameters
#
############################################################################################################

package param;

sub new {
    my $that = shift;
    my $name = shift;
    my $deflt = shift;
    my $descr = shift;
    my $optionlet = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->_init($name, $deflt, $descr, $optionlet);
    $self;
}

sub _init {
    my $self = shift;
    my $name = shift;
    my $deflt = shift;
    my $descr = shift;
    my $optionlet = shift;
    $self->{NAME} = $name;
    $self->{DESCR} = $descr;
    $self->{DEFAULT} = $self->{VALUE} = $deflt;
    $self->{OPTIONLET} = $optionlet if $optionlet;
}

sub resetbools {
    my $self = shift;
}

sub name {
    my $self = shift;
    $self->{NAME};
}

sub optlet {
    my $self = shift;
    $self->{OPTIONLET};
}

sub ask {
    my $self = shift;
    my $chenv = shift;
    die "Ask in virtual proc";
}

# Parse takes a list of parameters including the thing that invoked it
# (to check for '-' on bool options) and pops off the args eaten.
# Return 0 in case of error

sub parse {
    my $self = shift;
    my $arglist = shift;
    die "Parse in virtual proc";
}

sub output {
    my $self = shift;
    my $fh = shift;
    return if defined $self->{OPTIONLET};

    print $fh <<EOT;
# $self->{DESCR}
$self->{NAME} $self->{VALUE}

EOT
}

# For tabulation purposes, return a 3-column array ref as name, value, description

sub display {
    my $self = shift;
    [ $self->{NAME}, $self->{VALUE}, $self->{DESCR} ];
}

sub cmdargs {
    my $self = shift;
    my $result = [];
    push (@$result, "-" . $self->{OPTIONLET}, $self->{VALUE}) if defined $self->{OPTIONLET};
    $result;
}

############################################################################################################
#
#  Integer parameters
#
############################################################################################################

package intparam;

our @ISA = qw(param);

sub ask {
    my $self = shift;
    my $chenv = shift;
    $self->{VALUE} = $chenv->asknum($self->{DESCR}, $self->{VALUE}, $self->name . "_pp");
}

sub parse {
    my $self = shift;
    my $arglist = shift;
    shift @$arglist;          # Eat whatever invoked it
    my $arg = shift @$arglist;
    return 0 if $arg !~ /^\d+$/;
    $self->{VALUE} = $arg + 0;
    $arglist;
}

# Version for where we don't put anything if it's the default
# Otherwise same as above.

package optintparam;

our @ISA = qw(intparam);

sub cmdargs {
    my $self = shift;
    return [] if $self->{VALUE} == $self->{DEFAULT};
    $self->SUPER::cmdargs();
}

############################################################################################################
#
#  Float parameters (always do nothing if default)
#
############################################################################################################

package optfloatparam;

our @ISA = qw(param);

sub ask {
    my $self = shift;
    my $chenv = shift;
    for  (;;)  {
        my $newval = $chenv->askstring($self->{DESCR}, $self->{VALUE}, $self->name . "_pp");
        return unless defined $newval;
        if  ($newval =~ /^\d+(\.\d+)?$/)  {
            $self->{VALUE} = $newval + 0.0;
            return;
        }
        print "Sorry please enter numeric (possibly fractional) value\n";
    }
}

sub parse {
    my $self = shift;
    my $arglist = shift;
    shift @$arglist;          # Eat whatever invoked it
    my $arg = shift @$arglist;
    return 0 if $arg !~ /^\d+(\.\d+)?$/;
    $self->{VALUE} = $arg + 0.0;
    $arglist;
}

sub cmdargs {
    my $self = shift;
    return [] if $self->{VALUE} == $self->{DEFAULT};
    $self->SUPER::cmdargs();
}

############################################################################################################
#
#  Boolean parameters
#
############################################################################################################

package boolparam;

our @ISA = qw(param);

sub resetbools {
    my $self = shift;
    $self->{VALUE} = 0;
}

sub ask {
    my $self = shift;
    my $chenv = shift;
    $self->{VALUE} = $chenv->askyorn($self->{DESCR}, $self->{VALUE}, $self->name . "_pp");
}

# Parse we have to be cute about interpreting the argument
# -x means set it. -name means unset it name means set it

sub parse {
    my $self = shift;
    my $arglist = shift;
    my $arg = shift @$arglist;
    $self->{VALUE} = (substr $arg, 0, 1) ne '-' or length($arg) == 2? 1: 0;
    $arglist;
}

sub output {
    my $self = shift;
    my $fh = shift;
    return if defined $self->{OPTIONLET};

    if ($self->{VALUE})  {
        print $fh <<EOT;
# $self->{DESCR}
$self->{NAME}

EOT
    }
     else  {
         print $fh <<EOT;
# $self->{DESCR}
#$self->{NAME}

EOT
    }
}

sub display {
    my $self = shift;
    [ $self->{NAME}, $self->{VALUE}? 'Yes': 'No', $self->{DESCR} ];
}


sub cmdargs {
    my $self = shift;
    my $result = [];
    push @$result, ("-" . $self->{OPTIONLET}) if defined $self->{OPTIONLET} && $self->{VALUE};
    $result;
}

############################################################################################################
#
#  Conditional Boolean parameters
#  Similar to Boolean parameters but we don't ask the question if a given otherparameter is set or unset
#  The name of the parameter is preceded by ! to say ask the question if the other question is not set
#  not if it's sets and vice versa without the !
#
############################################################################################################

package condboolparam;

our @ISA = qw(boolparam);

sub new {
    my $that = shift;
    my $name = shift;
    my $deflt = shift;
    my $descr = shift;
    my $cparm = shift;
    my $ol = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->_init($name, $deflt, $descr, $ol);
    $self->{NOTSET} = 0;
    if  ((substr $cparm, 0, 1) eq '!')  {
        $self->{NOTSET} = 1;
        $cparm = substr $cparm, 1;
    }
    $self->{CPARAM} = $cparm;
    $self;
}

sub ask {
    my $self = shift;
    my $chenv = shift;
    my $plist = shift;
    if  ($plist->{$self->{CPARAM}}->{VALUE})  {
        if ($self->{NOTSET})  {
            $self->{VALUE} = 0;
            return;
        }
    }
    else  {
        unless ($self->{NOTSET})  {
            $self->{VALUE} = 0;
            return;
        }
    }
    $self->SUPER::ask($chenv);
}

############################################################################################################
#
#  Optional string args
#
############################################################################################################

package optstringarg;

our @ISA = qw(param);

sub new {
    my $that = shift;
    my $name = shift;
    my $deflt = shift;
    my $descr = shift;
    my $optionlet = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->_init($name, $deflt, $descr, $optionlet);
    $self;
}

sub ask {
    my $self = shift;
    my $chenv = shift;
    my $newval = $chenv->askstring($self->{DESCR}, $self->{VALUE}, $self->name . "_pp");
    $self->{VALUE} = $newval if defined $newval;
}

sub parse {
    my $self = shift;
    my $arglist = shift;
    shift @$arglist;      # Skip over -x or name
    my $arg = shift @$arglist;
    return  0  unless  defined($arg);
    $self->{VALUE} = $arg;
    $arglist;
}

sub cmdargs {
    my $self = shift;
    return [] if $self->{VALUE} eq $self->{DEFAULT};
    $self->SUPER::cmdargs();
}

############################################################################################################
#
#  Selection rom a list
#
############################################################################################################

package optparam;

our @ISA = qw(param);

sub new {
    my $that = shift;
    my $name = shift;
    my $deflt = shift;
    my $descr = shift;
    my $options = shift;
    my $ol = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->_init($name, $deflt, $descr, $options, $ol);
    $self;
}

sub _init {
    my $self = shift;
    my $name = shift;
    my $deflt = shift;
    my $descr = shift;
    my $options = shift;
    my $ol = shift;
    $self->SUPER::_init($name, $deflt, $descr, $ol);
    $self->{OPTIONS} = $options;
}

sub ask {
    my $self = shift;
    my $chenv = shift;
    my $newval = $chenv->askopt($self->{DESCR}, $self->{OPTIONS}, $self->name . "_pp", $self->{VALUE});
    $self->{VALUE} = $newval if defined $newval;
}

sub parse {
    my $self = shift;
    my $arglist = shift;
    shift @$arglist;      # Skip over -x or name
    my $arg = shift @$arglist;
    return  0  unless  defined($arg);
    my %possargs;       # Validate it
    map { $possargs{$_} = 1; } @{$self->{OPTIONS}};
    return  0  unless $possargs{$arg};
    $self->{VALUE} = $arg;
    $arglist;
}


############################################################################################################
#
#  Host to send to
#
############################################################################################################

package hostparam;

our @ISA = qw(param);

sub new {
    my $that = shift;
    my $optionlet = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->SUPER::_init("host", '$SPOOLDEV', 'Host name or IP address of the printer', $optionlet);
    $self;
}

sub ask {
    my $self = shift;
    my $chenv = shift;
    for  (;;)  {
        my $newvalue = $chenv->askstring($self->{DESCR}, $self->{VALUE}, $self->name . "_pp");
        return unless defined $newvalue;
        if  ($newvalue =~ /^\d+\.\d+\.\d+\.\d+$/)  {
            unless  (inet_aton($newvalue))   {
                print "Invalid IP address $newvalue\n";
                redo;
            }
        }
        $self->{VALUE} = $newvalue;
        return;
    }
}

sub parse {
    my $self = shift;
    my $arglist = shift;
    shift @$arglist;      # Skip over -x or name
    my $arg = shift @$arglist;
    return  0  unless  defined($arg);
    $self->{VALUE} = $arg;
    $arglist;
}

############################################################################################################
#
#  Outgoing host
#
############################################################################################################

package optsender;

our @ISA = qw(param);

sub new {
    my $that = shift;
    my $name = shift;
    my $descr = shift;
    my $optionlet = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->SUPER::_init($name, "", $descr, $optionlet);
    $self;
}

sub ask {
    my $self = shift;
    my $chenv = shift;
    for  (;;)  {
        my $newvalue = $chenv->askstring($self->{DESCR}, $self->{VALUE}, $self->name . "_pp");
        return unless defined $newvalue;
        if  ($newvalue =~ /^\d+\.\d+\.\d+\.\d+$/)  {
            unless  (inet_aton($newvalue))   {
                print "Invalid IP address $newvalue\n";
                redo;
            }
        }
        $self->{VALUE} = $newvalue;
        return;
    }
}

sub parse {
    my $self = shift;
    my $arglist = shift;
    shift @$arglist;      # Skip over -x or name
    my $arg = shift @$arglist;
    return  0  unless  defined($arg);
    $self->{VALUE} = $arg;
    $arglist;
}

sub display {
    my $self = shift;
    [ $self->{NAME}, $self->{VALUE}? $self->{VALUE}: "(not def)", $self->{DESCR}];
}

sub cmdargs {
    my $self = shift;
    return [] if length($self->{VALUE}) == 0;
    $self->SUPER::cmdargs();
}

############################################################################################################
#
#  Port name or number
#
############################################################################################################

package portname;

our @ISA = qw(param);

sub new {
    my $that = shift;
    my $name = shift;
    my $deflt = shift;
    my $descr = shift;
    my $optionlet = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->SUPER::_init($name, $deflt, $descr, $optionlet);
    $self;
}

sub ask {
    my $self = shift;
    my $chenv = shift;
    for  (;;)  {
        my $v = $chenv->askstring($self->{DESCR}, $self->{VALUE}, $self->name() . "_pp");
        return unless defined $v;
        if  ($v =~ /^\d+$/)  {
            if  ($v <= 0  ||  $v >= 65536)  {
                print "Sorry invalid port number $v\n";
                redo;
            }
        }
        elsif  (!getservbyname($v, "tcp"))  {
            print "Sorry, invalid sevice $v\n";
            redo;
        }
        $self->{VALUE} = $v;
        return;
    }
}

sub parse {
    my $self = shift;
    my $arglist = shift;
    shift @$arglist;      # Skip over -x or name
    my $arg = shift @$arglist;
    return  0  unless  defined($arg);
    $self->{VALUE} = $arg;
    $arglist;
}

# Value might be name or number, make sure to use the right compare.

sub cmdargs {
    my $self = shift;
    my $deflt = $self->{DEFAULT};
    my $value = $self->{VALUE};

    if  ($deflt =~ /^\d+$/)  {
        return $self->SUPER::cmdargs() if $value !~ /^\d+$/ or $deflt != $value;
        return [];
    }
    return [] if $deflt eq $value;
    $self->SUPER::cmdargs();
}

############################################################################################################
#
#  Name of control file
#
############################################################################################################

package ctrlfile;

our @ISA = qw(param);

sub new {
    my $that = shift;
    my $name = shift;
    my $deflt = shift;
    my $descr = shift;
    my $optionlet = shift;
    my $class = ref($that) || $that;
    my $self = {};
    bless $self, $class;
    $self->SUPER::_init($name, $deflt, $descr, $optionlet);
    $self;
}

sub fullpath {
    my $value = shift;
    return $value if (substr $value, 0, 1) eq '/';
    $main::SDATADIR . '/' . $value;
}

sub ask {
    my $self = shift;
    my $chenv = shift;
    for  (;;)  {
        my $newvalue = $chenv->askstring($self->{DESCR}, $self->{VALUE}, $self->name . "_pp");
        return unless defined $newvalue;
        unless  (length($newvalue) > 0)  {
            print "Please give a value\n";
            redo;
        }
        if  (-f fullpath($newvalue) or $chenv->askyorn("$newvalue does not exist - are you sure", 0, "ctrlne_pp"))  {
            $self->{VALUE} = $newvalue;
            return;
        }
    }
}

# Maybe we want to check it?

sub parse {
    my $self = shift;
    my $arglist = shift;
    shift @$arglist;      # Skip over -x or name
    my $arg = shift @$arglist;
    return  0  unless  defined($arg);
    $self->{VALUE} = $arg;
    $arglist;
}

sub cmdargs {
    my $self = shift;
    my $val = $self->{VALUE};
    $val = $main::SDATADIR . '/'. $val unless (substr $val, 0, 1) eq '/';
    [ '-' . $self->{OPTIONLET} , $val ];
}

1;
