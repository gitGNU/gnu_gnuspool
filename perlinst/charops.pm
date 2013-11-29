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

# Modules for interacting with the user to replace getchar etc

package charops;

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
    my %Schar;

    # Extract terminal control characters from stty output into %Schar

    open(ST, "stty -a|");
    while (<ST>) {
        chop;
        s/\b(intr|quit|erase|kill)\s*=\s*([^;]*);/$Schar{lc $1}=$2/ieg;
    }
    close ST;

    for my $s (keys %Schar)  {

        # Special case ^? for delete

        if ($Schar{$s} eq '^?') {
            $Schar{$s} = 127;
        }
        elsif ($Schar{$s} =~ /^\^(.)/)  {
            $Schar{$s} = ord($1) & 31;
        }
        else  {
            $Schar{$s} = ord($Schar{$s});
        }
    }
    $self->{ERASE} = $Schar{erase};
    $self->{KILL} = $Schar{kill};

    my $Resetty = `stty -g`;
    chop $Resetty;
    $self->{RESETTTY} = $Resetty;
    $self->{F1} = `tput kf1`;
    $self->{F2} = `tput kf2`;
    $self->{F8} = `tput kf8`;
    $self->{CLEAR} = `tput clear`;
    $self->{TAB} = ord("\t");
    $self->{SPACE} = ord(' ');
    $self->{CTRLW} = ord('w') & 0x1f;
    $self->{ESC} = ord("\e");
    $self->{HAVEF1} = 0;
}

sub clear {
    my $self = shift;
    if (length($self->{CLEAR}) > 0)  {
        print $self->{CLEAR};
    }
    else  {
        print "\n" x 20;
    }
}

# Get a character, interpret F2 and F8 here

sub getchar {
    my $self = shift;
    system("stty raw -echo time 3");
    my $res;
    my $cnt;
    do  {
        $cnt = sysread STDIN, $res, 10;
    }  while $cnt == 0;
    system("stty $self->{RESETTTY}");
    if ($res eq $self->{F8})  {
        print "\n\nQuitting.....\n\n";
        exit 200;
    }
    $res = "\n" if $res eq "\r" or $res eq $self->{F2};
    $res;
}

# See if help key pressed and do appropriate stuff

sub checkhelp {
    my $self = shift;
    my $ans = shift;
    my $helpfile = shift;
    return 0 unless $ans eq $self->{F1} or ($self->{HAVEF1} == 0 && $ans eq '?');
    $self->{HAVEF1}++ if $ans ne '?';
    disphelp::displayhelp($helpfile, $self);
    1;
}

# Pause after getting screenful of stuff for someone to type something
# Also display help if required

sub pausehelp {
    my $self = shift;
    my $help = shift;

    for (;;)  {
        print "[Please press ENTER to continue]";
        my $ans = $self->getchar;
        print "\n";
        return unless $ans eq $self->{F1} or ($self->{HAVEF1} == 0 && $ans eq '?');
        $self->{HAVEF1}++ if $ans ne '?';
        disphelp::displayhelp($help, $self);
    }
}

# Ask question $quest expecting Y or N answer and display help if required.
# Arg 1 is question
# Arg 2 is default answer 1=yes 0=no
# Arg 3 is helpfile

sub askyorn {
    my $self = shift;
    my $quest = shift;
    my $defans = shift;
    my $helpfile = shift;
    my $d = $defans? 'Yes':'No';

    for  (;;)  {
        print "$quest [$d]? ";
        my $ans = $self->getchar;
        redo if $self->checkhelp($ans, $helpfile);
        if  ($ans eq "\n")  {
            print "$d\n";
            return $defans;
        }
        $ans = uc($ans);
        if  ($ans eq 'Y')  {
            print "Yes\n";
            return  1;
        }
        if  ($ans eq 'N')  {
            print "No\n";
            return  0;
        }
        print "\nPlease answer Y or N\n";
    }
}

# Ask which one of various possibilities
# Param 1 prompt
# Param 2 Possibilities array
# Param 3 help file name
# Param 4 default response or existing response take first if not there

sub askopt {
    my $self = shift;
    my $prompt = shift;
    my $poss = shift;
    my $help = shift;
    my $def = shift;
    my $indx = 0;
    my $n = 0;
    my %fchar;

    # If default specified, find its index in the possibilities list

    if  ($def)  {
        for my $p (@$poss)  {
            if  ($p eq $def)  {
                $indx = $n;
                last;
            }
            $n++;
        }
    }

    # Record index of first chars of each option
    # If that is duplicated save first one only

    $n = 0;
    for my $p (@$poss)  {
        my $fc = lc substr $p, 0, 1;
        $fchar{$fc} = {IND => $n} unless defined $fchar{$fc};
        $n++;
    }

    # Come back here for new prompt

    pr: for  (;;)  {
        my $choice = $poss->[$indx];

        print "$prompt: $choice";

        for  (;;)  {

            my $ch = $self->getchar;

            # Enter dignifies the end (we translated \r and F2)

            if ($ch eq "\n")  {
                print "\n";
                return  $choice;
            }

            redo pr if $self->checkhelp($ch, $help);

            # Go round options with space or tab

            my $och = ord($ch);
            if  ($och == $self->{SPACE} or $och == $self->{TAB})  {
                $indx++;
                $indx = 0 if $indx > $#$poss;
                print "\n";
                redo  pr;
            }

            if ($och == $self->{ESC})  {
                print "\n";
                return  undef;
            }

            redo if $och < $self->{SPACE} or $och > 126;

            # If we type something, move to it if we have seen the first letter

            $ch = lc $ch;
            redo unless defined $fchar{$ch};
            $indx = $fchar{$ch}->{IND};
            print "\n";
            redo  pr;
        }
    }
}

# For blanking out what we've got and repositioning the cursor

sub erase ($$) {
    my $disp = shift;
    my $ddef = shift;
    my $l = length($disp);
    print "\b" x $l unless $ddef;
    print " " x $l, "\b" x $l;
}

# Ask for a number
# Arg1: Prompt
# Arg2: Default/current value
# Arg3: Help file name

sub asknum {
    my $self = shift;
    my $prompt = shift;
    my $def = shift;
    my $help = shift;

    my $disp = $def;
    my $curr = 0;
    my $ddef = 1;

    pr: for(;;)  {

        print "$prompt: $disp";
        print "\b" x length($disp) if $ddef;

        for  (;;)  {
            my $ch = $self->getchar;
            redo  pr  if $self->checkhelp($ch, $help);

            # Enter signifies the end

            if ($ch eq "\n")  {
                print "\n";
                return  $disp;
            }

            my  $och = ord($ch);

            if  ($och == $self->{KILL})  {
                redo if $ddef;
                erase($disp, 0);
                $disp = $def;
                $ddef = 1;
                $curr = 0;
                print $disp, "\b" x length($disp);
                redo;
            }

            if  ($och == $self->{ERASE})  {
                redo if $ddef;
                erase($disp, 0);
                if  ($curr == 0)  {
                    $disp = $def;
                    $ddef = 1;
                    print $disp, "\b" x length($disp);
                    redo;
                }
                $curr = int($curr / 10);
                $disp = $curr;
                print $disp;
                redo;
            }

            $och -= ord('0');
            redo  if  $och < 0  or  $och > 9;
            $curr = $curr * 10 + $och;
            erase($disp, $ddef);
            $disp = $curr;
            $ddef = 0;
            print $disp;
        }
    }
}

# Ask for a string value, similar to above

sub askstring {
    my $self = shift;
    my $prompt = shift;
    my $def = shift;
    my $help = shift;
    my $disp = $def;
    my $curr = "";
    my $ddef = 1;

    pr: for(;;)  {

        print "$prompt: $disp";
        print "\b" x length($disp) if $ddef;

        for  (;;)  {
            my $ch = $self->getchar;
            redo pr if $self->checkhelp($ch, $help);

            # Enter signifies the end

            if ($ch eq "\n")  {
                print "\n";
                return  $disp;
            }

            my  $och = ord($ch);

            if  ($och == $self->{KILL})  {
                redo if $ddef;
                erase($disp, 0);
                $disp = $def;
                $curr = "";
                $ddef = 1;
                print $disp, "\b" x length($disp);
                redo;
            }

            if  ($och == $self->{ERASE})  {
                redo if $ddef;
                erase($disp, 0);
                if  (length($curr) == 0)  {
                    $disp = $def;
                    $ddef = 1;
                    print $disp, "\b" x length($disp);
                    redo;
                }
                $curr = substr $curr, 0, -1;
                $disp = $curr;
                print  $disp;
                redo;
            }

            if ($och == $self->{ESC})  {
                print "\n";
                return  undef;
            }

            redo if $och < $self->{SPACE} or $och > 126;
            erase($disp, $ddef) if $ddef;
            print $ch;
            $curr .= $ch;
            $disp = $curr;
            $ddef = 0;
         }
    }
}

# Stuff about asking for directories

# Internal routine to get a list of subdirectories of the specified directory (combination)
# Arg1 is the base directory
# Arg2 is what we've got so far
# Use "glob" to do the work
# Return a list ref of possibilities (possibly empty)

sub getpossdirs {
    my $basedir = shift;
    my $sofar = shift;
    my $fullp = "$basedir$sofar";
    my @bits = glob("$fullp*/");
    my @res;
    # Just get the names
    for my $b (@bits)  {
        # Some globs return self in some cases so skip it.
        next if $b eq $fullp;
        my @path = split('/', $b, -1);
        pop @path;
        push @res, (pop @path);
    }
    \@res;
}

# Internal routine to ask for directories and interpret control characters
# Arg 1: Description of directory
# Arg 2: Base directory - cannot go further up than this
# Arg 3: Optional starting point
# Returns add-on to based diretory, e.g. if base is /usr/spool and you type progs you'll get progs.

sub askdirectory {
    my $self = shift;
    my $dname = shift;
    my $basedir = shift;
    my $starting = shift;

    my $sofar = '/';
    $sofar = $starting if $starting =~ m;^/;;

    my $possdirs = [];      # Current list of possibles
    my $indx = -1;          # Index into that -1 indicates no list at present

    pr: for  (;;)  {        # Come back here for new prompt

        print "$dname: $basedir$sofar";

        for  (;;)  {
            my $ch = $self->getchar;
            redo if $self->checkhelp($ch, "dselect");

            # Has he finished input, insist we have some

            if ($ch eq "\n")  {
                if  (length($sofar) <= 1)  {
                    if  (length($basedir) == 0)  {
                        print  "\nSorry, cannot use root directory for install, please try again\n";
                    }
                    else  {
                        print "\nSorry, must use a subdirectory of $basedir\n";
                    }
                    redo pr;
                }
                print "\n";

                # Remove any trailing /

                $sofar =~ s;/$;;;
                return  $sofar;
            }

            # just get numeric chars to test for erase and tab

            my $och = ord($ch);
            if  ($och == $self->{ERASE}) {
                unless (length($sofar) <= 1)  {
                    $sofar = substr $sofar, 0, -1;
                    $indx = -1;
                    print "\b \b";
                }
                redo;       # Try to stay on line
            }

            # Space or tab case

            if  ($och == $self->{TAB} or $och == $self->{SPACE})  {
                # This is where we look for directories or subdirectories
                # First split the current path up

                my @dirparts = split('/', $sofar, -1);      # -1 to leave trailing items
                my $lastcomp = pop @dirparts;               # Last component (might be empty if path ended in /)

                # We were already looking in directory if $indx >= 0
                # We make a distinction here between space (look at subdirs) and tab (look at other ones)

                if  ($indx >= 0)  {
                    if  ($och == $self->{SPACE})  {

                        # Find any subdirectories of that
                        # If there aren't any, don't confuse the issue, just ignore it

                        my $pd = getpossdirs($basedir, $sofar);
                        redo unless $#$pd >= 0;

                        # We don't expect $lastcomp to have anything in in this case because we
                        # put an empty on the end last time

                        $possdirs = $pd;
                        $indx = 0;
                    }
                    else  {
                        pop @dirparts unless $#dirparts <= 0;
                        $indx++;
                        $indx = 0 if $indx > $#$possdirs;
                    }

                    push @dirparts, $possdirs->[$indx];
                    push @dirparts, "";
                    $sofar = join('/', @dirparts);
                    print "\n";
                    redo pr;
                }

                # Haven't previously looked in here, we don't make a distincion between tab and space
                # Get possible subdirectories, if none just start again
                # Note that if $lastcomp is not empty, we looked for something proefixed by what's given.

                $possdirs = getpossdirs($basedir, $sofar);
                if ($#$possdirs < 0)  {             # Nothing
                    print "\n";
                    redo pr;
                }

                # Pick first item out of list

                $indx = 0;
                pop @dirparts unless $#dirparts <= 0 || length($lastcomp) > 0;
                push @dirparts, $possdirs->[0];
                push @dirparts, "";
                $sofar = join('/', @dirparts);
                print "\n";
                redo pr;
            }

            # Control-W case, eat up last part

            if  ($och == $self->{CTRLW})  {
                redo if $sofar eq '/';  # Nothing to do
                my @dirparts = split('/', $sofar, -1);
                my $last = pop @dirparts;
                #  If it's got something on the end, it must be user types, so we delete that and go back to the start
                if  (length($last) > 0)  {
                    push @dirparts, "";
                    $sofar = join('/', @dirparts);
                    print "\n";
                    redo pr;
                }
                pop @dirparts;
                push @dirparts, "";
                $sofar = join('/', @dirparts);
                $indx = -1;
                print "\n";
                redo  pr;
            }
            redo if $och < $self->{SPACE} || $och > 126;
            $sofar .= $ch;
            $indx = -1;
            print $ch;
        }
    }
}

1;
