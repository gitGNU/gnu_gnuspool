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

# Routines for adding and removing printers from CUPSPY and Libreoffice
# We do Libreoffice directly rather than via CUPS as that is not reliable.

package ptrins_cupspy;
use strict;

# Install printer to Libreoffice
# Arg1: ptrinter name for Libreoffice
# Arg2: GNUspool queue
# Arg3: form type
# Arg4: Y if no colour N if colour (OK wrong way....)
# Arg5: Paper size name

sub lo_install_op ($$$$$) {
    my $loptr_name = shift;
    my $ptrname = shift;
    my $formtype = shift;
    my $nocol = shift;
    my $paper = shift;
    my @args;
    push @args, "$main::USERPATH/gspl-libreoffice-ins", "--quiet", "--add";
    push @args, "--ptr='$loptr_name'";
    push @args, "--size=$paper";
    push @args, "--form=$formtype";
    push @args, "--queue=$ptrname";
    push @args, "--nocolour=$nocol";
    print "Sorry, could not add to Libreoffice printers\n" if system(join(' ', @args)) != 0;
}

# Uninstall printer - use printer name using GNUspool printer, deletes all that use it.

sub lo_uninstall ($) {
    my $ptrname = shift;
    my @args;
    push @args, "$main::USERPATH/gspl-libreoffice-ins", "--quiet", "--delete";
    push @args, "--queue=$ptrname";
    print "Sorry, could not delete from Libreoffice printers\n" if system(join(' ', @args)) != 0;
}

# Get list of CUPSPY printers

sub get_cupspy_list {
    my %explist;
    return unless open(CP, "/etc/cups/cupspy/listptrs.py -t|");
    while (<CP>) {
        chop;
        my @l = split(/\s+/);
        $explist{$l[0]} = 1;
    }
    close CP;
    \%explist;
}

# Actual operation to install a printer to CUPSPY

sub cupspy_install_op ($$$$) {
    my $cupspy_name = shift;
    my $ptrname = shift;
    my $formtype = shift;
    my $descr = shift;
    my @args;
    push @args, "/etc/cups/cupspy/initconf.py";
    push @args, "-c", "cupspy.conf";
    push @args, "-p", $cupspy_name;
    push @args, "-q", $ptrname;
    push @args, "-f", $formtype;
    push @args, "-i", "'" . $descr . "'";
    print "***Warning: Cupspy install error" unless  system(join(' ', @args)) == 0;
}

# Operation to de-install a printer fron CUPSPY
# This deletes every virtual printer which uses it in one hit

sub cupspy_deinstall_op ($) {
    my $ptrname = shift;
    my @args;
    push @args, "/etc/cups/cupspy/initconf.py";
    push @args, "-c", "cupspy.conf";
    push @args, "-D";
    push @args, "-q", $ptrname;
    print "***Warning: Cupspy de-install error" unless  system(join(' ', @args)) == 0;
}

# Install a printer to CUPSPY
# We interpret the form suffix and description list for the emulation type

sub cupspy_ins ($$) {
    my $Env = shift;
    my $ptr = shift;
    my $ptrname = $ptr->name;
    my $descr = $ptr->descr;
    $descr = $ptr->printer_type unless length($descr) > 0;
    $descr =~ y/-.,a-zA-Z0-9 //cd;
    my $form = $ptr->insform;
    $form = $Env->stdform unless length($form) > 0;
    $form =~ s/(.*?)[-.].*/$1/;
    my $flist = $ptr->ptremul->cupspytypes;
    my $existing = get_cupspy_list();
    for my $f (@$flist)  {
        my ($suff, $d) = @$f;
        my $fulld = $descr;
        $fulld .= ' ' . $d if length($d) > 0;
        my $fullf = $form;
        $fullf .= '.' . $suff if length($suff) > 0;
        my $name = $ptr->name;
        $name .= '.' . $suff if length($suff) > 0;
        $name = lc $name;
        $name = ucfirst $name;
        $name =~ y/- .,/____/;
        my $cname = $name;
        if (defined $existing->{$cname}) {
            my $n = 1;
            $n++ while defined $existing->{$cname . "_$n"};
            $cname .+ "_$n";
        }
        cupspy_install_op($cname, $ptrname, $fullf, $fulld);
        $existing->{$cname} = 1;
    }
}

# De-install a CUPSPY printer set

sub cupspy_deins ($$)  {
    my $Env = shift;
    my $ptr = shift;
    my $ptrname = $ptr->name;
    cupspy_deinstall_op($ptrname);      # This does everything in one hit
}

# Install a printer to Libreoffice config file
# We interpret the form suffix and description list for the emulation type

sub libreoffice_ins ($$) {
    my $Env = shift;
    my $ptr = shift;
    my $ptrname = $ptr->name;
    my $descr = $ptr->descr;
    $descr = $ptr->printer_type unless length($descr) > 0;
    $descr =~ y/-.,a-zA-Z0-9 //cd;
    my $form = $ptr->insform;
    $form = $Env->stdform unless length($form) > 0;
    $form =~ s/(.*?)[-.].*/$1/;
    my $pemul = $ptr->ptremul;
    my $papsize = $Env->stdform;
    $papsize =~ s/(.*?)[-.].*/$1/;
    $papsize = $pemul->{PARAMS}->{papersize}->{VALUE} if defined $pemul->{PARAMS}->{papersize};
    my $colour = 1;
    my $colourdef = 1;
    $colour = $pemul->{PARAMS}->{colour}->{VALUE} if defined $pemul->{PARAMS}->{colour};
    $colourdef = $pemul->{PARAMS}->{colourdef}->{VALUE} if defined $pemul->{PARAMS}->{colourdef};
    $colourdef &&= $colour;

    my $flist = $ptr->ptremul->cupspytypes;
    for my $f (@$flist)  {
        my ($suff, $d) = @$f;
        my $fulld = ucfirst lc $ptrname;
        $fulld .= ' - ' . $descr;
        $fulld .= ' ' . $d if length($d) > 0;
        my $fullf = $form;
        $fullf .= '.' . $suff if length($suff) > 0;
        $fulld =~ y/- .,/____/;
        my $nc = $colourdef? "n": "y";
        $nc = "y" if $suff =~ /^m/;
        lo_install_op($fulld, $ptrname, $fullf, $nc, $papsize);
    }
}

# De-install a CUPSPY printer set

sub libreoffice_deins ($$)  {
    my $Env = shift;
    my $ptr = shift;
    my $ptrname = $ptr->name;
    lo_uninstall($ptrname);      # This does everything in one hit
}

# Install printer to CUPSPY and Libreoffice Config if applicable
# Use the service routines

sub cupspy_install ($$) {
    my $Env = shift;
    my $ptr = shift;
    cupspy_ins($Env, $ptr) if $Env->{CUPSPY};
    libreoffice_ins($Env, $ptr) if $Env->{LIBREOFF};
}

# De-Install printer from CUPSPY and Libreoffice Config if applicable
# Use the service routines

sub cupspy_deinstall ($$) {
    my $Env = shift;
    my $ptr = shift;
    cupspy_deins($Env, $ptr) if $Env->{CUPSPY};
    libreoffice_deins($Env, $ptr) if $Env->{LIBREOFF};
}

1;
