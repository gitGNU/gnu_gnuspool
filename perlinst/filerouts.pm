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

# File manipulation routines

package filerouts;

# Get full program name from path

sub findonpath {
    my $prog = shift;
    for my $p (split(':', $ENV{'PATH'}), '/usr/local/bin', '/usr/local/sbin', '/sbin', '/usr/sbin') {
        my $poss = "$p/$prog";
        return $poss if -x $poss;
    }
    undef;
}

# Get the first line from the file which matches the specified pattern

sub grepline {
    my $file = shift;
    my $patt = shift;
    return  undef  unless  open(GF, $file);
    while  (<GF>)  {
        chop;
        if (m;$patt;)  {
            close  GF;
            return  $_;
        }
    }
    close  GF;
    undef;
}

# See if program is on the path

sub onpath {
    my $d = shift;
    my %paths;
    map { $paths{$_} = 1; } split(':', $ENV{'PATH'});
    $paths{$d};
}

# Recursive make directory - NB assumes global $Daemuid and $Daemgid

sub rmkdir {
    my $dname = shift;
    my $lev = shift;
    unless  (-d $dname)  {
        my @dbits = split('/', $dname);
        pop @dbits;
        rmkdir(join('/', @dbits), $lev+1);
        unless  (mkdir $dname, 0775)  {
            print "Could not create $dname\n";
            return 0;
        }
    }
    chown $main::Daemuid, $main::Daemgid, $dname if $lev == 0;
    1;
}

# Try to discover where the app-defaults location is

sub findappdefloc {
    my $adl;

    # First see if we can find "xmkmf" and use that with a hand-generated Imakefile
    # to print out the location

    if  (my $ad = findonpath('xmkmf'))  {
        my $tmpdir = "tmp$$";
        mkdir $tmpdir or die "Cannot make temp dir";
        chdir $tmpdir;
        open(IM, ">Imakefile") or die "Cannot use temp dir";
        print IM <<EOT;
findappd:
\t\@echo \$(XAPPLOADDIR)
EOT
        close IM;
        if (system("$ad >/dev/null 2>&1") == 0)  {
            $adl=`make findappd 2>/dev/null`;
            chomp $adl;
            chdir '..';
            system("rm -rf $tmpdir");
            return $adl if length($adl) != 0;
        }
        chdir '..';
        system("rm -rf $tmpdir");
    }

    # OK that didn't work time for brute force

    print "***Looking for app-defaults please be patient....";
    $adl = `find /etc /usr -type d -name app-defaults -print 2>/dev/null|sed -e 1q`;
    print "  Finished search\n";
    chomp $adl;
    if (length($adl) == 0)  {
        print "***Warning could not find app-defaults directory\n";
        return  undef;
    }
    $adl;
}

1;
