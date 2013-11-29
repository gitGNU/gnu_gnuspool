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

# If argument looks like a file name, Output contents of help file from standard place.
# Help files are assumed only to be a few lines.

package disphelp;

sub displayhelp {
    my $file = shift;
    my $chenv = shift;
    print "\n";
    if ($file =~ /\s/)  {
        print "$file\n";
    }
    else  {
        my $fullp = "$main::INSTHELP/$file.txt";
        if (open(HF, $fullp))  {
            print while (<HF>);
            close HF;
        }
        else  {
            print <<EOT;
Sorry but helpfile

"$fullp"

is missing - please let us know.

EOT
        }
    }

    print "\n[Press any key to continue]";
    # Throw away char
    $chenv->getchar;
    print "\n";
}

1;
