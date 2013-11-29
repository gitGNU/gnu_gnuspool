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

########################################################################
#
# Functions for formatting display
#
########################################################################

package outformat;

# Underline a message

sub underline {
    my $fh = shift;
    my $txt = shift;
    my $und = shift || '-';
    print $fh $txt,"\n", $und x length($txt),"\n\n";
}

# Nice formatted column

sub multicol {
    my $fh = shift;
    my $arr = shift;
    my @cw;
    for my $a (@$arr) {
        my $c = 0;
        for my $e (@$a) {
               $cw[$c] = length($e) if $cw[$c] < length($e);
               $c++;
        }
    }
    for my $a (@$arr) {
        my @e = @$a;
        my $c = 0;
        while  ($#e > 0) {
               my $i = shift @e;
               print $fh $i, ' ' x ($cw[$c] - length($i) + 1);
               $c++;
           }
           my $i = shift @e;
           print $fh "$i\n";
    }
}

1;
